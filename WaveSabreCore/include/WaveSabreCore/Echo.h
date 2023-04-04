#ifndef __WAVESABRECORE_ECHO_H__
#define __WAVESABRECORE_ECHO_H__

#include "Device.h"
#include "DelayBuffer.h"
//#include "StateVariableFilter.h"
#include "Maj7Filter.hpp"

namespace WaveSabreCore
{
	struct Echo : public Device
	{
		enum class ParamIndices
		{
			LeftDelayCoarse,
			LeftDelayFine,

			RightDelayCoarse,
			RightDelayFine,

			LowCutFreq,
			HighCutFreq,

			Feedback,
			Cross,

			DryWet,

			NumParams,
		};

		int leftDelayCoarse = 3;
		int leftDelayFine = 0;
		int rightDelayCoarse = 4;
		int rightDelayFine = 0;
		float lowCutFreq = 20;
		float highCutFreq = 20000 - 20;
		float feedback = 0.5f;
		float cross = 0;
		float dryWet = 0.5f;

		DelayBuffer leftBuffer;
		DelayBuffer rightBuffer;

		M7::OnePoleFilter mLowCutFilter[2];
		M7::OnePoleFilter mHighCutFilter[2];

		Echo()
			: Device((int)ParamIndices::NumParams)
		{
		}

		virtual void Run(double songPosition, float** inputs, float** outputs, int numSamples) override
		{
			float delayScalar = 120.0f / Helpers::CurrentTempo / 8.0f * 1000.0f;
			float leftBufferLengthMs = leftDelayCoarse * delayScalar + leftDelayFine;
			float rightBufferLengthMs = rightDelayCoarse * delayScalar + rightDelayFine;
			leftBuffer.SetLength(leftBufferLengthMs);
			rightBuffer.SetLength(rightBufferLengthMs);

			for (int i = 0; i < 2; i++)
			{
				mLowCutFilter[i].SetParams(M7::FilterType::HP, lowCutFreq, 0);
				mHighCutFilter[i].SetParams(M7::FilterType::LP, highCutFreq, 0);
			}

			for (int i = 0; i < numSamples; i++)
			{
				float leftInput = inputs[0][i];
				float rightInput = inputs[1][i];

				float leftDelay = mLowCutFilter[0].ProcessSample(mHighCutFilter[0].ProcessSample(leftBuffer.ReadSample()));
				float rightDelay = mLowCutFilter[1].ProcessSample(mHighCutFilter[1].ProcessSample(rightBuffer.ReadSample()));

				leftBuffer.WriteSample(leftInput + (leftDelay * (1.0f - cross) + rightDelay * cross) * feedback);
				rightBuffer.WriteSample(rightInput + (rightDelay * (1.0f - cross) + leftDelay * cross) * feedback);

				outputs[0][i] = leftInput * (1.0f - dryWet) + leftDelay * dryWet;
				outputs[1][i] = rightInput * (1.0f - dryWet) + rightDelay * dryWet;
			}
		}

		virtual void SetParam(int index, float value) override;
		virtual float GetParam(int index) const override;
	};
}

#endif
