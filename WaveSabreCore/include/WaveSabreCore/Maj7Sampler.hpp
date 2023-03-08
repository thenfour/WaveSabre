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
			BoolParam mReleaseExitsLoop;
			EnumParam<LoopMode> mLoopMode;
			EnumParam<LoopBoundaryMode> mLoopSource;
			EnumParam<InterpolationMode> mInterpolationMode;

			IntParam mBaseNote;

			Float01Param mSampleStart;
			Float01Param mLoopStart;
			Float01Param mLoopLength; // 0-1 ?

			int mSampleLoopStart = 0; // in samples
			int mSampleLoopLength = 0; // in samples
			float mSampleOriginalSR = 0;
			int mSampleLoadSequence = 0; // just an ID to let the VST editor know when the sample data has changed

			GsmSample* mSample = nullptr;
			float mSampleRateCorrectionFactor = 0;

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
					(ParamIndices)(int(baseParamID) + int(SamplerParamIndexOffsets::KeyRangeMin)),
					(ParamIndices)(int(baseParamID) + int(SamplerParamIndexOffsets::KeyRangeMax)),
					ampEnvModSourceID,
					modDestBaseID,
					(ModDestination)(int(modDestBaseID) + int(SamplerModParamIndexOffsets::Volume)),
					(ModDestination)(int(modDestBaseID) + int(SamplerModParamIndexOffsets::AuxMix)),
					(ModDestination)(int(modDestBaseID) + int(SamplerModParamIndexOffsets::HiddenVolume))
				),
				mLegatoTrig(paramCache[(int)baseParamID + (int)SamplerParamIndexOffsets::LegatoTrig], true),
				mReverse(paramCache[(int)baseParamID + (int)SamplerParamIndexOffsets::Reverse], false),
				mReleaseExitsLoop(paramCache[(int)baseParamID + (int)SamplerParamIndexOffsets::ReleaseExitsLoop], true),
				mSampleStart(paramCache[(int)baseParamID + (int)SamplerParamIndexOffsets::SampleStart], 0),
				mLoopMode(paramCache[(int)baseParamID + (int)SamplerParamIndexOffsets::LoopMode], LoopMode::NumLoopModes, LoopMode::Repeat),
				mLoopSource(paramCache[(int)baseParamID + (int)SamplerParamIndexOffsets::LoopSource], LoopBoundaryMode::NumLoopBoundaryModes, LoopBoundaryMode::FromSample),
				mInterpolationMode(paramCache[(int)baseParamID + (int)SamplerParamIndexOffsets::InterpolationType], InterpolationMode::NumInterpolationModes, InterpolationMode::Linear),
				mLoopStart(paramCache[(int)baseParamID + (int)SamplerParamIndexOffsets::LoopStart], 0),
				mLoopLength(paramCache[(int)baseParamID + (int)SamplerParamIndexOffsets::LoopLength], 1),
				mBaseNote(paramCache[(int)baseParamID + (int)SamplerParamIndexOffsets::BaseNote], 0, 127, 60)
			{}

			// called when loading chunk, or by VST
			void LoadSample(char* compressedDataPtr, int compressedSize, int uncompressedSize, WAVEFORMATEX* waveFormatPtr)
			{
				if (mSample) delete mSample;

				mSample = new GsmSample(compressedDataPtr, compressedSize, uncompressedSize, waveFormatPtr);
				mSampleLoadSequence++;

				mSampleLoopStart = 0;
				mSampleLoopLength = mSample->SampleLength;
				mSampleOriginalSR = (float)waveFormatPtr->nSamplesPerSec;
			}

			virtual void BeginBlock(int samplesInBlock) override
			{
				// base note + original samplerate gives us a "native frequency" of the sample.
				// so let's say the sample is 22.05khz, base midi note 60 (261hz).
				// and you are requested to play it back at "1000hz" (or, midi note 88.7)
				// and imagine native sample rate is 44.1khz.
				// the base frequency is 261hz, and you're requesting to play 1000hz
				// so that's (play_hz / base_hz) = a rate of 3.83. plus factor the base samplerate, and it's
				// (base_sr / play_sr) = another rate of 0.5.
				// put together that's (play_hz / base_hz) * (base_sr / play_sr)
				// or, play_hz * (base_sr / (base_hz * play_sr))
				//               ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
				// i don't know play_hz yet at this stage, let's simplify & precalculate the rest
				float base_hz = MIDINoteToFreq((float)mBaseNote.GetIntValue());
				mSampleRateCorrectionFactor = mSampleOriginalSR / (2 * base_hz * Helpers::CurrentSampleRateF);// WHY * 2? because it corresponds more naturally to other synth octave ranges.
			}
		}; // struct SamplerDevice

		////////////////////////////////////////////////////////////////////////////////////////////////
		struct SamplerVoice : ISoundSourceDevice::Voice
		{
			SamplerDevice* mpSamplerDevice;
			SamplePlayer mSamplePlayer;
			bool mNoteIsOn = false;

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
				if (!mNoteIsOn && mpSamplerDevice->mReleaseExitsLoop.GetBoolValue()) {
					mSamplePlayer.LoopMode = LoopMode::Disabled;
				}
				else
				{
					mSamplePlayer.LoopMode = mpSamplerDevice->mLoopMode.GetEnumValue();
				}
				mSamplePlayer.LoopBoundaryMode = mpSamplerDevice->mLoopSource.GetEnumValue();
				mSamplePlayer.InterpolationMode = mpSamplerDevice->mInterpolationMode.GetEnumValue();
				mSamplePlayer.Reverse = mpSamplerDevice->mReverse.GetBoolValue();
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
				mNoteIsOn = true;

				ConfigPlayer();
				mSamplePlayer.InitPos();
			}

			virtual void NoteOff() override
			{
				mNoteIsOn = false;
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

				float pitchFineMod = mModMatrix.GetDestinationValue((int)mpSrcDevice->mModDestBaseID + (int)SamplerModParamIndexOffsets::PitchFine, 0);
				float freqMod = mModMatrix.GetDestinationValue((int)mpSrcDevice->mModDestBaseID + (int)SamplerModParamIndexOffsets::FrequencyParam, 0);

				midiNote += mpSamplerDevice->mPitchSemisParam.GetIntValue() + mpSamplerDevice->mPitchFineParam.GetN11Value(pitchFineMod) * gSourcePitchFineRangeSemis;
				float noteHz = MIDINoteToFreq(midiNote);
				float freq = mpSamplerDevice->mFrequencyParam.GetFrequency(noteHz, freqMod);
				freq *= detuneFreqMul;

				float rate = freq * mpSamplerDevice->mSampleRateCorrectionFactor;

				mSamplePlayer.SetPlayRate(rate);
			}

			float ProcessSample(size_t iSample)
			{
				return mSamplePlayer.Next();
			}

		}; // struct SamplerVoice

	} // namespace M7


} // namespace WaveSabreCore




