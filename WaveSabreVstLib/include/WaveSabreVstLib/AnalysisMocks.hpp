#pragma once

#ifndef SELECTABLE_OUTPUT_STREAM_SUPPORT

#include <vector>

// for optimized builds, output streams are unavailable. mock them for VSTs here.

    struct SpectrumBin
    {
        float frequency;    // Hz
        float magnitudeDB;  // dB
        //float phase;        // radians (if needed)
    };
    // Common interface for frequency analysis data providers
    class IFrequencyAnalysis
    {
    public:
        
        virtual ~IFrequencyAnalysis() = default;
        
        // Get spectrum data for rendering
        virtual const std::vector<SpectrumBin>& GetSpectrum() const = 0;
        
        // Get magnitude at specific frequency (interpolated if necessary)
        virtual float GetMagnitudeAtFrequency(float frequency) const = 0;
        
        // Get the frequency resolution (Hz per bin)
        virtual float GetFrequencyResolution() const = 0;
        
        // Get the maximum frequency represented
        virtual float GetNyquistFrequency() const = 0;
    };

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
  explicit AnalysisStream(double peakFalloffMS = 1200)
  {
  }

  void WriteSample(double s)
  {
  }

  virtual void Reset() override
  {
  }
};


struct MonoFFTAnalysis
{
  enum class FFTSize : int
  {
    FFT512 = 512,
    FFT1024 = 1024,
    FFT2048 = 2048,
    FFT4096 = 4096
  };

  enum class WindowType
  {
    Rectangular,
    Hanning,
    Hamming,
    Blackman
  };
};


// Stereo imaging analysis stream for real-time stereo field visualization
struct StereoImagingAnalysisStream
{
  //RMSDetector mLeftLevelDetector;   // *** LEFT CHANNEL RMS ***
  //RMSDetector mRightLevelDetector;  // *** RIGHT CHANNEL RMS ***
  //RMSDetector mWidthDetector;       // For stereo width RMS

  AnalysisStream mMidLevelDetector;
  AnalysisStream mSideLevelDetector;

  double mPhaseCorrelation = 0.0;
  double mStereoWidth = 0.0;
  double mStereoBalance = 0.0;
  //double mLeftLevel = 0.0;
  //double mRightLevel = 0.0;
  //static constexpr double gSmoothingFactor = 0.8;

  static constexpr size_t gHistorySize = 768;  // 256;
  struct StereoSample
  {
    float left = 0.0f;
    float right = 0.0f;
  };
  StereoSample mHistory[gHistorySize];
  size_t mHistoryIndex = 0;

  //double mLeftSum = 0.0;
  //double mRightSum = 0.0;
  //double mLeftSquaredSum = 0.0;
  //double mRightSquaredSum = 0.0;
  //double mLRProductSum = 0.0;
  //size_t mCorrelationSampleCount = 0;
  //static constexpr size_t gCorrelationWindowSize = 4800;

  //std::unique_ptr<MidSideFrequencyAnalyzer> mFrequencyAnalyzer;

  //explicit StereoImagingAnalysisStream()
  //{
  //  mLeftLevelDetector.SetWindowSize(200);
  //  mRightLevelDetector.SetWindowSize(200);
  //  mWidthDetector.SetWindowSize(200);
  //  mFrequencyAnalyzer = std::make_unique<MidSideFrequencyAnalyzer>();
  //  Reset();
  //}

  //inline void WriteStereoSample(double left, double right)
  //{
  //  mHistory[mHistoryIndex].left = static_cast<float>(left);
  //  mHistory[mHistoryIndex].right = static_cast<float>(right);
  //  mHistoryIndex = (mHistoryIndex + 1) % gHistorySize;

  //  mLeftLevel = mLeftLevelDetector.ProcessSample(left);
  //  mRightLevel = mRightLevelDetector.ProcessSample(right);

  //  double mid = (left + right) * 0.5;
  //  double side = (left - right) * 0.5;

  //  mMidLevelDetector.WriteSample(mid);
  //  mSideLevelDetector.WriteSample(side);

  //  if (mFrequencyAnalyzer && mFrequencyAnalyzer->IsEnabled())
  //  {
  //    mFrequencyAnalyzer->ProcessMidSideSamples(static_cast<float>(mid), static_cast<float>(side));
  //  }

  //  // Keep original gating – only update when mid peak is sufficiently strong
  //  //const double instantWidth = StereoWidthUtil::InstantWidthFromPeaks(
  //  //    mMidLevelDetector.mCurrentPeak,
  //  //    mSideLevelDetector.mCurrentPeak
  //  //);
  //  const double instantWidth = StereoWidthUtil::WidthFromLinearWithFloor((float)mMidLevelDetector.mCurrentRMSValue,
  //                                                                        (float)mSideLevelDetector.mCurrentRMSValue,
  //                                                                        -120.0f  // noise floor in dB
  //                                                                        )
  //                                  .width;  // Use width result directly
  //  mStereoWidth = mWidthDetector.ProcessSample(instantWidth);

  //  double totalLevel = mLeftLevel + mRightLevel;
  //  if (totalLevel > 0.0001)
  //  {
  //    mStereoBalance = (mRightLevel - mLeftLevel) / totalLevel;
  //  }
  //  else
  //  {
  //    mStereoBalance = 0.0;  // Reset balance if no valid levels
  //  }

  //  mLeftSum += left;
  //  mRightSum += right;
  //  mLeftSquaredSum += left * left;
  //  mRightSquaredSum += right * right;
  //  mLRProductSum += left * right;
  //  mCorrelationSampleCount++;
  //  if (mCorrelationSampleCount >= gCorrelationWindowSize)
  //  {
  //    UpdateCorrelation();
  //    mLeftSum *= 0.5;
  //    mRightSum *= 0.5;
  //    mLeftSquaredSum *= 0.5;
  //    mRightSquaredSum *= 0.5;
  //    mLRProductSum *= 0.5;
  //    mCorrelationSampleCount /= 2;
  //  }
  //}

  //inline void Reset()
  //{
  //  mPhaseCorrelation = 0.0;
  //  mStereoWidth = 0.0;
  //  mStereoBalance = 0.0;
  //  mLeftLevel = 0.0;
  //  mRightLevel = 0.0;
  //  mHistoryIndex = 0;
  //  mLeftLevelDetector.Reset();
  //  mRightLevelDetector.Reset();
  //  mMidLevelDetector.Reset();
  //  mSideLevelDetector.Reset();
  //  mWidthDetector.Reset();
  //  if (mFrequencyAnalyzer)
  //    mFrequencyAnalyzer->Reset();
  //  for (auto& s : mHistory)
  //  {
  //    s.left = s.right = 0.0f;
  //  }
  //  mLeftSum = mRightSum = 0.0;
  //  mLeftSquaredSum = mRightSquaredSum = 0.0;
  //  mLRProductSum = 0.0;
  //  mCorrelationSampleCount = 0;
  //}

  //inline void SetFrequencyAnalysisEnabled(bool enabled)
  //{
  //  if (mFrequencyAnalyzer)
  //    mFrequencyAnalyzer->SetEnabled(enabled);
  //}
  //bool IsFrequencyAnalysisEnabled() const
  //{
  //  return mFrequencyAnalyzer && mFrequencyAnalyzer->IsEnabled();
  //}
  //inline void ConfigureFrequencyAnalysis(MonoFFTAnalysis::FFTSize fftSize = MonoFFTAnalysis::FFTSize::FFT1024,
  //                                       MonoFFTAnalysis::WindowType windowType = MonoFFTAnalysis::WindowType::Hanning,
  //                                       float peakHoldTimeMs = 60.0f,
  //                                       float falloffTimeMs = 1200.0f)
  //{
  //  if (mFrequencyAnalyzer)
  //  {
  //    mFrequencyAnalyzer->SetFFTConfiguration(fftSize, windowType);
  //    mFrequencyAnalyzer->SetSmoothingParameters(peakHoldTimeMs, falloffTimeMs);
  //  }
  //}
  //const MidSideFrequencyAnalyzer* GetFrequencyAnalyzer() const
  //{
  //  return mFrequencyAnalyzer.get();
  //}
  const StereoSample* GetHistoryBuffer() const
  {
    return mHistory;
  }
  size_t GetHistorySize() const
  {
    return gHistorySize;
  }
  size_t GetCurrentHistoryIndex() const
  {
    return mHistoryIndex;
  }

}; // StereoImagingAnalysisStream


#endif