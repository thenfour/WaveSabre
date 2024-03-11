#ifndef __WAVESABRECORE_SAMPLEPLAYER_H__
#define __WAVESABRECORE_SAMPLEPLAYER_H__

#include <cstdint>

namespace WaveSabreCore
{
	enum class InterpolationMode : uint8_t
	{
		Nearest,
		Linear,

		NumInterpolationModes,
	};

	enum class LoopMode : uint8_t
	{
		Disabled,
		Repeat,
		PingPong,

		NumLoopModes,
	};

	enum class LoopBoundaryMode : uint8_t
	{
		FromSample,
		Manual,

		NumLoopBoundaryModes,
	};

	typedef struct
	{
		char tag[4];
		unsigned int size;
		short wChannels;
		int dwSamplesPerSec;
		int dwAvgBytesPerSec;
		short wBlockAlign;
	} Fmt;

	typedef struct
	{
		char tag[4];
		unsigned int size;
		unsigned short unityNote;
		short fineTune;
		int gain;
		int attenuation;
		unsigned int fulOptions;
		unsigned int loopCount;
		unsigned int loopSize;
		unsigned int loopType;
		unsigned int loopStart;
		unsigned int loopLength;
	} Wsmp;

	class SamplePlayer
	{
	public:
		SamplePlayer();

		void SetPlayRate(double ratio);
		void InitPos();
		void RunPrep();
		float Next();

		bool IsActive;
		bool Reverse;
		WaveSabreCore::LoopMode LoopMode;
		WaveSabreCore::LoopBoundaryMode LoopBoundaryMode;
		WaveSabreCore::InterpolationMode InterpolationMode;

		float SampleStart;
		float LoopStart;
		float LoopLength;

		const float *SampleData;
		int SampleLength;
		int SampleLoopStart;
		int SampleLoopLength;

		double samplePos;

	private:
		double sampleDelta;
		int roundedLoopStart;
		int roundedLoopLength;
		int roundedLoopEnd;
		bool reverse;
	};
}

#endif
