#include <WaveSabreCore/Twister.h>
#include <WaveSabreCore/Helpers.h>
#include "WaveSabreCore/Maj7Basic.hpp"

int const lookAhead = 4;

namespace WaveSabreCore
{
	Twister::Twister()
		: Device((int)ParamIndices::NumParams),
		mParams{mParamCache, 0}
	{
		//type = 0; // param
		//amount = 0; // param
		//feedback = 0.0f; // param
		//spread = Spread::Mono; // param
		//vibratoFreq = Helpers::ParamToVibratoFreq(0.0f); // param
		//vibratoAmount = 0.0f; // param
		
		vibratoPhase = 0.0;
		//
		//lowCutFreq = 20.0f; // param
		//highCutFreq = 20000.0f- 20.0f; // param

		//dryWet = .5f; // param

		leftBuffer.SetLength(1000);
		rightBuffer.SetLength(1000);

		lastLeft = 0.0f;
		lastRight = 0.0f;


		LoadDefaults();
	}

	Twister::~Twister()
	{
	}

	void Twister::Run(double songPosition, float **inputs, float **outputs, int numSamples)
	{
		double vibratoDelta = (vibratoFreq / Helpers::CurrentSampleRate) * 0.25f;
		float outputLeft = 0.0f;
		float outputRight = 0.0f;
		float positionLeft = 0.0f;
		float positionRight = 0.0f;

		for (int i = 0; i < numSamples; i++)
		{
			float leftInput = inputs[0][i];
			float rightInput = inputs[1][i];

			float freq = M7::math::sin((float)vibratoPhase) * vibratoAmount;

			positionLeft = M7::math::clamp01((amount + freq));
			switch (spread)
			{
			case Spread::Mono: 
			default:
				positionRight = positionLeft;
				break;
			case Spread::FullInvert:
				positionRight = (1.0f - M7::math::clamp01((amount + freq)));
				break;
			case Spread::ModInvert:
				positionRight = M7::math::clamp01((amount - freq));
				break;
			}

			// TODO: this is easily optimizable.
			switch (type)
			{
			default:
			case Type::Type0:
			case Type::Type1:
			{
				positionLeft *= 132.0f;
				positionRight *= 132.0f;

				float s0 = leftBuffer.ReadPosition(positionLeft + 2);
				s0 = lowCutFilter[0].SVFhigh(s0, lowCutFreq, 1);
				outputLeft = highCutFilter[0].SVFlow(s0, highCutFreq, 1);

				float s1 = rightBuffer.ReadPosition(positionRight + 2);
				s1 = lowCutFilter[1].SVFhigh(s1, lowCutFreq, 1);
				outputRight = highCutFilter[1].SVFlow(s1, highCutFreq, 1);

				float g = ((type == Type::Type0) ? 1.0f : -1.0f);

				leftBuffer.WriteSample(leftInput + g * (outputLeft * feedback));
				rightBuffer.WriteSample(rightInput + g * (outputRight * feedback));
				break;
			}
			case Type::Type2:
			case Type::Type3:
			{
				for (int i = 0; i < 6; i++) allPassLeft[i].Delay(positionLeft);
				for (int i = 0; i < 6; i++) allPassRight[i].Delay(positionRight);

				float g = ((type == Type::Type2) ? 1.0f : -1.0f);

				float s0 = leftInput + g * lastLeft * feedback;
				s0 = AllPassUpdateLeft(s0);
				s0 = lowCutFilter[0].SVFhigh(s0, lowCutFreq, 1);
				outputLeft = highCutFilter[0].SVFlow(s0, highCutFreq, 1);

				float s1 = rightInput + lastRight * feedback;
				s1 = AllPassUpdateLeft(s1);
				s1 = lowCutFilter[1].SVFhigh(s1, lowCutFreq, 1);
				outputRight = highCutFilter[1].SVFlow(s1, highCutFreq, 1);

				lastLeft = outputLeft;
				lastRight = outputRight;
				break;
			}
			}

			outputs[0][i] = M7::math::lerp(leftInput, outputLeft, dryWet);// (leftInput * (1.0f - dryWet)) + (outputLeft * dryWet);
			outputs[1][i] = M7::math::lerp(rightInput, outputRight, dryWet);// (rightInput * (1.0f - dryWet)) + (outputRight * dryWet);

			vibratoPhase += vibratoDelta;
		}
	}

	float Twister::AllPassUpdateLeft(float input)
	{
		for (int i = int(std::size(allPassLeft)) - 1; i >= 0; --i) {
			input = allPassLeft[i].Update(input);
		}
		//return(
		//	allPassLeft[0].Update(
		//	allPassLeft[1].Update(
		//	allPassLeft[2].Update(
		//	allPassLeft[3].Update(
		//	allPassLeft[4].Update(
		//	allPassLeft[5].Update(input)))))));
		return input;
	}

	float Twister::AllPassUpdateRight(float input)
	{
		for (int i = int(std::size(allPassLeft)) - 1; i >= 0; --i) {
			input = allPassRight[i].Update(input);
		}
		return input;
	}

	//void Twister::SetParam(int index, float value)
	//{
	//	switch ((ParamIndices)index)
	//	{
	//	case ParamIndices::Type: type = (int)(value * 3.0f); break;
	//	case ParamIndices::Amount: amount = value; break;
	//	case ParamIndices::Feedback: feedback = value; break;
	//	case ParamIndices::Spread: {
	//		M7::ParamAccessor p{&value, 0};
	//		spread = p.GetEnumValue<Spread>(0);
	//		//spread = Helpers::ParamToSpread(value);
	//		break;
	//	}
	//	case ParamIndices::VibratoFreq: vibratoFreq = Helpers::ParamToVibratoFreq(value); break;
	//	case ParamIndices::VibratoAmount: vibratoAmount = value; break;
	//	case ParamIndices::LowCutFreq: lowCutFreq = Helpers::ParamToFrequency(value); break;
	//	case ParamIndices::HighCutFreq: highCutFreq = Helpers::ParamToFrequency(value); break;
	//	case ParamIndices::DryWet: dryWet = value; break;
	//	}
	//}

	//float Twister::GetParam(int index) const
	//{
	//	switch ((ParamIndices)index)
	//	{
	//	case ParamIndices::Type: 
	//	default: 
	//		return type / 3.0f;

	//	case ParamIndices::Amount: return amount;
	//	case ParamIndices::Feedback: return feedback;
	//	case ParamIndices::Spread: {
	//		float ret;
	//		M7::ParamAccessor p{ &ret, 0 };
	//		p.SetEnumValue(0, this->spread);
	//		//return Helpers::SpreadToParam(spread);
	//		return ret;
	//	}
	//	case ParamIndices::VibratoFreq: return Helpers::VibratoFreqToParam(vibratoFreq);
	//	case ParamIndices::VibratoAmount: return vibratoAmount;
	//	case ParamIndices::LowCutFreq: return Helpers::FrequencyToParam(lowCutFreq);
	//	case ParamIndices::HighCutFreq: return Helpers::FrequencyToParam(highCutFreq);
	//	case ParamIndices::DryWet: return dryWet;
	//	}
	//}
}
