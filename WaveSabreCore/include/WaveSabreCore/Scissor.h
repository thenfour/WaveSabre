#ifndef __WAVESABRECORE_SCISSOR_H__
#define __WAVESABRECORE_SCISSOR_H__

#include "Device.h"
#include <WaveSabreCore/Maj7Basic.hpp>
#include <WaveSabreCore/Maj7ParamAccessor.hpp>

namespace WaveSabreCore
{
	struct Scissor : public Device
	{
		//enum class Type : uint8_t {
		//	Tanh,
		//	HardClip,
		//	SminQuad,
		//	Atan,
		//};
		//enum class ParamIndices : uint8_t
		//{
		//	Type, // enum.
		//	Drive, // -inf to +36db. don't think of as input gain because it will get gain compensated.
		//	Shape, // 0-1 continuous shaping of the waveshape
		//	EvenHarmonics, // mix in even harmonics
		//	Oversample, // 1x, 2x, 4x
		//	DryWetMix, // -1 = diff, 0 = dry, 1 = wet
		//	OutputVolume, // -inf to +12db

		//	NumParams,
		//};
//#define SCISSOR_PARAM_VST_NAMES(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::LevellerParamIndices::NumParams]{ \
//	{"MstVol"},\
//	{"HPFreq"},\
//	{"_none1"},\
//	{"HPQ"},\
//	{"P1Freq"},\
//	{"P1Gain"},\
//	{"P1Q"},\
//	{"P2Freq"},\
//	{"P2Gain"},\
//	{"P2Q"},\
//	{"P3Freq"},\
//	{"P3Gain"},\
//	{"P3Q"},\
//	{"LPFreq"},\
//	{"_none2"},\
//	{"LPQ"},\
//}

		//static constexpr M7::VolumeParamConfig gDriveVolumeCfg{ 63.09573444801933f, 36 };
		//static constexpr M7::VolumeParamConfig gDriveVolumeCfg{ 1, 12 };

		Scissor();

		virtual void Run(double songPosition, float **inputs, float **outputs, int numSamples);

		virtual void SetParam(int index, float value);
		virtual float GetParam(int index) const;

	private:

		//M7::ParamAccessor mParams;

		//enum class Oversampling
		//{
		//	X1,
		//	X2,
		//	X4,
		//};

		//float distort(float v, float driveScalar);

		//ShaperType type;
		//float drive, threshold, foldover, dryWet;
		//Oversampling oversampling;
		//float drive;
		//float lastSample[2];
	};
}

#endif
