#include "text_motion_track.h"

#include <algorithm>
#include <cmath>

static float clamp_float(float value, float minValue, float maxValue) {
  if (value < minValue) {
    return minValue;
  }
  if (value > maxValue) {
    return maxValue;
  }
  return value;
}

static uint8_t float_to_u8(float value) {
  const float v = clamp_float(value, 0.0f, 255.0f);
  return static_cast<uint8_t>(round_to_int(v));
}

void TextMotionTrack::Clear() { keyframes_.clear(); }

void TextMotionTrack::AddKeyframe(const TextMotionKeyframe &keyframe) {
  const auto it = std::lower_bound(
      keyframes_.begin(), keyframes_.end(), keyframe.timeMs,
      [](const TextMotionKeyframe &kf, int64_t t) { return kf.timeMs < t; });

  if (it != keyframes_.end() && it->timeMs == keyframe.timeMs) {
    *it = keyframe;
    return;
  }

  keyframes_.insert(it, keyframe);
}

bool TextMotionTrack::Empty() const { return keyframes_.empty(); }

std::size_t TextMotionTrack::Size() const { return keyframes_.size(); }

float TextMotionTrack::EaseValue(float t, MotionEase easing) {
  if (t <= 0.0f) {
    return 0.0f;
  }
  if (t >= 1.0f) {
    return 1.0f;
  }

  switch (easing) {
  case MotionEase::EaseIn:
    return t * t;
  case MotionEase::EaseOut:
    return 1.0f - (1.0f - t) * (1.0f - t);
  case MotionEase::EaseInOut:
    if (t < 0.5f) {
      return 2.0f * t * t;
    }
    return 1.0f - 2.0f * (1.0f - t) * (1.0f - t);
  case MotionEase::Linear:
  default:
    return t;
  }
}

float TextMotionTrack::Lerp(float a, float b, float t) {
  return a + (b - a) * t;
}

TextMotionState TextMotionTrack::Sample(int64_t timeMs) const {
  TextMotionState out{};
  if (keyframes_.empty()) {
    return out;
  }

  if (keyframes_.size() == 1) {
    const TextMotionKeyframe &kf = keyframes_.front();
    out.x = kf.x;
    out.y = kf.y;
    out.scale = kf.scale;
    out.colorR = kf.colorR;
    out.colorG = kf.colorG;
    out.colorB = kf.colorB;
    return out;
  }

  if (timeMs <= keyframes_.front().timeMs) {
    const TextMotionKeyframe &kf = keyframes_.front();
    out.x = kf.x;
    out.y = kf.y;
    out.scale = kf.scale;
    out.colorR = kf.colorR;
    out.colorG = kf.colorG;
    out.colorB = kf.colorB;
    return out;
  }

  if (timeMs >= keyframes_.back().timeMs) {
    const TextMotionKeyframe &kf = keyframes_.back();
    out.x = kf.x;
    out.y = kf.y;
    out.scale = kf.scale;
    out.colorR = kf.colorR;
    out.colorG = kf.colorG;
    out.colorB = kf.colorB;
    return out;
  }

  const auto upper = std::lower_bound(
      keyframes_.begin(), keyframes_.end(), timeMs,
      [](const TextMotionKeyframe &kf, int64_t t) { return kf.timeMs < t; });

  const std::size_t rightIndex =
      static_cast<std::size_t>(std::distance(keyframes_.begin(), upper));
  const std::size_t leftIndex = rightIndex - 1;
  const TextMotionKeyframe &left = keyframes_[leftIndex];
  const TextMotionKeyframe &right = keyframes_[rightIndex];

  const int64_t duration = right.timeMs - left.timeMs;
  float t = 0.0f;
  if (duration > 0) {
    t = static_cast<float>(timeMs - left.timeMs) / static_cast<float>(duration);
  }
  t = EaseValue(t, left.easeToNext);

  out.x = Lerp(left.x, right.x, t);
  out.y = Lerp(left.y, right.y, t);
  out.scale = Lerp(left.scale, right.scale, t);
  out.colorR = Lerp(left.colorR, right.colorR, t);
  out.colorG = Lerp(left.colorG, right.colorG, t);
  out.colorB = Lerp(left.colorB, right.colorB, t);

  return out;
}

TextDrawOptions
TextMotionTrack::ApplyToTextOptions(int64_t timeMs,
                                    const TextDrawOptions &base) const {
  TextDrawOptions out = base;
  const TextMotionState state = Sample(timeMs);

  out.x = static_cast<int>(round_to_int(state.x));
  out.y = static_cast<int>(round_to_int(state.y));
  out.scale = state.scale;
  out.color.r = float_to_u8(state.colorR);
  out.color.g = float_to_u8(state.colorG);
  out.color.b = float_to_u8(state.colorB);
  return out;
}
