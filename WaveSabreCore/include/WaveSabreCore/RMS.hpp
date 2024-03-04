
#pragma once

#include "Maj7Basic.hpp"

namespace WaveSabreCore
{
	static inline float CalcFollowerCoef(float ms) {
		return M7::math::expf(-1.0f / (Helpers::CurrentSampleRateF * ms / 1000.0f));
	}

	struct RMSDetector
	{
		float mContinuousValue = 0;

        float mWindowMS = 0;
		float mAlpha = 0;

        void Reset() {
            mContinuousValue = 0;
        }
		// because this is not a perfect curve, the "milliseconds" is a misnomer, it's a sort of approximation.
		void SetWindowSize(float ms) {
			if (ms == mWindowMS) return;
			mWindowMS = ms;
			mAlpha = 1.0f - CalcFollowerCoef(ms);
		}

		// returns the current "RMS"
		float ProcessSample(float s) {
			// a sort of one-pole LP filter to smooth continuously, without requiring a memory buffer
			mContinuousValue = M7::math::lerp(mContinuousValue, s * s, mAlpha);
			return M7::math::sqrt(mContinuousValue);
		}

	}; // struct RMSDetector

    struct PeakDetector
    {
        // configuration
        int mClipHoldSamples;
        int mPeakHoldSamples;
        float mPeakFalloffPerSample;

        // state
        int mClipHoldCounter = 0;
        int mPeakHoldCounter = 0;

        // running values
        bool mClipIndicator = 0;
        float mCurrentPeak = 0; // the current peak rectified value, accounting for hold & falloff

        void SetParams(float clipHoldMS, float peakHoldMS, float peakFalloffMaxMS) {
            mClipHoldSamples = int(clipHoldMS * Helpers::CurrentSampleRateF / 1000);
            mPeakHoldSamples = int(peakHoldMS * Helpers::CurrentSampleRateF / 1000);
            mPeakFalloffPerSample = std::max(0.000001f, 1.0f / (peakFalloffMaxMS * Helpers::CurrentSampleRateF / 1000));
        }

        void Reset() {
            mClipHoldCounter = 0;
            mPeakHoldCounter = 0;
            mClipIndicator = 0;
            mCurrentPeak = 0;
        }

        void ProcessSample(float s) {
            float rectifiedSample = fabs(s); // Assuming 's' is the input sample

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

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
    // for VST-only, things like VU meters and history graphs require outputting many additional channels of audio.
    // this class goes in the Device, feed it samples. the VST can then access them in a safe way.
    //struct OutputStream
    //{
    //    std::vector<float> mSamples;
    //    void WriteSample(float s) {
    //        mSamples.push_back(s);
    //    }
    //    //
    //};

    struct AnalysisStream {
        //std::vector<float> mSamples;
        RMSDetector mRMSDetector;
        PeakDetector mPeakDetector;
        PeakDetector mPeakHoldDetector;

        float mCurrentRMSValue = 0;
        bool mClipIndicator = 0;
        float mCurrentPeak = 0;
        float mCurrentHeldPeak = 0; 

        explicit AnalysisStream(float peakFalloffMS) {
            mRMSDetector.SetWindowSize(140);
            mPeakDetector.SetParams(0, 0, peakFalloffMS);
            mPeakHoldDetector.SetParams(1000, 1000, 600);
        }

        void WriteSample(float s) {
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
