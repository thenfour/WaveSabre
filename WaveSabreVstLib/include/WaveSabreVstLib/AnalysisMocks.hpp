#pragma once

#ifndef SELECTABLE_OUTPUT_STREAM_SUPPORT

#include <vector>

// for optimized builds, output streams are unavailable. mock them for VSTs here.

namespace WaveSabreCore
{

#ifndef WAVESABRE_FREQUENCY_ANALYSIS_TYPES_DEFINED
#define WAVESABRE_FREQUENCY_ANALYSIS_TYPES_DEFINED

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

#endif  // WAVESABRE_FREQUENCY_ANALYSIS_TYPES_DEFINED

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
    FFT4096 = 4096,
    FFT8192 = 8192,
    FFT16384 = 16384,
    Default = FFT4096
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
  enum class BalanceBallisticsSpeed : int
  {
    Momentary,
    Fast,
    Medium,
    Slow,
    Section,
    Default = Fast,
  };

  AnalysisStream mMidLevelDetector;
  AnalysisStream mSideLevelDetector;

  double mPhaseCorrelation = 0.0;
  double mStereoWidth = 0.0;
  double mStereoBalance = 0.0;
  BalanceBallisticsSpeed mBalanceBallisticsSpeed = BalanceBallisticsSpeed::Default;

  static constexpr double GetBalanceBallisticsWindowMs(BalanceBallisticsSpeed speed)
  {
    switch (speed)
    {
      case BalanceBallisticsSpeed::Momentary:
        return 50.0;
      case BalanceBallisticsSpeed::Fast:
        return 200.0;
      case BalanceBallisticsSpeed::Medium:
        return 1000.0;
      case BalanceBallisticsSpeed::Slow:
        return 3000.0;
      case BalanceBallisticsSpeed::Section:
        return 10000.0;
    }

    return 200.0;
  }

  static constexpr const char* GetBalanceBallisticsSpeedLabel(BalanceBallisticsSpeed speed)
  {
    switch (speed)
    {
      case BalanceBallisticsSpeed::Momentary:
        return "Momentary";
      case BalanceBallisticsSpeed::Fast:
        return "Fast";
      case BalanceBallisticsSpeed::Medium:
        return "Medium";
      case BalanceBallisticsSpeed::Slow:
        return "Slow";
      case BalanceBallisticsSpeed::Section:
        return "Section";
    }

    return "Fast";
  }

  static constexpr const char* GetBalanceBallisticsSpeedDescription(BalanceBallisticsSpeed speed)
  {
    switch (speed)
    {
      case BalanceBallisticsSpeed::Momentary:
        return "50 ms - track quick left/right shifts.";
      case BalanceBallisticsSpeed::Fast:
        return "200 ms - responsive general-purpose balance view.";
      case BalanceBallisticsSpeed::Medium:
        return "1 s - smoother musical balance view.";
      case BalanceBallisticsSpeed::Slow:
        return "3 s - stable phrase-level energy balance.";
      case BalanceBallisticsSpeed::Section:
        return "10 s - very slow section-level energy balance.";
    }

    return "200 ms - responsive general-purpose balance view.";
  }

  BalanceBallisticsSpeed GetBalanceBallisticsSpeed() const
  {
    return mBalanceBallisticsSpeed;
  }

  void SetBalanceBallisticsSpeed(BalanceBallisticsSpeed speed)
  {
    mBalanceBallisticsSpeed = speed;
  }

  static constexpr size_t gHistorySize = 768;  // 256;
  struct StereoSample
  {
    float left = 0.0f;
    float right = 0.0f;
  };
  StereoSample mHistory[gHistorySize];
  size_t mHistoryIndex = 0;

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

}  // namespace WaveSabreCore


#endif