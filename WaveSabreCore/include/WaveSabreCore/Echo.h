#ifndef __WAVESABRECORE_ECHO_H__
#define __WAVESABRECORE_ECHO_H__

#include "Device.h"
#include "DelayBuffer.h"
#include "Maj7Basic.hpp"
#include "Maj7Filter.hpp"

namespace WaveSabreCore
{
	struct Echo : public Device
	{
		static constexpr M7::IntParamConfig gDelayCoarseCfg{ 0, 16 };
		static constexpr M7::IntParamConfig gDelayFineCfg{ 0, 200 };

		enum class ParamIndices
		{
			LeftDelayCoarse, // 0-1 => 0-16 inclusive integral
			LeftDelayFine, // 0-1 => 0-200 inclusive integral

			RightDelayCoarse,// 0-1 => 0-16 inclusive integral
			RightDelayFine,// 0-1 => 0-200 inclusive integral

			LowCutFreq, // Helpers::ParamToFrequency(value)
			HighCutFreq, // Helpers::ParamToFrequency(value)

			Feedback, // 0-1
			Cross, // 0-1

			DryWet, // 0-1

			NumParams,
		};

#define ECHO_PARAM_VST_NAMES(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::Echo::ParamIndices::NumParams]{ \
	{"LdlyC"},\
	{"LdlyF"},\
	{"RdlyC"},\
	{"RdlyF"},\
	{"LCF"},\
	{"HCF"},\
	{"Fb"},\
	{"Cross"},\
	{"DryWet"},\
}
		static constexpr char gJSONTagName[] = "WSEcho";

		static_assert((int)Echo::ParamIndices::NumParams == 9, "param count probably changed and this needs to be regenerated.");
		static constexpr int16_t gDefaults16[9] = {
		  6746, // LdlyC = 0.20588235557079315186
		  81, // LdlyF = 0.0024875621311366558075
		  8673, // RdlyC = 0.26470589637756347656
		  81, // RdlyF = 0.0024875621311366558075
		  -2109, // LCF = -0.064385592937469482422
		  30541, // HCF = 0.93204843997955322266
		  16384, // Fb = 0.5
		  0, // Cross = 0
		  16384, // DryWet = 0.5
		};


		float mParamCache[(int)ParamIndices::NumParams];
		M7::ParamAccessor mParams { mParamCache, 0 };

		DelayBuffer leftBuffer;
		DelayBuffer rightBuffer;

		M7::OnePoleFilter mLowCutFilter[2];
		M7::OnePoleFilter mHighCutFilter[2];

		Echo()
			: Device((int)ParamIndices::NumParams)
		{
			LoadDefaults();
		}

		virtual void Run(double songPosition, float** inputs, float** outputs, int numSamples) override
		{
			float delayScalar = 120.0f / Helpers::CurrentTempo / 8.0f * 1000.0f;
			float leftBufferLengthMs = mParams.GetIntValue(ParamIndices::LeftDelayCoarse, gDelayCoarseCfg) * delayScalar + mParams.GetIntValue(ParamIndices::LeftDelayFine, gDelayFineCfg);
			float rightBufferLengthMs = mParams.GetIntValue(ParamIndices::RightDelayCoarse, gDelayCoarseCfg) * delayScalar + mParams.GetIntValue(ParamIndices::RightDelayFine, gDelayFineCfg);
			leftBuffer.SetLength(leftBufferLengthMs);
			rightBuffer.SetLength(rightBufferLengthMs);

			for (int i = 0; i < 2; i++)
			{
				mLowCutFilter[i].SetParams(M7::FilterType::HP, mParams.GetFrequency(ParamIndices::LowCutFreq, -1, M7::gFilterFreqConfig, 0, 0), 0);
				mHighCutFilter[i].SetParams(M7::FilterType::LP, mParams.GetFrequency(ParamIndices::HighCutFreq, -1, M7::gFilterFreqConfig, 0, 0), 0);
			}

			for (int i = 0; i < numSamples; i++)
			{
				float leftInput = inputs[0][i];
				float rightInput = inputs[1][i];

				float leftDelay = mLowCutFilter[0].ProcessSample(mHighCutFilter[0].ProcessSample(leftBuffer.ReadSample()));
				float rightDelay = mLowCutFilter[1].ProcessSample(mHighCutFilter[1].ProcessSample(rightBuffer.ReadSample()));

				float feedback = mParams.Get01Value(ParamIndices::Feedback, 0);
				float cross = mParams.Get01Value(ParamIndices::Cross, 0);
				float dryWet = mParams.Get01Value(ParamIndices::DryWet, 0);
				leftBuffer.WriteSample(leftInput + (leftDelay * (1.0f - cross) + rightDelay * cross) * feedback);
				rightBuffer.WriteSample(rightInput + (rightDelay * (1.0f - cross) + leftDelay * cross) * feedback);

				outputs[0][i] = leftInput * (1.0f - dryWet) + leftDelay * dryWet;
				outputs[1][i] = rightInput * (1.0f - dryWet) + rightDelay * dryWet;
			}
		}


		virtual void LoadDefaults() override
		{
			M7::ImportDefaultsArray(std::size(gDefaults16), gDefaults16, mParamCache);
		}

		virtual void SetParam(int index, float value) override
		{
			mParamCache[index] = value;
		}

		virtual float GetParam(int index) const override
		{
			return mParamCache[index];
		}

		// minified
		virtual void SetChunk(void* data, int size) override
		{
			M7::Deserializer ds{ (const uint8_t*)data };
			SetMaj7StyleChunk(ds);
		}

	};
}

#endif
