#ifndef __WAVESABRECORE_COMB_H__
#define __WAVESABRECORE_COMB_H__

#include "DelayBuffer.h"

namespace WaveSabreCore
{
	class Comb
	{
	public:
//#ifndef MIN_SIZE_REL
//		virtual ~Comb();
//#endif
//
		void SetBufferSize(int size);
		float Process(float inp);
		void SetParams(float damp, float feedback);

	private:
		float feedback;
		float filterStore = 0;
		float damp1;
		//float *buffer = nullptr;
		//int bufferSize;
		AudioBuffer mBuffer;
		int bufferIndex;
	};
}

#endif
