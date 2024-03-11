#ifndef __WAVESABRECORE_DELAYBUFFER_H__
#define __WAVESABRECORE_DELAYBUFFER_H__

#include <WaveSabreCore/Helpers.h>

namespace WaveSabreCore
{
	// not a template to avoid code size bloat.
	struct AudioBuffer
	{
#ifdef MIN_SIZE_REL
#pragma message("Maj7::~Maj7() Leaking memory to save bits.")
#else
		~AudioBuffer()
		{
			delete[] mBuffer;
		}
#endif

		int mLength;
		int mCursor;
		float* mBuffer = nullptr;

		// seems that this gets called on a separate thread than audio processing; make it naively thread-safe.
		void SetLengthSamples(size_t sampleCount);

		void WriteAndAdvance(float sample);

		float PeekAtCursor() const
		{
			return mBuffer[mCursor];
		}

		float& operator [](size_t i) {
			CCASSERT(i > 0);
			CCASSERT(i < mLength);
			return mBuffer[i];
		}
	};


	// --------------------------------------------------------------------------------------------------------------------------------------------
	class DelayBuffer
	{
	public:
		void SetLength(float lengthMs);

		void WriteSample(float sample);
		float ReadSample() const;

	private:
		AudioBuffer mBuffer;
	};
}

#endif