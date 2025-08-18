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

    // this is not so different from the other peak detector but i don't want to try and combine them together.
    struct AttenuationPeakDetector
    {
        int mClipHoldSamples;
        int mPeakHoldSamples;
        double mPeakFalloffPerSample;

        // state
        int mClipHoldCounter = 0;
        int mPeakHoldCounter = 0;

        // running values
        bool mClipIndicator = 0;
        double mCurrentPeak = 1; // the current peak rectified value, accounting for hold & falloff

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
            mCurrentPeak = 1;
        }

        void ProcessSample(double s) {
            double rectifiedSample = fabs(s); // Assuming 's' is the input sample

            // Clip detection
            if (rectifiedSample < 1) {
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
            if (rectifiedSample <= mCurrentPeak) {
                mCurrentPeak = rectifiedSample; // new peak; reset hold & falloff
                mPeakHoldCounter = mPeakHoldSamples;
            }
            else if (mPeakHoldCounter > 0) {
                --mPeakHoldCounter; // holding phase
            }
            else if (mCurrentPeak > 0) {
                mCurrentPeak += mPeakFalloffPerSample; // falloff phase
                if (mCurrentPeak < 0) mCurrentPeak = 0; // Ensure mCurrentPeak doesn't go below 0
            }

        }
    };


    struct AttenuationAnalysisStream : IAnalysisStream {
        AttenuationPeakDetector mPeakDetector;
        AttenuationPeakDetector mPeakHoldDetector;

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
