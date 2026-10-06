#include "h264decoder.h"
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>

h264decoder::h264decoder() {
  if (WelsCreateDecoder(&pDecoder) || (NULL == pDecoder)) {
    LOG_ERROR("WelsCreateDecoder failed!\n");
    return;
  }
}

CM_RETURN h264decoder::ReInitpDecoder() {

  if (pDecoder == NULL) {
    LOG_ERROR("Decoder is not created successfully!\n");
    return cmUnknownReason;
  }

  int32_t iThreadCount = 0;
  pDecoder->Uninitialize();

  CM_RETURN ret = static_cast<CM_RETURN>(
      pDecoder->SetOption(DECODER_OPTION_NUM_OF_THREADS, &iThreadCount));
  if (ret != cmResultSuccess) {
    LOG_ERROR("h264decoder SetOption DECODER_OPTION_NUM_OF_THREADS failed!\n");
  }

  SDecodingParam sDecParam = {0};
  sDecParam.sVideoProperty.size = sizeof(sDecParam.sVideoProperty);
  sDecParam.uiTargetDqLayer = (uint8_t)-1;
  sDecParam.eEcActiveIdc = ERROR_CON_SLICE_COPY;
  sDecParam.sVideoProperty.eVideoBsType = VIDEO_BITSTREAM_DEFAULT;

  ret = static_cast<CM_RETURN>(pDecoder->Initialize(&sDecParam));

  if (ret != cmResultSuccess) {
    LOG_ERROR("h264decoder Initialize failed!\n");
    return ret;
  }

  ret = static_cast<CM_RETURN>(pDecoder->SetOption(DECODER_OPTION_ERROR_CON_IDC,
                                                   &sDecParam.eEcActiveIdc));
  if (ret != cmResultSuccess) {
    LOG_ERROR("h264decoder SetOption DECODER_OPTION_ERROR_CON_IDC failed!\n");
    return ret;
  }

  return ret;
}

h264decoder::~h264decoder() {
  if (pDecoder) {
    pDecoder->Uninitialize();
    WelsDestroyDecoder(pDecoder);
    pDecoder = NULL;
  }
}

int32_t h264decoder::inputData(const char *filepath) {
  std::ifstream infile(filepath, std::ios::in | std::ios::binary);

  if (!infile.is_open()) {
    LOG_ERROR("Failed to open file: %s\n", filepath);
    return -1;
  }

  int32_t frameCount = 0;

  // 读取全部数据 去除 endoffile
  size_t filesize = infile.seekg(0, std::ios::end).tellg();
  infile.seekg(0, std::ios::beg);

  h264buffer.resize(filesize + 4); // 预留4字节添加结束符

//   LOG_INFO("File size: %zu kB\n", filesize / 1024);

  infile.read(reinterpret_cast<char *>(h264buffer.data()), filesize);

  h264buffer[filesize] = 0;
  h264buffer[filesize + 1] = 0;
  h264buffer[filesize + 2] = 0;
  h264buffer[filesize + 3] = 1; // 添加结束符，确保

  infile.close();

  this->perstartpos = 0;
  this->uiTimeStamp = 0;

  this->ReInitpDecoder();


  return static_cast<int32_t>(h264buffer.size());
}

int32_t h264decoder::findStartCode(size_t startindex) {
  size_t i = startindex;
  for (; i < h264buffer.size() - 4; i++) {
    if ((h264buffer[i] == 0 && h264buffer[i + 1] == 0 &&
         h264buffer[i + 2] == 0 && h264buffer[i + 3] == 1) ||
        (h264buffer[i] == 0 && h264buffer[i + 1] == 0 &&
         h264buffer[i + 2] == 1)) {
      break;
    }
  }
  return i;
}

