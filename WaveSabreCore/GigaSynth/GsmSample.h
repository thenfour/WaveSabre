#ifndef __WAVESABRECORE_GSMSAMPLE_H__
#define __WAVESABRECORE_GSMSAMPLE_H__

#include "../Basic/DSPMath.hpp"
#include "../Basic/PodVector.hpp"

#ifdef UNICODE
#define _UNICODE
#endif

#include <mmreg.h> // must be before MSAcm.h
#include <MSAcm.h>

#include "SampleSource.hpp"

namespace WaveSabreCore::M7
{
#ifdef MAJ7_INCLUDE_GSM_SUPPORT
	class GsmSample : public ISampleSource
	{
	public:
		GsmSample(const char *data, int compressedSize, int uncompressedSize, WAVEFORMATEX *waveFormat);
		~GsmSample();

		PodVector<uint8_t> WaveFormatData;
		int CompressedSize, UncompressedSize;

		PodVector<uint8_t> CompressedData;
		PodVector<float> SampleData;

		int mSampleRate;

		virtual const float* GetSampleData() const override
		{
			return SampleData.data();
		}
		virtual size_t GetSampleLength() const override {
			return SampleData.size();
		}
		virtual int GetSampleLoopStart() const override { return 0; }
		virtual int GetSampleLoopLength() const override { return SampleData.size(); }
		virtual int GetSampleRate() const override { return mSampleRate; }

	private:
		static BOOL __stdcall driverEnumCallback(HACMDRIVERID driverId, DWORD_PTR dwInstance, DWORD fdwSupport);
		static BOOL __stdcall formatEnumCallback(HACMDRIVERID driverId, LPACMFORMATDETAILS formatDetails, DWORD_PTR dwInstance, DWORD fdwSupport);

		static HACMDRIVERID driverId;
	};
#endif  // MAJ7_INCLUDE_GSM_SUPPORT	
  }  // namespace WaveSabreCore::M7

#endif
