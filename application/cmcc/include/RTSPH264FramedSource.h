/*
 * RTSPH264FramedSource.h
 * A custom FramedSource that acquires H.264 NAL units via a
 * user-registered callback, and delivers them via the live555 event loop.
 */

#ifndef _H264_FRAMED_SOURCE_HH
#define _H264_FRAMED_SOURCE_HH

#include <FramedSource.hh>

// ============================================================
// Callback type for acquiring a single H.264 NAL unit frame
//
// Parameters:
//   userData  - opaque pointer registered by the caller
//   buffer    - output buffer (pre-allocated, maxSize bytes)
//   maxSize   - maximum bytes that can be written to buffer
//   frameSize - [out] actual NAL unit size written (without start code)
//   presentationTime - [out] optional presentation timestamp;
//                    if tv_sec==0 && tv_usec==0, internal pacing is used
//
// Returns:
//   true  - a frame is ready; frameSize and presentationTime are valid
//   false - no frame available yet (triggers retry after a short delay)
// ============================================================
typedef bool (*FrameAcquireCallback)(void* userData,
                                     unsigned char* buffer,
                                     unsigned maxSize,
                                     unsigned& frameSize,
                                     struct timeval& presentationTime);

class RTSPH264FramedSource : public FramedSource {
public:
    /*
     * Create using a callback.
     * @param env        The usage environment
     * @param callback   Function to call when a frame is needed
     * @param userData   Opaque pointer passed to the callback
     * @param fps        Frames per second (for internal timestamp pacing,
     *                   used only when callback returns tv_sec=tv_usec=0)
     */
    static RTSPH264FramedSource* createNew(UsageEnvironment& env,
                                       FrameAcquireCallback callback,
                                       void* userData,
                                       unsigned fps = 15);

    /*
     * Convenience: create from file (backward-compatible).
     * Internally registers a built-in file-reading callback.
     */
    static RTSPH264FramedSource* createNewFromFile(UsageEnvironment& env,
                                               char const* fileName,
                                               unsigned fps = 15);

protected:
    RTSPH264FramedSource(UsageEnvironment& env,
                     FrameAcquireCallback callback,
                     void* userData,
                     unsigned fps);
    virtual ~RTSPH264FramedSource();

private:
    // Called by live555 scheduler when data is needed
    virtual void doGetNextFrame();

    // Background task that triggers delivery of next frame
    static void deliverFrame0(void* clientData);
    void deliverFrame();

    // Built-in file-reading callback
    static bool fileReadCallback(void* userData,
                                 unsigned char* buffer,
                                 unsigned maxSize,
                                 unsigned& frameSize,
                                 struct timeval& presentationTime);

private:
    FrameAcquireCallback  fCallback;          // User callback
    void*                 fUserData;          // User data for callback
    unsigned              fFps;               // Frame rate for pacing
    unsigned              fPreferredFrameSize;
    struct timeval        fLastDeliveryTime;  // For internal pacing
    unsigned              fFrameCount;        // Total frames delivered
    bool                  fHaveStarted;       // First delivery flag

    // Event trigger ID for scheduling deliveries
    EventTriggerId        fEventTriggerId;

    // ---- Only used by built-in file callback ----
    struct FileContext {
        FILE*        fid;
        unsigned char* readBuffer;
        unsigned     readBufferSize;
        unsigned     readOffset;
        bool         fileEnded;
    };
    FileContext*          fFileCtx;           // NULL unless using file callback

    // Start-code parsing helpers (used by file callback, but could be static utils)
    static unsigned findStartCode(unsigned char* buf, unsigned bufSize);
};

#endif // _H264_FRAMED_SOURCE_HH