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
		// NB: frequency params typically go down to like 31.25hz or so. catch that to give the user the option of bypassing.
		this->thru = (freq > 21000) || (freq < 32);

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

/*

LPF:        H(s) = 1 / (s^2 + s/Q + 1)

			b0 =  (1 - cos(w0))/2
			b1 =   1 - cos(w0)
			b2 =  (1 - cos(w0))/2
			a0 =   1 + alpha
			a1 =  -2*cos(w0)
			a2 =   1 - alpha


*/


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
			break;
		}

		case BiquadFilterType::LowShelf:
		{
			float A = Helpers::Exp10F(gain / 40.0f);
			float sqrtAtimesAlphaTimes2 = M7::math::sqrt(A) * 2 * alpha;
			float Ap1 = A + 1;
			float Am1 = A - 1;
			float Am1CosW0 = Am1 * cosw0;
			b0 = A * (Ap1 - Am1CosW0 + sqrtAtimesAlphaTimes2);
			b2 = A * (Ap1 - Am1CosW0 - sqrtAtimesAlphaTimes2);
			a0 = Ap1 + Am1CosW0 + sqrtAtimesAlphaTimes2;
			a2 = Ap1 + Am1CosW0 - sqrtAtimesAlphaTimes2;
			b1 = 2 * A * (Am1 - Ap1 * cosw0);
			a1 = -2 * (Am1 + Ap1 * cosw0);
				
				/*
		lowShelf: H(s) = A * (s ^ 2 + (sqrt(A) / Q) * s + A) / (A * s ^ 2 + (sqrt(A) / Q) * s + 1)

			b0 = A * ((A + 1) - (A - 1) * cos(w0) + 2 * sqrt(A) * alpha)
			b1 = 2 * A * ((A - 1) - (A + 1) * cos(w0))
			b2 = A * ((A + 1) - (A - 1) * cos(w0) - 2 * sqrt(A) * alpha)
			a0 = (A + 1) + (A - 1) * cos(w0) + 2 * sqrt(A) * alpha
			a1 = -2 * ((A - 1) + (A + 1) * cos(w0))
			a2 = (A + 1) + (A - 1) * cos(w0) - 2 * sqrt(A) * alpha
			*/

			break;
		}
		case BiquadFilterType::HighShelf:
		{
			float A = Helpers::Exp10F(gain / 40.0f);
			float sqrtAtimesAlphaTimes2 = M7::math::sqrt(A) * 2 * alpha;
			float Ap1 = A + 1;
			float Am1 = A - 1;
			float Am1CosW0 = Am1 * cosw0;

			b0 = A * (Ap1 + Am1CosW0 + sqrtAtimesAlphaTimes2);
			b2 = A * (Ap1 + Am1CosW0 - sqrtAtimesAlphaTimes2);
			a0 = Ap1 - Am1CosW0 + sqrtAtimesAlphaTimes2;
			a2 = Ap1 - Am1CosW0 - sqrtAtimesAlphaTimes2;
			b1 = -2 * A * (Am1 + Ap1 * cosw0);
			a1 = 2 * (Am1 - Ap1 * cosw0);
			/*
				highShelf: H(s) = A * (A * s ^ 2 + (sqrt(A) / Q) * s + 1) / (s ^ 2 + (sqrt(A) / Q) * s + A)

					b0 = A * ((A + 1) + (A - 1) * cos(w0) + 2 * sqrt(A) * alpha)
					b1 = -2 * A * ((A - 1) + (A + 1) * cos(w0))
					b2 = A * ((A + 1) + (A - 1) * cos(w0) - 2 * sqrt(A) * alpha)
					a0 = (A + 1) - (A - 1) * cos(w0) + 2 * sqrt(A) * alpha
					a1 = 2 * ((A - 1) - (A + 1) * cos(w0))
					a2 = (A + 1) - (A - 1) * cos(w0) - 2 * sqrt(A) * alpha
					*/
			break;
		}


		/*

BPF:        H(s) = s / (s^2 + s/Q + 1)  (constant skirt gain, peak gain = Q)

			b0 =   sin(w0)/2  =   Q*alpha
			b1 =   0
			b2 =  -sin(w0)/2  =  -Q*alpha
			a0 =   1 + alpha
			a1 =  -2*cos(w0)
			a2 =   1 - alpha


BPF:        H(s) = (s/Q) / (s^2 + s/Q + 1)      (constant 0 dB peak gain)

			b0 =   alpha
			b1 =   0
			b2 =  -alpha
			a0 =   1 + alpha
			a1 =  -2*cos(w0)
			a2 =   1 - alpha



notch:      H(s) = (s^2 + 1) / (s^2 + s/Q + 1)

			b0 =   1
			b1 =  -2*cos(w0)
			b2 =   1
			a0 =   1 + alpha
			a1 =  -2*cos(w0)
			a2 =   1 - alpha



APF:        H(s) = (s^2 - s/Q + 1) / (s^2 + s/Q + 1)

			b0 =   1 - alpha
			b1 =  -2*cos(w0)
			b2 =   1 + alpha
			a0 =   1 + alpha
			a1 =  -2*cos(w0)
			a2 =   1 - alpha



lowShelf: H(s) = A * (s^2 + (sqrt(A)/Q)*s + A)/(A*s^2 + (sqrt(A)/Q)*s + 1)

			b0 =    A*( (A+1) - (A-1)*cos(w0) + 2*sqrt(A)*alpha )
			b1 =  2*A*( (A-1) - (A+1)*cos(w0)                   )
			b2 =    A*( (A+1) - (A-1)*cos(w0) - 2*sqrt(A)*alpha )
			a0 =        (A+1) + (A-1)*cos(w0) + 2*sqrt(A)*alpha
			a1 =   -2*( (A-1) + (A+1)*cos(w0)                   )
			a2 =        (A+1) + (A-1)*cos(w0) - 2*sqrt(A)*alpha



highShelf: H(s) = A * (A*s^2 + (sqrt(A)/Q)*s + 1)/(s^2 + (sqrt(A)/Q)*s + A)

			b0 =    A*( (A+1) + (A-1)*cos(w0) + 2*sqrt(A)*alpha )
			b1 = -2*A*( (A-1) + (A+1)*cos(w0)                   )
			b2 =    A*( (A+1) + (A-1)*cos(w0) - 2*sqrt(A)*alpha )
			a0 =        (A+1) - (A-1)*cos(w0) + 2*sqrt(A)*alpha
			a1 =    2*( (A-1) - (A+1)*cos(w0)                   )
			a2 =        (A+1) - (A-1)*cos(w0) - 2*sqrt(A)*alpha


*/
		}

		this->ma0 = a0;
		c1 = b0 / a0;
		c2 = b1 / a0;
		c3 = b2 / a0;
		c4 = a1 / a0;
		c5 = a2 / a0;
	}
}