int32_t h264decoder::getYUV420Frame(std::vector<uint8_t> &yuv420frame) {

  int32_t YUV420FrameSize = 0;
  int32_t startPos = this->findStartCode(perstartpos);
  SBufferInfo sBufInfo;
  int32_t frameSize = 0;

  auto startTime = std::chrono::high_resolution_clock::now();
  while (startPos < h264buffer.size()) {
    int32_t nextStartPos = this->findStartCode(startPos + 1);
    uint8_t *pData[3] = {NULL};
    uint8_t *pDst[3] = {NULL};
    uint8_t *pFrameData = &h264buffer[startPos];
    frameSize = nextStartPos - startPos;
    int32_t iFrameSize = static_cast<int32_t>(frameSize);

    memset(&sBufInfo, 0, sizeof(SBufferInfo));
    sBufInfo.uiInBsTimeStamp = uiTimeStamp;
    pDecoder->DecodeFrameNoDelay(pFrameData, iFrameSize, pData, &sBufInfo);
    if (sBufInfo.iBufferStatus == 1) {
      uiTimeStamp++;
      SSysMEMBuffer sSystemBuffer = sBufInfo.UsrData.sSystemBuffer;
      uint8_t *pData[3] = {NULL};
      pDst[0] = sBufInfo.pDst[0];
      pDst[1] = sBufInfo.pDst[1];
      pDst[2] = sBufInfo.pDst[2];

      if (pDst[0] != NULL && pDst[1] != NULL && pDst[2] != NULL) {
        int32_t ySize = sSystemBuffer.iWidth * sSystemBuffer.iHeight;
        int32_t uvSize = ySize / 4;

        yuv420frame.resize(ySize * 3 / 2);

        for (int row = 0; row < sSystemBuffer.iHeight; row++) {
          memcpy(&yuv420frame[row * sSystemBuffer.iWidth],
                 pDst[0] + row * sSystemBuffer.iStride[0],
                 sSystemBuffer.iWidth);
        }

        for (int row = 0; row < sSystemBuffer.iHeight / 2; row++) {
          memcpy(&yuv420frame[ySize + row * (sSystemBuffer.iWidth / 2)],
                 pDst[1] + row * sSystemBuffer.iStride[1],
                 sSystemBuffer.iWidth / 2);
        }

        for (int row = 0; row < sSystemBuffer.iHeight / 2; row++) {
          memcpy(
              &yuv420frame[ySize + uvSize + row * (sSystemBuffer.iWidth / 2)],
              pDst[2] + row * sSystemBuffer.iStride[1],
              sSystemBuffer.iWidth / 2);
        }

        YUV420FrameSize = ySize * 3 / 2;

        perstartpos = nextStartPos;

        break;
      } else {
        YUV420FrameSize = -1;
        LOG_ERROR("Decoded frame data is NULL\n");
      }
    }
    startPos = nextStartPos;
  }

  if (startPos >= h264buffer.size()) {
    auto size = this->Flushframe(yuv420frame);
    if (size > 0) {
      YUV420FrameSize = size;
    } else {
      YUV420FrameSize = 0;
      ReInitpDecoder();
      perstartpos = 0;
    }
  }

  return YUV420FrameSize;
}

int32_t h264decoder::Flushframe(std::vector<uint8_t> &yuv420frame) {
  int32_t num_of_frames_in_buffer = 0;

  pDecoder->GetOption(DECODER_OPTION_NUM_OF_FRAMES_REMAINING_IN_BUFFER,
                      &num_of_frames_in_buffer);

  uint8_t *pData[3] = {NULL};
  uint8_t *pDst[3] = {NULL};

  SBufferInfo sDstBufInfo;

  if (num_of_frames_in_buffer > 0) {
    // LOG_INFO("Flushing decoder, %d frames remaining in buffer.\n",
    //          num_of_frames_in_buffer);

    sDstBufInfo.uiInBsTimeStamp = uiTimeStamp;
    pData[0] = NULL;
    pData[1] = NULL;
    pData[2] = NULL;
    pDecoder->FlushFrame(pData, &sDstBufInfo);

    if (sDstBufInfo.iBufferStatus == 1) {
      uiTimeStamp++;
      SSysMEMBuffer sSystemBuffer = sDstBufInfo.UsrData.sSystemBuffer;
      uint8_t *pData[3] = {NULL};
      pDst[0] = sDstBufInfo.pDst[0];
      pDst[1] = sDstBufInfo.pDst[1];
      pDst[2] = sDstBufInfo.pDst[2];

      if (pDst[0] != NULL && pDst[1] != NULL && pDst[2] != NULL) {
        int32_t ySize = sSystemBuffer.iWidth * sSystemBuffer.iHeight;
        int32_t uvSize = ySize / 4;

        yuv420frame.resize(ySize * 3 / 2);

        for (int row = 0; row < sSystemBuffer.iHeight; row++) {
          memcpy(&yuv420frame[row * sSystemBuffer.iWidth],
                 pDst[0] + row * sSystemBuffer.iStride[0],
                 sSystemBuffer.iWidth);
        }

        for (int row = 0; row < sSystemBuffer.iHeight / 2; row++) {
          memcpy(&yuv420frame[ySize + row * (sSystemBuffer.iWidth / 2)],
                 pDst[1] + row * sSystemBuffer.iStride[1],
                 sSystemBuffer.iWidth / 2);
        }

        for (int row = 0; row < sSystemBuffer.iHeight / 2; row++) {
          memcpy(
              &yuv420frame[ySize + uvSize + row * (sSystemBuffer.iWidth / 2)],
              pDst[2] + row * sSystemBuffer.iStride[1],
              sSystemBuffer.iWidth / 2);
        }

        return ySize * 3 / 2;
      }

    } else {
      LOG_INFO("No frames remaining in buffer to flush.\n");
      return 0;
    }

    return 0;
  }
}

void h264decoder::YUV420ToNV12(const uint8_t *yuv420, uint8_t *nv12, int width,
                               int height) {
  int ySize = width * height;
  memcpy(nv12, yuv420, ySize);
  uint8_t *uvSrc = const_cast<uint8_t *>(yuv420 + ySize);
  uint8_t *uvDst = nv12 + ySize;

  for (int i = 0; i < ySize / 4; i++) {
    uvDst[i * 2] = uvSrc[i];                 // U
    uvDst[i * 2 + 1] = uvSrc[i + ySize / 4]; // V
  }
}