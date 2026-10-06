#pragma once
#include "codec_api.h"
#include "codec_app_def.h"
#include "codec_def.h"
#include <cstdint>
#include <cstring>
#include <fstream>
#include <limits>
#include <stdio.h>
#include <stdlib.h>
#include <vector>
#include "sys_logger.h"


inline long getProcessRssKB() {
  std::ifstream in("/proc/self/status");
  if (!in.is_open()) {
    return -1;
  }
  std::string key;
  while (in >> key) {
    if (key == "VmRSS:") {
      long kb = -1;
      in >> kb; // kB
      return kb;
    }
    // 跳过该行剩余内容
    in.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
  }
  return -1;
}

class h264decoder {
private:
  /* data */
  ISVCDecoder *pDecoder = NULL;

  std::vector<uint8_t> h264buffer;
  uint64_t uiTimeStamp = 0;
  uint32_t perstartpos = 0;

  int32_t findStartCode(size_t startCode);

  int32_t Flushframe(std::vector<uint8_t> &yuv420frame);

public:
  h264decoder();

  /**
   * @return 0表示成功，其他值表示失败
   */
  CM_RETURN ReInitpDecoder();

  ~h264decoder();

  /**
   * @param filepath H.264码流文件路径
   * @return 返回读取文件流大小，单位字节；如果读取失败，返回负数错误码
   */
  int32_t inputData(const char *filepath);

  /**
   * @param yuv420frame 解码后的YUV420数据
   * @return 解码后的YUV420数据的大小，单位字节；如果解码失败，返回负数错误码
   */
  int32_t getYUV420Frame(std::vector<uint8_t> &yuv420frame);

  
  int32_t getNV12Frame(std::vector<uint8_t> &nv12frame, int width, int height)
  {
    
    static std::vector<uint8_t> yuv420frame;
    int32_t ret = getYUV420Frame(yuv420frame);
    if (ret <= 0) {
      return ret; // 解码失败，返回错误码
    }
    
    if(nv12frame.capacity() != width * height * 3 / 2) {
      nv12frame.resize(width * height * 3 / 2);
    }
   
    YUV420ToNV12(yuv420frame.data(), nv12frame.data(), width, height);
    return static_cast<int32_t>(nv12frame.size());
  }

  

  static void YUV420ToNV12(const uint8_t *yuv420, uint8_t *nv12, int width, int height);

};
