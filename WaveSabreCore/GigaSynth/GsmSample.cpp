
#include "GsmSample.h"

namespace WaveSabreCore::M7
{
#ifdef MAJ7_INCLUDE_GSM_SUPPORT
	GsmSample::GsmSample(const char *data, int compressedSize, int uncompressedSize, WAVEFORMATEX *waveFormat)
		: CompressedSize(compressedSize)
		, UncompressedSize(uncompressedSize)
	{
		size_t waveFormatSize = sizeof(WAVEFORMATEX) + waveFormat->cbSize;
		WaveFormatData.reserve(waveFormatSize);
		auto waveFormatBytes = reinterpret_cast<const uint8_t*>(waveFormat);
		for (size_t i = 0; i < waveFormatSize; ++i)
		{
			WaveFormatData.push_back(waveFormatBytes[i]);
		}

		CompressedData.reserve(compressedSize);
		auto compressedBytes = reinterpret_cast<const uint8_t*>(data);
		for (int i = 0; i < compressedSize; ++i)
		{
			CompressedData.push_back(compressedBytes[i]);
		}

		acmDriverEnum(driverEnumCallback, NULL, NULL);
		HACMDRIVER driver = NULL;
		acmDriverOpen(&driver, driverId, 0);

		WAVEFORMATEX dstWaveFormat =
		{
			WAVE_FORMAT_PCM,
			1,
			waveFormat->nSamplesPerSec,
			waveFormat->nSamplesPerSec * 2,
			sizeof(short),
			sizeof(short) * 8,
			0
		};

		HACMSTREAM stream = NULL;
		acmStreamOpen(&stream, driver, waveFormat, &dstWaveFormat, NULL, NULL, NULL, ACM_STREAMOPENF_NONREALTIME);

		ACMSTREAMHEADER streamHeader;
		memset(&streamHeader, 0, sizeof(ACMSTREAMHEADER));
		streamHeader.cbStruct = sizeof(ACMSTREAMHEADER);
		streamHeader.pbSrc = (LPBYTE)CompressedData.data();
		streamHeader.cbSrcLength = compressedSize;
		auto uncompressedData = new short[uncompressedSize * 2];
		streamHeader.pbDst = (LPBYTE)uncompressedData;
		streamHeader.cbDstLength = uncompressedSize * 2;
		acmStreamPrepareHeader(stream, &streamHeader, 0);

		acmStreamConvert(stream, &streamHeader, 0);

		acmStreamClose(stream, 0);
		acmDriverClose(driver, 0);

		mSampleRate = waveFormat->nSamplesPerSec;
		auto sampleLength = int(streamHeader.cbDstLengthUsed / sizeof(short));
		SampleData.resize(sampleLength);
		for (int i = 0; i < sampleLength; i++)
		{
			SampleData[i] = M7::math::Sample16To32Bit(uncompressedData[i]);
		}

		delete [] uncompressedData;
	}

	GsmSample::~GsmSample()
	{
	}

	HACMDRIVERID GsmSample::driverId = NULL;

	BOOL __stdcall GsmSample::driverEnumCallback(HACMDRIVERID driverId, DWORD_PTR dwInstance, DWORD fdwSupport)
	{
		if (GsmSample::driverId) return 1;

		HACMDRIVER driver = NULL;
		acmDriverOpen(&driver, driverId, 0);

		int waveFormatSize = 0;
		acmMetrics(NULL, ACM_METRIC_MAX_SIZE_FORMAT, &waveFormatSize);
		auto waveFormat = (WAVEFORMATEX *)(new char[waveFormatSize]);
		memset(waveFormat, 0, waveFormatSize);
		ACMFORMATDETAILS formatDetails;
		memset(&formatDetails, 0, sizeof(formatDetails));
		formatDetails.cbStruct = sizeof(formatDetails);
		formatDetails.pwfx = waveFormat;
		formatDetails.cbwfx = waveFormatSize;
		formatDetails.dwFormatTag = WAVE_FORMAT_UNKNOWN;
		acmFormatEnum(driver, &formatDetails, formatEnumCallback, NULL, NULL);

		delete [] (char *)waveFormat;

		acmDriverClose(driver, 0);

		return 1;
	}

	BOOL __stdcall GsmSample::formatEnumCallback(HACMDRIVERID driverId, LPACMFORMATDETAILS formatDetails, DWORD_PTR dwInstance, DWORD fdwSupport)
	{
		if (formatDetails->pwfx->wFormatTag == WAVE_FORMAT_GSM610 &&
			formatDetails->pwfx->nChannels == 1 &&
			formatDetails->pwfx->nSamplesPerSec == 44100)
		{
			GsmSample::driverId = driverId;
		}
		return 1;
	}

	#endif  // MAJ7_INCLUDE_GSM_SUPPORT
  }  //  namespace WaveSabreCore::M7
