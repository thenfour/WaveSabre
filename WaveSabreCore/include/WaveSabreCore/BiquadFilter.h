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

		// https://forum.juce.com/t/draw-frequency-response-of-filter-from-transfer-function/20669
		// https://www.musicdsp.org/en/latest/Analysis/186-frequency-response-from-biquad-coefficients.html

		// ehhhhh, i didn't try much, and it failed. another time.

		float calcMagnitude(float freq) {
			//auto w = 2 * M7::math::gPI * filter.freq() / 44100.f;
			//auto alpha = sin(w) / (2 * filter.res());

			//auto b0 = (1 - cos(w)) / 2.f;
			//auto b1 = 1 - cos(w);
			//auto b2 = (1 - cos(w)) / 2.f;
			//auto a0 = 1 + alpha;
			//auto a1 = -2 * cos(w);
			//auto a2 = 1 - alpha;

			//auto const piw0 = freq * M7::math::gPI;
			//auto const cosw = std::cos(piw0);
			//auto const sinw = std::sin(piw0);

			//auto square = [](double z) { return z * z; };

			//auto const numerator = M7::math::sqrt(square(a0 * square(cosw) - a0 * square(sinw) + a1 * cosw + a2) + square(2 * a0 * cosw * sinw + a1 * (sinw)));
			//auto const denominator = M7::math::sqrt(square(square(cosw) - square(sinw) + b1 * cosw + b2) + square(2 * cosw * sinw + b1 * (sinw)));

			//return numerator / denominator;

			using M7::math::gPI;

			float w0 = M7::math::gPITimes2 * freq * Helpers::CurrentSampleRateRecipF;

			auto const piw0 = w0 * gPI;
			auto const cosw = std::cos(piw0);
			auto const sinw = std::sin(piw0);

			auto square = [](auto z) { return z * z; };

			auto a1 = c4 * ma0;
			auto a2 = c5 * ma0;
			auto b1 = c2 * ma0;
			auto b2 = c3 * ma0;

			auto const numerator = M7::math::sqrt(square(ma0 * square(cosw) - ma0 * square(sinw) + a1 * cosw + a2) + square(2 * ma0 * cosw * sinw + a1 * (sinw)));
			auto const denominator = M7::math::sqrt(square(square(cosw) - square(sinw) + b1 * cosw + b2) + square(2 * cosw * sinw + b1 * (sinw)));

			/*auto const w = std::sin(cpl::simd::consts<T>::pi * w / 2);
			auto const phi = w * w;
			auto const phi2 = phi * phi;

			// black magic. supposedly gives better precision than direct evaluation of H(z)
			auto const numerator = 10 * std::log10((b0 + b1 + b2) * (b0 + b1 + b2) - 4 * (b0 * b1 + 4 * b0 * b2 + b1 * b2) * phi + 16 * b0 * b2 * phi2);
			auto const denominator = -10 * std::log10((a0 + a1 + a2) * (a0 + a1 + a2) - 4 * (a0 * a1 + 4 * a0 * a2 + a1 * a2) * phi + 16 * a0 * a2 * phi2);
			return numerator + denominator; */

			return (numerator / denominator); // linear sample value, which could be converted to dB.

			//w = frequency (0 < w < PI)
//square(x) = x*x

			//y = 20 * log((sqrt(square(a0 * square(cos(w)) - a0 * square(sin(w)) + a1 * cos(w) + a2) + square(2 * a0 * cos(w) * sin(w) + a1 * (sin(w)))) /
			//	sqrt(square(square(cos(w)) - square(sin(w)) + b1 * cos(w) + b2) + square(2 * cos(w) * sin(w) + b1 * (sin(w))))));
			//from musicdsp :
		}

	private:
		//bool recalculate;
		bool thru;

		BiquadFilterType type;
		float freq;
		float q;
		float gain;

		float ma0; // store a0 so we can calculate the coeffs from c12345
		float c1, c2, c3, c4, c5;

		float lastInput, lastLastInput;
		float lastOutput, lastLastOutput;
	};
}

#endif
