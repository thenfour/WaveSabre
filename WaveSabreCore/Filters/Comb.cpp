#include "Comb.h"

namespace WaveSabreCore
{
	void Comb::SetBufferSize(int size)
	{
		mBuffer.SetLengthSamples(size);
	}

	void Comb::SetParams(float damp, float feedback)
	{
		damp1 = damp;
		this->feedback = feedback;
	}

	float Comb::Process(float input)
	{
		float output = mBuffer.PeekAtCursor();
		filterStore = M7::math::lerp(output, filterStore, damp1);
		float x = input + (filterStore * feedback);
		mBuffer.WriteAndAdvance(x);
		return output;
	}
}
