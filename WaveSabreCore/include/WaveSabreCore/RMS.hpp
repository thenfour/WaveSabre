#pragma once

#include "Maj7Basic.hpp"

namespace WaveSabreCore
{
	static inline float CalcFollowerCoef(float ms) {
        auto samples = M7::math::MillisecondsToSamples(ms);
        return M7::math::expf(-1.0f / samples);
        //return M7::math::expf(-1.0f / (Helpers::CurrentSampleRateF * ms / 1000.0f));
    }

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT

	struct RMSDetector
	{
		double mContinuousValue = 0;

        double mWindowMS = 0;
        double mAlpha = 0;

        void Reset() {
            mContinuousValue = 0;
        }
		// because this is not a perfect curve, the "milliseconds" is a misnomer, it's a sort of approximation.
		void SetWindowSize(double ms) {
			if (ms == mWindowMS) return;
			mWindowMS = ms;
			mAlpha = 1.0 - CalcFollowerCoef(float(ms));
		}

		// returns the current "RMS"
		double ProcessSample(double s) {
			// a sort of one-pole LP filter to smooth continuously, without requiring a memory buffer
			mContinuousValue = M7::math::lerpD(mContinuousValue, s * s, mAlpha);
			return M7::math::sqrt(float(mContinuousValue));
		}

	}; // struct RMSDetector

    struct PeakDetector
    {
        int mClipHoldSamples;
        int mPeakHoldSamples;
        double mPeakFalloffPerSample;

        // state
        int mClipHoldCounter = 0;
        int mPeakHoldCounter = 0;

        // running values
        bool mClipIndicator = 0;
        double mCurrentPeak = 0; // the current peak rectified value, accounting for hold & falloff

        void SetParams(double clipHoldMS, double peakHoldMS, double peakFalloffMaxMS) {
            mClipHoldSamples = int(clipHoldMS * Helpers::CurrentSampleRateF / 1000);
            mPeakHoldSamples = int(peakHoldMS * Helpers::CurrentSampleRateF / 1000);
            mPeakFalloffPerSample = std::max(0.0000001, 1.0 / (peakFalloffMaxMS * Helpers::CurrentSampleRateF / 1000));
            Reset();
        }

        void Reset() {
            mClipHoldCounter = 0;
            mPeakHoldCounter = 0;
            mClipIndicator = 0;
            mCurrentPeak = 0;
        }

        void ProcessSample(double s) {
            double rectifiedSample = fabs(s); // Assuming 's' is the input sample

            // Clip detection
            if (rectifiedSample >= 1) {
                mClipIndicator = true;
                mClipHoldCounter = mClipHoldSamples;
            }
            else if (mClipHoldCounter > 0) {
                --mClipHoldCounter;
            }
            else {
                mClipIndicator = false;
            }

            // Peak detection and hold
            if (rectifiedSample >= mCurrentPeak) {
                mCurrentPeak = rectifiedSample; // new peak; reset hold & falloff
                mPeakHoldCounter = mPeakHoldSamples;
            }
            else if (mPeakHoldCounter > 0) {
                --mPeakHoldCounter; // holding phase
            }
            else if (mCurrentPeak > 0) {
                mCurrentPeak -= mPeakFalloffPerSample; // falloff phase
                if (mCurrentPeak < 0) mCurrentPeak = 0; // Ensure mCurrentPeak doesn't go below 0
            }

        }
    };

    struct IAnalysisStream {
        double mCurrentRMSValue = 0;
        bool mClipIndicator = 0;
        double mCurrentPeak = 0;
        double mCurrentHeldPeak = 0;
        virtual void Reset() = 0;
    };

    // for VST-only, things like VU meters and history graphs require outputting many additional channels of audio.
    // this class goes in the Device, feed it samples. the VST can then access them in a safe way.
    struct AnalysisStream : IAnalysisStream {
        RMSDetector mRMSDetector;
        PeakDetector mPeakDetector;
        PeakDetector mPeakHoldDetector;

        explicit AnalysisStream(double peakFalloffMS = 1200) {
            mRMSDetector.SetWindowSize(200);
            mPeakDetector.SetParams(0, 0, peakFalloffMS);
            mPeakHoldDetector.SetParams(1000, 1000, 600);
        }

        void WriteSample(double s) {
            mCurrentRMSValue = mRMSDetector.ProcessSample(s);
            mPeakDetector.ProcessSample(s);
            mPeakHoldDetector.ProcessSample(s);
            mClipIndicator = mPeakHoldDetector.mClipIndicator;
            mCurrentPeak = mPeakDetector.mCurrentPeak;
            mCurrentHeldPeak = mPeakHoldDetector.mCurrentPeak;
        }

        virtual void Reset() override {
            mRMSDetector.Reset();
            mPeakHoldDetector.Reset();
            mPeakDetector.Reset();
            mCurrentRMSValue = 0;
            mClipIndicator = 0;
            mCurrentPeak = 0;
            mCurrentHeldPeak = 0;
        }
    };

    // Stereo imaging analysis stream for real-time stereo field visualization
    struct StereoImagingAnalysisStream {
        // Current analysis values (smoothed for display)
        double mPhaseCorrelation = 0.0;  // -1 to +1, -1 = out of phase, 0 = uncorrelated, +1 = mono
        double mStereoWidth = 0.0;       // 0 = mono, 1 = normal stereo, >1 = wide
        double mMidLevel = 0.0;          // Mid channel level
        double mSideLevel = 0.0;         // Side channel level
        
        // Smoothing filters for display stability
        static constexpr double gSmoothingFactor = 0.8; // Higher = more smoothing
        
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
        
        explicit StereoImagingAnalysisStream() {
            Reset();
        }
        
        void WriteStereoSample(double left, double right) {
            // Update history buffer for phase scope
            mHistory[mHistoryIndex].left = static_cast<float>(left);
            mHistory[mHistoryIndex].right = static_cast<float>(right);
            mHistoryIndex = (mHistoryIndex + 1) % gHistorySize;
            
            // Calculate mid-side components
            double mid = (left + right) * 0.5;
            double side = (left - right) * 0.5;
            
            // Update mid/side levels with smoothing
            double midAbs = std::abs(mid);
            double sideAbs = std::abs(side);
            mMidLevel = mMidLevel * gSmoothingFactor + midAbs * (1.0 - gSmoothingFactor);
            mSideLevel = mSideLevel * gSmoothingFactor + sideAbs * (1.0 - gSmoothingFactor);
            
            // Calculate stereo width (ratio of side to mid energy)
            if (mMidLevel > 0.0001) {
                double instantWidth = mSideLevel / mMidLevel;
                mStereoWidth = mStereoWidth * gSmoothingFactor + instantWidth * (1.0 - gSmoothingFactor);
            }
            
            // Update correlation calculation
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
        
        void Reset() {
            mPhaseCorrelation = 0.0;
            mStereoWidth = 0.0;
            mMidLevel = 0.0;
            mSideLevel = 0.0;
            mHistoryIndex = 0;
            
            for (auto& sample : mHistory) {
                sample.left = sample.right = 0.0f;
            }
            
            mLeftSum = mRightSum = 0.0;
            mLeftSquaredSum = mRightSquaredSum = 0.0;
            mLRProductSum = 0.0;
            mCorrelationSampleCount = 0;
        }
        
        // Get the current history buffer for visualization
        const StereoSample* GetHistoryBuffer() const { return mHistory; }
        size_t GetHistorySize() const { return gHistorySize; }
        size_t GetCurrentHistoryIndex() const { return mHistoryIndex; }
        
    private:
        void UpdateCorrelation() {
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

    // Attenuation/gain reduction analysis for compressor/clipper visualization
    struct AttenuationAnalysisStream : IAnalysisStream {
        PeakDetector mPeakDetector;
        PeakDetector mPeakHoldDetector;

        explicit AttenuationAnalysisStream(double peakFalloffMS = 1200) {
            mPeakDetector.SetParams(0, 0, peakFalloffMS);
            mPeakHoldDetector.SetParams(1000, 1000, peakFalloffMS);
        }

        void WriteSample(double s) {
            mPeakDetector.ProcessSample(s);
            mPeakHoldDetector.ProcessSample(s);
            mClipIndicator = mPeakHoldDetector.mClipIndicator;
            mCurrentPeak = mPeakDetector.mCurrentPeak;
            mCurrentHeldPeak = mPeakHoldDetector.mCurrentPeak;
        }

        virtual void Reset() override {
            mPeakHoldDetector.Reset();
            mPeakDetector.Reset();
            mClipIndicator = 0;
            mCurrentPeak = 0;
            mCurrentHeldPeak = 0;
        }
    };

    // Professional logarithmic peak detector for spectrum display
    // Uses exponential falloff in linear domain to achieve linear falloff in dB domain
    struct LogarithmicPeakDetector
    {
        int mClipHoldSamples;
        int mPeakHoldSamples;
        double mPeakFalloffMultiplierPerSample; // Exponential falloff multiplier for linear dB falloff

        // state
        int mClipHoldCounter = 0;
        int mPeakHoldCounter = 0;

        // running values
        bool mClipIndicator = 0;
        double mCurrentPeak = 0; // the current peak rectified value, accounting for hold & falloff

        void SetParams(double clipHoldMS, double peakHoldMS, double peakFalloffMaxMS) {
            mClipHoldSamples = int(clipHoldMS * Helpers::CurrentSampleRateF / 1000);
            mPeakHoldSamples = int(peakHoldMS * Helpers::CurrentSampleRateF / 1000);
            
            // Calculate exponential falloff multiplier for linear dB falloff
            // Goal: Fall 60dB in peakFalloffMaxMS milliseconds
            // 60dB = 20 * log10(ratio) ? ratio = 10^(60/20) = 1000
            // So we need: linearValue * multiplier^samples = linearValue / 1000
            // multiplier^samples = 1/1000 ? multiplier = (1/1000)^(1/samples)
            double falloffSamples = peakFalloffMaxMS * Helpers::CurrentSampleRateF / 1000.0;
            const double dBFalloffRange = 60.0; // Fall 60dB
            double linearFalloffRatio = std::pow(10.0, -dBFalloffRange / 20.0); // 10^(-60/20) = 0.001
            mPeakFalloffMultiplierPerSample = std::pow(linearFalloffRatio, 1.0 / falloffSamples);
            
            Reset();
        }

        void Reset() {
            mClipHoldCounter = 0;
            mPeakHoldCounter = 0;
            mClipIndicator = 0;
            mCurrentPeak = 0;
        }

        void ProcessSample(double s) {
            double rectifiedSample = fabs(s);

            // Clip detection
            if (rectifiedSample >= 1) {
                mClipIndicator = true;
                mClipHoldCounter = mClipHoldSamples;
            }
            else if (mClipHoldCounter > 0) {
                --mClipHoldCounter;
            }
            else {
                mClipIndicator = false;
            }

            // Peak detection and hold
            if (rectifiedSample >= mCurrentPeak) {
                mCurrentPeak = rectifiedSample; // new peak; reset hold & falloff
                mPeakHoldCounter = mPeakHoldSamples;
            }
            else if (mPeakHoldCounter > 0) {
                --mPeakHoldCounter; // holding phase
            }
            else if (mCurrentPeak > 0) {
                // EXPONENTIAL falloff in linear domain = LINEAR falloff in dB domain
                mCurrentPeak *= mPeakFalloffMultiplierPerSample;
                
                // Prevent underflow
                if (mCurrentPeak < 1e-10) mCurrentPeak = 0;
            }
        }
    };

    // Professional frequency-dependent peak detector for spectrum display
    // Uses different falloff rates per frequency band (like Pro-Q3, Ozone)
    struct FrequencyDependentPeakDetector
    {
        int mClipHoldSamples;
        int mPeakHoldSamples;
        double mBaseFalloffMultiplierPerSample; // Base exponential falloff multiplier
        float mFrequency; // The frequency this detector represents
        
        // Frequency-dependent falloff model parameters
        static constexpr float kLowFreqThreshold = 200.0f;   // Below this = slow falloff
        static constexpr float kHighFreqThreshold = 2000.0f; // Above this = fast falloff
        static constexpr float kLowFreqFactor = 0.3f;        // Low freq falloff multiplier (slower)
        static constexpr float kMidFreqFactor = 1.0f;        // Mid freq falloff multiplier (baseline)
        static constexpr float kHighFreqFactor = 2.5f;       // High freq falloff multiplier (faster)

        // state
        int mClipHoldCounter = 0;
        int mPeakHoldCounter = 0;

        // running values
        bool mClipIndicator = 0;
        double mCurrentPeak = 0;

        void SetParams(double clipHoldMS, double peakHoldMS, double peakFalloffMaxMS, float frequency) {
            mClipHoldSamples = int(clipHoldMS * Helpers::CurrentSampleRateF / 1000);
            mPeakHoldSamples = int(peakHoldMS * Helpers::CurrentSampleRateF / 1000);
            mFrequency = frequency;
            
            // Calculate base falloff multiplier (60dB falloff)
            double falloffSamples = peakFalloffMaxMS * Helpers::CurrentSampleRateF / 1000.0;
            const double dBFalloffRange = 60.0;
            double linearFalloffRatio = std::pow(10.0, -dBFalloffRange / 20.0); // 10^(-60/20) = 0.001
            
            // Apply frequency-dependent scaling
            float frequencyFactor = CalculateFrequencyFactor(frequency);
            double adjustedFalloffSamples = falloffSamples / frequencyFactor; // Faster = fewer samples
            
            mBaseFalloffMultiplierPerSample = std::pow(linearFalloffRatio, 1.0 / adjustedFalloffSamples);
            
            Reset();
        }

        void Reset() {
            mClipHoldCounter = 0;
            mPeakHoldCounter = 0;
            mClipIndicator = 0;
            mCurrentPeak = 0;
        }

        void ProcessSample(double s) {
            double rectifiedSample = fabs(s);

            // Clip detection
            if (rectifiedSample >= 1) {
                mClipIndicator = true;
                mClipHoldCounter = mClipHoldSamples;
            }
            else if (mClipHoldCounter > 0) {
                --mClipHoldCounter;
            }
            else {
                mClipIndicator = false;
            }

            // Peak detection and hold
            if (rectifiedSample >= mCurrentPeak) {
                mCurrentPeak = rectifiedSample;
                mPeakHoldCounter = mPeakHoldSamples;
            }
            else if (mPeakHoldCounter > 0) {
                --mPeakHoldCounter;
            }
            else if (mCurrentPeak > 0) {
                // Frequency-dependent exponential falloff
                mCurrentPeak *= mBaseFalloffMultiplierPerSample;
                
                if (mCurrentPeak < 1e-10) mCurrentPeak = 0;
            }
        }

    private:
        // Professional frequency-dependent falloff model
        static float CalculateFrequencyFactor(float frequency) {
            // Clamp frequency to reasonable range
            frequency = std::max(20.0f, std::min(20000.0f, frequency));
            
            if (frequency < kLowFreqThreshold) {
                // Low frequencies: Slow falloff (bass persistence)
                // Linear interpolation from 20Hz to 200Hz
                float t = (frequency - 20.0f) / (kLowFreqThreshold - 20.0f);
                return kLowFreqFactor * (1.0f - t) + kMidFreqFactor * t;
            }
            else if (frequency < kHighFreqThreshold) {
                // Mid frequencies: Baseline falloff 
                // Linear interpolation from 200Hz to 2000Hz
                float t = (frequency - kLowFreqThreshold) / (kHighFreqThreshold - kLowFreqThreshold);
                return kMidFreqFactor * (1.0f - t) + kMidFreqFactor * t; // Flat in mid range
            }
            else {
                // High frequencies: Fast falloff (transient detail)
                // Exponential curve from 2000Hz to 20000Hz for realistic high-freq behavior
                float t = (frequency - kHighFreqThreshold) / (20000.0f - kHighFreqThreshold);
                float exponentialT = 1.0f - std::exp(-3.0f * t); // Smooth exponential rise
                return kMidFreqFactor * (1.0f - exponentialT) + kHighFreqFactor * exponentialT;
            }
        }
    };
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT


} // namespace WaveSabreCore
