#ifndef __WAVESABRECORE_BIQUADFILTER_H__
#define __WAVESABRECORE_BIQUADFILTER_H__

#include "WaveSabreCore/Maj7Basic.hpp"

namespace WaveSabreCore
{
	enum class BiquadFilterType
	{
		Lowpass,
		Highpass,
		Peak,
		HighShelf,
		LowShelf,
	};

	class BiquadFilter
	{
	public:
		BiquadFilter();

		float Next(float input);

		void SetParams(BiquadFilterType type, float freq, float q, float gain);

		bool thru;

		BiquadFilterType type;
		float freq;
		float q;
		float gain;

		//float ma0, ma1, ma2, mb0, mb1, mb2; // store a0 so we can calculate the coeffs from c12345
		//float c1, c2, c3, c4, c5;
		float coeffs[6]; // a0, a1, a2, b0, b1, b2

		// map to normalized against a0:
		// a0, a1, a2, b0, b1, b2 but all divided by a0. so normCoeffs[0] is just 1.
		// 0   1   2   3   4   5
		float normCoeffs[6];

		float lastInput, lastLastInput;
		float lastOutput, lastLastOutput;
	};
}

#endif
