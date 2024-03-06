#ifndef __WAVESABRECORE_CRUSHER_H__
#define __WAVESABRECORE_CRUSHER_H__

#include "Device.h"
#include "WaveSabreCore/Maj7Basic.hpp"
#include "Maj7Filter.hpp"

namespace WaveSabreCore
{
	class Crusher : public Device
	{
	public:
		enum class ParamIndices
		{
			Vertical,
			Horizontal,
			DryWet,
			NumParams,
		};

		// NB: max 8 chars per string.
#define CRUSHER_PARAM_VST_NAMES(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::Crusher::ParamIndices::NumParams]{ \
	{"Vert"},\
	{"Horiz"},\
	{"DryWet"},\
}

		float mParamCache[(int)ParamIndices::NumParams];
		M7::ParamAccessor mParams;

		static_assert((int)ParamIndices::NumParams == 3, "param count probably changed and this needs to be regenerated.");
		static constexpr int16_t gParamDefaults[3] = {
			0,
			0,
			0,
		};

		Crusher();

		virtual void Run(double songPosition, float **inputs, float **outputs, int numSamples);

		virtual void LoadDefaults() override {
			M7::ImportDefaultsArray(std::size(gParamDefaults), gParamDefaults, mParamCache);
			SetParam(0, mParamCache[0]); // force recalcing
		}

		virtual void SetParam(int index, float value) override
		{
			mParamCache[index] = value;

			vertical = mParams.Get01Value(ParamIndices::Vertical, 0);
			horizontal = mParams.Get01Value(ParamIndices::Horizontal, 0);
			dryWet = mParams.Get01Value(ParamIndices::DryWet, 0);

		}

		virtual float GetParam(int index) const override
		{
			return mParamCache[index];
		}
	private:
		float vertical, horizontal;
		float dryWet;

		float phase[2];
		float hold[2];
	};
}

#endif
