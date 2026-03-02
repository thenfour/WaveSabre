#ifndef __WAVESABRECORE_GSMSAMPLE_H__
#define __WAVESABRECORE_GSMSAMPLE_H__

#include "../Basic/DSPMath.hpp"

#ifdef UNICODE
#define _UNICODE
#endif

#include <mmreg.h> // must be before MSAcm.h
#include <MSAcm.h>

#include "SampleSource.hpp"

namespace WaveSabreCore::M7
{
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
  }  // namespace WaveSabreCore::M7

#endif
