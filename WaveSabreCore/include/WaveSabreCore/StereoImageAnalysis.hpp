#pragma once

#include "Device.h"
#include <vector>
#include <complex>
#include <cmath>
#include <mutex>
#include <atomic>

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT

#include "RMS.hpp" // for base analysis stream stuff
#include "FFTAnalysis.hpp"

namespace WaveSabreCore
{
    // Mid-Side frequency analyzer for stereo width analysis by frequency
    class MidSideFrequencyAnalyzer : public IFrequencyAnalysis {
    private:
        std::unique_ptr<SmoothedMonoFFT> mMidAnalyzer;
        std::unique_ptr<SmoothedMonoFFT> mSideAnalyzer;
        mutable std::vector<SpectrumBin> mWidthSpectrum; // Computed width spectrum (Side/Mid ratio) - mutable for lazy evaluation
        
        bool mEnabled = false;
        
        // Thread-safe lazy evaluation system
        mutable std::mutex mSpectrumMutex; // Protects mWidthSpectrum
        mutable std::atomic<bool> mNeedsWidthUpdate{false}; // Audio thread sets, GUI thread clears
        
    public:
        MidSideFrequencyAnalyzer() = default;
        ~MidSideFrequencyAnalyzer() = default;
        
        // Configuration
        inline void SetEnabled(bool enabled) {
            // Thread-safe configuration change
            std::lock_guard<std::mutex> lock(mSpectrumMutex);
            
            if (enabled && !mEnabled) {
                // Initialize FFT analyzers (mono)
                mMidAnalyzer = std::make_unique<SmoothedMonoFFT>();
                mSideAnalyzer = std::make_unique<SmoothedMonoFFT>();
                
                // Configure for stereo width analysis - use smaller FFT size for responsive display
                mMidAnalyzer->SetSampleRate(Helpers::CurrentSampleRateF);
                mSideAnalyzer->SetSampleRate(Helpers::CurrentSampleRateF);
                
                // Set professional display parameters
                mMidAnalyzer->SetFFTSmoothing(0.3f);  // Light technical smoothing
                mSideAnalyzer->SetFFTSmoothing(0.3f);
                mMidAnalyzer->SetOverlapFactor(4);    // 75% overlap for smooth updates
                mSideAnalyzer->SetOverlapFactor(4);
                mMidAnalyzer->SetPeakHoldTime(60.0f); // Short hold for responsive width display
                mSideAnalyzer->SetPeakHoldTime(60.0f);
                mMidAnalyzer->SetFalloffRate(1200.0f); // Medium falloff
                mSideAnalyzer->SetFalloffRate(1200.0f);
                
            } else if (!enabled && mEnabled) {
                // Clean up analyzers
                mMidAnalyzer.reset();
                mSideAnalyzer.reset();
                mWidthSpectrum.clear();
            }
            
            mEnabled = enabled;
            mNeedsWidthUpdate.store(false, std::memory_order_release);
        }
        
        bool IsEnabled() const { return mEnabled; }
        
        inline void SetSampleRate(float sampleRate) {
            if (mMidAnalyzer) mMidAnalyzer->SetSampleRate(sampleRate);
            if (mSideAnalyzer) mSideAnalyzer->SetSampleRate(sampleRate);
        }
        
        inline void SetFFTConfiguration(MonoFFTAnalysis::FFTSize /*fftSize*/, MonoFFTAnalysis::WindowType windowType) {
            if (mMidAnalyzer) mMidAnalyzer->SetWindowType(windowType);
            if (mSideAnalyzer) mSideAnalyzer->SetWindowType(windowType);
            // Note: FFT size changes would require reconstruction - not implemented for simplicity
        }
        
        inline void SetSmoothingParameters(float peakHoldTimeMs, float falloffTimeMs) {
            if (mMidAnalyzer) {
                mMidAnalyzer->SetPeakHoldTime(peakHoldTimeMs);
                mMidAnalyzer->SetFalloffRate(falloffTimeMs);
            }
            if (mSideAnalyzer) {
                mSideAnalyzer->SetPeakHoldTime(peakHoldTimeMs);
                mSideAnalyzer->SetFalloffRate(falloffTimeMs);
            }
        }
        
        // Processing
        // AUDIO THREAD: Lightweight processing - feed samples and only mark update when new spectrum arrives
        inline void ProcessMidSideSamples(float midSample, float sideSample) {
            if (!mEnabled || !mMidAnalyzer || !mSideAnalyzer) return;
            
            // Feed mono analyzers (true mono, not duplicated stereo)
            mMidAnalyzer->ProcessSample(midSample);
            mSideAnalyzer->ProcessSample(sideSample);
            
            // Only signal width update when either analyzer has new output ready
            if (mMidAnalyzer->HasNewOutput() || mSideAnalyzer->HasNewOutput()) {
                mNeedsWidthUpdate.store(true, std::memory_order_release);
                // Consume output flags so we don't spin recomputing
                mMidAnalyzer->ConsumeOutput();
                mSideAnalyzer->ConsumeOutput();
            }
        }
        
        inline void Reset() {
            // Thread-safe reset with mutex protection
            std::lock_guard<std::mutex> lock(mSpectrumMutex);
            
            if (mMidAnalyzer) mMidAnalyzer->Reset();
            if (mSideAnalyzer) mSideAnalyzer->Reset();
            mWidthSpectrum.clear();
            mNeedsWidthUpdate.store(false, std::memory_order_release);
        }
        
        // IFrequencyAnalysis implementation - returns width spectrum (THREAD-SAFE)
        const std::vector<SpectrumBin>& GetSpectrum() const override { 
            // Trigger lazy evaluation if needed (GUI thread)
            UpdateWidthSpectrumIfNeeded();
            
            // Return the spectrum (protected by mutex in UpdateWidthSpectrumIfNeeded)
            return mWidthSpectrum; 
        }
        
        inline float GetMagnitudeAtFrequency(float frequency) const override {
            // Trigger lazy evaluation if needed
            UpdateWidthSpectrumIfNeeded();
            
            // Thread-safe access to spectrum data
            std::lock_guard<std::mutex> lock(mSpectrumMutex);
            return GetMagnitudeAtFrequencyFromSpectrum(mWidthSpectrum, frequency);
        }
        
        float GetFrequencyResolution() const override {
            if (mMidAnalyzer) {
                return mMidAnalyzer->GetFrequencyResolution();
            }
            return 0.0f;
        }
        
        float GetNyquistFrequency() const override {
            if (mMidAnalyzer) {
                return mMidAnalyzer->GetNyquistFrequency();
            }
            return 24000.0f; // Fallback
        }
        
        // Access individual spectra for advanced visualization
        const IFrequencyAnalysis* GetMidAnalyzer() const { return mMidAnalyzer.get(); }
        const IFrequencyAnalysis* GetSideAnalyzer() const { return mSideAnalyzer.get(); }
        
    private:
        inline float GetMagnitudeAtFrequencyFromSpectrum(const std::vector<SpectrumBin>& spectrum, float frequency) const {
            if (spectrum.empty() || frequency <= 0.0f || frequency >= GetNyquistFrequency()) {
                return 0.0f; // Return 0 width for out of range
            }
            
            const float frequencyResolution = GetFrequencyResolution();
            if (frequencyResolution <= 0.0f) return 0.0f;
            
            const float binIndex = frequency / frequencyResolution;
            const int lowerBin = static_cast<int>(std::floor(binIndex));
            const int upperBin = std::min(lowerBin + 1, static_cast<int>(spectrum.size()) - 1);
            
            if (lowerBin >= static_cast<int>(spectrum.size()) || lowerBin < 0) {
                return 0.0f;
            }
            
            if (lowerBin == upperBin) {
                return spectrum[lowerBin].magnitudeDB;
            }
            
            // Linear interpolation between bins
            const float fraction = binIndex - lowerBin;
            return spectrum[lowerBin].magnitudeDB * (1.0f - fraction) + 
                   spectrum[upperBin].magnitudeDB * fraction;
        }
        
        // GUI THREAD: Expensive width computation - only when GUI requests it
        void UpdateWidthSpectrumIfNeeded() const {
            // Quick atomic check - avoid mutex if no update needed
            if (!mNeedsWidthUpdate.load(std::memory_order_acquire)) {
                return;
            }
            
            // Lock mutex for thread-safe access to spectrum data
            std::lock_guard<std::mutex> lock(mSpectrumMutex);
            
            // Double-check pattern - another thread might have updated already
            if (!mNeedsWidthUpdate.load(std::memory_order_acquire)) {
                return;
            }
            
            // Get current spectra from analyzers
            const auto& midSpectrum = mMidAnalyzer->GetSpectrum();
            const auto& sideSpectrum = mSideAnalyzer->GetSpectrum();
            
            // Safety check - ensure both spectra are valid
            if (midSpectrum.empty() || sideSpectrum.empty()) {
                return; // Wait for analyzers to have data
            }
            
            // Use the smaller size to avoid mismatch stalls
            size_t spectrumSize = std::min(midSpectrum.size(), sideSpectrum.size());
            
            // Resize width spectrum if needed (protected by mutex)
            if (mWidthSpectrum.size() != spectrumSize) {
                mWidthSpectrum.resize(spectrumSize);
            }
            
            // Compute width using dB delta -> linear ratio
            for (size_t i = 0; i < spectrumSize; ++i) {
                float frequency = midSpectrum[i].frequency;
                float midMagnitudeDB = midSpectrum[i].magnitudeDB;
                float sideMagnitudeDB = sideSpectrum[i].magnitudeDB;
                
                float widthLinear = 0.0f;
                float deltaDB = sideMagnitudeDB - midMagnitudeDB; // Side/Mid in dB
                // Convert to linear ratio (and clamp)
                widthLinear = M7::math::DecibelsToLinear(deltaDB);
                widthLinear = std::min(3.0f, std::max(0.0f, widthLinear));
                
                mWidthSpectrum[i].frequency = frequency;
                mWidthSpectrum[i].magnitudeDB = widthLinear; // store as linear width ratio
            }
            
            // Clear the update flag (we've computed the width)
            mNeedsWidthUpdate.store(false, std::memory_order_release);
        }
        
        bool HasNewSpectrum() const { 
            return mNeedsWidthUpdate.load(std::memory_order_acquire); 
        }
        
        void ConsumeSpectrum() { 
            mNeedsWidthUpdate.store(false, std::memory_order_release); 
        }
    };

    // Stereo imaging analysis stream for real-time stereo field visualization
    struct StereoImagingAnalysisStream {
        RMSDetector mLeftLevelDetector;   // *** LEFT CHANNEL RMS ***
        RMSDetector mRightLevelDetector;  // *** RIGHT CHANNEL RMS ***
        RMSDetector mWidthDetector;       // For stereo width RMS

        AnalysisStream mMidLevelDetector;
        AnalysisStream mSideLevelDetector;

        // Current analysis values (professionally smoothed)
        double mPhaseCorrelation = 0.0;  // -1 to +1, -1 = out of phase, 0 = uncorrelated, +1 = mono
        double mStereoWidth = 0.0;       // 0 = mono, 1 = normal stereo, >1 = wide  
        double mStereoBalance = 0.0;     // -1 to +1, -1 = full left, 0 = center, +1 = full right
        double mLeftLevel = 0.0;         // *** LEFT CHANNEL RMS LEVEL ***
        double mRightLevel = 0.0;        // *** RIGHT CHANNEL RMS LEVEL ***
        // Smoothing filters for display stability
        static constexpr double gSmoothingFactor = 0.8; // For correlation (sample-accurate calculation)

        // History buffer for phase scope visualization  
        static constexpr size_t gHistorySize = 256;
        struct StereoSample {
            float left = 0.0f;
            float right = 0.0f;
        };
        StereoSample mHistory[gHistorySize];
        size_t mHistoryIndex = 0;

        // Correlation calculation helpers
        double mLeftSum = 0.0;
        double mRightSum = 0.0;
        double mLeftSquaredSum = 0.0;
        double mRightSquaredSum = 0.0;
        double mLRProductSum = 0.0;
        size_t mCorrelationSampleCount = 0;
        static constexpr size_t gCorrelationWindowSize = 4800; // ~100ms at 48kHz

        // NEW: Frequency-domain analysis for stereo imaging
        std::unique_ptr<MidSideFrequencyAnalyzer> mFrequencyAnalyzer;

        explicit StereoImagingAnalysisStream() {
            // Configure professional RMS detectors with appropriate time constants
            mLeftLevelDetector.SetWindowSize(200);
            mRightLevelDetector.SetWindowSize(200);
            mWidthDetector.SetWindowSize(200);
            
            // Initialize frequency analyzer (disabled by default for performance)
            mFrequencyAnalyzer = std::make_unique<MidSideFrequencyAnalyzer>();
            
            Reset();
        }

