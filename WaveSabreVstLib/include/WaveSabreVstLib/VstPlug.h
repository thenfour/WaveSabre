#ifndef __WAVESABREVSTLIB_VSTPLUG_H__
#define __WAVESABREVSTLIB_VSTPLUG_H__

#undef _WIN32_WINNT
#include "audioeffectx.h"
#include <WaveSabreCore.h>

namespace WaveSabreVstLib
{
	template <typename Treal, size_t N>
	class MovingAverage
	{
	public:
		void Update(Treal sample)
		{
			if (num_samples_ < N)
			{
				samples_[num_samples_++] = sample;
				total_ += sample;
			}
			else
			{
				Treal& oldest = samples_[num_samples_++ % N];
				total_ += sample - oldest;
				oldest = sample;
			}
		}

		Treal GetValue() const
		{
			return total_ / GetValidSampleCount();// std::min(num_samples_, N);
		}

		size_t GetValidSampleCount() const
		{
			return std::min(num_samples_, N);
		}
		size_t GetTotalSamplesTaken() const
		{
			return num_samples_;
		}

		void Clear()
		{
			total_ = 0;
			num_samples_ = 0;
		}

		Treal GetSample(size_t n) const
		{
			return samples_[n % N];
		}

	private:
		Treal samples_[N];
		size_t num_samples_ = 0;
		Treal total_ = 0;
	};

	class VstPlug : public AudioEffectX
	{
	public:
		VstPlug(audioMasterCallback audioMaster, int numParams, int numInputs, int numOutputs, VstInt32 id, WaveSabreCore::Device *device, bool synth = false);
		virtual ~VstPlug();

		virtual void processReplacing(float **inputs, float **outputs, VstInt32 sampleFrames);
		virtual VstInt32 processEvents(VstEvents *ev);

		virtual void setProgramName(char *name);
		virtual void getProgramName(char *name);

		virtual void setSampleRate(float sampleRate);
		virtual void setTempo(int tempo);

		virtual void setParameter(VstInt32 index, float value);
		virtual float getParameter(VstInt32 index);
		virtual void getParameterLabel(VstInt32 index, char *label);
		virtual void getParameterDisplay(VstInt32 index, char *text);
		virtual void getParameterName(VstInt32 index, char *text);

		virtual VstInt32 getChunk(void **data, bool isPreset);
		virtual VstInt32 setChunk(void *data, VstInt32 byteSize, bool isPreset);

		virtual bool getEffectName(char *name);
		virtual bool getVendorString(char *text);
		virtual bool getProductString(char *text);
		virtual VstInt32 getVendorVersion();

		virtual VstInt32 canDo(char *text);
		virtual VstInt32 getNumMidiInputChannels();

		double GetCPUUsage01() const {
			return mCPUUsage.GetValue();
		}

	protected:
		void setEditor(class VstEditor *editor);
		WaveSabreCore::Device *getDevice() const;

	private:
		int numParams, numInputs, numOutputs;
		bool synth;

		LARGE_INTEGER perfFreq;
		MovingAverage<double, 100> mCPUUsage;

		char programName[kVstMaxProgNameLen + 1];

		WaveSabreCore::Device *device;
	};
}

#endif
