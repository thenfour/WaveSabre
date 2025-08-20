#pragma once

#include "Device.h"
#include <vector>
#include <complex>
#include <cmath>

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT

#include "RMS.hpp"  // Include for PeakDetector

namespace WaveSabreCore
{
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

    // Single-channel FFT analyzer - the core algorithm
    class MonoFFTAnalysis
    {
    public:
        // Common FFT sizes for audio analysis
        enum class FFTSize : int
        {
            FFT512 = 512,
            FFT1024 = 1024,
            FFT2048 = 2048,
            FFT4096 = 4096
        };

        // Window function types for better frequency resolution
        enum class WindowType
        {
            Rectangular,
            Hanning,
            Hamming,
            Blackman
        };

    private:
        FFTSize mFFTSize;
        WindowType mWindowType;
        int mFFTSizeInt;
        float mSampleRate;
        int mOverlapFactor;     // 2 = 50% overlap, 4 = 75% overlap
        
        // Input buffer (mono)
        std::vector<float> mInputBuffer;
        
        // Window function coefficients
        std::vector<float> mWindow;
        
        // FFT working buffer
        std::vector<std::complex<float>> mFFTBuffer;
        
        // Output spectrum data
        std::vector<SpectrumBin> mSpectrum;
        
        // Basic smoothing for technical stability (light)
        std::vector<float> mMagnitudeHistory;
        float mSmoothingFactor;
        
        // Input management
        int mInputIndex;
        int mSamplesUntilProcess;
        bool mHasNewData;
        
        void GenerateWindow();
        void PerformFFT(std::vector<std::complex<float>>& buffer);
        void ComputeSpectrum();

    public:
        MonoFFTAnalysis(FFTSize fftSize = FFTSize::FFT2048, 
                       WindowType windowType = WindowType::Hanning,
                       float sampleRate = Helpers::CurrentSampleRate);
        
        ~MonoFFTAnalysis();
        
        // Configuration
        void SetSampleRate(float sampleRate);
        void SetSmoothingFactor(float smoothing); // 0.0 = no smoothing, 0.9 = heavy smoothing
        void SetOverlapFactor(int factor); // 1, 2, 4, 8, or 16
        //void SetDisplayBoost(float boostDB); // artificial boost for display purposes (default +18dB)
        void SetWindowType(WindowType windowType); // Change window function at runtime
        
		WindowType GetWindowType() const { return mWindowType; }
		FFTSize GetFFTSize() const { return mFFTSize; }
		int GetFFTSizeInt() const { return mFFTSizeInt; }
        void SetFFTSize(FFTSize fftSize) {
            if (mFFTSize != fftSize) {
                mFFTSize = fftSize;
                mFFTSizeInt = static_cast<int>(fftSize);
                mInputBuffer.resize(mFFTSizeInt);
                mFFTBuffer.resize(mFFTSizeInt);
                // Keep spectrum size consistent with constructor: positive frequencies only
                mSpectrum.resize(mFFTSizeInt / 2);
                // Keep magnitude history in sync with spectrum
                mMagnitudeHistory.resize(mSpectrum.size(), -80.0f);
                GenerateWindow();
                Reset();
            }
		}
		float GetSampleRate() const { return mSampleRate; }
		int GetOverlapFactor() const { return mOverlapFactor; }
		float GetSmoothingFactor() const { return mSmoothingFactor; }
		//float GetDisplayBoost() const { return mDisplayBoostDB; }

        // Input processing (call once per sample)
        void ProcessSample(float sample);
        
        // Check if new spectrum data is available
        bool HasNewSpectrum() const { return mHasNewData; }
        
        // Mark spectrum as consumed
        void ConsumeSpectrum() { mHasNewData = false; }
        
        // Get spectrum data
        const std::vector<SpectrumBin>& GetSpectrum() const { return mSpectrum; }
        
        // Get magnitude at specific frequency (interpolated if necessary)
        float GetMagnitudeAtFrequency(float frequency) const;
        
        // Get the frequency resolution (Hz per bin)
        float GetFrequencyResolution() const { return mSampleRate / mFFTSizeInt; }
        
        // Get the maximum frequency represented
        float GetNyquistFrequency() const { return mSampleRate * 0.5f; }
        
        // Clear/reset state
        void Reset();

    //private:
        //float mDisplayBoostDB; // artificial boost for better visual display
    };

	// Stereo FFT analyzer wrapper; wraps 2 mono fft analyzers and provides a single spectrum
    class FFTAnalysis : public IFrequencyAnalysis
    {
    private:
        MonoFFTAnalysis mAnalyzers[2]; // Left and right channel analyzers
        mutable std::vector<SpectrumBin> mCombinedSpectrum; // per-instance buffer to avoid static
        
    public:
        using FFTSize = MonoFFTAnalysis::FFTSize;
        using WindowType = MonoFFTAnalysis::WindowType;
        
        FFTAnalysis(FFTSize fftSize = FFTSize::FFT2048, 
                   WindowType windowType = WindowType::Hanning,
                   float sampleRate = Helpers::CurrentSampleRate);
        
        // Configuration
        void SetSampleRate(float sampleRate);
        void SetSmoothingFactor(float smoothing);
        void SetOverlapFactor(int factor);
        //void SetDisplayBoost(float boostDB);
        void SetWindowType(WindowType windowType); // Change window function at runtime
        
		WindowType GetWindowType() const { return mAnalyzers[0].GetWindowType(); }
		FFTSize GetFFTSize() const { return mAnalyzers[0].GetFFTSize(); }
		int GetFFTSizeInt() const { return mAnalyzers[0].GetFFTSizeInt(); }
		float GetSampleRate() const { return mAnalyzers[0].GetSampleRate(); }
        int GetOverlapFactor() const { return mAnalyzers[0].GetOverlapFactor(); }
		float GetSmoothingFactor() const { return mAnalyzers[0].GetSmoothingFactor(); }

        // Input processing (call once per sample)
        void ProcessSamples(float leftSample, float rightSample);
        
        // Check if new spectrum data is available
        bool HasNewSpectrum() const;
        
        // Mark spectrum as consumed
        void ConsumeSpectrum();
        
        // IFrequencyAnalysis implementation
        virtual const std::vector<SpectrumBin>& GetSpectrum() const override;
        virtual float GetMagnitudeAtFrequency(float frequency) const override;
        virtual float GetFrequencyResolution() const override;
        virtual float GetNyquistFrequency() const override;
        
        // Clear/reset state
        void Reset();
    };

	// wraps FFTAnalysis and provides smoothing.
    class SmoothedStereoFFT : public IFrequencyAnalysis
    {
    private:
		FFTAnalysis mFFTAnalysis; // Underlying FFT analysis
        std::vector<FrequencyDependentPeakDetector> mPeakDetectors;
        std::vector<SpectrumBin> mBuffers[2]; // double-buffered output
        std::atomic<int> mActiveBuffer{0};
        std::atomic<bool> mHasNewOutput{false};

        int mSamplesPerFFTUpdate; // How many samples between FFT updates (e.g., 512)

        // Track current settings to avoid overwriting each other
        float mCurrentHoldTimeMs;
        float mCurrentFalloffTimeMs;

        // Helper to get inactive buffer index
        int InactiveBufferIndex() const { return 1 - mActiveBuffer.load(std::memory_order_acquire); }
        void PublishBuffer(int idx) { mActiveBuffer.store(idx, std::memory_order_release); mHasNewOutput.store(true, std::memory_order_release); }

    public:
        SmoothedStereoFFT();

        // Configure FFT/analyzer behavior
        void SetSampleRate(float sampleRate) { mFFTAnalysis.SetSampleRate(sampleRate); }
        void SetWindowType(MonoFFTAnalysis::WindowType windowType) { mFFTAnalysis.SetWindowType(windowType); }
        void SetFFTSmoothing(float smoothing) { mFFTAnalysis.SetSmoothingFactor(smoothing); }
        void SetOverlapFactor(int factor) { mFFTAnalysis.SetOverlapFactor(factor); SetFFTUpdateRate(mFFTAnalysis.GetFFTSizeInt(), factor); }
        // Proxy getters for UI
        MonoFFTAnalysis::WindowType GetWindowType() const { return mFFTAnalysis.GetWindowType(); }
        MonoFFTAnalysis::FFTSize GetFFTSize() const { return mFFTAnalysis.GetFFTSize(); }
        int GetFFTSizeInt() const { return mFFTAnalysis.GetFFTSizeInt(); }
        float GetFFTSmoothing() const { return mFFTAnalysis.GetSmoothingFactor(); }
        int GetOverlapFactor() const { return mFFTAnalysis.GetOverlapFactor(); }

        // Configure display behavior
        void SetPeakHoldTime(float holdTimeMs);
        void SetFalloffRate(float falloffTimeMs);  // Time for -60dB falloff
        void SetFFTUpdateRate(int fftSize, int overlapFactor); // Update timing parameters

        float GetPeakHoldTime() const { return mCurrentHoldTimeMs; }
        float GetFalloffTime() const { return mCurrentFalloffTimeMs; }

        // Feed samples and update smoothing when new spectrum is available
        void ProcessSamples(float leftSample, float rightSample);
        
        // Process new FFT data for display (optional external use)
        void ProcessSpectrum(const std::vector<SpectrumBin>& rawSpectrum);

        // IFrequencyAnalysis implementation (display-ready smoothed spectrum)
        const std::vector<SpectrumBin>& GetSpectrum() const { return mBuffers[mActiveBuffer.load(std::memory_order_acquire)]; }
        float GetMagnitudeAtFrequency(float frequency) const override;
        float GetFrequencyResolution() const override { return mFFTAnalysis.GetFrequencyResolution(); }
        float GetNyquistFrequency() const override { return mFFTAnalysis.GetNyquistFrequency(); }

        void Reset();

        // Output update signalling
        bool HasNewOutput() const { return mHasNewOutput.load(std::memory_order_acquire); }
        void ConsumeOutput() { mHasNewOutput.store(false, std::memory_order_release); }
    };

    // Mono version to avoid redundant processing for M/S
    class SmoothedMonoFFT : public IFrequencyAnalysis
    {
    private:
        MonoFFTAnalysis mFFTAnalysis; // Underlying FFT analysis (mono)
        std::vector<FrequencyDependentPeakDetector> mPeakDetectors;
        std::vector<SpectrumBin> mBuffers[2];
        std::atomic<int> mActiveBuffer{0};
        std::atomic<bool> mHasNewOutput{false};

        int mSamplesPerFFTUpdate; // How many samples between FFT updates

        float mCurrentHoldTimeMs;
        float mCurrentFalloffTimeMs;

        int InactiveBufferIndex() const { return 1 - mActiveBuffer.load(std::memory_order_acquire); }
        void PublishBuffer(int idx) { mActiveBuffer.store(idx, std::memory_order_release); mHasNewOutput.store(true, std::memory_order_release); }

    public:
        SmoothedMonoFFT()
            : mFFTAnalysis()
            , mSamplesPerFFTUpdate(512)
            , mCurrentHoldTimeMs(0.0f)
            , mCurrentFalloffTimeMs(200.0f)
        {
            // Reasonable defaults similar to stereo variant
            mFFTAnalysis.SetSmoothingFactor(0.7f);
            mFFTAnalysis.SetOverlapFactor(2);
            SetPeakHoldTime(60);
            SetFalloffRate(1200);
            SetFFTUpdateRate(mFFTAnalysis.GetFFTSizeInt(), mFFTAnalysis.GetOverlapFactor());
        }

        // Configure FFT/analyzer behavior
        void SetSampleRate(float sampleRate) { mFFTAnalysis.SetSampleRate(sampleRate); }
        void SetWindowType(MonoFFTAnalysis::WindowType windowType) { mFFTAnalysis.SetWindowType(windowType); }
        void SetFFTSmoothing(float smoothing) { mFFTAnalysis.SetSmoothingFactor(smoothing); }
		void SetFFTSize(MonoFFTAnalysis::FFTSize fftSize) { 
            mFFTAnalysis.SetFFTSize(fftSize);
            SetFFTUpdateRate(mFFTAnalysis.GetFFTSizeInt(), mFFTAnalysis.GetOverlapFactor());
        }
        void SetOverlapFactor(int factor) { mFFTAnalysis.SetOverlapFactor(factor); SetFFTUpdateRate(mFFTAnalysis.GetFFTSizeInt(), factor); }
        // Proxy getters for UI
        MonoFFTAnalysis::WindowType GetWindowType() const { return mFFTAnalysis.GetWindowType(); }
        MonoFFTAnalysis::FFTSize GetFFTSize() const { return mFFTAnalysis.GetFFTSize(); }
        int GetFFTSizeInt() const { return mFFTAnalysis.GetFFTSizeInt(); }
        float GetFFTSmoothing() const { return mFFTAnalysis.GetSmoothingFactor(); }
        int GetOverlapFactor() const { return mFFTAnalysis.GetOverlapFactor(); }

        // Configure display behavior
        void SetPeakHoldTime(float holdTimeMs)
        {
            mCurrentHoldTimeMs = holdTimeMs;
            for (size_t i = 0; i < mPeakDetectors.size(); ++i)
            {
                float frequency = (i < mBuffers[mActiveBuffer.load()].size()) ? mBuffers[mActiveBuffer.load()][i].frequency : 0.0f;
                mPeakDetectors[i].SetParams(0, mCurrentHoldTimeMs, mCurrentFalloffTimeMs, frequency);
            }
        }
        void SetFalloffRate(float falloffTimeMs)
        {
            mCurrentFalloffTimeMs = falloffTimeMs;
            for (size_t i = 0; i < mPeakDetectors.size(); ++i)
            {
                float frequency = (i < mBuffers[mActiveBuffer.load()].size()) ? mBuffers[mActiveBuffer.load()][i].frequency : 0.0f;
                mPeakDetectors[i].SetParams(0, mCurrentHoldTimeMs, mCurrentFalloffTimeMs, frequency);
            }
        }
        void SetFFTUpdateRate(int fftSize, int overlapFactor) { mSamplesPerFFTUpdate = (overlapFactor > 0) ? (fftSize / overlapFactor) : fftSize; }

        float GetPeakHoldTime() const { return mCurrentHoldTimeMs; }
        float GetFalloffTime() const { return mCurrentFalloffTimeMs; }

        // Feed samples and update smoothing when new spectrum is available
        void ProcessSample(float sample)
        {
            mFFTAnalysis.ProcessSample(sample);
            if (mFFTAnalysis.HasNewSpectrum())
            {
                const auto& raw = mFFTAnalysis.GetSpectrum();
                // Process and publish
                // Resize if needed
                if (mPeakDetectors.size() != raw.size())
                {
                    mPeakDetectors.resize(raw.size());
                    // initialize detectors with current settings and per-bin frequency
                    for (size_t i = 0; i < mPeakDetectors.size(); ++i)
                    {
                        float frequency = (i < raw.size()) ? raw[i].frequency : 0.0f;
                        mPeakDetectors[i].SetParams(0, mCurrentHoldTimeMs, mCurrentFalloffTimeMs, frequency);
                    }
                    // ensure both buffers sized
                    mBuffers[0].resize(raw.size());
                    mBuffers[1].resize(raw.size());
                }

                int inactive = InactiveBufferIndex();
                auto& out = mBuffers[inactive];
                out.resize(raw.size());

                // Per-frame smoothing using N-step processing
                for (size_t i = 0; i < raw.size(); ++i)
                {
                    float magnitudeLinear = M7::math::DecibelsToLinear(raw[i].magnitudeDB);
                    mPeakDetectors[i].ProcessSampleMulti(magnitudeLinear, mSamplesPerFFTUpdate);
                    float smoothedMagnitudeDB = M7::math::LinearToDecibels((float)mPeakDetectors[i].mCurrentPeak);
                    out[i].frequency = raw[i].frequency;
                    out[i].magnitudeDB = smoothedMagnitudeDB;
                }

                PublishBuffer(inactive);
                mFFTAnalysis.ConsumeSpectrum();
            }
        }

        // IFrequencyAnalysis implementation (display-ready smoothed spectrum)
        const std::vector<SpectrumBin>& GetSpectrum() const { return mBuffers[mActiveBuffer.load(std::memory_order_acquire)]; }
        float GetMagnitudeAtFrequency(float frequency) const override
        {
            const auto& output = GetSpectrum();
            if (output.empty() || frequency <= 0.0f || frequency >= mFFTAnalysis.GetNyquistFrequency())
                return -80.0f;
            const float frequencyResolution = GetFrequencyResolution();
            const float binIndex = frequency / frequencyResolution;
            const int lowerBin = static_cast<int>(std::floor(binIndex));
            const int upperBin = std::min(lowerBin + 1, static_cast<int>(output.size()) - 1);
            if (lowerBin >= static_cast<int>(output.size()) || lowerBin < 0)
                return -80.0f;
            if (lowerBin == upperBin)
                return output[lowerBin].magnitudeDB;
            const float fraction = binIndex - lowerBin;
            return output[lowerBin].magnitudeDB * (1.0f - fraction) + 
                   output[upperBin].magnitudeDB * fraction;
        }
        float GetFrequencyResolution() const override { return mFFTAnalysis.GetFrequencyResolution(); }
        float GetNyquistFrequency() const override { return mFFTAnalysis.GetNyquistFrequency(); }

        void Reset()
        {
            for (auto& detector : mPeakDetectors) detector.Reset();
            mFFTAnalysis.Reset();
            mHasNewOutput.store(false, std::memory_order_release);
        }

        // Output update signalling
        bool HasNewOutput() const { return mHasNewOutput.load(std::memory_order_acquire); }
        void ConsumeOutput() { mHasNewOutput.store(false, std::memory_order_release); }
    };
}

#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT
