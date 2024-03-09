#include <WaveSabreCore/DelayBuffer.h>
#include <WaveSabreCore/Helpers.h>

namespace WaveSabreCore
{
	DelayBuffer::~DelayBuffer()
	{
		delete[] buffer;
	}

	void DelayBuffer::SetLength(float lengthMs)
	{
		int newLength = (int)(lengthMs * (Helpers::CurrentSampleRateF / 1000.0f));
		if (newLength < 1) newLength = 1;
		if (newLength != length || !buffer)
		{
			auto newBuffer = new float[newLength];
			memset(newBuffer, 0, sizeof(float) * newLength);
			//for (int i = 0; i < newLength; i++) newBuffer[i] = 0.0f;
			currentPosition = 0;
			delete[] buffer;
			buffer = newBuffer;
			length = newLength;
		}
	}

	void DelayBuffer::WriteSample(float sample)
	{
		buffer[currentPosition] = sample;
		currentPosition = (currentPosition + 1) % length;
	}

	float DelayBuffer::ReadSample() const
	{
		return buffer[currentPosition];
	}
}