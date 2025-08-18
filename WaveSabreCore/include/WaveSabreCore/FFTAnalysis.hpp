#pragma once

#include "Device.h"
#include <vector>
#include <complex>
#include <cmath>

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT

#include "RMS.hpp"  // Include for PeakDetector

namespace WaveSabreCore
{
    // Common interface for frequency analysis data providers
    class IFrequencyAnalysis
    {
    public:
        struct SpectrumBin
        {
            float frequency;    // Hz
            float magnitudeDB;  // dB
            float phase;        // radians (if needed)
        };
        
        virtual ~IFrequencyAnalysis() = default;
        
        // Get spectrum data for rendering
        virtual const std::vector<SpectrumBin>& GetSpectrumLeft() const = 0;
        virtual const std::vector<SpectrumBin>& GetSpectrumRight() const = 0;
        
        // Get magnitude at specific frequency (interpolated if necessary)
        virtual float GetMagnitudeAtFrequency(float frequency, bool useRightChannel = false) const = 0;
        
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

        struct SpectrumBin
        {
            float frequency;    // Hz
            float magnitudeDB;  // dB
            float phase;        // radians (if needed)
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

    // Single-channel spectrum display smoother - now uses existing PeakDetector!
    class MonoSpectrumDisplaySmoother
    {
    private:
        // Use frequency-dependent peak detectors for professional behavior
        std::vector<FrequencyDependentPeakDetector> mPeakDetectors;
        std::vector<MonoFFTAnalysis::SpectrumBin> mOutput;
        
        float mSampleRate;
        int mSamplesPerFFTUpdate; // How many samples between FFT updates (e.g., 512)
        
        // Track current settings to avoid overwriting each other
        float mCurrentHoldTimeMs;
        float mCurrentFalloffTimeMs;

    public:
        MonoSpectrumDisplaySmoother();
        
        // Configure display behavior
        void SetPeakHoldTime(float holdTimeMs, float sampleRate);
        void SetFalloffRate(float falloffTimeMs, float sampleRate);  // Time for -60dB falloff
        void SetFFTUpdateRate(int fftSize, int overlapFactor); // Update timing parameters

		float GetPeakHoldTime() const { return mCurrentHoldTimeMs; }
		float GetFalloffTime() const { return mCurrentFalloffTimeMs; }

        // Process new FFT data for display
        void ProcessSpectrum(const std::vector<MonoFFTAnalysis::SpectrumBin>& rawSpectrum);
        
        // Get display-ready smoothed spectrum
        const std::vector<MonoFFTAnalysis::SpectrumBin>& GetSpectrum() const { return mOutput; }
        
        // Get magnitude at specific frequency (interpolated if necessary)
        float GetMagnitudeAtFrequency(float frequency, float sampleRate) const;
        
        void Reset();
    };

    // Stereo FFT analyzer wrapper - implements IFrequencyAnalysis interface
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
		//int GetFFTSizeInt() const { return mAnalyzers[0].GetFFTSizeInt(); }
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
        virtual const std::vector<SpectrumBin>& GetSpectrumLeft() const override;
        virtual const std::vector<SpectrumBin>& GetSpectrumRight() const override;
        virtual float GetMagnitudeAtFrequency(float frequency, bool useRightChannel = false) const override;
        virtual float GetFrequencyResolution() const override;
        virtual float GetNyquistFrequency() const override;
        
        // Clear/reset state
        void Reset();
    };

    // Stereo spectrum display smoother wrapper - implements IFrequencyAnalysis interface
    class SpectrumDisplaySmoother : public IFrequencyAnalysis
    {
    private:
        MonoSpectrumDisplaySmoother mSmoothers[2]; // Left and right channel smoothers
        float mSampleRate;
        
    public:
        SpectrumDisplaySmoother();
        
        // Configure display behavior
        void SetPeakHoldTime(float holdTimeMs, float sampleRate);
        void SetFalloffRate(float falloffTimeMs, float sampleRate);
        void SetFFTUpdateRate(int fftSize, int overlapFactor); // Update timing parameters
        
        // get peak hold time.
		float GetPeakHoldTime() const { return mSmoothers[0].GetPeakHoldTime(); }
		float GetFalloffTime() const { return mSmoothers[0].GetFalloffTime(); }

        // Process new FFT data for display
        void ProcessSpectrum(const std::vector<SpectrumBin>& rawSpectrum, bool isRightChannel);
        
        // IFrequencyAnalysis implementation
        virtual const std::vector<SpectrumBin>& GetSpectrumLeft() const override;
        virtual const std::vector<SpectrumBin>& GetSpectrumRight() const override;
        virtual float GetMagnitudeAtFrequency(float frequency, bool useRightChannel = false) const override;
        virtual float GetFrequencyResolution() const override;
        virtual float GetNyquistFrequency() const override;
        
        void Reset();
    };
}

#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT
