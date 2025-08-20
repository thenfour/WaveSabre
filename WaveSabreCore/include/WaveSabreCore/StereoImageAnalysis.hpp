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
        mutable std::mutex mAnalyzerMutex; // protects mMidAnalyzer/mSideAnalyzer and their access
        
    public:
        MidSideFrequencyAnalyzer() = default;
        ~MidSideFrequencyAnalyzer() = default;
        
        // Configuration
        inline void SetEnabled(bool enabled) {
            // Lock analyzers while changing
            std::lock_guard<std::mutex> aLock(mAnalyzerMutex);
            // Lock spectrum buffer while changing
            std::lock_guard<std::mutex> sLock(mSpectrumMutex);
            
            if (enabled && !mEnabled) {
                mMidAnalyzer = std::make_unique<SmoothedMonoFFT>();
                mSideAnalyzer = std::make_unique<SmoothedMonoFFT>();
                
                mMidAnalyzer->SetSampleRate(Helpers::CurrentSampleRateF);
                mSideAnalyzer->SetSampleRate(Helpers::CurrentSampleRateF);

                mMidAnalyzer->SetFFTSize(MonoFFTAnalysis::FFTSize::FFT512);
                mSideAnalyzer->SetFFTSize(MonoFFTAnalysis::FFTSize::FFT512);

                mMidAnalyzer->SetFFTSmoothing(0.7f);
                mSideAnalyzer->SetFFTSmoothing(0.7f);
                mMidAnalyzer->SetOverlapFactor(4);
                mSideAnalyzer->SetOverlapFactor(4);

                mMidAnalyzer->SetPeakHoldTime(100);
                mSideAnalyzer->SetPeakHoldTime(100);

                mMidAnalyzer->SetFalloffRate(3000.0f);
                mSideAnalyzer->SetFalloffRate(3000.0f);
                
            } else if (!enabled && mEnabled) {
                // release analyzers safely
                mMidAnalyzer.reset();
                mSideAnalyzer.reset();
                mWidthSpectrum.clear();
            }
            
            mEnabled = enabled;
            mNeedsWidthUpdate.store(false, std::memory_order_release);
        }
        
        bool IsEnabled() const { return mEnabled; }
        
        inline void SetSampleRate(float sampleRate) {
            std::lock_guard<std::mutex> aLock(mAnalyzerMutex);
            if (mMidAnalyzer) mMidAnalyzer->SetSampleRate(sampleRate);
            if (mSideAnalyzer) mSideAnalyzer->SetSampleRate(sampleRate);
        }
        
        inline void SetFFTConfiguration(MonoFFTAnalysis::FFTSize /*fftSize*/, MonoFFTAnalysis::WindowType windowType) {
            std::lock_guard<std::mutex> aLock(mAnalyzerMutex);
            if (mMidAnalyzer) mMidAnalyzer->SetWindowType(windowType);
            if (mSideAnalyzer) mSideAnalyzer->SetWindowType(windowType);
        }
        
        inline void SetSmoothingParameters(float peakHoldTimeMs, float falloffTimeMs) {
            std::lock_guard<std::mutex> aLock(mAnalyzerMutex);
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
        // AUDIO THREAD
        inline void ProcessMidSideSamples(float midSample, float sideSample) {
            if (!mEnabled) return;
            std::lock_guard<std::mutex> aLock(mAnalyzerMutex);
            if (!mMidAnalyzer || !mSideAnalyzer) return;
            
            mMidAnalyzer->ProcessSample(midSample);
            mSideAnalyzer->ProcessSample(sideSample);
            
            if (mMidAnalyzer->HasNewOutput() || mSideAnalyzer->HasNewOutput()) {
                mNeedsWidthUpdate.store(true, std::memory_order_release);
                mMidAnalyzer->ConsumeOutput();
                mSideAnalyzer->ConsumeOutput();
            }
        }
        
        inline void Reset() {
            std::lock_guard<std::mutex> aLock(mAnalyzerMutex);
            std::lock_guard<std::mutex> sLock(mSpectrumMutex);
            if (mMidAnalyzer) mMidAnalyzer->Reset();
            if (mSideAnalyzer) mSideAnalyzer->Reset();
            mWidthSpectrum.clear();
            mNeedsWidthUpdate.store(false, std::memory_order_release);
        }
        
        // IFrequencyAnalysis implementation - THREAD-SAFE
        const std::vector<SpectrumBin>& GetSpectrum() const override { 
            UpdateWidthSpectrumIfNeeded();
            return mWidthSpectrum; 
        }
        
        inline float GetMagnitudeAtFrequency(float frequency) const override {
            UpdateWidthSpectrumIfNeeded();
            std::lock_guard<std::mutex> lock(mSpectrumMutex);
            return GetMagnitudeAtFrequencyFromSpectrum(mWidthSpectrum, frequency);
        }
        
        float GetFrequencyResolution() const override {
            std::lock_guard<std::mutex> aLock(mAnalyzerMutex);
            if (mMidAnalyzer) {
                return mMidAnalyzer->GetFrequencyResolution();
            }
            return 0.0f;
        }
        
        float GetNyquistFrequency() const override {
            std::lock_guard<std::mutex> aLock(mAnalyzerMutex);
            if (mMidAnalyzer) {
                return mMidAnalyzer->GetNyquistFrequency();
            }
            return 24000.0f; // Fallback
        }
        
        // Access individual spectra for advanced visualization
        const IFrequencyAnalysis* GetMidAnalyzer() const { std::lock_guard<std::mutex> aLock(mAnalyzerMutex); return mMidAnalyzer.get(); }
        const IFrequencyAnalysis* GetSideAnalyzer() const { std::lock_guard<std::mutex> aLock(mAnalyzerMutex); return mSideAnalyzer.get(); }
        
    private:
        inline float GetMagnitudeAtFrequencyFromSpectrum(const std::vector<SpectrumBin>& spectrum, float frequency) const {
            if (spectrum.empty() || frequency <= 0.0f || frequency >= GetNyquistFrequency()) {
                return 0.0f;
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
            const float fraction = binIndex - lowerBin;
            return spectrum[lowerBin].magnitudeDB * (1.0f - fraction) + 
                   spectrum[upperBin].magnitudeDB * fraction;
        }
        
        // GUI THREAD
        void UpdateWidthSpectrumIfNeeded() const {
            if (!mNeedsWidthUpdate.load(std::memory_order_acquire)) return;
            std::lock_guard<std::mutex> aLock(mAnalyzerMutex);
            if (!mMidAnalyzer || !mSideAnalyzer) return;
            
            // Collect current spectra from analyzers (thread-safe by analyzer mutex)
            const auto& midSpectrum = mMidAnalyzer->GetSpectrum();
            const auto& sideSpectrum = mSideAnalyzer->GetSpectrum();
            if (midSpectrum.empty() || sideSpectrum.empty()) return;
            
            size_t spectrumSize = std::min(midSpectrum.size(), sideSpectrum.size());
            std::lock_guard<std::mutex> sLock(mSpectrumMutex);
            if (mWidthSpectrum.size() != spectrumSize) mWidthSpectrum.resize(spectrumSize);
            
            // Confidence weighting thresholds for stability in low-energy bins (use Mid only)
            const float kFadeStartDB = -30.0f;  // start trusting less below this
            const float kFadeEndDB   = -80.0f;  // fully faded (treated as mono) below this

            auto saturate01 = [](float x) { return x < 0.f ? 0.f : (x > 1.f ? 1.f : x); };
            auto smoothstep = [&](float lo, float hi, float x) {
                // lo < hi; returns 0..1
                float t = saturate01((x - lo) / (hi - lo));
                return t * t * (3.0f - 2.0f * t);
            };
            
            for (size_t i = 0; i < spectrumSize; ++i) {
                float frequency = midSpectrum[i].frequency;
                float midMagnitudeDB = midSpectrum[i].magnitudeDB;
                float sideMagnitudeDB = sideSpectrum[i].magnitudeDB;
                float deltaDB = sideMagnitudeDB - midMagnitudeDB;
                float widthLinear = M7::math::DecibelsToLinear(deltaDB);
                widthLinear = std::min(3.0f, std::max(0.0f, widthLinear));

                // Confidence based on Mid energy (denominator)
                const float t = smoothstep(kFadeEndDB, kFadeStartDB, midMagnitudeDB);

                // Silence considered mono: blend width to 0 as Mid energy vanishes
                const float widthConfident = widthLinear * t;
                
                mWidthSpectrum[i].frequency = frequency;
                mWidthSpectrum[i].magnitudeDB = widthConfident; // carries linear width for renderer
            }
            mNeedsWidthUpdate.store(false, std::memory_order_release);
        }
        
        bool HasNewSpectrum() const { return mNeedsWidthUpdate.load(std::memory_order_acquire); }
        void ConsumeSpectrum() { mNeedsWidthUpdate.store(false, std::memory_order_release); }
    };

    // Stereo imaging analysis stream for real-time stereo field visualization
    struct StereoImagingAnalysisStream {
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

        static constexpr size_t gHistorySize = 256;
        struct StereoSample { float left = 0.0f; float right = 0.0f; };
        StereoSample mHistory[gHistorySize];
        size_t mHistoryIndex = 0;

        double mLeftSum = 0.0; double mRightSum = 0.0; double mLeftSquaredSum = 0.0; double mRightSquaredSum = 0.0; double mLRProductSum = 0.0; size_t mCorrelationSampleCount = 0;
        static constexpr size_t gCorrelationWindowSize = 4800;

        std::unique_ptr<MidSideFrequencyAnalyzer> mFrequencyAnalyzer;

        explicit StereoImagingAnalysisStream() {
            mLeftLevelDetector.SetWindowSize(200);
            mRightLevelDetector.SetWindowSize(200);
            mWidthDetector.SetWindowSize(200);
            mFrequencyAnalyzer = std::make_unique<MidSideFrequencyAnalyzer>();
            Reset();
        }

        inline void WriteStereoSample(double left, double right) {
            mHistory[mHistoryIndex].left = static_cast<float>(left);
            mHistory[mHistoryIndex].right = static_cast<float>(right);
            mHistoryIndex = (mHistoryIndex + 1) % gHistorySize;

            mLeftLevel = mLeftLevelDetector.ProcessSample(left);
            mRightLevel = mRightLevelDetector.ProcessSample(right);

            double mid = (left + right) * 0.5;
            double side = (left - right) * 0.5;

            mMidLevelDetector.WriteSample(mid);
            mSideLevelDetector.WriteSample(side);

            if (mFrequencyAnalyzer && mFrequencyAnalyzer->IsEnabled()) {
                mFrequencyAnalyzer->ProcessMidSideSamples(static_cast<float>(mid), static_cast<float>(side));
            }

            if (mMidLevelDetector.mCurrentPeak > 0.0001) {
                double instantWidth = mSideLevelDetector.mCurrentPeak / mMidLevelDetector.mCurrentPeak;
                mStereoWidth = mWidthDetector.ProcessSample(instantWidth);
            }

            double totalLevel = mLeftLevel + mRightLevel;
            if (totalLevel > 0.0001) mStereoBalance = (mRightLevel - mLeftLevel) / totalLevel;

            mLeftSum += left; mRightSum += right; mLeftSquaredSum += left * left; mRightSquaredSum += right * right; mLRProductSum += left * right; mCorrelationSampleCount++;
            if (mCorrelationSampleCount >= gCorrelationWindowSize) {
                UpdateCorrelation();
                mLeftSum *= 0.5; mRightSum *= 0.5; mLeftSquaredSum *= 0.5; mRightSquaredSum *= 0.5; mLRProductSum *= 0.5; mCorrelationSampleCount /= 2;
            }
        }

        inline void Reset() {
            mPhaseCorrelation = 0.0; mStereoWidth = 0.0; mStereoBalance = 0.0; mLeftLevel = 0.0; mRightLevel = 0.0; mHistoryIndex = 0;
            mLeftLevelDetector.Reset(); mRightLevelDetector.Reset(); mMidLevelDetector.Reset(); mSideLevelDetector.Reset(); mWidthDetector.Reset();
            if (mFrequencyAnalyzer) mFrequencyAnalyzer->Reset();
            for (auto& s : mHistory) { s.left = s.right = 0.0f; }
            mLeftSum = mRightSum = 0.0; mLeftSquaredSum = mRightSquaredSum = 0.0; mLRProductSum = 0.0; mCorrelationSampleCount = 0;
        }

        inline void SetFrequencyAnalysisEnabled(bool enabled) { if (mFrequencyAnalyzer) mFrequencyAnalyzer->SetEnabled(enabled); }
        bool IsFrequencyAnalysisEnabled() const { return mFrequencyAnalyzer && mFrequencyAnalyzer->IsEnabled(); }
        inline void ConfigureFrequencyAnalysis(MonoFFTAnalysis::FFTSize fftSize = MonoFFTAnalysis::FFTSize::FFT1024, MonoFFTAnalysis::WindowType windowType = MonoFFTAnalysis::WindowType::Hanning, float peakHoldTimeMs = 60.0f, float falloffTimeMs = 1200.0f) {
            if (mFrequencyAnalyzer) { mFrequencyAnalyzer->SetFFTConfiguration(fftSize, windowType); mFrequencyAnalyzer->SetSmoothingParameters(peakHoldTimeMs, falloffTimeMs); }
        }
        const MidSideFrequencyAnalyzer* GetFrequencyAnalyzer() const { return mFrequencyAnalyzer.get(); }
        const StereoSample* GetHistoryBuffer() const { return mHistory; }
        size_t GetHistorySize() const { return gHistorySize; }
        size_t GetCurrentHistoryIndex() const { return mHistoryIndex; }

    private:
        inline void UpdateCorrelation() {
            if (mCorrelationSampleCount < 2) { mPhaseCorrelation = 0.0; return; }
            double n = static_cast<double>(mCorrelationSampleCount);
            double leftMean = mLeftSum / n; double rightMean = mRightSum / n;
            double leftVariance = (mLeftSquaredSum / n) - (leftMean * leftMean);
            double rightVariance = (mRightSquaredSum / n) - (rightMean * rightMean);
            double covariance = (mLRProductSum / n) - (leftMean * rightMean);
            double denominator = std::sqrt(leftVariance * rightVariance);
            if (denominator > 0.0001) {
                double instantCorrelation = covariance / denominator;
                instantCorrelation = std::max(-1.0, std::min(1.0, instantCorrelation));
                mPhaseCorrelation = mPhaseCorrelation * gSmoothingFactor + instantCorrelation * (1.0 - gSmoothingFactor);
            }
        }
    };
}

#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT
