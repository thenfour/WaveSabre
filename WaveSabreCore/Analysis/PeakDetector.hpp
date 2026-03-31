#pragma once

#include "../Basic/DSPMath.hpp"
#include "../Basic/Helpers.h"
#include <algorithm>
#include <deque>

namespace WaveSabreCore
{
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT

struct IPeakDetector
{
    virtual ~IPeakDetector() = default;
    virtual void SetParams(double clipHoldMS, double peakHoldMS, double peakFalloffMaxMS) = 0;
    virtual void Reset() = 0;
    virtual void ProcessSample(double s) = 0;
    virtual void ProcessSampleMulti(double s, int count) = 0;
    virtual void SetUseExponentialFalloff(bool enabled) = 0;
    virtual void SetAveragingWindowMS(double averagingWindowMS) = 0;
};

// basic peak detector; does not support averaging.
struct PeakDetector : public IPeakDetector
{
  int mClipHoldSamples;
  int mPeakHoldSamples;
  double mPeakFalloffPerSample;
  double mPeakFalloffMultiplierPerSample;
  bool mUseExponentialFalloff = false;

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

    const double falloffSamples = std::max(1.0f, M7::math::MillisecondsToSamples((float)peakFalloffMaxMS));
    mPeakFalloffPerSample = std::max(0.0000001, 1.0 / falloffSamples);
    mPeakFalloffMultiplierPerSample = std::pow(0.001, 1.0 / falloffSamples);
    Reset();
  }

  void SetUseExponentialFalloff(bool enabled)
  {
    mUseExponentialFalloff = enabled;
  }

  void SetAveragingWindowMS(double averagingWindowMS) { }

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
      if (mUseExponentialFalloff)
      {
        mCurrentPeak *= mPeakFalloffMultiplierPerSample;
        if (mCurrentPeak < 1e-10)
          mCurrentPeak = 0;
      }
      else
      {
        mCurrentPeak -= mPeakFalloffPerSample;  // falloff phase
        if (mCurrentPeak < 0)
          mCurrentPeak = 0;  // Ensure mCurrentPeak doesn't go below 0
      }
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
      if (mUseExponentialFalloff)
      {
        mCurrentPeak *= std::pow(mPeakFalloffMultiplierPerSample, (double)remaining);
        if (mCurrentPeak < 1e-10)
          mCurrentPeak = 0;
      }
      else
      {
        mCurrentPeak -= mPeakFalloffPerSample * remaining;
        if (mCurrentPeak < 0)
          mCurrentPeak = 0;
      }
    }
  }
};

// basically wraps PeakDetector with averaging over time, another smoothing mechanism  for  the FFT displays.
// unfortunately it doesn't smooth much more than the existing methods.
struct AveragingPeakDetector : public IPeakDetector
{
  struct AveragingChunk
  {
    double value;
    int sampleCount;
  };

  PeakDetector mPeakDetector;
  std::deque<AveragingChunk> mAveragingChunks;
  double mAveragingWindowMS = 1000.0;
  int mAveragingWindowSamples = 1;
  double mAveragingWeightedSum = 0;
  int mAveragingTotalSamples = 0;

  bool mClipIndicator = 0;
  double mCurrentPeak = 0;
  double mCurrentAverage = 0;

  AveragingPeakDetector()
  {
    UpdateAveragingWindowSamples();
  }

  void SetParams(double clipHoldMS, double peakHoldMS, double peakFalloffMaxMS) override
  {
    mPeakDetector.SetParams(clipHoldMS, peakHoldMS, peakFalloffMaxMS);
    SyncOutputs();
  }

  void SetUseExponentialFalloff(bool enabled)
  {
    mPeakDetector.SetUseExponentialFalloff(enabled);
    SyncOutputs();
  }

  void SetAveragingWindowMS(double averagingWindowMS)
  {
    mAveragingWindowMS = std::max(0.0, averagingWindowMS);
    UpdateAveragingWindowSamples();
    TrimAveragingWindow();
    UpdateAverageFromWindow();
    SyncOutputs();
  }

  void Reset() override
  {
    mPeakDetector.Reset();
    mAveragingChunks.clear();
    mAveragingWeightedSum = 0;
    mAveragingTotalSamples = 0;
    mCurrentAverage = 0;
    SyncOutputs();
  }

  void ProcessSample(double s) override
  {
    ProcessSampleMulti(s, 1);
  }

  void ProcessSampleMulti(double s, int count)
  {
    if (count <= 0)
    {
      return;
    }

    const double rectifiedSample = fabs(s);
    mAveragingChunks.push_back({ rectifiedSample, count });
    mAveragingWeightedSum += rectifiedSample * (double)count;
    mAveragingTotalSamples += count;

    TrimAveragingWindow();
    UpdateAverageFromWindow();

    mPeakDetector.ProcessSampleMulti(mCurrentAverage, count);
    SyncOutputs();
  }

private:
  void UpdateAveragingWindowSamples()
  {
    mAveragingWindowSamples = (int)std::max(1.0f, M7::math::MillisecondsToSamples((float)mAveragingWindowMS));
  }

  void TrimAveragingWindow()
  {
    while (!mAveragingChunks.empty() && mAveragingTotalSamples > mAveragingWindowSamples)
    {
      AveragingChunk& chunk = mAveragingChunks.front();
      const int excessSamples = mAveragingTotalSamples - mAveragingWindowSamples;
      const int trimmedSamples = std::min(chunk.sampleCount, excessSamples);

      mAveragingWeightedSum -= chunk.value * (double)trimmedSamples;
      mAveragingTotalSamples -= trimmedSamples;
      chunk.sampleCount -= trimmedSamples;

      if (chunk.sampleCount == 0)
      {
        mAveragingChunks.pop_front();
      }
    }
  }

  void UpdateAverageFromWindow()
  {
    if (mAveragingTotalSamples > 0)
    {
      mCurrentAverage = mAveragingWeightedSum / (double)mAveragingTotalSamples;
    }
    else
    {
      mCurrentAverage = 0;
    }
  }

  void SyncOutputs()
  {
    mClipIndicator = mPeakDetector.mClipIndicator;
    mCurrentPeak = mPeakDetector.mCurrentPeak;
  }
};

#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT


}  // namespace WaveSabreCore
