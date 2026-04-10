#include "Helpers.h"
#include "../rendered.hpp"

#ifdef _DEBUG
namespace cc
{
	thread_local extern size_t gBufferCount = 0;
	thread_local size_t gLogIndent = 0;
}
#endif


#include <inttypes.h>
#define _USE_MATH_DEFINES
#include <math.h>

namespace WaveSabreCore
{
#ifndef MIN_SIZE_REL
	int Helpers::CurrentSampleRateI = 44100;
	double Helpers::CurrentSampleRate = 44100.0;
	float Helpers::CurrentSampleRateF = 44100.0f;
	float Helpers::CurrentSampleRateRecipF = 1.0f/44100.0f;
	float Helpers::NyquistHz = 22050.0f;

	void Helpers::SetSampleRate(int sampleRate)
	{
		CurrentSampleRateI = sampleRate;
		CurrentSampleRate = static_cast<double>(sampleRate);
		CurrentSampleRateF = static_cast<float>(sampleRate);
		CurrentSampleRateRecipF = 1.0f / static_cast<float>(sampleRate);
		NyquistHz = CurrentSampleRateF * 0.5f;
	}

	int Helpers::CurrentTempo = 120;
#endif  // MIN_SIZE_REL
}

//thread_local size_t cc::gLogIndent = 0;
