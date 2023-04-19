#ifndef __WAVESABRECORE_SAMPLEPLAYER_H__
#define __WAVESABRECORE_SAMPLEPLAYER_H__

namespace WaveSabreCore
{
	enum class InterpolationMode
	{
		Nearest,
		Linear,

		NumInterpolationModes,
	};

	enum class LoopMode
	{
		Disabled,
		Repeat,
		PingPong,

		NumLoopModes,
	};

	enum class LoopBoundaryMode
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
		void CalcPitch(double note);
		void InitPos();
		void RunPrep();
		float Next();

		bool IsActive;

		float SampleStart;
		bool Reverse;
		WaveSabreCore::LoopMode LoopMode;
		WaveSabreCore::LoopBoundaryMode LoopBoundaryMode;
		float LoopStart;
		float LoopLength;

		WaveSabreCore::InterpolationMode InterpolationMode;

		const float *SampleData;
		int SampleLength;
		int SampleLoopStart;
		int SampleLoopLength;


		double samplePos;

	private:
		double sampleDelta;
		int roundedLoopStart, roundedLoopLength, roundedLoopEnd;
		bool reverse;
	};
}

#endif
