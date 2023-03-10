#ifndef __WAVESABRECORE_GSMSAMPLE_H__
#define __WAVESABRECORE_GSMSAMPLE_H__

#include "Helpers.h"

#include <Windows.h>

#ifdef UNICODE
#define _UNICODE
#endif

#include <mmreg.h>
#include <MSAcm.h>

namespace WaveSabreCore
{

	struct ISampleSource
	{
		virtual ~ISampleSource()
		{}

		virtual const float* GetSampleData() const = 0;
		virtual int GetSampleLength() const = 0;
		virtual int GetSampleLoopStart() const = 0;
		virtual int GetSampleLoopLength() const = 0;
		virtual int GetSampleRate() const = 0;
	};

	class GsmSample : public ISampleSource
	{
	public:
		GsmSample(const char *data, int compressedSize, int uncompressedSize, WAVEFORMATEX *waveFormat);
		~GsmSample();

		char *WaveFormatData;
		int CompressedSize, UncompressedSize;

		char *CompressedData;
		float *SampleData;

		int SampleLength;
		int mSampleRate;

		virtual const float* GetSampleData() const override
		{
			return SampleData;
		}
		virtual int GetSampleLength() const override {
			return SampleLength;
		}
		virtual int GetSampleLoopStart() const override { return 0; }
		virtual int GetSampleLoopLength() const override { return SampleLength; }
		virtual int GetSampleRate() const override { return mSampleRate; }

	private:
		static BOOL __stdcall driverEnumCallback(HACMDRIVERID driverId, DWORD_PTR dwInstance, DWORD fdwSupport);
		static BOOL __stdcall formatEnumCallback(HACMDRIVERID driverId, LPACMFORMATDETAILS formatDetails, DWORD_PTR dwInstance, DWORD fdwSupport);

		static HACMDRIVERID driverId;
	};
}

#endif
