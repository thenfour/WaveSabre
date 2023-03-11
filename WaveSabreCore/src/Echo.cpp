#include <WaveSabreCore/Echo.h>
#include <WaveSabreCore/Helpers.h>

#include <math.h>

namespace WaveSabreCore
{

	void Echo::SetParam(int index, float value)
	{
		switch ((ParamIndices)index)
		{
		case ParamIndices::LeftDelayCoarse: leftDelayCoarse = (int)(value * 16.0f); break;
		case ParamIndices::LeftDelayFine: leftDelayFine = (int)(value * 200.0f); break;
		case ParamIndices::RightDelayCoarse: rightDelayCoarse = (int)(value * 16.0f); break;
		case ParamIndices::RightDelayFine: rightDelayFine = (int)(value * 200.0f); break;
		case ParamIndices::LowCutFreq: lowCutFreq = Helpers::ParamToFrequency(value); break;
		case ParamIndices::HighCutFreq: highCutFreq = Helpers::ParamToFrequency(value); break;
		case ParamIndices::Feedback: feedback = value; break;
		case ParamIndices::Cross: cross = value; break;
		case ParamIndices::DryWet: dryWet = value; break;
		}
	}

	float Echo::GetParam(int index) const
	{
		switch ((ParamIndices)index)
		{
		case ParamIndices::LeftDelayCoarse:
		default:
			return (float)leftDelayCoarse / 16.0f;

		case ParamIndices::LeftDelayFine: return (float)leftDelayFine / 200.0f;
		case ParamIndices::RightDelayCoarse: return (float)rightDelayCoarse / 16.0f;
		case ParamIndices::RightDelayFine: return (float)rightDelayFine / 200.0f;
		case ParamIndices::LowCutFreq: return Helpers::FrequencyToParam(lowCutFreq);
		case ParamIndices::HighCutFreq: return Helpers::FrequencyToParam(highCutFreq);
		case ParamIndices::Feedback: return feedback;
		case ParamIndices::Cross: return cross;
		case ParamIndices::DryWet: return dryWet;
		}
	}
}
