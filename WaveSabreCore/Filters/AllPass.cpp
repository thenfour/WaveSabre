
#include "AllPass.h"

namespace WaveSabreCore
{
	void AllPass::SetBufferSize(int size)
	{
		mBuffer.SetLengthSamples(size);
	}

	void AllPass::SetFeedback(float value)
	{
		feedback = value;
	}

	float AllPass::Process(float input)
	{
		float output;
		float bufferOut;

		bufferOut = mBuffer.PeekAtCursor();

		output = -input + bufferOut;
		mBuffer.WriteAndAdvance(input + (bufferOut * feedback));

		return output;
	}
/*
for reference comb filter:,

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


delay buffer code:

void DelayBuffer::SetLength(float lengthMs)
{
  int newLength = (int)M7::math::MillisecondsToSamples(lengthMs);
  mBuffer.SetLengthSamples(newLength);
}

void DelayBuffer::WriteSample(float sample)
{
  mBuffer.WriteAndAdvance(sample);
}

float DelayBuffer::ReadSample() const
{
  return mBuffer.PeekAtCursor();
}

*/

}
