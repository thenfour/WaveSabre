#pragma once

#include "../Basic/Helpers.h"

namespace WaveSabreCore
{
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT

struct PeakDetector
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
    mClipHoldSamples = int(clipHoldMS * Helpers::CurrentSampleRateF / 1000);
    mPeakHoldSamples = int(peakHoldMS * Helpers::CurrentSampleRateF / 1000);
    mPeakFalloffPerSample = std::max(0.0000001, 1.0 / (peakFalloffMaxMS * Helpers::CurrentSampleRateF / 1000));
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
};

#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT


}  // namespace WaveSabreCore
