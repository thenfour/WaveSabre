
#include <cstdint>
#include <WaveSabreCore/Cathedral.h>
#include <WaveSabreCore/Helpers.h>

namespace WaveSabreCore
{
	Cathedral::Cathedral()
		: Device((int)ParamIndices::NumParams, mParamCache, gParamDefaults),
		mParams{mParamCache, 0}
	{
		static constexpr int16_t CombTuning[] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
		static constexpr int16_t AllPassTuning[] = { 556, 441, 341, 225 };
		static constexpr int16_t stereoSpread = 23;

		//roomSize = 0.5f;
		//damp = 0.0f;
		//width = 1.0f;
		//freeze = false;
		//lowCutFreq = 20.0f;
		//highCutFreq = 20000.0f - 20.0f;
		//preDelayMS = 0.0f;

		//for (int i = 0; i < 2; i++)
		//{
		//	lowCutFilter[i].SetType(StateVariableFilterType::Highpass);
		//	highCutFilter[i].SetType(StateVariableFilterType::Lowpass);
		//}

		for (int i = 0; i < numCombs; i++)
		{
			combLeft[i].SetBufferSize(CombTuning[i]);
			combRight[i].SetBufferSize(CombTuning[i] + stereoSpread);
		}

		for (int i = 0; i < numAllPasses; i++)
		{
			allPassLeft[i].SetBufferSize(AllPassTuning[i]);
			allPassRight[i].SetBufferSize(AllPassTuning[i] + stereoSpread);

			// NOTE: this is never set again, so effectively it means a fixed feedback amount of 0.5.
			allPassLeft[i].SetFeedback(/*roomSize*/0.5f);
			allPassRight[i].SetFeedback(/*roomSize*/0.5f);
		}

		LoadDefaults();
	}

	void Cathedral::Run(double songPosition, float **inputs, float **outputs, int numSamples)
	{
		//for (int i = 0; i < 2; i++)
		//{
		//	lowCutFilter[i].SetFreq(lowCutFreq);
		//	highCutFilter[i].SetFreq(highCutFreq);
		//}

		preDelayBuffer.SetLength(preDelayMS);

		float dryMul = mParams.GetLinearVolume(ParamIndices::DryOut, M7::gVolumeCfg12db);
		float wetMul = mParams.GetLinearVolume(ParamIndices::WetOut, M7::gVolumeCfg12db);

		static constexpr float SVQ = 1;
		//float SVQ = mParams.GetScaledRealValue(ParamIndices::X, 0.1, 3, 0);

		for (int s = 0; s < numSamples; s++)
		{
			float leftInput = inputs[0][s];
			float rightInput = inputs[1][s];
			float input = (leftInput + rightInput) * gain;

			if (preDelayMS > 0)
			{
				preDelayBuffer.WriteSample(input);
				input = preDelayBuffer.ReadSample();
			}

			float outL = 0;
			float outR = 0;

			// Accumulate comb filters in parallel
			for (int i = 0; i < numCombs; i++)
			{
				outL += combLeft[i].Process(input);
				outR += combRight[i].Process(input);
			}
			
			// Feed through allpasses in series
			for (int i = 0; i < numAllPasses; i++)
			{
				outL = allPassLeft[i].Process(outL);
				outR = allPassRight[i].Process(outR);
			}

			//outL = lowCutFilter[0].Next(highCutFilter[0].Next(outL));
			//outR = lowCutFilter[1].Next(highCutFilter[1].Next(outR));

			outL = lowCutFilter[0].SVFhigh(highCutFilter[0].SVFlow(outL, highCutFreq, SVQ), lowCutFreq, SVQ);
			outR = lowCutFilter[1].SVFhigh(highCutFilter[1].SVFlow(outR, highCutFreq, SVQ), lowCutFreq, SVQ);

			outL = outL*wet1 + outR*wet2;
			outR = outR*wet1 + outL*wet2;

			//outputs[0][s] = M7::math::lerp(leftInput, outL, dryWet);// leftInput* (1.0f - dryWet) + outL * dryWet;
			//outputs[1][s] = M7::math::lerp(rightInput, outR, dryWet); //rightInput * (1.0f - dryWet) + outR * dryWet;
			outputs[0][s] = leftInput* dryMul + outL * wetMul;
			outputs[1][s] = rightInput * dryMul + outR * wetMul;
		}
	}

	void Cathedral::OnParamsChanged()
	{
		freeze = mParams.GetBoolValue(ParamIndices::Freeze);
		roomSize = mParams.Get01Value(ParamIndices::RoomSize);
		preDelayMS = mParams.Get01Value(ParamIndices::PreDelay) * 500.0f;
		damp = mParams.Get01Value(ParamIndices::Damp);
		width = mParams.Get01Value(ParamIndices::Width);
		lowCutFreq = mParams.GetFrequency(ParamIndices::LowCutFreq, M7::gFilterFreqConfig);
		highCutFreq = mParams.GetFrequency(ParamIndices::HighCutFreq, M7::gFilterFreqConfig);

		wet1 = (width / 2 + 0.5f);
		wet2 = ((1 - width) / 2);

		if (freeze)
		{
			roomSize1 = 1;
			damp1 = 0;
			gain = 0.0f;
		}
		else
		{
			roomSize1 = roomSize;
			damp1 = damp;
			gain = 0.015f;
		}

		//// this does not exist in the original. it feels like it should belong but causes a lot of ringing and ugliness.
		//for (int i = 0; i < numAllPasses; i++)
		//{
		//	allPassLeft[i].SetFeedback(roomSize);
		//	allPassRight[i].SetFeedback(roomSize);
		//}

		for (int i = 0; i < numCombs; i++)
		{
			combLeft[i].SetFeedback(roomSize1);
			combRight[i].SetFeedback(roomSize1);
			combLeft[i].SetDamp(damp1);
			combRight[i].SetDamp(damp1);
		}
	}

}
