#ifndef __WAVESABRECORE_CATHEDRAL_H__
#define __WAVESABRECORE_CATHEDRAL_H__

#include "Device.h"
#include "DelayBuffer.h"
#include "Comb.h"
#include "AllPass.h"
#include "Maj7Filter.hpp"

namespace WaveSabreCore
{
	// interesting to compare the 2 SVF.
	//enum class StateVariableFilterType
	//{
	//	Lowpass,
	//	Highpass,
	//	Bandpass,
	//	Notch,
	//};

	//struct StateVariableFilter
	//{
	//	bool recalculate;

	//	StateVariableFilterType type;
	//	float freq;
	//	float q;

	//	float lastInput;
	//	float low, band;

	//	float f;

	//	StateVariableFilter()
	//	{
	//		recalculate = true;

	//		type = StateVariableFilterType::Lowpass;
	//		freq = 20.0f;
	//		q = 1.0f;

	//		lastInput = 0.0f;
	//		low = band = 0.0f;
	//	}

	//	float Next(float input)
	//	{
	//		if (recalculate)
	//		{
	//			//f = (float)(1.5f * Helpers::FastSin(3.141592 * (double)freq / 2.0 / Helpers::CurrentSampleRate));
	//			f = (float)(1.5f * M7::math::sin(M7::math::gPI * (double)freq / 2.0 / Helpers::CurrentSampleRate));

	//			recalculate = false;
	//		}

	//		float ret = (run((lastInput + input) / 2.0f) + run(input)) / 2.0f;
	//		lastInput = input;
	//		return ret;
	//	}

	//	void SetType(StateVariableFilterType type)
	//	{
	//		if (type == this->type)
	//			return;

	//		this->type = type;
	//		recalculate = true;
	//	}

	//	void SetFreq(float freq)
	//	{
	//		if (freq == this->freq)
	//			return;

	//		this->freq = freq;
	//		recalculate = true;
	//	}

	//	void SetQ(float q)
	//	{
	//		if (q == this->q)
	//			return;

	//		this->q = q;
	//		recalculate = true;
	//	}

	//	float run(float input)
	//	{
	//		low = low + f * band;
	//		float high = q * (input - band) - low;
	//		band = band + f * high;
	//		switch (type)
	//		{
	//		case StateVariableFilterType::Lowpass:
	//		default:
	//			return low;

	//		case StateVariableFilterType::Highpass: return high;
	//		case StateVariableFilterType::Bandpass: return band;
	//		case StateVariableFilterType::Notch: return low + high;
	//		}
	//	}


	//};







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
			DryOut,
			WetOut,
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
	{"DryOut"},\
	{"WetOut"},\
	{"PreDly"},\
}

		float mParamCache[(int)ParamIndices::NumParams];
		M7::ParamAccessor mParams;

		static_assert((int)Cathedral::ParamIndices::NumParams == 9, "param count probably changed and this needs to be regenerated.");
		static constexpr int16_t gParamDefaults[9] = {
		  0, // Freeze = 0
		  24576, // RoomSize = 0.75
		  0, // Damp = 0
		  29491, // Width = 0.89999997615814208984
		  5498, // LCF = 0.16780719161033630371
		  32767, // HCF = 1
		  16422, // DryOut = 0.50118720531463623047
		  11626, // WetOut = 0.35481339693069458008
		  819, // PreDly = 0.025000000372529029846
		};

		Cathedral();

		virtual void Run(double songPosition, float **inputs, float **outputs, int numSamples);

		virtual void SetParam(int index, float value) override
		{
			mParamCache[index] = value;
			UpdateParams();
		}

	private:
		static constexpr int numCombs = 8;
		static constexpr int numAllPasses = 4;

		float gain;
		float roomSize, roomSize1;
		float damp, damp1;
		float width;
		float lowCutFreq, highCutFreq;
		float wet1, wet2;
		bool freeze;
		float preDelayMS;

		void UpdateParams();

		//StateVariableFilter lowCutFilter[2], highCutFilter[2];
		M7::SVFilter lowCutFilter[2], highCutFilter[2];

		Comb combLeft[numCombs];
		Comb combRight[numCombs];

		AllPass	allPassLeft[numAllPasses];
		AllPass	allPassRight[numAllPasses];

		DelayBuffer preDelayBuffer;
	};
}

#endif
