#include <WaveSabreCore/DelayBuffer.h>
#include <WaveSabreCore/Helpers.h>

namespace WaveSabreCore
{

		void AudioBuffer::SetLengthSamples(size_t sampleCount) {
			if (sampleCount == mLength) return;
			if (sampleCount < 1) sampleCount = 1;
			auto oldBuf = mBuffer;
			auto newBuffer = new float[sampleCount];
			memset(newBuffer, 0, sizeof(float) * sampleCount);
			mCursor = 0;
			mLength = (int)sampleCount;
			mBuffer = newBuffer;
			delete[] oldBuf;
		}

		void AudioBuffer::WriteAndAdvance(float sample) {
			mBuffer[mCursor] = sample;
			mCursor = (mCursor + 1) % mLength;
		}



	//DelayBuffer::~DelayBuffer()
	//{
	//	delete[] buffer;
	//}

	void DelayBuffer::SetLength(float lengthMs)
	{
		int newLength = (int)(lengthMs * (Helpers::CurrentSampleRateF / 1000.0f));
		//if (newLength < 1) newLength = 1;
		//currentPosition = 0;
		mBuffer.SetLengthSamples(newLength);
		//if (newLength != length || !buffer)
		//{
		//	// seems that this gets called on a separate thread than audio processing; make it naively thread-safe.
		//	auto oldBuf = buffer;
		//	auto newBuffer = new float[newLength];
		//	memset(newBuffer, 0, sizeof(float) * newLength);
		//	currentPosition = 0;
		//	length = newLength;
		//	buffer = newBuffer;
		//	delete[] oldBuf;
		//}
	}

	void DelayBuffer::WriteSample(float sample)
	{
		mBuffer.WriteAndAdvance(sample);
		//mBuffer[currentPosition] = sample;
		//currentPosition = (currentPosition + 1) % mBuffer.mLength;
	}

	float DelayBuffer::ReadSample() const
	{
		return mBuffer.PeekAtCursor();
	}
}