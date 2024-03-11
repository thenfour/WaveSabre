#include <WaveSabreCore/Comb.h>
#include <WaveSabreCore/Helpers.h>

#include <math.h>

namespace WaveSabreCore
{
//#ifdef MIN_SIZE_REL
//#pragma message("Maj7::~Maj7() Leaking memory to save bits.")
//#else
//	Comb::~Comb()
//	{
//		if (buffer) delete[] buffer;
//	}
//#endif

	void Comb::SetBufferSize(int size)
	{
		mBuffer.SetLengthSamples(size);
		//if (size < 1) size = 1;
		//bufferSize = size;
		//auto newBuffer = new float[size];
		//for (int i = 0; i < size; i++) newBuffer[i] = 0.0f;
		//bufferIndex = 0;
		//auto oldBuffer = buffer;
		//buffer = newBuffer;
		//if (oldBuffer) delete[] oldBuffer;
	}

	void Comb::SetParams(float damp, float feedback)
	{
		damp1 = damp;
		this->feedback = feedback;
	}

	//void Comb::SetFeedback(float val)
	//{
	//	feedback = val;
	//}

	float Comb::Process(float input)
	{
		float output = mBuffer.PeekAtCursor();//  mBuffer[bufferIndex];
		filterStore = (output * (1 - damp1)) + (filterStore * damp1);
		float x = input + (filterStore * feedback);
		mBuffer.WriteAndAdvance(x);
		//mBuffer[bufferIndex] = ;
		//bufferIndex = (bufferIndex + 1) % mBuffer.mLength;
		return output;
	}
}
