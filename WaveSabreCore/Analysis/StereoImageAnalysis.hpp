#pragma once

//#include "Device.h"

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
#include <atomic>
#include <cmath>
#include <complex>
#include <mutex>
#include <vector>

  #include "FFTAnalysis.hpp"
  #include "RMS.hpp"  // for base analysis stream stuff

namespace WaveSabreCore
{
// Reusable stereo-width helpers (no behavior changes)
struct StereoWidthUtil
{
  // Convert dB (amplitude) to linear amplitude squared (power)
  static inline float PowFromDb(float db)
  {
    const float amp = M7::math::DecibelsToLinear(db);
    return amp * amp;
  }

  // Compute width from mid/side powers (0..1): side / (mid + side). 0 if both are 0
  static inline float WidthFromPowers(float midPow, float sidePow)
  {
    const float total = midPow + sidePow;
    return (total > 0.0f) ? (sidePow / total) : 0.0f;
  }

  // Result of width computation with a simple energy floor
  struct WidthResult
  {
    float width;      // width in [0..1] or a tiny placeholder value when below floor
    bool belowFloor;  // whether total energy is below the floor
  };

  // Compute width from mid/side amplitudes in dB with a power floor.
  // Behavior mirrors the existing implementation:
  // - If total power < noiseFloorPow => treat as "mono near silence" and return tiny placeholder (noiseFloorAmp).
  // - Else width = side / (mid + side) using powers.
  static inline WidthResult WidthFromDbWithFloor(float midDb, float sideDb, float noiseFloorDb /* e.g. -120 dB */)
  {
    const float midPow = PowFromDb(midDb);
    const float sidePow = PowFromDb(sideDb);
    const float totalPow = midPow + sidePow;

    const float floorAmp = M7::math::DecibelsToLinear(noiseFloorDb);
    const float floorPow = floorAmp * floorAmp;

    if (totalPow < floorPow)
    {
      return {floorAmp, true};  // tiny placeholder to avoid later GUI mapping issues
    }

    return {WidthFromPowers(midPow, sidePow), false};
  }

  // Width from mid/side linear RMS (amplitude) with noise-floor guard
  static inline WidthResult WidthFromLinearWithFloor(float midAmp, float sideAmp, float noiseFloorDb)
  {
    const float midPow = midAmp * midAmp;
    const float sidePow = sideAmp * sideAmp;
    const float totalPow = midPow + sidePow;

    const float floorAmp = M7::math::DecibelsToLinear(noiseFloorDb);
    const float floorPow = floorAmp * floorAmp;

    if (totalPow < floorPow)
      return {floorAmp, true};

    return {WidthFromPowers(midPow, sidePow), false};
  }

  // Instant width from peaks (side/mid), clamped via epsilon to avoid div-by-zero
  //     static inline double InstantWidthFromPeaks(double midPeak, double sidePeak, double eps = 1e-8)
  //     {
  //if (midPeak < eps && sidePeak < eps) return 0.0; // both zero, return zero width
  //         return (midPeak > eps) ? (sidePeak / midPeak) : 0.0;
  //     }
};

// Mid-Side frequency analyzer for stereo width analysis by frequency
class MidSideFrequencyAnalyzer : public IFrequencyAnalysis
{
private:
  std::unique_ptr<SmoothedMonoFFT> mMidAnalyzer;
  std::unique_ptr<SmoothedMonoFFT> mSideAnalyzer;
  mutable std::vector<SpectrumBin>
      mWidthSpectrum;  // Computed width spectrum (Side^2 / (Mid^2+Side^2)) - mutable for lazy evaluation

  bool mEnabled = false;

  // Thread-safe lazy evaluation system
  mutable std::mutex mSpectrumMutex;                   // Protects mWidthSpectrum
  mutable std::atomic<bool> mNeedsWidthUpdate{false};  // Audio thread sets, GUI thread clears
  mutable std::mutex mAnalyzerMutex;                   // protects mMidAnalyzer/mSideAnalyzer and their access

  // Shared setting used in current implementation
  static constexpr float kNoiseFloorDB = -120.0f;

public:
  MidSideFrequencyAnalyzer() = default;
  ~MidSideFrequencyAnalyzer() = default;

