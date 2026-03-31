#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT

#include "FFTAnalysis.hpp"
#include "../Basic/DSPMath.hpp"

  //#include <WaveSabreCore/FFTAnalysis.hpp>
  //#include <WaveSabreCore/Helpers.h>
  //#include <WaveSabreCore/RMS.hpp>
  //#include <algorithm>
  //#include <cstring>

namespace WaveSabreCore
{
MonoFFTAnalysis::MonoFFTAnalysis(FFTSize fftSize, WindowType windowType, float sampleRate)
    : mFFTSize(fftSize)
    , mWindowType(windowType)
    , mFFTSizeInt(static_cast<int>(fftSize))
    , mSampleRate(sampleRate)
    , mOverlapFactor(4)
    , mSmoothingFactor(0.3f)  // Light technical smoothing only
    , mInputIndex(0)
    , mSamplesUntilProcess(mFFTSizeInt / mOverlapFactor)
    , mHasNewData(false)
//, mDisplayBoostDB(18.0f) // Default +18dB boost for visual appeal
{
  // Initialize buffers
  mInputBuffer.resize(mFFTSizeInt, 0.0f);
  mFFTBuffer.resize(mFFTSizeInt);

  int spectrumSize = mFFTSizeInt / 2;  // Only need positive frequencies (exclude Nyquist)
  mSpectrum.resize(spectrumSize);
  mMagnitudeHistory.resize(spectrumSize, -80.0f);

  GenerateWindow();
}

MonoFFTAnalysis::~MonoFFTAnalysis() {}

void MonoFFTAnalysis::SetSampleRate(float sampleRate)
{
  mSampleRate = sampleRate;
}

void MonoFFTAnalysis::SetSmoothingFactor(float smoothing)
{
  mSmoothingFactor = std::max(0.0f, std::min(0.99f, smoothing));
}

void MonoFFTAnalysis::SetOverlapFactor(int factor)
{
  mOverlapFactor = std::max(1, std::min(16, factor));  // Allow 1x overlap for completeness
  mSamplesUntilProcess = mFFTSizeInt / mOverlapFactor;
}


void MonoFFTAnalysis::SetWindowType(WindowType windowType)
{
  if (mWindowType != windowType)
  {
    mWindowType = windowType;
    GenerateWindow();  // Regenerate window coefficients
                       // Note: This takes effect immediately on next FFT frame
  }
}

void MonoFFTAnalysis::GenerateWindow()
{
  mWindow.resize(mFFTSizeInt);
  const float N = static_cast<float>(mFFTSizeInt - 1);

  for (int i = 0; i < mFFTSizeInt; ++i)
  {
    const float n = static_cast<float>(i);

    switch (mWindowType)
    {
      case WindowType::Rectangular:
        mWindow[i] = 1.0f;
        break;

      case WindowType::Hanning:
        mWindow[i] = 0.5f * (1.0f - std::cos(2.0f * 3.14159265f * n / N));
        break;

      case WindowType::Hamming:
        mWindow[i] = 0.54f - 0.46f * std::cos(2.0f * 3.14159265f * n / N);
        break;

      case WindowType::Blackman:
        mWindow[i] = 0.42f - 0.5f * std::cos(2.0f * 3.14159265f * n / N) + 0.08f * std::cos(4.0f * 3.14159265f * n / N);
        break;
    }
  }
}

void MonoFFTAnalysis::PerformFFT(std::vector<std::complex<float>>& buffer)
{
  // Simple decimation-in-time FFT implementation
  // Bit-reverse reordering
  int j = 0;
  for (int i = 1; i < mFFTSizeInt; ++i)
  {
    int bit = mFFTSizeInt >> 1;
    while (j & bit)
    {
      j ^= bit;
      bit >>= 1;
    }
    j ^= bit;
    if (i < j)
    {
      std::swap(buffer[i], buffer[j]);
    }
  }

  // FFT computation
  for (int length = 2; length <= mFFTSizeInt; length <<= 1)
  {
    const float angle = -2.0f * 3.14159265f / length;
    const std::complex<float> wlen(std::cos(angle), std::sin(angle));

    for (int i = 0; i < mFFTSizeInt; i += length)
    {
      std::complex<float> w(1.0f, 0.0f);
      for (int j = 0; j < length / 2; ++j)
      {
        const std::complex<float> u = buffer[i + j];
        const std::complex<float> v = buffer[i + j + length / 2] * w;
        buffer[i + j] = u + v;
        buffer[i + j + length / 2] = u - v;
        w *= wlen;
      }
    }
  }
}

void MonoFFTAnalysis::ComputeSpectrum()
{
  const float frequencyResolution = GetFrequencyResolution();
  const int spectrumSize = static_cast<int>(mSpectrum.size());

  for (int i = 0; i < spectrumSize; ++i)
  {
    // Calculate magnitude and convert to dB
    const float real = mFFTBuffer[i].real();
    const float imag = mFFTBuffer[i].imag();
    const float magnitude = std::sqrt(real * real + imag * imag);

    // Convert to dB with floor to prevent log(0), and apply display boost
    // todo: use a more sophisticated scaling
    const float magScaleFact = mFFTSizeInt * 0.5f;                                   // Scale factor for normalization
    const float magnitudeDB = M7::math::LinearToDecibels(magnitude / magScaleFact);  // magnitude > 1e-10f ?

    // Light technical smoothing only (no peak-hold here)
    mMagnitudeHistory[i] = mMagnitudeHistory[i] * mSmoothingFactor + magnitudeDB * (1.0f - mSmoothingFactor);

    // Store in spectrum bin
    mSpectrum[i].frequency = i * frequencyResolution;
    mSpectrum[i].magnitudeDB = mMagnitudeHistory[i];
    //mSpectrum[i].phase = std::atan2(imag, real);
  }
}

void MonoFFTAnalysis::ProcessSample(float sample)
{
  // Add sample to circular buffer
  mInputBuffer[mInputIndex] = sample;

  ++mInputIndex;
  if (mInputIndex >= mFFTSizeInt)
  {
    mInputIndex = 0;
  }
  --mSamplesUntilProcess;

  if (mSamplesUntilProcess <= 0)
  {
    // Time to process FFT
    mSamplesUntilProcess = mFFTSizeInt / mOverlapFactor;

    // Copy data to FFT buffer in correct order (oldest first)
    int dstIndex = 0;

    for (int srcIndex = mInputIndex; srcIndex < mFFTSizeInt; ++srcIndex, ++dstIndex)
    {
      mFFTBuffer[dstIndex] = std::complex<float>(mInputBuffer[srcIndex] * mWindow[dstIndex], 0.0f);
    }

    for (int srcIndex = 0; srcIndex < mInputIndex; ++srcIndex, ++dstIndex)
    {
      mFFTBuffer[dstIndex] = std::complex<float>(mInputBuffer[srcIndex] * mWindow[dstIndex], 0.0f);
    }

    // Perform FFT and compute spectrum
    PerformFFT(mFFTBuffer);
    ComputeSpectrum();

    mHasNewData = true;
  }
}

float MonoFFTAnalysis::GetMagnitudeAtFrequency(float frequency) const
{
  const float frequencyResolution = GetFrequencyResolution();

  if (frequency <= 0.0f || frequency >= GetNyquistFrequency())
    return -80.0f;  // Return low dB for out of range

  const float binIndex = frequency / frequencyResolution;
  const int lowerBin = static_cast<int>(std::floor(binIndex));
  const int upperBin = std::min(lowerBin + 1, static_cast<int>(mSpectrum.size()) - 1);

  if (lowerBin >= static_cast<int>(mSpectrum.size()) || lowerBin < 0)
    return -80.0f;

  if (lowerBin == upperBin)
    return mSpectrum[lowerBin].magnitudeDB;

  // Linear interpolation between bins
  const float fraction = binIndex - lowerBin;
  return mSpectrum[lowerBin].magnitudeDB * (1.0f - fraction) + mSpectrum[upperBin].magnitudeDB * fraction;
}

void MonoFFTAnalysis::Reset()
{
  std::fill(mInputBuffer.begin(), mInputBuffer.end(), 0.0f);
  std::fill(mMagnitudeHistory.begin(), mMagnitudeHistory.end(), -80.0f);

  mInputIndex = 0;
  mSamplesUntilProcess = mFFTSizeInt / mOverlapFactor;
  mHasNewData = false;
}


SmoothedStereoFFT::SmoothedStereoFFT()
    : mFFTAnalysis()
  , mLeftView(this, 0)
  , mRightView(this, 1)
    , mSamplesPerFFTUpdate(512)  // initial default; will be updated below
    , mInputDecimationFactor(1)
    , mInputDecimationCounter(0)
    , mCurrentHoldTimeMs(0.0f)       // Default hold time
    , mCurrentFalloffTimeMs(200.0f)  // Default falloff time
    , mCurrentAveragingWindowMs(1000.0f)
{
  SetFFTSmoothing(0.7f);
  // Set analyzer overlap and then sync our update rate from the analyzer's actual settings
  SetOverlapFactor(2);
  SetPeakHoldTime(60);
  SetFalloffRate(1200);
  SetAveragingWindow(1000);
  // Ensure update rate matches current analyzer configuration (no hardcoded size)
  SetFFTUpdateRate(mFFTAnalysis.GetFFTSizeInt(), mFFTAnalysis.GetOverlapFactor());
}

const std::vector<SpectrumBin>& SmoothedStereoFFT::ChannelView::GetSpectrum() const
{
  return mOwner->GetChannelSpectrum(mChannel);
}

float SmoothedStereoFFT::ChannelView::GetMagnitudeAtFrequency(float frequency) const
{
  return mOwner->GetChannelMagnitudeAtFrequency(mChannel, frequency);
}

float SmoothedStereoFFT::ChannelView::GetFrequencyResolution() const
{
  return mOwner->GetFrequencyResolution();
}

float SmoothedStereoFFT::ChannelView::GetNyquistFrequency() const
{
  return mOwner->GetNyquistFrequency();
}

void SmoothedStereoFFT::ConfigurePeakDetector(PeakDetector& detector)
{
  detector.SetParams(mCurrentHoldTimeMs, mCurrentHoldTimeMs, mCurrentFalloffTimeMs);
  detector.SetUseExponentialFalloff(true);
  detector.SetAveragingWindowMS(mCurrentAveragingWindowMs);
}

void SmoothedStereoFFT::ConfigurePeakDetectors()
{
  for (auto& detectorSet : mPeakDetectors)
  {
    for (auto& detector : detectorSet)
    {
      ConfigurePeakDetector(detector);
    }
  }
}

void SmoothedStereoFFT::SetPeakHoldTime(float holdTimeMs)
{
  mCurrentHoldTimeMs = holdTimeMs;  // Store the new hold time
  ConfigurePeakDetectors();
}

void SmoothedStereoFFT::SetFalloffRate(float falloffTimeMs)
{
  mCurrentFalloffTimeMs = falloffTimeMs;  // Store the new falloff time
  ConfigurePeakDetectors();
}

void SmoothedStereoFFT::SetAveragingWindow(float averagingWindowMs)
{
  mCurrentAveragingWindowMs = averagingWindowMs;
  ConfigurePeakDetectors();
}

void SmoothedStereoFFT::SetFFTUpdateRate(int fftSize, int overlapFactor)
{
  // Calculate how many samples occur between FFT updates
  const int baseUpdate = (overlapFactor > 0) ? (fftSize / overlapFactor) : fftSize;
  mSamplesPerFFTUpdate = std::max(1, baseUpdate * mInputDecimationFactor);
}

void SmoothedStereoFFT::SetInputDecimationFactor(int factor)
{
  mInputDecimationFactor = std::max(1, std::min(16, factor));
  mInputDecimationCounter = 0;
  SetFFTUpdateRate(mFFTAnalysis.GetFFTSizeInt(), mFFTAnalysis.GetOverlapFactor());
}

void SmoothedStereoFFT::ProcessSpectrum(const std::vector<SpectrumBin>& rawSpectrum)
{
  int inactive = InactiveBufferIndex();
  ProcessSpectrumView(rawSpectrum, mPeakDetectors[0], mBuffers[0][inactive]);
  PublishBuffer(inactive);
}

void SmoothedStereoFFT::ProcessSpectrumView(const std::vector<SpectrumBin>& rawSpectrum,
                                           std::vector<PeakDetector>& detectors,
                                           std::vector<SpectrumBin>& out)
{
  if (detectors.size() != rawSpectrum.size())
  {
    detectors.resize(rawSpectrum.size());
    for (auto& detector : detectors)
    {
      ConfigurePeakDetector(detector);
    }
  }

  out.resize(rawSpectrum.size());
  for (size_t i = 0; i < rawSpectrum.size(); ++i)
  {
    float magnitudeLinear = M7::math::DecibelsToLinear(rawSpectrum[i].magnitudeDB);
    detectors[i].ProcessSampleMulti(magnitudeLinear, mSamplesPerFFTUpdate);
    float smoothedMagnitudeDB = M7::math::LinearToDecibels((float)detectors[i].mCurrentPeak);
    out[i].frequency = rawSpectrum[i].frequency;
    out[i].magnitudeDB = smoothedMagnitudeDB;
  }
}

void SmoothedStereoFFT::ProcessSamples(float leftSample, float rightSample)
{
  if (mInputDecimationFactor > 1)
  {
    ++mInputDecimationCounter;
    if (mInputDecimationCounter < mInputDecimationFactor)
    {
      return;
    }
    mInputDecimationCounter = 0;
  }

  // Feed samples to the owned FFTAnalysis
  mFFTAnalysis.ProcessSamples(leftSample, rightSample);

  if (mFFTAnalysis.HasNewSpectrum())
  {
    const auto& rawCombined = mFFTAnalysis.GetSpectrum();
    const auto& rawLeft = mFFTAnalysis.GetChannelSpectrum(0);
    const auto& rawRight = mFFTAnalysis.GetChannelSpectrum(1);

    int inactive = InactiveBufferIndex();
    ProcessSpectrumView(rawCombined, mPeakDetectors[0], mBuffers[0][inactive]);
    ProcessSpectrumView(rawLeft, mPeakDetectors[1], mBuffers[1][inactive]);
    ProcessSpectrumView(rawRight, mPeakDetectors[2], mBuffers[2][inactive]);
    PublishBuffer(inactive);
    mFFTAnalysis.ConsumeSpectrum();
  }
}
void SmoothedStereoFFT::ProcessSamples(const M7::FloatPair& s)
{
  ProcessSamples(s.Left(), s.Right());
}

float SmoothedStereoFFT::GetMagnitudeAtFrequency(float frequency) const
{
  return InterpolateMagnitude(GetSpectrum(), frequency);
}

float SmoothedStereoFFT::InterpolateMagnitude(const std::vector<SpectrumBin>& output, float frequency) const
{
  if (output.empty() || frequency <= 0.0f || frequency >= mFFTAnalysis.GetNyquistFrequency())
    return -80.0f;

  // Find the appropriate bin (assuming uniform frequency spacing)
  const float frequencyResolution = GetFrequencyResolution();
  const float binIndex = frequency / frequencyResolution;
  const int lowerBin = static_cast<int>(std::floor(binIndex));
  const int upperBin = std::min(lowerBin + 1, static_cast<int>(output.size()) - 1);

  if (lowerBin >= static_cast<int>(output.size()) || lowerBin < 0)
    return -80.0f;

  if (lowerBin == upperBin)
    return output[lowerBin].magnitudeDB;

  // Linear interpolation between bins
  const float fraction = binIndex - lowerBin;
  return output[lowerBin].magnitudeDB * (1.0f - fraction) + output[upperBin].magnitudeDB * fraction;
}

const std::vector<SpectrumBin>& SmoothedStereoFFT::GetChannelSpectrum(int channel) const
{
  const int viewIndex = channel == 0 ? 1 : 2;
  return mBuffers[viewIndex][mActiveBuffer.load(std::memory_order_acquire)];
}

float SmoothedStereoFFT::GetChannelMagnitudeAtFrequency(int channel, float frequency) const
{
  return InterpolateMagnitude(GetChannelSpectrum(channel), frequency);
}

void SmoothedStereoFFT::Reset()
{
  for (auto& detectorSet : mPeakDetectors)
  {
    for (auto& detector : detectorSet)
    {
      detector.Reset();
    }
  }
  mFFTAnalysis.Reset();
  mInputDecimationCounter = 0;
  mHasNewOutput.store(false, std::memory_order_release);
}

FFTAnalysis::FFTAnalysis(FFTSize fftSize, WindowType windowType, float sampleRate)
    : mAnalyzers{MonoFFTAnalysis(fftSize, windowType, sampleRate), MonoFFTAnalysis(fftSize, windowType, sampleRate)}
{
}

void FFTAnalysis::SetSampleRate(float sampleRate)
{
  for (auto& analyzer : mAnalyzers)
    analyzer.SetSampleRate(sampleRate);
}

void FFTAnalysis::SetSmoothingFactor(float smoothing)
{
  for (auto& analyzer : mAnalyzers)
    analyzer.SetSmoothingFactor(smoothing);
}

void FFTAnalysis::SetOverlapFactor(int factor)
{
  for (auto& analyzer : mAnalyzers)
    analyzer.SetOverlapFactor(factor);
}

void FFTAnalysis::SetWindowType(WindowType windowType)
{
  for (auto& analyzer : mAnalyzers)
    analyzer.SetWindowType(windowType);
}

void FFTAnalysis::ProcessSamples(float leftSample, float rightSample)
{
  mAnalyzers[0].ProcessSample(leftSample);
  mAnalyzers[1].ProcessSample(rightSample);
}

bool FFTAnalysis::HasNewSpectrum() const
{
  return mAnalyzers[0].HasNewSpectrum() || mAnalyzers[1].HasNewSpectrum();
}

void FFTAnalysis::ConsumeSpectrum()
{
  mAnalyzers[0].ConsumeSpectrum();
  mAnalyzers[1].ConsumeSpectrum();
}

const std::vector<SpectrumBin>& FFTAnalysis::GetChannelSpectrum(int channel) const
{
  return mAnalyzers[channel == 0 ? 0 : 1].GetSpectrum();
}

float FFTAnalysis::GetChannelMagnitudeAtFrequency(int channel, float frequency) const
{
  return mAnalyzers[channel == 0 ? 0 : 1].GetMagnitudeAtFrequency(frequency);
}

const std::vector<SpectrumBin>& FFTAnalysis::GetSpectrum() const
{
  // Return a combined spectrum for both channels using per-instance buffer
  mCombinedSpectrum.clear();

  const auto& leftSpectrum = mAnalyzers[0].GetSpectrum();
  const auto& rightSpectrum = mAnalyzers[1].GetSpectrum();

  size_t maxSize = std::max(leftSpectrum.size(), rightSpectrum.size());
  mCombinedSpectrum.resize(maxSize);

  for (size_t i = 0; i < maxSize; ++i)
  {
    if (i < leftSpectrum.size())
      mCombinedSpectrum[i].frequency = leftSpectrum[i].frequency;
    else
      mCombinedSpectrum[i].frequency = rightSpectrum[i].frequency;
    mCombinedSpectrum[i].magnitudeDB = std::max(i < leftSpectrum.size() ? leftSpectrum[i].magnitudeDB : -80.0f,
                                                i < rightSpectrum.size() ? rightSpectrum[i].magnitudeDB : -80.0f);
  }

  return mCombinedSpectrum;
}

float FFTAnalysis::GetMagnitudeAtFrequency(float frequency) const
{
  float leftMagnitude = GetChannelMagnitudeAtFrequency(0, frequency);
  float rightMagnitude = GetChannelMagnitudeAtFrequency(1, frequency);

  // Return the maximum magnitude from both channels
  return std::max(leftMagnitude, rightMagnitude);
}

float FFTAnalysis::GetFrequencyResolution() const
{
  return mAnalyzers[0].GetFrequencyResolution();
}

float FFTAnalysis::GetNyquistFrequency() const
{
  return mAnalyzers[0].GetNyquistFrequency();
}

void FFTAnalysis::Reset()
{
  for (auto& analyzer : mAnalyzers)
    analyzer.Reset();
}
}  // namespace WaveSabreCore


#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT