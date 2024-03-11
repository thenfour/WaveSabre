
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
        // configuration
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
            if (rectifiedSample >= 1.0f) { // Assuming clipping occurs at 1.0f
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
            if (rectifiedSample > mCurrentPeak) {
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

    // for VST-only, things like VU meters and history graphs require outputting many additional channels of audio.
    // this class goes in the Device, feed it samples. the VST can then access them in a safe way.

    struct AnalysisStream {
        RMSDetector mRMSDetector;
        PeakDetector mPeakDetector;
        PeakDetector mPeakHoldDetector;

        double mCurrentRMSValue = 0;
        bool mClipIndicator = 0;
        double mCurrentPeak = 0;
        double mCurrentHeldPeak = 0;

        explicit AnalysisStream(double peakFalloffMS = 1200) {
            mRMSDetector.SetWindowSize(140);
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

        void Reset() {
            mRMSDetector.Reset();
            mPeakHoldDetector.Reset();
            mPeakDetector.Reset();
            mCurrentRMSValue = 0;
            mClipIndicator = 0;
            mCurrentPeak = 0;
            mCurrentHeldPeak = 0;
        }
    };

#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT


} // namespace WaveSabreCore
