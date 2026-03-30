#pragma once

#include "../Basic/Helpers.h"
#include "../Basic/DSPMath.hpp"
#include <algorithm>

namespace WaveSabreCore
{
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT

struct IPeakDetector
{
    virtual ~IPeakDetector() = default;
    virtual void SetParams(double clipHoldMS, double peakHoldMS, double peakFalloffMaxMS) = 0;
    virtual void Reset() = 0;
    virtual void ProcessSample(double s) = 0;
    //bool GetClipIndicator() const { return mClipIndicator; }
    //double GetCurrentPeak() const { return mCurrentPeak; }
};

struct PeakDetector : public IPeakDetector
{
  int mClipHoldSamples;
  int mPeakHoldSamples;
  double mPeakFalloffPerSample;

  // state
  int mClipHoldCounter = 0;
  int mPeakHoldCounter = 0;

  // running values
  bool mClipIndicator = 0;
  double mCurrentPeak = 0;  // the current peak rectified value, accounting for hold & falloff

  void SetParams(double clipHoldMS, double peakHoldMS, double peakFalloffMaxMS)
  {
    mClipHoldSamples = (int)M7::math::MillisecondsToSamples((float)clipHoldMS);
    mPeakHoldSamples = (int)M7::math::MillisecondsToSamples((float)peakHoldMS);

    mPeakFalloffPerSample = std::max(0.0000001, 1.0 / M7::math::MillisecondsToSamples((float)peakFalloffMaxMS));
    Reset();
  }

  void Reset()
  {
    mClipHoldCounter = 0;
    mPeakHoldCounter = 0;
    mClipIndicator = 0;
    mCurrentPeak = 0;
  }

  void ProcessSample(double s)
  {
    double rectifiedSample = fabs(s);  // Assuming 's' is the input sample

    // Clip detection
    if (rectifiedSample >= 1)
    {
      mClipIndicator = true;
      mClipHoldCounter = mClipHoldSamples;
    }
    else if (mClipHoldCounter > 0)
    {
      --mClipHoldCounter;
    }
    else
    {
      mClipIndicator = false;
    }

    // Peak detection and hold
    if (rectifiedSample >= mCurrentPeak)
    {
      mCurrentPeak = rectifiedSample;  // new peak; reset hold & falloff
      mPeakHoldCounter = mPeakHoldSamples;
    }
    else if (mPeakHoldCounter > 0)
    {
      --mPeakHoldCounter;  // holding phase
    }
    else if (mCurrentPeak > 0)
    {
      mCurrentPeak -= mPeakFalloffPerSample;  // falloff phase
      if (mCurrentPeak < 0)
        mCurrentPeak = 0;  // Ensure mCurrentPeak doesn't go below 0
    }
  }

  // optimized for multi-sample processing (e.g., per FFT window). effectively the same as calling ProcessSample in a loop
  void ProcessSampleMulti(double s, int count)
  {
    double rectifiedSample = fabs(s);

    if (rectifiedSample >= 1)
    {
      mClipIndicator = true;
      mClipHoldCounter = mClipHoldSamples;
    }
    else if (mClipHoldCounter > 0)
    {
      mClipHoldCounter = (mClipHoldCounter > count) ? (mClipHoldCounter - count) : 0;
      if (mClipHoldCounter == 0)
        mClipIndicator = false;
    }
    else
    {
      mClipIndicator = false;
    }

    if (rectifiedSample >= mCurrentPeak)
    {
      mCurrentPeak = rectifiedSample;
      mPeakHoldCounter = mPeakHoldSamples;
      return;
    }

    int remaining = count;
    if (mPeakHoldCounter > 0)
    {
      if (mPeakHoldCounter >= remaining)
      {
        mPeakHoldCounter -= remaining;
        return;
      }

      remaining -= mPeakHoldCounter;
      mPeakHoldCounter = 0;
    }

    if (remaining > 0 && mCurrentPeak > 0)
    {
      mCurrentPeak -= mPeakFalloffPerSample * remaining;
      if (mCurrentPeak < 0)
        mCurrentPeak = 0;
    }
  }
};

#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT


}  // namespace WaveSabreCore