        inline void WriteStereoSample(double left, double right) {
            // Update history buffer for phase scope
            mHistory[mHistoryIndex].left = static_cast<float>(left);
            mHistory[mHistoryIndex].right = static_cast<float>(right);
            mHistoryIndex = (mHistoryIndex + 1) % gHistorySize;

            // *** PROCESS L/R CHANNELS WITH DEDICATED RMS DETECTORS ***
            mLeftLevel = mLeftLevelDetector.ProcessSample(left);   // Direct L/R RMS
            mRightLevel = mRightLevelDetector.ProcessSample(right); // No abs() needed - RMS detector handles it internally

            // Calculate mid-side components
            double mid = (left + right) * 0.5;
            double side = (left - right) * 0.5;

            mMidLevelDetector.WriteSample(mid);
            mSideLevelDetector.WriteSample(side);

            // Process frequency-domain analysis if enabled
            if (mFrequencyAnalyzer && mFrequencyAnalyzer->IsEnabled()) {
                mFrequencyAnalyzer->ProcessMidSideSamples(static_cast<float>(mid), static_cast<float>(side));
            }

            if (mMidLevelDetector.mCurrentPeak > 0.0001) {
                double instantWidth = mSideLevelDetector.mCurrentPeak / mMidLevelDetector.mCurrentPeak;
                mStereoWidth = mWidthDetector.ProcessSample(instantWidth);
            }

            double totalLevel = mLeftLevel + mRightLevel;
            if (totalLevel > 0.0001) {
                mStereoBalance = (mRightLevel - mLeftLevel) / totalLevel;
            }

            // Update correlation calculation (keep existing sample-accurate method)
            mLeftSum += left;
            mRightSum += right;
            mLeftSquaredSum += left * left;
            mRightSquaredSum += right * right;
            mLRProductSum += left * right;
            mCorrelationSampleCount++;

            // Calculate correlation periodically to avoid overflow and keep it current
            if (mCorrelationSampleCount >= gCorrelationWindowSize) {
                UpdateCorrelation();
                // Reset accumulators but keep some history for stability
                mLeftSum *= 0.5;
                mRightSum *= 0.5;
                mLeftSquaredSum *= 0.5;
                mRightSquaredSum *= 0.5;
                mLRProductSum *= 0.5;
                mCorrelationSampleCount /= 2;
            }
        }

        inline void Reset() {
            mPhaseCorrelation = 0.0;
            mStereoWidth = 0.0;
            mStereoBalance = 0.0;
            mLeftLevel = 0.0;
            mRightLevel = 0.0;
            mHistoryIndex = 0;

            // Reset professional detectors
            mLeftLevelDetector.Reset();
            mRightLevelDetector.Reset();
            mMidLevelDetector.Reset();
            mSideLevelDetector.Reset();
            mWidthDetector.Reset();

            // Reset frequency analyzer
            if (mFrequencyAnalyzer) {
                mFrequencyAnalyzer->Reset();
            }

            for (auto& sample : mHistory) {
                sample.left = sample.right = 0.0f;
            }

            mLeftSum = mRightSum = 0.0;
            mLeftSquaredSum = mRightSquaredSum = 0.0;
            mLRProductSum = 0.0;
            mCorrelationSampleCount = 0;
        }

        // Frequency analysis control methods
        inline void SetFrequencyAnalysisEnabled(bool enabled) {
            if (mFrequencyAnalyzer) {
                mFrequencyAnalyzer->SetEnabled(enabled);
            }
        }
        
        bool IsFrequencyAnalysisEnabled() const {
            return mFrequencyAnalyzer && mFrequencyAnalyzer->IsEnabled();
        }
        
        inline void ConfigureFrequencyAnalysis(MonoFFTAnalysis::FFTSize fftSize = MonoFFTAnalysis::FFTSize::FFT1024,
                                       MonoFFTAnalysis::WindowType windowType = MonoFFTAnalysis::WindowType::Hanning,
                                       float peakHoldTimeMs = 60.0f,
                                       float falloffTimeMs = 1200.0f) {
            if (mFrequencyAnalyzer) {
                mFrequencyAnalyzer->SetFFTConfiguration(fftSize, windowType);
                mFrequencyAnalyzer->SetSmoothingParameters(peakHoldTimeMs, falloffTimeMs);
            }
        }

        // Access frequency analysis data for visualization
        const MidSideFrequencyAnalyzer* GetFrequencyAnalyzer() const {
            return mFrequencyAnalyzer.get();
        }

        // Get the current history buffer for visualization
        const StereoSample* GetHistoryBuffer() const { return mHistory; }
        size_t GetHistorySize() const { return gHistorySize; }
        size_t GetCurrentHistoryIndex() const { return mHistoryIndex; }

    private:
        inline void UpdateCorrelation() {
            if (mCorrelationSampleCount < 2) {
                mPhaseCorrelation = 0.0;
                return;
            }

            double n = static_cast<double>(mCorrelationSampleCount);

            // Calculate means
            double leftMean = mLeftSum / n;
            double rightMean = mRightSum / n;

            // Calculate variances and covariance
            double leftVariance = (mLeftSquaredSum / n) - (leftMean * leftMean);
            double rightVariance = (mRightSquaredSum / n) - (rightMean * rightMean);
            double covariance = (mLRProductSum / n) - (leftMean * rightMean);

            // Calculate correlation coefficient
            double denominator = std::sqrt(leftVariance * rightVariance);
            if (denominator > 0.0001) {
                double instantCorrelation = covariance / denominator;
                // Clamp to valid range and apply smoothing
                instantCorrelation = std::max(-1.0, std::min(1.0, instantCorrelation));
                mPhaseCorrelation = mPhaseCorrelation * gSmoothingFactor + instantCorrelation * (1.0 - gSmoothingFactor);
            }
        }
    };
}

#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT
