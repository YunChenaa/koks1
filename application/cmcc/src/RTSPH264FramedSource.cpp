/*
 * RTSPH264FramedSource.cpp
 *
 * Implementation of a live555 FramedSource that acquires H.264 NAL units
 * via a user-registered callback, and delivers them through the live555
 * event loop.
 */

#include "RTSPH264FramedSource.h"
#include "sys_logger.h"
#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

// ===================== H264FramedSource =====================

RTSPH264FramedSource* RTSPH264FramedSource::createNew(UsageEnvironment& env,
                                               FrameAcquireCallback callback,
                                               void* userData,
                                               unsigned fps)
{
    return new RTSPH264FramedSource(env, callback, userData, fps);
}

// Built-in file-reader context
struct FileReadContext {
    char*          fileName;
    FILE*          fid;
    unsigned char* readBuffer;
    unsigned       readBufferSize;
    unsigned       readOffset;
    bool           fileEnded;
};

RTSPH264FramedSource* RTSPH264FramedSource::createNewFromFile(UsageEnvironment& env,
                                                       char const* fileName,
                                                       unsigned fps)
{
    // Allocate a FileReadContext that the file callback will use
    FileReadContext* ctx = new FileReadContext;
    ctx->fileName = NULL;
    ctx->fid = NULL;
    ctx->readBuffer = NULL;
    ctx->readBufferSize = 512 * 1024;
    ctx->readOffset = 0;
    ctx->fileEnded = false;

    if (fileName != NULL) {
        ctx->fileName = strdup(fileName);
        ctx->fid = fopen(fileName, "rb");
        if (ctx->fid == NULL) {
            env << "H264FramedSource: cannot open file \""
                << fileName << "\": " << strerror(errno) << "\n";
        }
    }

    ctx->readBuffer = new unsigned char[ctx->readBufferSize];

    return new RTSPH264FramedSource(env, fileReadCallback, ctx, fps);
}

RTSPH264FramedSource::RTSPH264FramedSource(UsageEnvironment& env,
                                     FrameAcquireCallback callback,
                                     void* userData,
                                     unsigned fps)
    : FramedSource(env),
      fCallback(callback),
      fUserData(userData),
      fFps(fps),
      fPreferredFrameSize(256 * 1024),
      fFrameCount(0),
      fHaveStarted(false),
      fFileCtx(NULL)
{
    // Seed timing
    gettimeofday(&fLastDeliveryTime, NULL);

    // Create event trigger for async scheduling
    fEventTriggerId = envir().taskScheduler().createEventTrigger(
                        deliverFrame0);
}

RTSPH264FramedSource::~RTSPH264FramedSource()
{
    envir().taskScheduler().deleteEventTrigger(fEventTriggerId);

    // Clean up file context if using built-in file callback
    if (fCallback == fileReadCallback && fUserData != NULL) {
        FileReadContext* ctx = (FileReadContext*)fUserData;
        if (ctx->fid != NULL) fclose(ctx->fid);
        delete[] ctx->readBuffer;
        free(ctx->fileName);
        delete ctx;
    }
}

// ============================================================
// Static helpers: NAL unit start-code parsing
// ============================================================
unsigned RTSPH264FramedSource::findStartCode(unsigned char* buf,
                                          unsigned bufSize)
{
    for (unsigned i = 0; i + 3 < bufSize; ++i) {
        if (buf[i] == 0x00 && buf[i+1] == 0x00) {
            if (buf[i+2] == 0x01) {
                return i;           // 3-byte start code
            }
            if (buf[i+2] == 0x00 && buf[i+3] == 0x01) {
                return i;           // 4-byte start code
            }
        }
    }
    return bufSize;  // not found
}

// ============================================================
// Built-in file-reading callback
// ============================================================
bool RTSPH264FramedSource::fileReadCallback(void* userData,
                                         unsigned char* buffer,
                                         unsigned maxSize,
                                         unsigned& frameSize,
                                         struct timeval& /*presentationTime*/)
{
    FileReadContext* ctx = (FileReadContext*)userData;
    if (ctx == NULL || ctx->fid == NULL) return false;

