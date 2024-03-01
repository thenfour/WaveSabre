#include <WaveSabreCore/BiquadFilter.h>
#include <WaveSabreCore/Helpers.h>
#include "WaveSabreCore/Maj7Basic.hpp"

#include <math.h>

// this is an impl of the RBJ cookbook filters. (search term "Cookbook formulae for audio EQ biquad filter coefficients")

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

		float& c1 = this->normCoeffs[3];
		float& c2 = this->normCoeffs[4];
		float& c3 = this->normCoeffs[5];
		float& c4 = this->normCoeffs[1];
		float& c5 = this->normCoeffs[2];

		float output =
			c1 * input // c1 = b0/a0
			+ c2 * lastInput // c2 = b1/a0
			+ c3 * lastLastInput // c3 = b2 / a0;
			- c4 * lastOutput // c4 = a1 / a0
			- c5 * lastLastOutput; // c5 = a2 / a0

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
		const float A = Helpers::Exp10F(gain / 40.0f);

		// these ranges can cause instability and are effectively out of usable range
		// NB: frequency params typically go down to like 31.25hz or so. catch that to give the user the option of bypassing.
		this->thru = (freq > 21000) || (freq < 32);

		const float w0 = M7::math::gPITimes2 * freq * Helpers::CurrentSampleRateRecipF;
		const float alpha = M7::math::sin(w0) / (q * 2);
		float cosw0 = M7::math::cos(w0);

		float& a0 = this->coeffs[0];
		float& a1 = this->coeffs[1];
		float& a2 = this->coeffs[2];
		float& b0 = this->coeffs[3];
		float& b1 = this->coeffs[4];
		float& b2 = this->coeffs[5];

		a1 = cosw0 * -2;
		a0 = 1 + alpha;
		a2 = 1 - alpha;
		switch (type)
		{
		default:
		// you may be tempted to try and unify these the same way as lowshelf / highshelf. but you don't gain anything (measured.)
		case BiquadFilterType::Lowpass:
			b1 = 1.0f - cosw0;
			b0 = b2 = b1 * 0.5f;
			break;
		case BiquadFilterType::Highpass:
			b1 = -(1.0f + cosw0);
			b0 = b2 = b1 * -0.5f;
			break;
		case BiquadFilterType::Peak:
		{
			a0 = 1.0f + (alpha / A);
			a2 = 1.0f - (alpha / A);
			b0 = 1.0f + alpha * A;
			b1 = -2.0f * cosw0;
			b2 = 1.0f - alpha * A;
			break;
		}
		case BiquadFilterType::LowShelf:
		case BiquadFilterType::HighShelf:
		{
			float sqrtAtimesAlphaTimes2 = M7::math::sqrt(A) * 2 * alpha;
			float Ap1 = A + 1;
			float Am1 = A - 1;
			float two = 2;
			if (type == BiquadFilterType::HighShelf) {
				two = -2;
				cosw0 = -cosw0;
			}
			float Am1CosW0 = Am1 * cosw0;

			b0 = A * (Ap1 - Am1CosW0 + sqrtAtimesAlphaTimes2);
			b2 = A * (Ap1 - Am1CosW0 - sqrtAtimesAlphaTimes2);
			a0 = Ap1 + Am1CosW0 + sqrtAtimesAlphaTimes2;
			a2 = Ap1 + Am1CosW0 - sqrtAtimesAlphaTimes2;
			b1 = two * A * (Am1 - Ap1 * cosw0);
			a1 = -two * (Am1 + Ap1 * cosw0);

			break;
		}

		}

		for (int i = 1; i < 6; ++i) {
			normCoeffs[i] = this->coeffs[i] / a0;
		}
	}
}
