#ifndef __WAVESABRECORE_CHAMBER_H__
#define __WAVESABRECORE_CHAMBER_H__

#include "Device.h"
#include "DelayBuffer.h"
//#include "StateVariableFilter.h"
#include "Maj7Filter.hpp"

namespace WaveSabreCore
{
	struct Chamber : public Device
	{
		enum class ParamIndices
		{
			Mode,
			Feedback,
			LowCutFreq,
			HighCutFreq,
			DryWet,
			PreDelay,
			NumParams,
		};

			// NB: max 8 chars per string.
#define CHAMBER_PARAM_VST_NAMES(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::Chamber::ParamIndices::NumParams]{ \
	{"Mode"},\
	{"Feedback"},\
	{"LCF"},\
	{"HCF"},\
	{"DryWet"},\
	{"PreDly"},\
}

		float mParamCache[(int)ParamIndices::NumParams];
		M7::ParamAccessor mParams;

		static_assert((int)ParamIndices::NumParams == 6, "param count probably changed and this needs to be regenerated.");
		static constexpr int16_t gParamDefaults[6] = {
			0,
			0,
			0,
			0,
			0,
		};

		static constexpr int numBuffers = 8;
		static constexpr int numBuffersDiv2 = numBuffers / 2;

		enum class Mode : uint8_t {
			Mode0,
			Mode1,
			Mode2,
			Count__,
		};

		static constexpr float multipliers[(int)Mode::Count__] =
		{
			1, 5, 10
		};

		Mode mode = Mode::Mode1; // default 1
		float lowCutFreq = 200; // 200
		float highCutFreq = 8000; // 8000
		float feedback = 0.88f; // 0.88
		float dryWet = 0.27f; // 0.27;
		float preDelay = 0;

		DelayBuffer delayBuffers[numBuffers];

		DelayBuffer preDelayBuffers[2];

		M7::OnePoleFilter mLowCutFilter[2];
		M7::OnePoleFilter mHighCutFilter[2];

		Chamber()
			: Device((int)ParamIndices::NumParams),
			mParams{mParamCache, 0}
		{
			LoadDefaults();
		}

		static constexpr float delayLengths[] =
		{
			7,
			21,
			17,
			13,
			3,
			11,
			23,
			31
		};

		virtual void Run(double songPosition, float** inputs, float** outputs, int numSamples) override
		{
			for (int i = 0; i < numBuffers; i++)
			{
				delayBuffers[i].SetLength(delayLengths[i] * multipliers[(int)mode]);
			}

			for (int i = 0; i < 2; i++)
			{
				preDelayBuffers[i].SetLength(preDelay * 500.0f);
				mLowCutFilter[i].SetParams(M7::FilterType::HP, lowCutFreq, 0);//; .SetFreq(lowCutFreq);
				mHighCutFilter[i].SetParams(M7::FilterType::LP, highCutFreq, 0);// .SetFreq(highCutFreq);
			}

			for (int i = 0; i < numSamples; i++)
			{
				float inputSamples[2], filteredInputSamples[2];
				for (int j = 0; j < 2; j++)
				{
					inputSamples[j] = inputs[j][i];
					if (preDelay > 0)
					{
						preDelayBuffers[j].WriteSample(inputSamples[j]);
						filteredInputSamples[j] = mLowCutFilter[j].ProcessSample(mHighCutFilter[j].ProcessSample(preDelayBuffers[j].ReadSample()));
					}
					else
					{
						filteredInputSamples[j] = mLowCutFilter[j].ProcessSample(mHighCutFilter[j].ProcessSample(inputSamples[j]));
					}
					outputs[j][i] = 0.0f;
				}
				for (int j = 0; j < numBuffers; j++)
				{
					int channelIndex = (int)(j < numBuffers / 2);
					float feedbackSample = delayBuffers[numBuffers - 1 - j].ReadSample();
					delayBuffers[j].WriteSample(filteredInputSamples[channelIndex] + feedbackSample * feedback);
					outputs[channelIndex][i] += inputSamples[channelIndex] * (1.0f - dryWet) + feedbackSample * dryWet;
				}
				outputs[0][i] /= numBuffersDiv2;
				outputs[1][i] /= numBuffersDiv2;
			}
		}

		virtual void LoadDefaults() override {
			M7::ImportDefaultsArray(std::size(gParamDefaults), gParamDefaults, mParamCache);
			SetParam(0, mParamCache[0]); // force recalcing
		}

		virtual void SetParam(int index, float value) override
		{
			mParamCache[index] = value;

			//	case ParamIndices::Mode: mode = (int)(value * 2.0f); break;
			//	case ParamIndices::Feedback: feedback = value * .5f + .5f; break;
			//	case ParamIndices::LowCutFreq: lowCutFreq = Helpers::ParamToFrequency(value); break;
			//	case ParamIndices::HighCutFreq: highCutFreq = Helpers::ParamToFrequency(value); break;
			//	case ParamIndices::DryWet: dryWet = value; break;
			//	case ParamIndices::PreDelay: preDelay = value; break;

			// 0, 1, 2
			mode = mParams.GetEnumValue<Mode>(ParamIndices::Mode);// (int)(value * 2.0f); break;
			feedback = mParams.GetScaledRealValue(ParamIndices::Feedback, 0.5f, 1.0f, 0);// value * .5f + .5f; break;
			lowCutFreq = mParams.GetFrequency(ParamIndices::LowCutFreq, -1, M7::gFilterFreqConfig, 0, 0);// Helpers::ParamToFrequency(value); break;
			highCutFreq = mParams.GetFrequency(ParamIndices::HighCutFreq, -1, M7::gFilterFreqConfig, 0, 0);// Helpers::ParamToFrequency(value); break;
			dryWet = mParams.Get01Value(ParamIndices::DryWet, 0);
			preDelay = mParams.Get01Value(ParamIndices::PreDelay, 0);

		}

		virtual float GetParam(int index) const override
		{
			return mParamCache[index];
		}
	};
}

#endif
