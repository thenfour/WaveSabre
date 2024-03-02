
#pragma once

#include "Device.h"
#include "Maj7Basic.hpp"

namespace WaveSabreCore
{
	struct Maj7Comp: public Device
	{
		enum class ParamIndices
		{
			OutputGain,
			NumParams,
		};

		// NB: max 8 chars per string.
#define MAJ7COMP_PARAM_VST_NAMES(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::Maj7Comp::ParamIndices::NumParams]{ \
	{"OutGain"},\
}

		static_assert((int)ParamIndices::NumParams == 1, "param count probably changed and this needs to be regenerated.");
		static constexpr int16_t gParamDefaults[1] = {
		  0, // HSEn = 0
		};

		float mParamCache[(int)ParamIndices::NumParams];

		M7::ParamAccessor mParams;

		Maj7Comp() :
			Device((int)ParamIndices::NumParams),
			mParams(mParamCache, 0)
		{
			LoadDefaults();
		}

		virtual void LoadDefaults() override
		{
			M7::ImportDefaultsArray(std::size(gParamDefaults), gParamDefaults, mParamCache);
			SetParam(0, mParamCache[0]); // force recalcing some things
		}

		virtual void SetParam(int index, float value) override
		{
			mParamCache[index] = value;
		}

		virtual float GetParam(int index) const override
		{
			return mParamCache[index];
		}

		virtual void Run(double songPosition, float** inputs, float** outputs, int numSamples) override
		{
			for (size_t i = 0; i < (size_t)numSamples; ++i)
			{
				outputs[0][i] = inputs[0][i];
				outputs[1][i] = inputs[1][i];
			}
		}
	};
}