    // Refill buffer if needed
    if (ctx->readOffset == 0 ||
        ctx->readOffset >= ctx->readBufferSize - 4 ||
        ctx->readBufferSize == 0) {

        if (ctx->fileEnded) return false;

        // Keep up to 4 bytes of potential start-code prefix
        unsigned keep = 0;
        if (ctx->readBufferSize > 4 && ctx->readOffset > 0) {
            keep = (ctx->readOffset < 4) ? ctx->readOffset : 4;
            if (keep > 0) {
                memmove(ctx->readBuffer,
                        ctx->readBuffer + ctx->readOffset - keep,
                        keep);
            }
        }

        size_t bytesRead = fread(ctx->readBuffer + keep, 1,
                                  ctx->readBufferSize - keep, ctx->fid);
        if (bytesRead == 0) {
            ctx->fileEnded = true;
            return false;
        }
        ctx->readOffset = 0;
        ctx->readBufferSize = keep + (unsigned)bytesRead;
    }

    // Find first start code
    unsigned startPos = findStartCode(ctx->readBuffer + ctx->readOffset,
                                       ctx->readBufferSize - ctx->readOffset);
    if (startPos >= ctx->readBufferSize - ctx->readOffset) {
        ctx->readOffset = ctx->readBufferSize;
        return false;
    }

    unsigned nalHeaderEnd = ctx->readOffset + startPos;
    unsigned scLen = (ctx->readBuffer[nalHeaderEnd + 2] == 0x01) ? 3 : 4;
    unsigned nalStart = nalHeaderEnd + scLen;

    // Find next start code (end of this NAL)
    unsigned nextScPos = findStartCode(ctx->readBuffer + nalStart,
                                        ctx->readBufferSize - nalStart);
    unsigned nalEnd = nalStart + nextScPos;

    unsigned nalSize = nalEnd - nalStart;
    if (nalSize > maxSize) {
        // Truncate to max allowed
        nalSize = maxSize;
    }

    // Copy NAL data (without start code) to output buffer
    if (nalSize > 0) {
        memcpy(buffer, ctx->readBuffer + nalStart, nalSize);
    }

    // Advance read pointer past this NAL
    ctx->readOffset = nalEnd;

    frameSize = nalSize;
    return true;
}

// ============================================================
// Live555 event loop integration
// ============================================================
void RTSPH264FramedSource::doGetNextFrame()
{
    envir().taskScheduler().triggerEvent(fEventTriggerId, this);
}

void RTSPH264FramedSource::deliverFrame0(void* clientData)
{
    ((RTSPH264FramedSource*)clientData)->deliverFrame();
}

void RTSPH264FramedSource::deliverFrame()
{
    if (!isCurrentlyAwaitingData()) return;

    unsigned frameSize = 0;
    struct timeval callbackTime = {0, 0};  // zero means "use internal pacing"

    bool gotFrame = fCallback(fUserData, fTo, fMaxSize,
                               frameSize, callbackTime);

    if (!gotFrame) {
        LOG_ERROR("RTSPH264FramedSource: no frame available, retrying after delay\n");
        // No data available — retry after a short delay
        nextTask() = envir().taskScheduler().scheduleDelayedTask(
                10000, (TaskFunc*)deliverFrame0, this);
        return;
    }

    fFrameSize = frameSize;
    fNumTruncatedBytes = 0;

    // Determine presentation time
    if (callbackTime.tv_sec == 0 && callbackTime.tv_usec == 0) {
        // Callback didn't provide a timestamp → use internal FPS pacing
        gettimeofday(&fPresentationTime, NULL);
        if (fHaveStarted) {
            unsigned uSeconds = 1000000 / fFps;
            fPresentationTime.tv_usec += uSeconds;
            while (fPresentationTime.tv_usec >= 1000000) {
                fPresentationTime.tv_usec -= 1000000;
                fPresentationTime.tv_sec += 1;
            }
        } else {
            fHaveStarted = true;
        }
    } else {
        // Use timestamp provided by callback
        fPresentationTime = callbackTime;
        fHaveStarted = true;
    }

    fLastDeliveryTime = fPresentationTime;
    fFrameCount++;
    LOG_INFO("RTSPH264FramedSource: delivered frame %u, size %u bytes\n",
              fFrameCount, fFrameSize); 

    FramedSource::afterGetting(this);
}