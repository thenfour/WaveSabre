
#pragma once

#include "Maj7Basic.hpp"

namespace WaveSabreCore
{
	struct RMSDetector
	{
		float mContinuousValue = 0;
		float mWindowMS = 0;
		float mAlpha = 0;

		// because this is not a perfect curve, the "milliseconds" is a misnomer, it's a sort of approximation.
		void SetWindowSize(float ms) {
			if (ms == mWindowMS) return;
			mWindowMS = ms;

			float windowSizeSamples = (mWindowMS / 1000.0f) * Helpers::CurrentSampleRateF;
			mAlpha = 1.0f - std::exp(-1.0f / windowSizeSamples);
		}

		// returns the current "RMS"
		float ProcessSample(float s) {
			// a sort of one-pole LP filter to smooth continuously, without requiring a memory buffer
			mContinuousValue = M7::math::lerp(mContinuousValue, s * s, mAlpha);
			return M7::math::sqrt(mContinuousValue);
		}

	}; // struct RMSDetector
} // namespace WaveSabreCore
