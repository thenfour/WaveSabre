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
        std::vector<SpectrumBin> mOutput;

        int mSamplesPerFFTUpdate; // How many samples between FFT updates (e.g., 512)

        // Track current settings to avoid overwriting each other
        float mCurrentHoldTimeMs;
        float mCurrentFalloffTimeMs;

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
        const std::vector<SpectrumBin>& GetSpectrum() const { return mOutput; }
        float GetMagnitudeAtFrequency(float frequency) const override;
        float GetFrequencyResolution() const override { return mFFTAnalysis.GetFrequencyResolution(); }
        float GetNyquistFrequency() const override { return mFFTAnalysis.GetNyquistFrequency(); }

        void Reset();
    };
}

#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT
