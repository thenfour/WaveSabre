#ifndef __WAVESABRECORE_CATHEDRAL_H__
#define __WAVESABRECORE_CATHEDRAL_H__

#include "Device.h"
#include "DelayBuffer.h"
#include "Comb.h"
#include "AllPass.h"
#include "Maj7Filter.hpp"
#include "RMS.hpp"

namespace WaveSabreCore
{
	class Cathedral : public Device
	{
	public:
		enum class ParamIndices
		{
			//Freeze,
			RoomSize,
			Damp,
			Width,
			LowCutFreq,
			HighCutFreq,
			DryOut,
			WetOut,
			PreDelay,
			NumParams,
		};

		// NB: max 8 chars per string.
#define CATHEDRAL_PARAM_VST_NAMES(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::Cathedral::ParamIndices::NumParams]{ \
	{"RoomSize"},\
	{"Damp"},\
	{"Width"},\
	{"LCF"},\
	{"HCF"},\
	{"DryOut"},\
	{"WetOut"},\
	{"PreDly"},\
}

		float mParamCache[(int)ParamIndices::NumParams];
		M7::ParamAccessor mParams;

		static_assert((int)Cathedral::ParamIndices::NumParams == 8, "param count probably changed and this needs to be regenerated.");
		static constexpr int16_t gParamDefaults[(int)Cathedral::ParamIndices::NumParams] = {
		  16384, // RoomSize = 0.5
		  4915, // Damp = 0.15000000596046447754
		  29491, // Width = 0.89999401569366455078
		  7255, // LCF = 0.22141247987747192383
		  24443, // HCF = 0.74594312906265258789
		  16422, // DryOut = 0.50118720531463623047
		  11626, // WetOut = 0.35481339693069458008
		  0, // PreDly = 0
		};

		Cathedral();

		virtual void Run(double songPosition, float **inputs, float **outputs, int numSamples);

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
		AnalysisStream mInputAnalysis[2];
		AnalysisStream mOutputAnalysis[2];
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

		virtual void OnParamsChanged() override;

	private:
		static constexpr int numCombs = 8;
		static constexpr int numAllPasses = 4;

		float gain;
		float roomSize;// , roomSize1;
		float damp;// , damp1;
		float width;
		float lowCutFreq, highCutFreq;
		//float wet1, wet2;
		//bool freeze;
		float preDelayMS;

		M7::SVFilter lowCutFilter[2], highCutFilter[2];

		Comb combLeft[numCombs];
		Comb combRight[numCombs];

		AllPass	allPassLeft[numAllPasses];
		AllPass	allPassRight[numAllPasses];

		DelayBuffer preDelayBuffer;
	};
}

#endif
