#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT

  #include <WaveSabreCore/FFTAnalysis.hpp>
  #include <WaveSabreCore/Helpers.h>
  #include <WaveSabreCore/RMS.hpp>
  #include <algorithm>
  #include <cstring>

namespace WaveSabreCore
{
///////////////////////////////////////////////////////////////////////////
// MonoFFTAnalysis Implementation - Core single-channel algorithm
///////////////////////////////////////////////////////////////////////////

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

//void MonoFFTAnalysis::SetDisplayBoost(float boostDB)
//{
//    mDisplayBoostDB = boostDB;
//}


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

  mInputIndex = (mInputIndex + 1) % mFFTSizeInt;
  --mSamplesUntilProcess;

  if (mSamplesUntilProcess <= 0)
  {
    // Time to process FFT
    mSamplesUntilProcess = mFFTSizeInt / mOverlapFactor;

    // Copy data to FFT buffer in correct order (oldest first)
    for (int i = 0; i < mFFTSizeInt; ++i)
    {
      int srcIndex = (mInputIndex + i) % mFFTSizeInt;
      mFFTBuffer[i] = std::complex<float>(mInputBuffer[srcIndex] * mWindow[i], 0.0f);
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

///////////////////////////////////////////////////////////////////////////
// SmoothedStereoFFT Implementation - uses FrequencyDependentPeakDetector
///////////////////////////////////////////////////////////////////////////

SmoothedStereoFFT::SmoothedStereoFFT()
    : mFFTAnalysis()
    , mSamplesPerFFTUpdate(512)      // initial default; will be updated below
    , mCurrentHoldTimeMs(0.0f)       // Default hold time
    , mCurrentFalloffTimeMs(200.0f)  // Default falloff time
{
  SetFFTSmoothing(0.7f);
  // Set analyzer overlap and then sync our update rate from the analyzer's actual settings
  SetOverlapFactor(2);
  SetPeakHoldTime(60);
  SetFalloffRate(1200);
  // Ensure update rate matches current analyzer configuration (no hardcoded size)
  SetFFTUpdateRate(mFFTAnalysis.GetFFTSizeInt(), mFFTAnalysis.GetOverlapFactor());
}

void SmoothedStereoFFT::SetPeakHoldTime(float holdTimeMs)
{
  mCurrentHoldTimeMs = holdTimeMs;  // Store the new hold time

  // Update all existing peak detectors with BOTH current settings + their frequencies
  for (size_t i = 0; i < mPeakDetectors.size(); ++i)
  {
    const auto& active = mBuffers[mActiveBuffer.load(std::memory_order_acquire)];
    float frequency = (i < active.size()) ? active[i].frequency : 0.0f;
    mPeakDetectors[i].SetParams(0, mCurrentHoldTimeMs, mCurrentFalloffTimeMs, frequency);
  }
}

void SmoothedStereoFFT::SetFalloffRate(float falloffTimeMs)
{
  mCurrentFalloffTimeMs = falloffTimeMs;  // Store the new falloff time

  // Update all existing peak detectors with BOTH current settings + their frequencies
  for (size_t i = 0; i < mPeakDetectors.size(); ++i)
  {
    const auto& active = mBuffers[mActiveBuffer.load(std::memory_order_acquire)];
    float frequency = (i < active.size()) ? active[i].frequency : 0.0f;
    mPeakDetectors[i].SetParams(0, mCurrentHoldTimeMs, mCurrentFalloffTimeMs, frequency);
  }
}

void SmoothedStereoFFT::SetFFTUpdateRate(int fftSize, int overlapFactor)
{
  // Calculate how many samples occur between FFT updates
  mSamplesPerFFTUpdate = (overlapFactor > 0) ? (fftSize / overlapFactor) : fftSize;
}

void SmoothedStereoFFT::ProcessSpectrum(const std::vector<SpectrumBin>& rawSpectrum)
{
  // Resize if needed
  if (mPeakDetectors.size() != rawSpectrum.size())
  {
    mPeakDetectors.resize(rawSpectrum.size());
    // initialize detectors with current settings and their frequencies
    for (size_t i = 0; i < mPeakDetectors.size(); ++i)
    {
      float frequency = (i < rawSpectrum.size()) ? rawSpectrum[i].frequency : 0.0f;
      mPeakDetectors[i].SetParams(0, mCurrentHoldTimeMs, mCurrentFalloffTimeMs, frequency);
    }
    // resize buffers
    mBuffers[0].resize(rawSpectrum.size());
    mBuffers[1].resize(rawSpectrum.size());
  }

  int inactive = InactiveBufferIndex();
  auto& out = mBuffers[inactive];
  out.resize(rawSpectrum.size());

  // Process each bin once per FFT update using multi-step falloff
  for (size_t i = 0; i < rawSpectrum.size(); ++i)
  {
    float magnitudeLinear = M7::math::DecibelsToLinear(rawSpectrum[i].magnitudeDB);
    mPeakDetectors[i].ProcessSampleMulti(magnitudeLinear, mSamplesPerFFTUpdate);
    float smoothedMagnitudeDB = M7::math::LinearToDecibels((float)mPeakDetectors[i].mCurrentPeak);
    out[i].frequency = rawSpectrum[i].frequency;
    out[i].magnitudeDB = smoothedMagnitudeDB;
  }

  PublishBuffer(inactive);
}

void SmoothedStereoFFT::ProcessSamples(float leftSample, float rightSample)
{
  // Feed samples to the owned FFTAnalysis
  mFFTAnalysis.ProcessSamples(leftSample, rightSample);

  if (mFFTAnalysis.HasNewSpectrum())
  {
    const auto& raw = mFFTAnalysis.GetSpectrum();
    ProcessSpectrum(raw);
    mFFTAnalysis.ConsumeSpectrum();
  }
}
void SmoothedStereoFFT::ProcessSamples(const M7::FloatPair& s)
{
  ProcessSamples(s.Left(), s.Right());
}

float SmoothedStereoFFT::GetMagnitudeAtFrequency(float frequency) const
{
  const auto& output = GetSpectrum();
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

void SmoothedStereoFFT::Reset()
{
  for (auto& detector : mPeakDetectors)
  {
    detector.Reset();  // Use the built-in Reset() method
  }
  mFFTAnalysis.Reset();
  mHasNewOutput.store(false, std::memory_order_release);
}

///////////////////////////////////////////////////////////////////////////
// FFTAnalysis Implementation - Stereo wrapper
///////////////////////////////////////////////////////////////////////////

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
  float leftMagnitude = mAnalyzers[0].GetMagnitudeAtFrequency(frequency);
  float rightMagnitude = mAnalyzers[1].GetMagnitudeAtFrequency(frequency);

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