  // Configuration
  inline void SetEnabled(bool enabled)
  {
    // Lock analyzers while changing
    std::lock_guard<std::mutex> aLock(mAnalyzerMutex);
    // Lock spectrum buffer while changing
    std::lock_guard<std::mutex> sLock(mSpectrumMutex);

    if (enabled && !mEnabled)
    {
      mMidAnalyzer = std::make_unique<SmoothedMonoFFT>();
      mSideAnalyzer = std::make_unique<SmoothedMonoFFT>();

      mMidAnalyzer->SetSampleRate(Helpers::CurrentSampleRateF);
      mSideAnalyzer->SetSampleRate(Helpers::CurrentSampleRateF);

      mMidAnalyzer->SetFFTSize(MonoFFTAnalysis::FFTSize::Default);
      mSideAnalyzer->SetFFTSize(MonoFFTAnalysis::FFTSize::Default);

      mMidAnalyzer->SetFFTSmoothing(0.3f);
      mSideAnalyzer->SetFFTSmoothing(0.3f);
      mMidAnalyzer->SetOverlapFactor(4);
      mSideAnalyzer->SetOverlapFactor(4);

      mMidAnalyzer->SetPeakHoldTime(100);
      mSideAnalyzer->SetPeakHoldTime(100);

      mMidAnalyzer->SetFalloffRate(3000.0f);
      mSideAnalyzer->SetFalloffRate(3000.0f);
    }
    else if (!enabled && mEnabled)
    {
      // release analyzers safely
      mMidAnalyzer.reset();
      mSideAnalyzer.reset();
      mWidthSpectrum.clear();
    }

    mEnabled = enabled;
    mNeedsWidthUpdate.store(false, std::memory_order_release);
  }

  bool IsEnabled() const
  {
    return mEnabled;
  }

  inline void SetSampleRate(float sampleRate)
  {
    std::lock_guard<std::mutex> aLock(mAnalyzerMutex);
    if (mMidAnalyzer)
      mMidAnalyzer->SetSampleRate(sampleRate);
    if (mSideAnalyzer)
      mSideAnalyzer->SetSampleRate(sampleRate);
  }

  inline void SetFFTConfiguration(MonoFFTAnalysis::FFTSize /*fftSize*/, MonoFFTAnalysis::WindowType windowType)
  {
    std::lock_guard<std::mutex> aLock(mAnalyzerMutex);
    if (mMidAnalyzer)
      mMidAnalyzer->SetWindowType(windowType);
    if (mSideAnalyzer)
      mSideAnalyzer->SetWindowType(windowType);
  }

  inline void SetSmoothingParameters(float peakHoldTimeMs, float falloffTimeMs)
  {
    std::lock_guard<std::mutex> aLock(mAnalyzerMutex);
    if (mMidAnalyzer)
    {
      mMidAnalyzer->SetPeakHoldTime(peakHoldTimeMs);
      mMidAnalyzer->SetFalloffRate(falloffTimeMs);
    }
    if (mSideAnalyzer)
    {
      mSideAnalyzer->SetPeakHoldTime(peakHoldTimeMs);
      mSideAnalyzer->SetFalloffRate(falloffTimeMs);
    }
  }

  // Processing
  // AUDIO THREAD
  inline void ProcessMidSideSamples(float midSample, float sideSample)
  {
    if (!mEnabled)
      return;
    std::lock_guard<std::mutex> aLock(mAnalyzerMutex);
    if (!mMidAnalyzer || !mSideAnalyzer)
      return;

    mMidAnalyzer->ProcessSample(midSample);
    mSideAnalyzer->ProcessSample(sideSample);

    // Only publish a new width spectrum when BOTH mid and side have produced a new frame.
    // This keeps frames aligned and avoids width beating from mixed-age spectra.
    const bool midNew = mMidAnalyzer->HasNewOutput();
    const bool sideNew = mSideAnalyzer->HasNewOutput();
    if (midNew && sideNew)
    {
      mNeedsWidthUpdate.store(true, std::memory_order_release);
      mMidAnalyzer->ConsumeOutput();
      mSideAnalyzer->ConsumeOutput();
    }
  }

  inline void Reset()
  {
    std::lock_guard<std::mutex> aLock(mAnalyzerMutex);
    std::lock_guard<std::mutex> sLock(mSpectrumMutex);
    if (mMidAnalyzer)
      mMidAnalyzer->Reset();
    if (mSideAnalyzer)
      mSideAnalyzer->Reset();
    mWidthSpectrum.clear();
    mNeedsWidthUpdate.store(false, std::memory_order_release);
  }

  // IFrequencyAnalysis implementation - THREAD-SAFE
  const std::vector<SpectrumBin>& GetSpectrum() const override
  {
    UpdateWidthSpectrumIfNeeded();
    return mWidthSpectrum;
  }

  inline float GetMagnitudeAtFrequency(float frequency) const override
  {
    UpdateWidthSpectrumIfNeeded();
    std::lock_guard<std::mutex> lock(mSpectrumMutex);
    return GetMagnitudeAtFrequencyFromSpectrum(mWidthSpectrum, frequency);
  }

  float GetFrequencyResolution() const override
  {
    std::lock_guard<std::mutex> aLock(mAnalyzerMutex);
    if (mMidAnalyzer)
    {
      return mMidAnalyzer->GetFrequencyResolution();
    }
    return 0.0f;
  }

  float GetNyquistFrequency() const override
  {
    std::lock_guard<std::mutex> aLock(mAnalyzerMutex);
    if (mMidAnalyzer)
    {
      return mMidAnalyzer->GetNyquistFrequency();
    }
    return 24000.0f;  // Fallback
  }

  // Access individual spectra for advanced visualization
  const IFrequencyAnalysis* GetMidAnalyzer() const
  {
    std::lock_guard<std::mutex> aLock(mAnalyzerMutex);
    return mMidAnalyzer.get();
  }
  const IFrequencyAnalysis* GetSideAnalyzer() const
  {
    std::lock_guard<std::mutex> aLock(mAnalyzerMutex);
    return mSideAnalyzer.get();
  }

private:
  inline float GetMagnitudeAtFrequencyFromSpectrum(const std::vector<SpectrumBin>& spectrum, float frequency) const
  {
    if (spectrum.empty() || frequency <= 0.0f || frequency >= GetNyquistFrequency())
    {
      return 0.0f;
    }
    const float frequencyResolution = GetFrequencyResolution();
    if (frequencyResolution <= 0.0f)
      return 0.0f;
    const float binIndex = frequency / frequencyResolution;
    const int lowerBin = static_cast<int>(std::floor(binIndex));
    const int upperBin = std::min(lowerBin + 1, static_cast<int>(spectrum.size()) - 1);
    if (lowerBin >= static_cast<int>(spectrum.size()) || lowerBin < 0)
    {
      return 0.0f;
    }
    if (lowerBin == upperBin)
    {
      return spectrum[lowerBin].magnitudeDB;
    }
    const float fraction = binIndex - lowerBin;
    return spectrum[lowerBin].magnitudeDB * (1.0f - fraction) + spectrum[upperBin].magnitudeDB * fraction;
  }

  // GUI THREAD
  void UpdateWidthSpectrumIfNeeded() const
  {
    if (!mNeedsWidthUpdate.load(std::memory_order_acquire))
      return;
    std::lock_guard<std::mutex> aLock(mAnalyzerMutex);
    if (!mMidAnalyzer || !mSideAnalyzer)
      return;

    // Collect current spectra from analyzers (thread-safe by analyzer mutex)
    const auto& midSpectrum = mMidAnalyzer->GetSpectrum();
    const auto& sideSpectrum = mSideAnalyzer->GetSpectrum();
    if (midSpectrum.empty() || sideSpectrum.empty())
      return;

    size_t spectrumSize = std::min(midSpectrum.size(), sideSpectrum.size());
    std::lock_guard<std::mutex> sLock(mSpectrumMutex);
    if (mWidthSpectrum.size() != spectrumSize)
      mWidthSpectrum.resize(spectrumSize);

    for (size_t i = 0; i < spectrumSize; ++i)
    {
      mWidthSpectrum[i].frequency = midSpectrum[i].frequency;

      const float midDb = midSpectrum[i].magnitudeDB;
      const float sideDb = sideSpectrum[i].magnitudeDB;

      // Use shared helper with same noise-floor behavior
      const auto wr = StereoWidthUtil::WidthFromDbWithFloor(midDb, sideDb, kNoiseFloorDB);

      // magnitudeDB stores width [0..1] or tiny placeholder when below energy floor
      mWidthSpectrum[i].magnitudeDB = wr.width;
    }
    mNeedsWidthUpdate.store(false, std::memory_order_release);
  }

