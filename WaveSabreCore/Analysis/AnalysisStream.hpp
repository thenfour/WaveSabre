#pragma once

#include "PeakDetector.hpp"
#include "RMS.hpp"

namespace WaveSabreCore
{
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT


struct IAnalysisStream
{
  double mCurrentRMSValue = 0;
  bool mClipIndicator = 0;
  double mCurrentPeak = 0;
  double mCurrentHeldPeak = 0;
  virtual void Reset() = 0;
};

// for VST-only, things like VU meters and history graphs require outputting many additional channels of audio.
// this class goes in the Device, feed it samples. the VST can then access them in a safe way.
struct AnalysisStream : IAnalysisStream
{
  RMSDetector mRMSDetector;
  PeakDetector mPeakDetector;
  PeakDetector mPeakHoldDetector;

  explicit AnalysisStream(double peakFalloffMS = 1200)
  {
    mRMSDetector.SetWindowSize(200);
    mPeakDetector.SetParams(0, 0, peakFalloffMS);
    mPeakHoldDetector.SetParams(1000, 1000, 600);
  }

  void WriteSample(double s)
  {
    mCurrentRMSValue = mRMSDetector.ProcessSample(s);
    mPeakDetector.ProcessSample(s);
    mPeakHoldDetector.ProcessSample(s);
    mClipIndicator = mPeakHoldDetector.mClipIndicator;
    mCurrentPeak = mPeakDetector.mCurrentPeak;
    mCurrentHeldPeak = mPeakHoldDetector.mCurrentPeak;
  }

  virtual void Reset() override
  {
    mRMSDetector.Reset();
    mPeakHoldDetector.Reset();
    mPeakDetector.Reset();
    mCurrentRMSValue = 0;
    mClipIndicator = 0;
    mCurrentPeak = 0;
    mCurrentHeldPeak = 0;
  }
};


// Attenuation/gain reduction analysis for compressor/clipper visualization
struct AttenuationAnalysisStream : IAnalysisStream
{
  PeakDetector mPeakDetector;
  PeakDetector mPeakHoldDetector;

  explicit AttenuationAnalysisStream(double peakFalloffMS = 1200)
  {
    mPeakDetector.SetParams(0, 0, peakFalloffMS);
    mPeakHoldDetector.SetParams(1000, 1000, peakFalloffMS);
  }

  void WriteSample(double s)
  {
    mPeakDetector.ProcessSample(s);
    mPeakHoldDetector.ProcessSample(s);
    mClipIndicator = mPeakHoldDetector.mClipIndicator;
    mCurrentPeak = mPeakDetector.mCurrentPeak;
    mCurrentHeldPeak = mPeakHoldDetector.mCurrentPeak;
  }

  virtual void Reset() override
  {
    mPeakHoldDetector.Reset();
    mPeakDetector.Reset();
    mClipIndicator = 0;
    mCurrentPeak = 0;
    mCurrentHeldPeak = 0;
  }
};

#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT


}  // namespace WaveSabreCore
