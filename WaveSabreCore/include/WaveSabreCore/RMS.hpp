
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
} // namespace WaveSabreCore
