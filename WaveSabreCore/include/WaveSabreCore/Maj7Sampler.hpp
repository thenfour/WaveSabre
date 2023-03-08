#pragma once

#include <WaveSabreCore/Maj7Basic.hpp>
#include <WaveSabreCore/Maj7ModMatrix.hpp>
#include <WaveSabreCore/Specimen.h>
#include "Maj7Oscillator.hpp"

namespace WaveSabreCore
{
	namespace M7
	{
		////////////////////////////////////////////////////////////////////////////////////////////////
		struct SamplerDevice : ISoundSourceDevice
		{
			BoolParam mLegatoTrig;
			BoolParam mReverse;
			EnumParam<LoopMode> mLoopMode;
			EnumParam<LoopBoundaryMode> mLoopSource;
			EnumParam<InterpolationMode> mInterpolationMode;

			Float01Param mSampleStart;
			Float01Param mLoopStart;
			Float01Param mLoopLength; // 0-1 ?

			int mSampleLoopStart = 0; // in samples
			int mSampleLoopLength = 0; // in samples

			GsmSample* mSample = nullptr;

			explicit SamplerDevice(float* paramCache, ModulationSpec* ampEnvModulation,
				ParamIndices baseParamID, ModSource ampEnvModSourceID, ModDestination modDestBaseID
			) :
				ISoundSourceDevice(paramCache, ampEnvModulation, baseParamID,
					(ParamIndices)(int(baseParamID) + int(SamplerParamIndexOffsets::Enabled)),
					(ParamIndices)(int(baseParamID) + int(SamplerParamIndexOffsets::Volume)),
					(ParamIndices)(int(baseParamID) + int(SamplerParamIndexOffsets::AuxMix)),
					(ParamIndices)(int(baseParamID) + int(SamplerParamIndexOffsets::TuneSemis)),
					(ParamIndices)(int(baseParamID) + int(SamplerParamIndexOffsets::TuneFine)),
					(ParamIndices)(int(baseParamID) + int(SamplerParamIndexOffsets::FreqParam)),
					(ParamIndices)(int(baseParamID) + int(SamplerParamIndexOffsets::FreqKT)),
					ampEnvModSourceID,
					modDestBaseID,
					(ModDestination)(int(modDestBaseID) + int(SamplerModParamIndexOffsets::Volume)),
					(ModDestination)(int(modDestBaseID) + int(SamplerModParamIndexOffsets::AuxMix)),
					(ModDestination)(int(modDestBaseID) + int(SamplerModParamIndexOffsets::HiddenVolume))
				),
				mLegatoTrig(paramCache[(int)baseParamID + (int)SamplerParamIndexOffsets::LegatoTrig], true),
				mReverse(paramCache[(int)baseParamID + (int)SamplerParamIndexOffsets::Reverse], false),
				mSampleStart(paramCache[(int)baseParamID + (int)SamplerParamIndexOffsets::SampleStart], 0),
				mLoopMode(paramCache[(int)baseParamID + (int)SamplerParamIndexOffsets::LoopMode], LoopMode::NumLoopModes, LoopMode::Repeat),
				mLoopSource(paramCache[(int)baseParamID + (int)SamplerParamIndexOffsets::LoopSource], LoopBoundaryMode::NumLoopBoundaryModes, LoopBoundaryMode::FromSample),
				mInterpolationMode(paramCache[(int)baseParamID + (int)SamplerParamIndexOffsets::InterpolationType], InterpolationMode::NumInterpolationModes, InterpolationMode::Linear),
				mLoopStart(paramCache[(int)baseParamID + (int)SamplerParamIndexOffsets::LoopStart], 0),
				mLoopLength(paramCache[(int)baseParamID + (int)SamplerParamIndexOffsets::LoopLength], 1)
			{}

			// called when loading chunk, or by VST
			void LoadSample(char* compressedDataPtr, int compressedSize, int uncompressedSize, WAVEFORMATEX* waveFormatPtr)
			{
				if (mSample) delete mSample;

				mSample = new GsmSample(compressedDataPtr, compressedSize, uncompressedSize, waveFormatPtr);

				mSampleLoopStart = 0;
				mSampleLoopLength = mSample->SampleLength;
				//mLoopStart.SetParamValue(0);
				//mLoopLength.SetParamValue(mSample->SampleLength);
			}

		}; // struct SamplerDevice

		////////////////////////////////////////////////////////////////////////////////////////////////
		struct SamplerVoice : ISoundSourceDevice::Voice
		{
			SamplerDevice* mpSamplerDevice;
			SamplePlayer mSamplePlayer;

			SamplerVoice(SamplerDevice* pDevice, ModMatrixNode& modMatrix, EnvelopeNode* pAmpEnv) :
				ISoundSourceDevice::Voice(pDevice, modMatrix, pAmpEnv),
				mpSamplerDevice(pDevice)
			{
			}

			void ConfigPlayer()
			{
				mSamplePlayer.SampleStart = mpSamplerDevice->mSampleStart.Get01Value();
				mSamplePlayer.LoopStart = mpSamplerDevice->mLoopStart.Get01Value();
				mSamplePlayer.LoopLength = mpSamplerDevice->mLoopLength.Get01Value();
				mSamplePlayer.LoopMode = mpSamplerDevice->mLoopMode.GetEnumValue();
				mSamplePlayer.LoopBoundaryMode = mpSamplerDevice->mLoopSource.GetEnumValue();
				mSamplePlayer.InterpolationMode = mpSamplerDevice->mInterpolationMode.GetEnumValue();
				mSamplePlayer.Reverse = mpSamplerDevice->mReverse.GetBoolValue();
			}

			virtual void BeginBlock(real_t midiNote, float voiceShapeMod, float detuneFreqMul, float fmScale, int samplesInBlock) override
			{
				if (!mpSamplerDevice->mEnabledParam.GetBoolValue()) {
					return;
				}
				if (!mpSamplerDevice->mSample) {
					return;
				}

				ConfigPlayer();
				mSamplePlayer.RunPrep();
			}

			virtual void NoteOn(bool legato) override
			{
				if (!mpSamplerDevice->mSample) {
					return;
				}
				if (!mpSamplerDevice->mEnabledParam.GetBoolValue()) {
					return;
				}
				if (legato && !mpSamplerDevice->mLegatoTrig.GetBoolValue()) {
					return;
				}

				mSamplePlayer.SampleData = mpSamplerDevice->mSample->SampleData;
				mSamplePlayer.SampleLength = mpSamplerDevice->mSample->SampleLength;
				mSamplePlayer.SampleLoopStart = mpSamplerDevice->mSampleLoopStart; // used for boundary mode from sample
				mSamplePlayer.SampleLoopLength = mpSamplerDevice->mSampleLoopLength; // used for boundary mode from sample

				ConfigPlayer();
				mSamplePlayer.InitPos();

			}

			float ProcessSample(size_t iSample)
			{
				// TODO: calculate rate.
				mSamplePlayer.SetPlayRate(1);
				return mSamplePlayer.Next();
			}

		}; // struct SamplerVoice

	} // namespace M7


} // namespace WaveSabreCore




