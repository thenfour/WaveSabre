#ifndef __WAVESABRECORE_TWISTER_H__
#define __WAVESABRECORE_TWISTER_H__

#include "Device.h"
#include "ResampleBuffer.h"
#include "AllPassDelay.h"
//#include "StateVariableFilter.h"
#include "Maj7Filter.hpp"

namespace WaveSabreCore
{
	enum class Spread
	{
		Mono,
		FullInvert,
		ModInvert
	};

	class Twister : public Device
	{
	public:
		enum class ParamIndices
		{
			Type,

			Amount,
			Feedback,

			Spread,

			VibratoFreq,
			VibratoAmount,

			LowCutFreq,
			HighCutFreq,

			DryWet,

			NumParams,
		};

		// NB: max 8 chars per string.
#define TWISTER_PARAM_VST_NAMES(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::Twister::ParamIndices::NumParams]{ \
	{"Type"},\
	{"Amt"},\
	{"Feedback"},\
	{"Spread"},\
	{"VibFreq"},\
	{"VibAmt"},\
	{"LCF"},\
	{"HCF"},\
	{"DryWet"},\
}

		float mParamCache[(int)ParamIndices::NumParams];
		M7::ParamAccessor mParams;

		static_assert((int)ParamIndices::NumParams == 9, "param count probably changed and this needs to be regenerated.");
		static constexpr int16_t gParamDefaults[9] = {
			0,
			0,
			0,
			0,
			0,
			0,
			0,
			0,
		};

		enum class Type : uint8_t {
			Type0,
			Type1,
			Type2,
			Type3,
			Count__,
		};


		Twister();
		virtual ~Twister();

		virtual void Run(double songPosition, float **inputs, float **outputs, int numSamples);

		virtual void LoadDefaults() override {
			M7::ImportDefaultsArray(std::size(gParamDefaults), gParamDefaults, mParamCache);
			SetParam(0, mParamCache[0]); // force recalcing
		}

		virtual void SetParam(int index, float value) override
		{
			mParamCache[index] = value;

			type = mParams.GetEnumValue<Type>(ParamIndices::Type);
			amount = mParams.Get01Value(ParamIndices::Amount, 0);
			feedback = mParams.Get01Value(ParamIndices::Feedback, 0);
			spread = mParams.GetEnumValue<Spread>(ParamIndices::Spread);

			vibratoAmount = mParams.Get01Value(ParamIndices::VibratoAmount, 0);

			//case ParamIndices::VibratoFreq: vibratoFreq = Helpers::ParamToVibratoFreq(value); break;
			// original vibrato freq range is ~0.7hz to ~85hz.
			vibratoFreq = mParams.GetFrequency(ParamIndices::VibratoFreq, -1, M7::gLFOFreqConfig, 0, 0);// Helpers::ParamToFrequency(value); break;

			lowCutFreq = mParams.GetFrequency(ParamIndices::LowCutFreq, -1, M7::gFilterFreqConfig, 0, 0);// Helpers::ParamToFrequency(value); break;
			highCutFreq = mParams.GetFrequency(ParamIndices::HighCutFreq, -1, M7::gFilterFreqConfig, 0, 0);// Helpers::ParamToFrequency(value); break;
			dryWet = mParams.Get01Value(ParamIndices::DryWet, 0);
		}

		virtual float GetParam(int index) const override
		{
			return mParamCache[index];
		}

	private:
		Type type;
		float amount, feedback;
		Spread spread;
		double vibratoFreq;
		float vibratoAmount;
		double vibratoPhase;
		float lowCutFreq, highCutFreq;
		float dryWet;
		float lastLeft, lastRight;

		AllPassDelay allPassLeft[6];
		AllPassDelay allPassRight[6];
		float AllPassUpdateLeft(float input);
		float AllPassUpdateRight(float input);

		ResampleBuffer leftBuffer;
		ResampleBuffer rightBuffer;
		
		M7::SVFilter lowCutFilter[2], highCutFilter[2];
	};
}

#endif
