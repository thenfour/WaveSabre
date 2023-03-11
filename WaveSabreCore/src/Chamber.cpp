#include <WaveSabreCore/Chamber.h>
#include <WaveSabreCore/Helpers.h>

namespace WaveSabreCore
{

	void Chamber::SetParam(int index, float value)
	{
		switch ((ParamIndices)index)
		{
		case ParamIndices::Mode: mode = (int)(value * 2.0f); break;
		case ParamIndices::Feedback: feedback = value * .5f + .5f; break;
		case ParamIndices::LowCutFreq: lowCutFreq = Helpers::ParamToFrequency(value); break;
		case ParamIndices::HighCutFreq: highCutFreq = Helpers::ParamToFrequency(value); break;
		case ParamIndices::DryWet: dryWet = value; break;
		case ParamIndices::PreDelay: preDelay = value; break;
		}
	}

	float Chamber::GetParam(int index) const
	{
		switch ((ParamIndices)index)
		{
		case ParamIndices::Mode:
		default:
			return (float)mode / 2.0f;

		case ParamIndices::Feedback: return (feedback - .5f) * 2.0f;
		case ParamIndices::LowCutFreq: return Helpers::FrequencyToParam(lowCutFreq);
		case ParamIndices::HighCutFreq: return Helpers::FrequencyToParam(highCutFreq);
		case ParamIndices::DryWet: return dryWet;
		case ParamIndices::PreDelay: return preDelay;
		}
	}
}
