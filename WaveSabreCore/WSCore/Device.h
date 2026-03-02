#ifndef __WAVESABRECORE_DEVICE_H__
#define __WAVESABRECORE_DEVICE_H__

#include "../Basic/Serializer.hpp"

#include <Windows.h>
#include <stdint.h>
#include <atomic>


#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  //#define OPTIMIZE_OUT(x) x
  #define WRITE_ANALYSIS_SAMPLE(isGuiVisible, stream, value) if (isGuiVisible) { stream.WriteSample(value); }
  #define WRITE_SPECTRUM_SAMPLE(isGuiVisible, stream, value) if (isGuiVisible) { stream.ProcessSamples(value); }
#else
  //#define OPTIMIZE_OUT(x)
  #define WRITE_ANALYSIS_SAMPLE(isGuiVisible, stream, value)
  #define WRITE_SPECTRUM_SAMPLE(isGuiVisible, stream, value)
#endif


namespace WaveSabreCore
{
	class Device
	{
	public:
		explicit Device(int numParams, float* paramCache, const int16_t* defaults16);

		virtual ~Device();

		virtual void Run(float **inputs, float **outputs, int numSamples) = 0;

		virtual void AllNotesOff();
		virtual void NoteOn(int note, int velocity, int deltaSamples);
		virtual void NoteOff(int note, int deltaSamples);
		virtual void MidiCC(int ccNumber, int rawValue, int deltaSamples) {};
		virtual void PitchBend(int lsb, int msb, int deltaSamples) {};

#ifndef MIN_SIZE_REL
	    void SetSampleRate(float sampleRate);
#endif
		void SetTempo(int tempo);
		virtual void OnParamsChanged() {} // like Reaper's @slider section, run when params have changed. means you don't have to override setparam() to do that kind of recalcing.
		virtual void SetParam(int index, float value) {
			mParamCache__[index] = value;
			OnParamsChanged();
		}
		virtual float GetParam(int index) const {
			return mParamCache__[index];
		}

		// support for maj7 style chunks, which are 16-bit and differential from default values
		virtual void LoadDefaults();
		virtual void SetBinary16DiffChunk(M7::Deserializer& ds);

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
		std::atomic<bool> mGuiVisible{ false };
		bool IsGuiVisible() const {
			return mGuiVisible.load(std::memory_order_relaxed);
		}
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

	protected:
		void clearOutputs(float **outputs, int numSamples);

		int numParams;
		void *chunkData;

	private:
		float* mParamCache__;
		const int16_t* mDefaults16__;
	};
}

#endif