  bool HasNewSpectrum() const
  {
    return mNeedsWidthUpdate.load(std::memory_order_acquire);
  }
  void ConsumeSpectrum()
  {
    mNeedsWidthUpdate.store(false, std::memory_order_release);
  }
};

// Stereo imaging analysis stream for real-time stereo field visualization
struct StereoImagingAnalysisStream
{
  RMSDetector mLeftLevelDetector;   // *** LEFT CHANNEL RMS ***
  RMSDetector mRightLevelDetector;  // *** RIGHT CHANNEL RMS ***
  RMSDetector mWidthDetector;       // For stereo width RMS

  AnalysisStream mMidLevelDetector;
  AnalysisStream mSideLevelDetector;

  double mPhaseCorrelation = 0.0;
  double mStereoWidth = 0.0;
  double mStereoBalance = 0.0;
  double mLeftLevel = 0.0;
  double mRightLevel = 0.0;
  static constexpr double gSmoothingFactor = 0.8;

  static constexpr size_t gHistorySize = 768;  // 256;
  struct StereoSample
  {
    float left = 0.0f;
    float right = 0.0f;
  };
  StereoSample mHistory[gHistorySize];
  size_t mHistoryIndex = 0;

  double mLeftSum = 0.0;
  double mRightSum = 0.0;
  double mLeftSquaredSum = 0.0;
  double mRightSquaredSum = 0.0;
  double mLRProductSum = 0.0;
  size_t mCorrelationSampleCount = 0;
  static constexpr size_t gCorrelationWindowSize = 4800;

  std::unique_ptr<MidSideFrequencyAnalyzer> mFrequencyAnalyzer;

  explicit StereoImagingAnalysisStream()
  {
    mLeftLevelDetector.SetWindowSize(200);
    mRightLevelDetector.SetWindowSize(200);
    mWidthDetector.SetWindowSize(200);
    mFrequencyAnalyzer = std::make_unique<MidSideFrequencyAnalyzer>();
    Reset();
  }

  inline void WriteStereoSample(double left, double right)
  {
    mHistory[mHistoryIndex].left = static_cast<float>(left);
    mHistory[mHistoryIndex].right = static_cast<float>(right);
    mHistoryIndex = (mHistoryIndex + 1) % gHistorySize;

    mLeftLevel = mLeftLevelDetector.ProcessSample(left);
    mRightLevel = mRightLevelDetector.ProcessSample(right);

    double mid = (left + right) * 0.5;
    double side = (left - right) * 0.5;

    mMidLevelDetector.WriteSample(mid);
    mSideLevelDetector.WriteSample(side);

    if (mFrequencyAnalyzer && mFrequencyAnalyzer->IsEnabled())
    {
      mFrequencyAnalyzer->ProcessMidSideSamples(static_cast<float>(mid), static_cast<float>(side));
    }

    // Keep original gating � only update when mid peak is sufficiently strong
    //const double instantWidth = StereoWidthUtil::InstantWidthFromPeaks(
    //    mMidLevelDetector.mCurrentPeak,
    //    mSideLevelDetector.mCurrentPeak
    //);
    const double instantWidth = StereoWidthUtil::WidthFromLinearWithFloor((float)mMidLevelDetector.mCurrentRMSValue,
                                                                          (float)mSideLevelDetector.mCurrentRMSValue,
                                                                          -120.0f  // noise floor in dB
                                                                          )
                                    .width;  // Use width result directly
    mStereoWidth = mWidthDetector.ProcessSample(instantWidth);

    double totalLevel = mLeftLevel + mRightLevel;
    if (totalLevel > 0.0001)
    {
      mStereoBalance = (mRightLevel - mLeftLevel) / totalLevel;
    }
    else
    {
      mStereoBalance = 0.0;  // Reset balance if no valid levels
    }

    mLeftSum += left;
    mRightSum += right;
    mLeftSquaredSum += left * left;
    mRightSquaredSum += right * right;
    mLRProductSum += left * right;
    mCorrelationSampleCount++;
    if (mCorrelationSampleCount >= gCorrelationWindowSize)
    {
      UpdateCorrelation();
      mLeftSum *= 0.5;
      mRightSum *= 0.5;
      mLeftSquaredSum *= 0.5;
      mRightSquaredSum *= 0.5;
      mLRProductSum *= 0.5;
      mCorrelationSampleCount /= 2;
    }
  }

