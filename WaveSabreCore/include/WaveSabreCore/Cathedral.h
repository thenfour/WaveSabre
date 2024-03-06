#ifndef __WAVESABRECORE_CATHEDRAL_H__
#define __WAVESABRECORE_CATHEDRAL_H__

#include "Device.h"
#include "DelayBuffer.h"
//#include "StateVariableFilter.h"
#include "Comb.h"
#include "AllPass.h"
#include "Maj7Filter.hpp"

namespace WaveSabreCore
{
	class Cathedral : public Device
	{
	public:
		enum class ParamIndices
		{
			Freeze,
			RoomSize,
			Damp,
			Width,
			LowCutFreq,
			HighCutFreq,
			DryWet,
			PreDelay,
			NumParams,
		};

		// NB: max 8 chars per string.
#define CATHEDRAL_PARAM_VST_NAMES(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::Cathedral::ParamIndices::NumParams]{ \
	{"Freeze"},\
	{"RoomSize"},\
	{"Damp"},\
	{"Width"},\
	{"LCF"},\
	{"HCF"},\
	{"DryWet"},\
	{"PreDly"},\
}

		float mParamCache[(int)ParamIndices::NumParams];
		M7::ParamAccessor mParams;

		static_assert((int)ParamIndices::NumParams == 8, "param count probably changed and this needs to be regenerated.");
		static constexpr int16_t gParamDefaults[8] = {
			0, // left source
			0, // right source
			0, // RotationAngle
			0, // Side HP
			0, // mid side balance
			0, // pan
			0, // output gain
		};

		Cathedral();
		virtual ~Cathedral();

		virtual void Run(double songPosition, float **inputs, float **outputs, int numSamples);

		virtual void SetParam(int index, float value) override
		{
			mParamCache[index] = value;
			UpdateParams();
		}

		virtual float GetParam(int index) const override
		{
			return mParamCache[index];
		}

		virtual void LoadDefaults() override {
			M7::ImportDefaultsArray(std::size(gParamDefaults), gParamDefaults, mParamCache);
			SetParam(0, mParamCache[0]); // force recalcing
		}

	private:
		static constexpr int numCombs = 8;
		static constexpr int numAllPasses = 4;

		float gain;
		float roomSize, roomSize1;
		float damp, damp1;
		float width;
		float lowCutFreq, highCutFreq;
		float dryWet;
		float wet1, wet2;
		bool freeze;
		float preDelayMS;

		void UpdateParams();

		M7::SVFilter lowCutFilter[2], highCutFilter[2];

		Comb combLeft[numCombs];
		Comb combRight[numCombs];

		AllPass	allPassLeft[numAllPasses];
		AllPass	allPassRight[numAllPasses];

		DelayBuffer preDelayBuffer;
	};
}

#endif
