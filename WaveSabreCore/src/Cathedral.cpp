
#include <cstdint>
#include <WaveSabreCore/Cathedral.h>
#include <WaveSabreCore/Helpers.h>

namespace WaveSabreCore
{
	Cathedral::Cathedral()
		: Device((int)ParamIndices::NumParams, mParamCache, gParamDefaults),
		mParams{mParamCache, 0}
	{
		static constexpr int16_t CombTuning[numCombs] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
		static constexpr int16_t AllPassTuning[numAllPasses] = { 556, 441, 341, 225 };
		static constexpr int16_t stereoSpread = 23;

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
		preDelayBuffer.SetLength(preDelayMS);

		float dryMul = mParams.GetLinearVolume(ParamIndices::DryOut, M7::gVolumeCfg12db);
		float wetMul = mParams.GetLinearVolume(ParamIndices::WetOut, M7::gVolumeCfg12db);

		static constexpr float SVQ = 1;

		for (int s = 0; s < numSamples; s++)
		{
			float leftInput = inputs[0][s];
			float rightInput = inputs[1][s];
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
			mInputAnalysis[0].WriteSample(leftInput);
			mInputAnalysis[1].WriteSample(rightInput);
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

			float input = (leftInput + rightInput) * 0.015f;

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

			outL = highCutFilter[0].SVFlow(outL, highCutFreq, SVQ);
			outL = lowCutFilter[0].SVFhigh(outL, lowCutFreq, SVQ);

			outR = highCutFilter[1].SVFlow(outR, highCutFreq, SVQ);
			outR = lowCutFilter[1].SVFhigh(outR, lowCutFreq, SVQ);

			//outL = outL*wet1 + outR*wet2;
			//outR = outR*wet1 + outL*wet2;

			outputs[0][s] = leftInput* dryMul + outL * wetMul;
			outputs[1][s] = rightInput * dryMul + outR * wetMul;
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
			mOutputAnalysis[0].WriteSample(outputs[0][s]);
			mOutputAnalysis[1].WriteSample(outputs[1][s]);
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT
		}
	}

	void Cathedral::OnParamsChanged()
	{
		//freeze = mParams.GetBoolValue(ParamIndices::Freeze);
		roomSize = mParams.Get01Value(ParamIndices::RoomSize);
		preDelayMS = mParams.Get01Value(ParamIndices::PreDelay) * 500.0f;
		damp = mParams.Get01Value(ParamIndices::Damp);
		//width = mParams.Get01Value(ParamIndices::Width);
		lowCutFreq = mParams.GetFrequency(ParamIndices::LowCutFreq, M7::gFilterFreqConfig);
		highCutFreq = mParams.GetFrequency(ParamIndices::HighCutFreq, M7::gFilterFreqConfig);

		// roomsize is not a linear param.
		roomSize = 1.0f - roomSize;
		M7::ParamAccessor pa{&roomSize, 0};
		float t = pa.GetDivCurvedValue(0, { 0.0f, 1.0f, 1.140f }, 0);
		roomSize = 1.0f - t;
		roomSize = M7::math::clamp01(roomSize);

		//wet1 = (width / 2 + 0.5f);
		//wet2 = ((1 - width) / 2);

		//if (freeze)
		//{
		//	roomSize1 = 1;
		//	damp1 = 0;
		//	gain = 0.0f;
		//}
		//else
		//{
		//	roomSize1 = roomSize;
		//	damp1 = damp;
		//	gain = 0.015f;
		//}

		//// this does not exist in the original. it feels like it should belong but causes a lot of ringing and ugliness.
		//for (int i = 0; i < numAllPasses; i++)
		//{
		//	allPassLeft[i].SetFeedback(roomSize);
		//	allPassRight[i].SetFeedback(roomSize);
		//}

		for (int i = 0; i < numCombs; i++)
		{
			combLeft[i].SetParams(damp, roomSize);
			combRight[i].SetParams(damp, roomSize);
		}
	}

}
