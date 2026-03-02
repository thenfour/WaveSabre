
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
}
