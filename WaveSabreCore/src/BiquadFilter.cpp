#include <WaveSabreCore/BiquadFilter.h>
#include <WaveSabreCore/Helpers.h>
#include "WaveSabreCore/Maj7Basic.hpp"

#include <math.h>

// this is an impl of the RBJ cookbook filters. (search term "Cookbook formulae for audio EQ biquad filter coefficients")
// variants exist for notch, shelves as well.

namespace WaveSabreCore
{
	BiquadFilter::BiquadFilter()
	{
		lastInput = lastLastInput = 0.0f;
		lastOutput = lastLastOutput = 0.0f;
	}

	float BiquadFilter::Next(float input)
	{
		if (this->thru)
			return input;

		float output =
			c1 * input
			+ c2 * lastInput
			+ c3 * lastLastInput
			- c4 * lastOutput
			- c5 * lastLastOutput;

		lastLastInput = lastInput;
		lastInput = input;
		lastLastOutput = lastOutput;
		lastOutput = output;

		return output;
	}

	void BiquadFilter::SetParams(BiquadFilterType type, float freq, float q, float gain)
	{
		if (type == this->type && freq == this->freq && this->q == q && this->gain == gain)
			return;
		this->type = type;
		this->freq = freq;
		this->q = q;
		this->gain = gain;

		// these ranges can cause instability and are effectively out of usable range
		this->thru = false;
		if (type == BiquadFilterType::Lowpass && freq > 21000) {
			this->thru = true;
		}
		if (type == BiquadFilterType::Highpass && freq < 30) {
			this->thru = true;
		}
		float w0 = M7::math::gPITimes2 * freq * Helpers::CurrentSampleRateRecipF;
		float cosw0 = M7::math::cos(w0);
		float alpha = M7::math::sin(w0) / (q * 2);

		float a0 = 1;
		float a2 = 1;
		float a1 = cosw0 * -2;
		float b0, b1, b2;
		switch (type)
		{
		default:
		case BiquadFilterType::Lowpass:
			a0 += alpha;
			a2 -= alpha;
			b1 = 1.0f - cosw0;
			b0 = b2 = b1 * 0.5f;
			break;

		case BiquadFilterType::Highpass:
			a0 += alpha;
			a2 -= alpha;
			b1 = -(1.0f + cosw0);
			b0 = b2 = b1 * -0.5f;
			break;
		case BiquadFilterType::Peak:
		{
			float A = Helpers::Exp10F(gain / 40.0f);
			a0 += alpha / A;
			a2 -= alpha / A;
			b0 = 1.0f + alpha * A;
			b1 = -2.0f * cosw0;
			b2 = 1.0f - alpha * A;
		}
		break;
		}

		c1 = b0 / a0;
		c2 = b1 / a0;
		c3 = b2 / a0;
		c4 = a1 / a0;
		c5 = a2 / a0;
	}
}
