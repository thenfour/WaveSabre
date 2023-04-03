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

		static constexpr int numBuffers = 8;
		static constexpr int numBuffersDiv2 = numBuffers / 2;

		int mode = 1;
		float lowCutFreq = 200;
		float highCutFreq = 8000;
		float feedback = 0.88f;
		float dryWet = 0.27f;
		float preDelay = 0;

		DelayBuffer delayBuffers[numBuffers];

		DelayBuffer preDelayBuffers[2];

		M7::OnePoleFilter mLowCutFilter[2];
		M7::OnePoleFilter mHighCutFilter[2];

		Chamber()
			: Device((int)ParamIndices::NumParams)
		{
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
		static constexpr float multipliers[] =
		{
			1, 5, 10
		};

		virtual void Run(double songPosition, float** inputs, float** outputs, int numSamples) override
		{
			for (int i = 0; i < numBuffers; i++)
			{
				delayBuffers[i].SetLength(delayLengths[i] * multipliers[mode]);
			}

			for (int i = 0; i < 2; i++)
			{
				preDelayBuffers[i].SetLength(preDelay * 500.0f);
				mLowCutFilter[i].SetParams(M7::FilterType::HP, lowCutFreq, 0, 0);//; .SetFreq(lowCutFreq);
				mHighCutFilter[i].SetParams(M7::FilterType::LP, highCutFreq, 0, 0);// .SetFreq(highCutFreq);
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

		virtual void SetParam(int index, float value);
		virtual float GetParam(int index) const;
	};
}

#endif
