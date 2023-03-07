#pragma once

#include <WaveSabreCore/Maj7Basic.hpp>
#include <WaveSabreCore/Maj7ModMatrix.hpp>
#include <WaveSabreCore/Specimen.h>
#include "Maj7Oscillator.hpp"

namespace WaveSabreCore
{
	namespace M7
	{
		//////////////////////////////////////////////////////////////////////////////////////////////////
		//struct SamplerDevice : ISoundSourceDevice
		//{
		//	BoolParam mLegatoTrig;
		//	BoolParam mReverse;
		//	EnumParam<LoopMode> mLoopMode;
		//	EnumParam<LoopBoundaryMode> mLoopSource;
		//	EnumParam<InterpolationMode> mInterpolationMode;

		//	Float01Param mLoopStart;
		//	Float01Param mLoopLength;

		//	GsmSample* mSample = nullptr;

		//	explicit SamplerDevice(real_t* paramCache, ParamIndices paramBaseID)
		//	{
		//	}

		//	// called when loading chunk, or by VST
		//	void LoadSample(char* compressedDataPtr, int compressedSize, int uncompressedSize, WAVEFORMATEX* waveFormatPtr)
		//	{
		//		if (mSample) delete mSample;

		//		mSample = new GsmSample(compressedDataPtr, compressedSize, uncompressedSize, waveFormatPtr);

		//		mLoopStart.SetParamValue(0);
		//		mLoopLength.SetParamValue(mSample->SampleLength);
		//	}

		//}; // struct SamplerDevice

		//////////////////////////////////////////////////////////////////////////////////////////////////
		//struct SamplerVoice : ISoundSourceDevice::Voice
		//{
		//	void BeginBlock(SamplerDevice& sampler, ModMatrixNode& modMatrix, float midiNote, float voiceShapeMod, float detuneFreqMul, int samplesInBlock)
		//	{
		//		//if (!mEnabled.GetBoolValue()) {
		//		//	return;
		//		//}

		//		//samplePlayer.SampleStart = specimen->sampleStart;
		//		//samplePlayer.LoopStart = specimen->loopStart;
		//		//samplePlayer.LoopLength = specimen->loopLength;
		//		//samplePlayer.LoopMode = specimen->loopMode;
		//		//samplePlayer.LoopBoundaryMode = specimen->loopBoundaryMode;
		//		//samplePlayer.InterpolationMode = specimen->interpolationMode;
		//		//samplePlayer.Reverse = specimen->reverse;

		//		//samplePlayer.RunPrep();

		//		//float amp = Helpers::VolumeToScalar(specimen->masterLevel);
		//		//float panLeft = Helpers::PanToScalarLeft(Pan);
		//		//float panRight = Helpers::PanToScalarRight(Pan);

		//		//for (int i = 0; i < numSamples; i++)
		//		//{
		//		//	calcPitch();

		//		//	filter.SetFreq(Helpers::Clamp(specimen->filterFreq + modEnv.GetValue() * (20000.0f - 20.0f) * (specimen->filterModAmt * 2.0f - 1.0f), 0.0f, 20000.0f - 20.0f));

		//		//	float sample = samplePlayer.Next();
		//		//	if (!samplePlayer.IsActive)
		//		//	{
		//		//		IsOn = false;
		//		//		break;
		//		//	}

		//		//	sample = filter.Next(sample) * ampEnv.GetValue() * velocity * amp;
		//		//	outputs[0][i] += sample * panLeft;
		//		//	outputs[1][i] += sample * panRight;

		//		//	modEnv.Next();
		//		//	ampEnv.Next();
		//		//	if (ampEnv.State == EnvelopeState::Finished)
		//		//	{
		//		//		IsOn = false;
		//		//		break;
		//		//	}
		//		//}

		//	}

		//	void NoteOn(SamplerDevice& sampler, ModMatrixNode& modMatrix, bool legato)
		//	{
		//		//if (!specimen->sample) return;

		//		//samplePlayer.SampleData = specimen->sample->SampleData;
		//		//samplePlayer.SampleLength = specimen->sample->SampleLength;
		//		//samplePlayer.SampleLoopStart = specimen->sampleLoopStart;
		//		//samplePlayer.SampleLoopLength = specimen->sampleLoopLength;

		//		//samplePlayer.SampleStart = specimen->sampleStart;
		//		//samplePlayer.LoopStart = specimen->loopStart;
		//		//samplePlayer.LoopLength = specimen->loopLength;
		//		//samplePlayer.LoopMode = specimen->loopMode;
		//		//samplePlayer.LoopBoundaryMode = specimen->loopBoundaryMode;
		//		//samplePlayer.InterpolationMode = specimen->interpolationMode;
		//		//samplePlayer.Reverse = specimen->reverse;

		//		//calcPitch();
		//		//samplePlayer.InitPos();

		//		//this->velocity = (float)velocity / 128.0f;
		//	}

		//	float ProcessSample(SamplerDevice& sampler, ModMatrixNode& modMatrix, size_t iSample)
		//	{
		//		return 0;
		//	}

		//}; // struct SamplerVoice

	} // namespace M7


} // namespace WaveSabreCore




