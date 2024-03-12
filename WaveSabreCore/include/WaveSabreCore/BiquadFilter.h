#ifndef __WAVESABRECORE_BIQUADFILTER_H__
#define __WAVESABRECORE_BIQUADFILTER_H__

#include "WaveSabreCore/Maj7Basic.hpp"
#include "WaveSabreCore/Maj7ParamAccessor.hpp"
#include "WaveSabreCore/Filters/FilterBase.hpp"

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

	class BiquadFilter : public M7::IFilter
	{
	public:
		BiquadFilter();

		//float Next(float input);

		// q is ~0 to ~12
		void SetParams(BiquadFilterType type, float freq, float q, float gain);

		//bool thru;

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

		// IFilter
		virtual void SetParams(M7::FilterType type, float cutoffHz, float reso01) override {
			BiquadFilterType bt;
			switch (type) {
			case M7::FilterType::HP2:
			case M7::FilterType::HP4:
				bt = BiquadFilterType::Highpass;
				break;
			default:
			case M7::FilterType::LP2:
			case M7::FilterType::LP4:
				bt = BiquadFilterType::Lowpass;
				break;
			}

			M7::ParamAccessor pa{ &reso01, 0 };
			float q = pa.GetDivCurvedValue(0, M7::gBiquadFilterQCfg, 0);

			SetParams(bt, cutoffHz, q,q);
		}
		// IFilter
		virtual float ProcessSample(float x) override;

		// IFilter
		virtual void Reset() override {
			lastInput = lastLastInput = 0.0f;
			lastOutput = lastLastOutput = 0.0f;
		}

	};
}

#endif
