#include <WaveSabreCore/Device.h>
#include <cstring>
#include <WaveSabreCore/Helpers.h>
#include <WaveSabreCore/Maj7Basic.hpp>

namespace WaveSabreCore
{
	Device::Device(int numParams)
	{
		M7::Init();
		this->numParams = numParams;
		chunkData = nullptr;
	}

	Device::~Device()
	{
		delete (char *)chunkData;
	}

	void Device::AllNotesOff() { }
	void Device::NoteOn(int note, int velocity, int deltaSamples) { }
	void Device::NoteOff(int note, int deltaSamples) { }

	void Device::SetSampleRate(float sampleRate)
	{
		Helpers::CurrentSampleRate = (double)sampleRate;
		Helpers::CurrentSampleRateF = sampleRate;
		Helpers::CurrentSampleRateRecipF = 1.0f / sampleRate;
	}

	void Device::SetTempo(int tempo)
	{
		Helpers::CurrentTempo = tempo;
	}

	void Device::SetParam(int index, float value) { }

	float Device::GetParam(int index) const
	{
		return 0.0f;
	}

	void Device::SetChunk(void *data, int size)
	{
		auto params = (float *)data;
		// This may be different than our internal numParams value if this chunk was
		//  saved with an earlier version of the plug for example. It's important we
		//  don't read past the chunk data, so we set as many parameters as the
		//  chunk contains, not the amount of parameters we have available. The
		//  remaining parameters will retain their default values in that case, which
		//  if we've done our job right, shouldn't change the sound with respect to
		//  the parameters we read here.
		auto numChunkParams = (int)((size - sizeof(int)) / sizeof(float));
		for (int i = 0; i < numChunkParams; i++)
			SetParam(i, params[i]);
	}

	void Device::SetMaj7StyleChunk(M7::Deserializer& ds)
	{
		LoadDefaults();
		for (int i = 0; i < numParams; ++i)
		{
			float f = GetParam(i) + ds.ReadInt16NormalizedFloat();
			SetParam(i, f);
		}
	}

	// outputs all float params in a contiguous array, followed by a int32 of the chunk size that was just written (which tbh is pretty useless but maybe it was used as a sort of checksum at some point?)
	int Device::GetChunk(void **data)
	{
		int chunkSize = numParams * sizeof(float) + sizeof(int);
		if (!chunkData) chunkData = new char[chunkSize];

		for (int i = 0; i < numParams; i++)
			((float *)chunkData)[i] = GetParam(i);
		*(int *)((char *)chunkData + chunkSize - sizeof(int)) = chunkSize;

		*data = chunkData;
		return chunkSize;
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
