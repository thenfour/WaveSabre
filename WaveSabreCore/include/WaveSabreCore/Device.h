#ifndef __WAVESABRECORE_DEVICE_H__
#define __WAVESABRECORE_DEVICE_H__

#include <Windows.h>

namespace WaveSabreCore
{
	namespace M7
	{
		struct Deserializer;
	}
	class Device
	{
	public:
		Device(int numParams);
		virtual ~Device();

		virtual void Run(double songPosition, float **inputs, float **outputs, int numSamples) = 0;

		virtual void AllNotesOff();
		virtual void NoteOn(int note, int velocity, int deltaSamples);
		virtual void NoteOff(int note, int deltaSamples);
		virtual void MidiCC(int ccNumber, int rawValue, int deltaSamples) {};
		virtual void PitchBend(int lsb, int msb, int deltaSamples) {};

		void SetSampleRate(float sampleRate);
		void SetTempo(int tempo);

		virtual void SetParam(int index, float value);
		virtual float GetParam(int index) const;

		virtual void SetChunk(void* data, int size);
		virtual int GetChunk(void** data);

		// support for maj7 style chunks, which are 16-bit and differential from default values
		virtual void LoadDefaults() {
			::OutputDebugStringA("!!! Device::LoadDefaults");
		}
		void SetMaj7StyleChunk(M7::Deserializer& ds);

	protected:
		void clearOutputs(float **outputs, int numSamples);

		int numParams;
		void *chunkData;
	};
}

#endif
