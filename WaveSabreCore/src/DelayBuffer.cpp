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
			// seems that this gets called on a separate thread than audio processing; make it naively thread-safe.
			auto oldBuf = buffer;
			auto newBuffer = new float[newLength];
			memset(newBuffer, 0, sizeof(float) * newLength);
			currentPosition = 0;
			length = newLength;
			buffer = newBuffer;
			delete[] oldBuf;
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