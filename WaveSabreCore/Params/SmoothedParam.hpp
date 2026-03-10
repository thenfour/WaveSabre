#pragma once

// a param reader that smooths its output value over time.

#include "../Basic/DSPMath.hpp"

namespace WaveSabreCore::M7
{


class SmoothedParam
{
public:
  // smoothing coefficient: smaller = smoother (more lag, less ripple); larger = less smooth (less lag, more ripple)
  static constexpr float kCoeff = 1.0f / 64.0f;

  explicit SmoothedParam(float initialValue = 0.0f)
      : mTarget(initialValue)
      , mCurrent(initialValue)
  {
  }

  void setTarget(float target)
  {
    mTarget = target;
  }

  float onSampleAndGetSmoothed()
  {
    mCurrent += (mTarget - mCurrent) * kCoeff;
    return mCurrent;
  }

  float getTarget() const
  {
    return mTarget;
  }
  float getCurrent() const
  {
    return mCurrent;
  }

private:
  float mTarget;
  float mCurrent;
};


}  // namespace WaveSabreCore::M7
