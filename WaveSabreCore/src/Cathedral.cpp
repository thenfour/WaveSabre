
#include <cstdint>
#include <WaveSabreCore/Cathedral.h>
#include <WaveSabreCore/Helpers.h>

namespace WaveSabreCore
{
	Cathedral::Cathedral()
		: Device((int)ParamIndices::NumParams),
		mParams{mParamCache, 0}
	{
		static constexpr int16_t CombTuning[] = { 1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617 };
		static constexpr int16_t AllPassTuning[] = { 556, 441, 341, 225 };
		static constexpr int16_t stereoSpread = 23;

		roomSize = 0.5f;
		damp = 0.0f;
		width = 1.0f;
		freeze = false;
		dryWet = 0.25f;
		lowCutFreq = 20.0f;
		highCutFreq = 20000.0f - 20.0f;
		preDelay = 0.0f;

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
			allPassLeft[i].SetFeedback(roomSize);
			allPassRight[i].SetFeedback(roomSize);
		}

		LoadDefaults();
	}

	Cathedral::~Cathedral()
	{
	}

	void Cathedral::Run(double songPosition, float **inputs, float **outputs, int numSamples)
	{
		//for (int i = 0; i < 2; i++)
		//{
		//	lowCutFilter[i].SetFreq(lowCutFreq);
		//	highCutFilter[i].SetFreq(highCutFreq);
		//}

		preDelayBuffer.SetLength(preDelay * 500.0f);

		for (int s = 0; s < numSamples; s++)
		{
			float leftInput = inputs[0][s];
			float rightInput = inputs[1][s];
			float input = (leftInput + rightInput) * gain;

			if (preDelay > 0)
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

			outL = lowCutFilter[0].SVFhigh(highCutFilter[0].SVFlow(outL, highCutFreq, 1), lowCutFreq, 1);
			outR = lowCutFilter[1].SVFhigh(highCutFilter[1].SVFlow(outR, highCutFreq, 1), lowCutFreq, 1);

			outL = outL*wet1 + outR*wet2;
			outR = outR*wet1 + outL*wet2;

			outputs[0][s] = M7::math::lerp(leftInput, outL, dryWet);// leftInput* (1.0f - dryWet) + outL * dryWet;
			outputs[1][s] = M7::math::lerp(rightInput, outR, dryWet); //rightInput * (1.0f - dryWet) + outR * dryWet;
		}
	}

	void Cathedral::UpdateParams()
	{
		freeze = mParams.GetBoolValue(ParamIndices::Freeze);
		roomSize = mParams.Get01Value(ParamIndices::RoomSize, 0);
		damp = mParams.GetBoolValue(ParamIndices::Damp);
		width = mParams.GetBoolValue(ParamIndices::Width);
		lowCutFreq = mParams.GetFrequency(ParamIndices::LowCutFreq, -1, M7::gFilterFreqConfig, 0, 0);
		highCutFreq = mParams.GetFrequency(ParamIndices::HighCutFreq, -1, M7::gFilterFreqConfig, 0, 0);
		dryWet = mParams.Get01Value(ParamIndices::DryWet, 0);
		preDelay = mParams.Get01Value(ParamIndices::PreDelay, 0);

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

		for (int i = 0; i < numCombs; i++)
		{
			combLeft[i].SetFeedback(roomSize1);
			combRight[i].SetFeedback(roomSize1);
			combLeft[i].SetDamp(damp1);
			combRight[i].SetDamp(damp1);
		}
	}

	//void Cathedral::SetParam(int index, float value)
	//{
	//	switch ((ParamIndices)index)
	//	{
	//	case ParamIndices::Freeze: freeze = Helpers::ParamToBoolean(value); UpdateParams(); break;
	//	case ParamIndices::RoomSize: roomSize = value; UpdateParams();  break;
	//	case ParamIndices::Damp: damp = value; UpdateParams(); break;
	//	case ParamIndices::Width: width = value; UpdateParams(); break;
	//	case ParamIndices::LowCutFreq: lowCutFreq = Helpers::ParamToFrequency(value); break;
	//	case ParamIndices::HighCutFreq: highCutFreq = Helpers::ParamToFrequency(value); break;
	//	case ParamIndices::DryWet: dryWet = value; break;
	//	case ParamIndices::PreDelay: preDelay = value; break;
	//	}
	//}

	//float Cathedral::GetParam(int index) const
	//{
	//	switch ((ParamIndices)index)
	//	{
	//	case ParamIndices::Freeze:
	//	default:
	//		return Helpers::BooleanToParam(freeze);

	//	case ParamIndices::RoomSize: return roomSize;
	//	case ParamIndices::Damp: return damp;
	//	case ParamIndices::Width: return width;
	//	case ParamIndices::LowCutFreq: return Helpers::FrequencyToParam(lowCutFreq);
	//	case ParamIndices::HighCutFreq: return Helpers::FrequencyToParam(highCutFreq);
	//	case ParamIndices::DryWet: return dryWet;
	//	case ParamIndices::PreDelay: return preDelay;
	//	}
	//}
}
