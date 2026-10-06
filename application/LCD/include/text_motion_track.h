
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "draw_text_nv12.h"

enum class MotionEase { Linear, EaseIn, EaseOut, EaseInOut };

struct TextMotionKeyframe {
  int64_t timeMs = 0;
  float x = 0.0f;
  float y = 0.0f;
  float scale = 1.0f;

  float colorR = 255.0f;
  float colorG = 255.0f;
  float colorB = 255.0f;

  MotionEase easeToNext = MotionEase::Linear;

  TextMotionKeyframe() {}

  TextMotionKeyframe(int64_t inTimeMs, float inX, float inY, float inScale,
                     float inColorR, float inColorG, float inColorB,
                     MotionEase inEaseToNext)
      : timeMs(inTimeMs), x(inX), y(inY), scale(inScale), colorR(inColorR),
        colorG(inColorG), colorB(inColorB), easeToNext(inEaseToNext) {}
};

struct TextMotionState {
  float x = 0.0f;
  float y = 0.0f;
  float scale = 1.0f;

  float colorR = 255.0f;
  float colorG = 255.0f;
  float colorB = 255.0f;
};

class TextMotionTrack {
public:
  TextMotionTrack() {};
  TextMotionTrack(const std::vector<TextMotionKeyframe> &keyframes) {
    keyframes_ = keyframes; //拷贝构造
  }
      void Clear();
  void AddKeyframe(const TextMotionKeyframe &keyframe);

  bool Empty() const;
  std::size_t Size() const;

  TextMotionState Sample(int64_t timeMs) const;

  TextDrawOptions ApplyToTextOptions(int64_t timeMs,
                                     const TextDrawOptions &base) const;

private:
  static float EaseValue(float t, MotionEase easing);
  static float Lerp(float a, float b, float t);

  std::vector<TextMotionKeyframe> keyframes_;
};
