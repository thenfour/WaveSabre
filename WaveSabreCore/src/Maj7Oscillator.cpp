
#include <WaveSabreCore/Maj7Oscillator.hpp>

namespace WaveSabreCore
{
	namespace M7
	{

		// for Audio
		OscillatorDevice::OscillatorDevice(/*OscillatorIntentionAudio, */float* paramCache, ModulationList modulations, const SourceInfo& srcinfo) :
			ISoundSourceDevice(paramCache, modulations[srcinfo.mModulationIndex], srcinfo.mParamBase,
				srcinfo.mAmpModSource,
				srcinfo.mModDestBase,
				(ModDestination)(int(srcinfo.mModDestBase) + int(OscModParamIndexOffsets::Volume)),
				//(ModDestination)(int(srcinfo.mModDestBase) + int(OscModParamIndexOffsets::AuxMix)),
				(ModDestination)(int(srcinfo.mModDestBase) + int(OscModParamIndexOffsets::PreFMVolume))
			),
			mIntention(OscillatorIntention::Audio)
		{
		}

		// for LFO, we internalize many params
		OscillatorDevice::OscillatorDevice(/*OscillatorIntentionLFO, */float* paramCache, size_t ilfo) :
			ISoundSourceDevice(paramCache, nullptr, gLFOInfo[ilfo].mParamBase,
				ModSource::Invalid, // amp env mod source doesn't exist for lfo
				gLFOInfo[ilfo].mModBase,
				ModDestination::Invalid, // volume mod dest
				//ModDestination::Invalid, // AuxMix mod dest
				ModDestination::Invalid // PreFMVolume mod dest
			)
			,
			mIntention(OscillatorIntention::LFO)
		{
		}
	} // namespace M7


} // namespace WaveSabreCore




