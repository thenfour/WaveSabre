#ifndef __WAVESABRECORE_STATEVARIABLEFILTER_H__
#define __WAVESABRECORE_STATEVARIABLEFILTER_H__

namespace WaveSabreCore
{
	enum class StateVariableFilterType
	{
		Lowpass,
		Highpass,
		Bandpass,
		Notch,
	};

	class StateVariableFilter
	{
	public:
		StateVariableFilter();

		float Next(float input);

		void SetType(StateVariableFilterType type);
		void SetFreq(float freq);
		void SetQ(float q);

		//float SVFlow(float v0, float cutoff, float Q) {
		//	SetType(StateVariableFilterType::Lowpass);
		//	SetQ(Q);
		//	SetFreq(cutoff);
		//	return Next(v0);
		//}
		//float SVFhigh(float v0, float cutoff, float Q) {
		//	SetType(StateVariableFilterType::Highpass);
		//	SetQ(Q);
		//	SetFreq(cutoff);
		//	return Next(v0);
		//}

	private:
		float run(float input);

		bool recalculate;

		StateVariableFilterType type;
		float freq;
		float q;

		float lastInput;
		float low, band;

		float f;
	};
}

#endif