  inline void Reset()
  {
    mPhaseCorrelation = 0.0;
    mStereoWidth = 0.0;
    mStereoBalance = 0.0;
    mLeftLevel = 0.0;
    mRightLevel = 0.0;
    mHistoryIndex = 0;
    mLeftLevelDetector.Reset();
    mRightLevelDetector.Reset();
    mMidLevelDetector.Reset();
    mSideLevelDetector.Reset();
    mWidthDetector.Reset();
    if (mFrequencyAnalyzer)
      mFrequencyAnalyzer->Reset();
    for (auto& s : mHistory)
    {
      s.left = s.right = 0.0f;
    }
    mLeftSum = mRightSum = 0.0;
    mLeftSquaredSum = mRightSquaredSum = 0.0;
    mLRProductSum = 0.0;
    mCorrelationSampleCount = 0;
  }

  inline void SetFrequencyAnalysisEnabled(bool enabled)
  {
    if (mFrequencyAnalyzer)
      mFrequencyAnalyzer->SetEnabled(enabled);
  }
  bool IsFrequencyAnalysisEnabled() const
  {
    return mFrequencyAnalyzer && mFrequencyAnalyzer->IsEnabled();
  }
  inline void ConfigureFrequencyAnalysis(MonoFFTAnalysis::FFTSize fftSize = MonoFFTAnalysis::FFTSize::Default,
                                         MonoFFTAnalysis::WindowType windowType = MonoFFTAnalysis::WindowType::Hanning,
                                         float peakHoldTimeMs = 60.0f,
                                         float falloffTimeMs = 1200.0f)
  {
    if (mFrequencyAnalyzer)
    {
      mFrequencyAnalyzer->SetFFTConfiguration(fftSize, windowType);
      mFrequencyAnalyzer->SetSmoothingParameters(peakHoldTimeMs, falloffTimeMs);
    }
  }
  const MidSideFrequencyAnalyzer* GetFrequencyAnalyzer() const
  {
    return mFrequencyAnalyzer.get();
  }
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

private:
  inline void UpdateCorrelation()
  {
    if (mCorrelationSampleCount < 2)
    {
      mPhaseCorrelation = 0.0;
      return;
    }
    double n = static_cast<double>(mCorrelationSampleCount);
    double leftMean = mLeftSum / n;
    double rightMean = mRightSum / n;
    double leftVariance = (mLeftSquaredSum / n) - (leftMean * leftMean);
    double rightVariance = (mRightSquaredSum / n) - (rightMean * rightMean);
    double covariance = (mLRProductSum / n) - (leftMean * rightMean);
    double denominator = std::sqrt(leftVariance * rightVariance);
    if (denominator > 0.0001)
    {
      double instantCorrelation = covariance / denominator;
      instantCorrelation = std::max(-1.0, std::min(1.0, instantCorrelation));
      mPhaseCorrelation = mPhaseCorrelation * gSmoothingFactor + instantCorrelation * (1.0 - gSmoothingFactor);
    }
    else
    {
      mPhaseCorrelation = 0.0;  // No valid correlation
    }
  }
}; // StereoImagingAnalysisStream


}  // namespace WaveSabreCore

#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
