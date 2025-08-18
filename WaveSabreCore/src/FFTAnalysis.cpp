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
        , mSmoothingFactor(0.3f) // Light technical smoothing only
        , mInputIndex(0)
        , mSamplesUntilProcess(mFFTSizeInt / mOverlapFactor)
        , mHasNewData(false)
        , mDisplayBoostDB(18.0f) // Default +18dB boost for visual appeal
    {
        // Initialize buffers
        mInputBuffer.resize(mFFTSizeInt, 0.0f);
        mFFTBuffer.resize(mFFTSizeInt);
        
        int spectrumSize = mFFTSizeInt / 2; // Only need positive frequencies
        mSpectrum.resize(spectrumSize);
        mMagnitudeHistory.resize(spectrumSize, -80.0f);
        
        GenerateWindow();
    }

    MonoFFTAnalysis::~MonoFFTAnalysis()
    {
    }

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
        mOverlapFactor = std::max(1, std::min(16, factor)); // Allow 1x overlap for completeness
        mSamplesUntilProcess = mFFTSizeInt / mOverlapFactor;
    }

    void MonoFFTAnalysis::SetDisplayBoost(float boostDB)
    {
        mDisplayBoostDB = boostDB;
    }

    void MonoFFTAnalysis::SetWindowType(WindowType windowType)
    {
        if (mWindowType != windowType)
        {
            mWindowType = windowType;
            GenerateWindow(); // Regenerate window coefficients
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
                    mWindow[i] = 0.42f - 0.5f * std::cos(2.0f * 3.14159265f * n / N) + 
                                0.08f * std::cos(4.0f * 3.14159265f * n / N);
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
            const float magnitudeDB = magnitude > 1e-10f ? 
                (20.0f * std::log10(magnitude / (mFFTSizeInt * 0.5f)) + mDisplayBoostDB) : (-200.0f + mDisplayBoostDB);
            
            // Light technical smoothing only (no peak-hold here)
            mMagnitudeHistory[i] = mMagnitudeHistory[i] * mSmoothingFactor + magnitudeDB * (1.0f - mSmoothingFactor);
            
            // Store in spectrum bin
            mSpectrum[i].frequency = i * frequencyResolution;
            mSpectrum[i].magnitudeDB = mMagnitudeHistory[i];
            mSpectrum[i].phase = std::atan2(imag, real);
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
            return -80.0f; // Return low dB for out of range
        
        const float binIndex = frequency / frequencyResolution;
        const int lowerBin = static_cast<int>(std::floor(binIndex));
        const int upperBin = std::min(lowerBin + 1, static_cast<int>(mSpectrum.size()) - 1);
        
        if (lowerBin >= static_cast<int>(mSpectrum.size()) || lowerBin < 0)
            return -80.0f;
        
        if (lowerBin == upperBin)
            return mSpectrum[lowerBin].magnitudeDB;
        
        // Linear interpolation between bins
        const float fraction = binIndex - lowerBin;
        return mSpectrum[lowerBin].magnitudeDB * (1.0f - fraction) + 
               mSpectrum[upperBin].magnitudeDB * fraction;
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
    // MonoSpectrumDisplaySmoother Implementation - Using proven PeakDetector!
    ///////////////////////////////////////////////////////////////////////////
    
    MonoSpectrumDisplaySmoother::MonoSpectrumDisplaySmoother()
        : mSampleRate(44100.0f)
        , mSamplesPerFFTUpdate(512) // Default: 2048 FFT with 75% overlap = 512 samples between updates
    {
    }

    void MonoSpectrumDisplaySmoother::SetPeakHoldTime(float holdTimeMs, float sampleRate)
    {
        mSampleRate = sampleRate;
        
        // Update all existing peak detectors
        for (auto& detector : mPeakDetectors)
        {
            float falloffTimeMs = 1200.0f; // Keep existing falloff time
            detector.SetParams(0, holdTimeMs, falloffTimeMs);
        }
    }

    void MonoSpectrumDisplaySmoother::SetFalloffRate(float falloffTimeMs, float sampleRate)
    {
        mSampleRate = sampleRate;
        
        // Update all existing peak detectors  
        for (auto& detector : mPeakDetectors)
        {
            float holdTimeMs = 300.0f; // Keep existing hold time
            detector.SetParams(0, holdTimeMs, falloffTimeMs);
        }
    }

    void MonoSpectrumDisplaySmoother::ProcessSpectrum(const std::vector<MonoFFTAnalysis::SpectrumBin>& rawSpectrum)
    {
        // Resize if needed
        if (mPeakDetectors.size() != rawSpectrum.size())
        {
            mPeakDetectors.resize(rawSpectrum.size());
            mOutput.resize(rawSpectrum.size());
            
            // Initialize new peak detectors with current settings
            // Default: 300ms hold, 1200ms falloff
            for (auto& detector : mPeakDetectors)
            {
                detector.SetParams(0, 300.0, 1200.0);
            }
        }
        
        // Process each bin using existing PeakDetector
        // Key insight: Simulate the missing samples between FFT updates
        for (size_t i = 0; i < rawSpectrum.size(); ++i)
        {
            // Convert dB to linear for PeakDetector (which expects linear values 0-1+)
            // Add offset to handle negative dB values properly
            float magnitudeLinear = std::pow(10.0f, rawSpectrum[i].magnitudeDB / 20.0f);
            
            // Since PeakDetector expects to be called every sample, we need to simulate
            // the missing samples by calling it multiple times with the same value
            // This maintains correct timing for hold/falloff calculations
            for (int s = 0; s < mSamplesPerFFTUpdate; ++s)
            {
                mPeakDetectors[i].ProcessSample(magnitudeLinear);
            }
            
            // Convert back to dB for output
            float smoothedMagnitudeDB = 20.0f * std::log10(std::max(1e-10f, (float)mPeakDetectors[i].mCurrentPeak));
            
            mOutput[i].frequency = rawSpectrum[i].frequency;
            mOutput[i].magnitudeDB = smoothedMagnitudeDB;
            mOutput[i].phase = rawSpectrum[i].phase;
        }
    }

    float MonoSpectrumDisplaySmoother::GetMagnitudeAtFrequency(float frequency, float sampleRate) const
    {
        if (mOutput.empty() || frequency <= 0.0f || frequency >= sampleRate * 0.5f)
            return -80.0f;
        
        // Find the appropriate bin (assuming uniform frequency spacing)
        const float frequencyResolution = mOutput.size() > 1 ? 
            (mOutput[1].frequency - mOutput[0].frequency) : (sampleRate / 2048.0f);
        const float binIndex = frequency / frequencyResolution;
        const int lowerBin = static_cast<int>(std::floor(binIndex));
        const int upperBin = std::min(lowerBin + 1, static_cast<int>(mOutput.size()) - 1);
        
        if (lowerBin >= static_cast<int>(mOutput.size()) || lowerBin < 0)
            return -80.0f;
        
        if (lowerBin == upperBin)
            return mOutput[lowerBin].magnitudeDB;
        
        // Linear interpolation between bins
        const float fraction = binIndex - lowerBin;
        return mOutput[lowerBin].magnitudeDB * (1.0f - fraction) + 
               mOutput[upperBin].magnitudeDB * fraction;
    }

    void MonoSpectrumDisplaySmoother::Reset()
    {
        for (auto& detector : mPeakDetectors)
        {
            detector.Reset(); // Use the built-in Reset() method
        }
    }

    ///////////////////////////////////////////////////////////////////////////
    // FFTAnalysis Implementation - Stereo wrapper
    ///////////////////////////////////////////////////////////////////////////
    
    FFTAnalysis::FFTAnalysis(FFTSize fftSize, WindowType windowType, float sampleRate)
        : mAnalyzers{MonoFFTAnalysis(fftSize, windowType, sampleRate),
                    MonoFFTAnalysis(fftSize, windowType, sampleRate)}
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

    void FFTAnalysis::SetDisplayBoost(float boostDB)
    {
        for (auto& analyzer : mAnalyzers)
            analyzer.SetDisplayBoost(boostDB);
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

    const std::vector<IFrequencyAnalysis::SpectrumBin>& FFTAnalysis::GetSpectrumLeft() const
    {
        return reinterpret_cast<const std::vector<IFrequencyAnalysis::SpectrumBin>&>(mAnalyzers[0].GetSpectrum());
    }

    const std::vector<IFrequencyAnalysis::SpectrumBin>& FFTAnalysis::GetSpectrumRight() const
    {
        return reinterpret_cast<const std::vector<IFrequencyAnalysis::SpectrumBin>&>(mAnalyzers[1].GetSpectrum());
    }

    float FFTAnalysis::GetMagnitudeAtFrequency(float frequency, bool useRightChannel) const
    {
        return mAnalyzers[useRightChannel ? 1 : 0].GetMagnitudeAtFrequency(frequency);
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

    ///////////////////////////////////////////////////////////////////////////
    // SpectrumDisplaySmoother Implementation - Stereo wrapper
    ///////////////////////////////////////////////////////////////////////////
    
    SpectrumDisplaySmoother::SpectrumDisplaySmoother()
        : mSampleRate(44100.0f)
    {
    }

    void SpectrumDisplaySmoother::SetPeakHoldTime(float holdTimeMs, float sampleRate)
    {
        mSampleRate = sampleRate;
        for (auto& smoother : mSmoothers)
            smoother.SetPeakHoldTime(holdTimeMs, sampleRate);
    }

    void SpectrumDisplaySmoother::SetFalloffRate(float falloffTimeMs, float sampleRate)
    {
        mSampleRate = sampleRate;
        for (auto& smoother : mSmoothers)
            smoother.SetFalloffRate(falloffTimeMs, sampleRate);
    }

    void SpectrumDisplaySmoother::ProcessSpectrum(const std::vector<SpectrumBin>& rawSpectrum, bool isRightChannel)
    {
        // Convert from interface SpectrumBin to MonoFFTAnalysis::SpectrumBin (they're identical)
        const auto& monoSpectrum = reinterpret_cast<const std::vector<MonoFFTAnalysis::SpectrumBin>&>(rawSpectrum);
        mSmoothers[isRightChannel ? 1 : 0].ProcessSpectrum(monoSpectrum);
    }

    const std::vector<IFrequencyAnalysis::SpectrumBin>& SpectrumDisplaySmoother::GetSpectrumLeft() const
    {
        return reinterpret_cast<const std::vector<IFrequencyAnalysis::SpectrumBin>&>(mSmoothers[0].GetSpectrum());
    }

    const std::vector<IFrequencyAnalysis::SpectrumBin>& SpectrumDisplaySmoother::GetSpectrumRight() const
    {
        return reinterpret_cast<const std::vector<IFrequencyAnalysis::SpectrumBin>&>(mSmoothers[1].GetSpectrum());
    }

    float SpectrumDisplaySmoother::GetMagnitudeAtFrequency(float frequency, bool useRightChannel) const
    {
        return mSmoothers[useRightChannel ? 1 : 0].GetMagnitudeAtFrequency(frequency, mSampleRate);
    }

    float SpectrumDisplaySmoother::GetFrequencyResolution() const
    {
        const auto& spectrum = mSmoothers[0].GetSpectrum();
        if (spectrum.empty())
            return 0.0f;
        
        // Assume uniform frequency spacing - calculate from first two bins
        if (spectrum.size() > 1)
            return spectrum[1].frequency - spectrum[0].frequency;
        else
            return mSampleRate / 2048.0f; // fallback default
    }

    float SpectrumDisplaySmoother::GetNyquistFrequency() const
    {
        return mSampleRate * 0.5f;
    }

    void SpectrumDisplaySmoother::Reset()
    {
        for (auto& smoother : mSmoothers)
            smoother.Reset();
    }
}


#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT