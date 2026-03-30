#pragma once

#include "Maj7Basic.hpp"
#include "PeakDetector.hpp"
#include "RMS.hpp"

namespace WaveSabreCore
{
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT

// frequency-dependent peak detector for spectrum display
// Uses different falloff rates per frequency band
struct FrequencyDependentPeakDetector : IPeakDetector
{
  int mClipHoldSamples;
  int mPeakHoldSamples;
  double mBaseFalloffMultiplierPerSample;  // Base exponential falloff multiplier
  float mFrequency;                        // The frequency this detector represents

  // Frequency-dependent falloff model parameters
  static constexpr float kLowFreqThreshold = 200.0f;    // Below this = slow falloff
  static constexpr float kHighFreqThreshold = 2000.0f;  // Above this = fast falloff
  static constexpr float kLowFreqFactor = 0.3f;         // Low freq falloff multiplier (slower)
  static constexpr float kMidFreqFactor = 1.0f;         // Mid freq falloff multiplier (baseline)
  static constexpr float kHighFreqFactor = 2.5f;        // High freq falloff multiplier (faster)

  // state
  int mClipHoldCounter = 0;
  int mPeakHoldCounter = 0;

  // running values
  bool mClipIndicator = 0;
  double mCurrentPeak = 0;

  void SetParams(double clipHoldMS, double peakHoldMS, double peakFalloffMaxMS, float frequency)
  {
    // Preserve current state so changing params doesn't hard-reset visuals
    int prevClipHoldCounter = mClipHoldCounter;
    int prevPeakHoldCounter = mPeakHoldCounter;
    double prevCurrentPeak = mCurrentPeak;
    bool prevClipIndicator = mClipIndicator;

    mClipHoldSamples = int(clipHoldMS * Helpers::CurrentSampleRateF / 1000);
    mPeakHoldSamples = int(peakHoldMS * Helpers::CurrentSampleRateF / 1000);
    mFrequency = frequency;

    // Calculate base falloff multiplier (60dB falloff)
    double falloffSamples = peakFalloffMaxMS * Helpers::CurrentSampleRateF / 1000.0;
    if (falloffSamples < 1.0)
      falloffSamples = 1.0;  // guard
    const double dBFalloffRange = 60.0;
    double linearFalloffRatio = std::pow(10.0, -dBFalloffRange / 20.0);  // 10^(-60/20) = 0.001

    // Apply frequency-dependent scaling
    float frequencyFactor = CalculateFrequencyFactor(frequency);
    double adjustedFalloffSamples = falloffSamples /
                                    std::max(0.0001, (double)frequencyFactor);  // Faster = fewer samples
    if (adjustedFalloffSamples < 1.0)
      adjustedFalloffSamples = 1.0;  // guard

    mBaseFalloffMultiplierPerSample = std::pow(linearFalloffRatio, 1.0 / adjustedFalloffSamples);

    // Preserve counters and peak where possible under new constraints
    mClipIndicator = prevClipIndicator;
    mCurrentPeak = prevCurrentPeak;
    mClipHoldCounter = std::min(prevClipHoldCounter, mClipHoldSamples);
    mPeakHoldCounter = std::min(prevPeakHoldCounter, mPeakHoldSamples);
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
    double rectifiedSample = fabs(s);

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
    if (rectifiedSample > mCurrentPeak)
    {
      mCurrentPeak = rectifiedSample;
      mPeakHoldCounter = mPeakHoldSamples;
    }
    else if (mPeakHoldCounter > 0)
    {
      --mPeakHoldCounter;
    }
    else if (mCurrentPeak > 0)
    {
      // Frequency-dependent exponential falloff
      mCurrentPeak *= mBaseFalloffMultiplierPerSample;

      if (mCurrentPeak < 1e-10)
        mCurrentPeak = 0;
    }
  }

  // Multi-step processing to simulate N identical samples efficiently
  void ProcessSampleMulti(double s, int count)
  {
    double rectifiedSample = fabs(s);

    // Clip detection bulk update
    if (rectifiedSample >= 1)
    {
      mClipIndicator = true;
      mClipHoldCounter = mClipHoldSamples;  // reset to full
    }
    else
    {
      if (mClipHoldCounter > 0)
      {
        mClipHoldCounter = (mClipHoldCounter > count) ? (mClipHoldCounter - count) : 0;
        if (mClipHoldCounter == 0)
          mClipIndicator = false;
      }
    }

    // Peak detection and hold/falloff in bulk
    if (rectifiedSample > mCurrentPeak)
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
        return;  // all within hold period
      }
      else
      {
        remaining -= mPeakHoldCounter;
        mPeakHoldCounter = 0;  // hold exhausted; fall through to apply falloff for remainder
      }
    }

    if (remaining > 0 && mCurrentPeak > 0)
    {
      double mult = std::pow(mBaseFalloffMultiplierPerSample, (double)remaining);
      mCurrentPeak *= mult;
      if (mCurrentPeak < 1e-10)
        mCurrentPeak = 0;
    }
  }

private:
  // frequency-dependent falloff model
  static float CalculateFrequencyFactor(float frequency)
  {
    // Clamp frequency to reasonable range
    frequency = std::max(20.0f, std::min(20000.0f, frequency));

    if (frequency < kLowFreqThreshold)
    {
      // Low frequencies: Slow falloff (bass persistence)
      // Linear interpolation from 20Hz to 200Hz
      float t = (frequency - 20.0f) / (kLowFreqThreshold - 20.0f);
      return kLowFreqFactor * (1.0f - t) + kMidFreqFactor * t;
    }
    else if (frequency < kHighFreqThreshold)
    {
      // Mid frequencies: Baseline falloff
      // Linear interpolation from 200Hz to 2000Hz
      float t = (frequency - kLowFreqThreshold) / (kHighFreqThreshold - kLowFreqThreshold);
      return kMidFreqFactor * (1.0f - t) + kMidFreqFactor * t;  // Flat in mid range
    }
    else
    {
      // High frequencies: Fast falloff (transient detail)
      // Exponential curve from 2000Hz to 20000Hz for realistic high-freq behavior
      float t = (frequency - kHighFreqThreshold) / (20000.0f - kHighFreqThreshold);
      float exponentialT = 1.0f - std::exp(-3.0f * t);  // Smooth exponential rise
      return kMidFreqFactor * (1.0f - exponentialT) + kHighFreqFactor * exponentialT;
    }
  }
};
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT


}  // namespace WaveSabreCore
