#include "Device.h"
//#include <cstring>
#include "../Basic/DSPMath.hpp"
#include "../Basic/Helpers.h"
//#include <WaveSabreCore/Maj7Basic.hpp>

namespace WaveSabreCore
{

	Device::Device(int numParams, float* paramCache, const int16_t* defaults16) :
		numParams(numParams),
		mParamCache__(paramCache),
		mDefaults16__(defaults16)
	{
		// this is the best place to do static init
      if (!M7::math::gLuts)
      {
          M7::math::gLuts = new M7::math::LUTs();
      }
		chunkData = nullptr;
	}

	Device::~Device()
	{
#ifdef MIN_SIZE_REL
		// this is a virtual method so don't gate it out entirely.
  #pragma message("Device::~Device() Leaking memory to save bits.")
#else
		delete (char *)chunkData;
#endif
	}
	void Device::AllNotesOff() { }
	void Device::NoteOn(int note, int velocity, int deltaSamples) { }
	void Device::NoteOff(int note, int deltaSamples) { }

#ifndef MIN_SIZE_REL
	void Device::SetSampleRate(float sampleRate)
	{
		Helpers::CurrentSampleRateI = (int)sampleRate;
		Helpers::CurrentSampleRate = (double)sampleRate;
		Helpers::CurrentSampleRateF = sampleRate;
		Helpers::CurrentSampleRateRecipF = 1.0f / sampleRate;
	}
#endif
	void Device::SetTempo(int tempo)
	{
		Helpers::CurrentTempo = tempo;
	}

	void Device::LoadDefaults() {
		M7::ImportDefaultsArray(numParams, mDefaults16__, mParamCache__);
		SetParam(0, mParamCache__[0]);
	}


	void Device::SetBinary16DiffChunk(M7::Deserializer& ds)
	{
		this->LoadDefaults(); // important for delta behavior to start with defaults.
		for (int i = 0; i < this->numParams; ++i)
		{
			float f = GetParam(i) + ds.ReadInt16NormalizedFloat();
			SetParam(i, f);
		}
	}

	void Device::clearOutputs(float **outputs, int numSamples)
	{
		for (int i = 0; i < 2; i++)
		{
			memset(outputs[i], 0, sizeof(float) * numSamples);
			//for (int j = 0; j < numSamples; j++) outputs[i][j] = 0.0f;
		}
	}
}
