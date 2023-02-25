// TODO: multiple filters? or add a 1pole lowcut/highcut to each oscillator
// TODO: time sync for LFOs? envelope times?

#pragma once

#include <WaveSabreCore/Helpers.h>
#include <WaveSabreCore/Maj7SynthDevice.hpp>
#include <WaveSabreCore/Maj7Basic.hpp>
#include <WaveSabreCore/Maj7Envelope.hpp>
#include <WaveSabreCore/Maj7Filter.hpp>
#include <WaveSabreCore/Maj7Oscillator.hpp>
#include <WaveSabreCore/Maj7ModMatrix.hpp>
//#include <Windows.h>

namespace WaveSabreCore
{
	namespace M7
	{
		struct Maj7 : public Maj7SynthDevice
		{
			static constexpr size_t gModulationCount = 8;
			static constexpr int gPitchBendMaxRange = 24;
			static constexpr int gUnisonoVoiceMax = 12;
			static constexpr real_t gMasterVolumeMaxDb = 6;

			// BASE PARAMS & state
			real_t mPitchBendN11 = 0;

			VolumeParam mMasterVolume{ mParamCache[(int)ParamIndices::MasterVolume], gMasterVolumeMaxDb, .5f };
			Float01Param mMacro1{ mParamCache[(int)ParamIndices::Macro1], 0.0f };
			Float01Param mMacro2{ mParamCache[(int)ParamIndices::Macro2], 0.0f };
			Float01Param mMacro3{ mParamCache[(int)ParamIndices::Macro3], 0.0f };
			Float01Param mMacro4{ mParamCache[(int)ParamIndices::Macro4], 0.0f };

			Float01Param mFMAmt1to2{ mParamCache[(int)ParamIndices::FMAmt1to2], 0.0f };
			Float01Param mFMAmt1to3{ mParamCache[(int)ParamIndices::FMAmt1to3], 0.0f };
			Float01Param mFMAmt2to1{ mParamCache[(int)ParamIndices::FMAmt2to1], 0.0f };
			Float01Param mFMAmt2to3{ mParamCache[(int)ParamIndices::FMAmt2to3], 0.0f };
			Float01Param mFMAmt3to1{ mParamCache[(int)ParamIndices::FMAmt3to1], 0.0f };
			Float01Param mFMAmt3to2{ mParamCache[(int)ParamIndices::FMAmt3to2], 0.0f };

			Float01Param mUnisonoDetune{ mParamCache[(int)ParamIndices::UnisonoDetune], 0.0f };
			Float01Param mOscillatorDetune{ mParamCache[(int)ParamIndices::OscillatorDetune], 0.0f };
			Float01Param mStereoSpread{ mParamCache[(int)ParamIndices::StereoSpread], 0.0f };
			Float01Param mFMBrightness{ mParamCache[(int)ParamIndices::FMBrightness], 0.5f };

			EnvTimeParam mPortamentoTime{ mParamCache[(int)ParamIndices::PortamentoTime], 0.2f };
			CurveParam mPortamentoCurve{ mParamCache[(int)ParamIndices::PortamentoCurve], 0.0f };
			IntParam mPitchBendRange{ mParamCache[(int)ParamIndices::PitchBendRange], -gPitchBendMaxRange, gPitchBendMaxRange, 2 };
			
			ModulationSpec mModulations[gModulationCount]{
				{ mParamCache, (int)ParamIndices::Mod1Enabled },
				{ mParamCache, (int)ParamIndices::Mod2Enabled },
				{ mParamCache, (int)ParamIndices::Mod3Enabled },
				{ mParamCache, (int)ParamIndices::Mod4Enabled },
				{ mParamCache, (int)ParamIndices::Mod5Enabled },
				{ mParamCache, (int)ParamIndices::Mod6Enabled },
				{ mParamCache, (int)ParamIndices::Mod7Enabled },
				{ mParamCache, (int)ParamIndices::Mod8Enabled },
			};

			enum class ParamIndices
			{
				MasterVolume,
				VoicingMode,
				Unisono,
				UnisonoDetune,
				OscillatorDetune,
				StereoSpread,
				FMBrightness,

				Macro1,
				Macro2,
				Macro3,
				Macro4,

				PortamentoTime,
				PortamentoCurve,
				PitchBendRange,

				Osc1Enabled,
				Osc1Volume,
				Osc1Waveform,
				Osc1Waveshape,
				Osc1PhaseRestart,
				Osc1PhaseOffset,
				Osc1SyncEnable,
				Osc1SyncFrequency,
				Osc1SyncFrequencyKT,
				Osc1FrequencyParam,
				Osc1FrequencyParamKT,
				Osc1PitchSemis,
				Osc1PitchFine,
				Osc1FreqMul,
				Osc1FMFeedback,

				Osc2Enabled,
				Osc2Volume,
				Osc2Waveform,
				Osc2Waveshape,
				Osc2PhaseRestart,
				Osc2PhaseOffset,
				Osc2SyncEnable,
				Osc2SyncFrequency,
				Osc2SyncFrequencyKT,
				Osc2FrequencyParam,
				Osc2FrequencyParamKT,
				Osc2PitchSemis,
				Osc2PitchFine,
				Osc2FreqMul,
				Osc2FMFeedback,

				Osc3Enabled,
				Osc3Volume,
				Osc3Waveform,
				Osc3Waveshape,
				Osc3PhaseRestart,
				Osc3PhaseOffset,
				Osc3SyncEnable,
				Osc3SyncFrequency,
				Osc3SyncFrequencyKT,
				Osc3FrequencyParam,
				Osc3FrequencyParamKT,
				Osc3PitchSemis,
				Osc3PitchFine,
				Osc3FreqMul,
				Osc3FMFeedback,

				AmpEnvDelayTime,
				AmpEnvAttackTime,
				AmpEnvAttackCurve,
				AmpEnvHoldTime,
				AmpEnvDecayTime,
				AmpEnvDecayCurve,
				AmpEnvSustainLevel,
				AmpEnvReleaseTime,
				AmpEnvReleaseCurve,
				AmpEnvLegatoRestart,

				Env1DelayTime,
				Env1AttackTime,
				Env1AttackCurve,
				Env1HoldTime,
				Env1DecayTime,
				Env1DecayCurve,
				Env1SustainLevel,
				Env1ReleaseTime,
				Env1ReleaseCurve,
				Env1LegatoRestart,

				Env2DelayTime,
				Env2AttackTime,
				Env2AttackCurve,
				Env2HoldTime,
				Env2DecayTime,
				Env2DecayCurve,
				Env2SustainLevel,
				Env2ReleaseTime,
				Env2ReleaseCurve,
				Env2LegatoRestart,

				LFO1Waveform,
				LFO1Waveshape,
				LFO1Restart, // if restart, then LFO is per voice. if no restart, then it's per synth.
				LFO1PhaseOffset,
				LFO1FrequencyParam,

				LFO2Waveform,
				LFO2Waveshape,
				LFO2Restart, // if restart, then LFO is per voice. if no restart, then it's per synth.
				LFO2PhaseOffset,
				LFO2FrequencyParam,

				FilterType,
				FilterQ,
				FilterSaturation,
				FilterFrequency,
				FilterFrequencyKT,

				FMAmt1to2,
				FMAmt1to3,
				FMAmt2to1,
				FMAmt2to3,
				FMAmt3to1,
				FMAmt3to2,

				Mod1Enabled,
				Mod1Source,
				Mod1Destination,
				Mod1Curve,
				Mod1Scale,

				Mod2Enabled,
				Mod2Source,
				Mod2Destination,
				Mod2Curve,
				Mod2Scale,

				Mod3Enabled,
				Mod3Source,
				Mod3Destination,
				Mod3Curve,
				Mod3Scale,

				Mod4Enabled,
				Mod4Source,
				Mod4Destination,
				Mod4Curve,
				Mod4Scale,

				Mod5Enabled,
				Mod5Source,
				Mod5Destination,
				Mod5Curve,
				Mod5Scale,

				Mod6Enabled,
				Mod6Source,
				Mod6Destination,
				Mod6Curve,
				Mod6Scale,

				Mod7Enabled,
				Mod7Source,
				Mod7Destination,
				Mod7Curve,
				Mod7Scale,

				Mod8Enabled,
				Mod8Source,
				Mod8Destination,
				Mod8Curve,
				Mod8Scale,

				NumParams,
			};

			real_t mParamCache[(int)ParamIndices::NumParams] = {0};

			Maj7() :
				Maj7SynthDevice((int)ParamIndices::NumParams)
			{
				for (auto& v : mVoices) {
					v = new Maj7Voice(this);
				}
			}

			virtual void HandlePitchBend(float pbN11) override
			{
				mPitchBendN11 = pbN11;
			}
			virtual void HandleMidiCC(int ccN, int val) override {
				// we don't care about this for the moment.
			}

			void Maj7::SetParam(int index, float value)
			{
				if (index < 0) return;
				if (index >= (int)ParamIndices::NumParams) return;

				switch (index)
				{
					case (int)ParamIndices::VoicingMode:
					{
						float t;
						EnumParam<WaveSabreCore::VoiceMode> voicingModeParam{ t, WaveSabreCore::VoiceMode::Count, WaveSabreCore::VoiceMode::Polyphonic };
						voicingModeParam.SetParamValue(value);
						this->SetVoiceMode(voicingModeParam.GetEnumValue());
						break;
					}
					case (int)ParamIndices::Unisono:
					{
						float t;
						IntParam p{ t, 1, gUnisonoVoiceMax, 0 };
						p.SetParamValue(value);
						this->SetUnisonoVoices(p.GetIntValue());
						break;
					}
				}

				mParamCache[index] = value;
			}

			float Maj7::GetParam(int index) const
			{
				if (index < 0) return 0;
				if (index >= (int)ParamIndices::NumParams) return 0;
				return mParamCache[index];
			}

			//

			virtual void ProcessBlock(double songPosition, float* const* const outputs, int numSamples) override
			{
				for (auto* v : mVoices)
				{
					v->ProcessAndMix(songPosition, outputs, numSamples);
				}
				// apply post FX.
				real_t masterGain = mMasterVolume.GetLinearGain();
				for (size_t iSample = 0; iSample < numSamples; ++iSample)
				{
					outputs[0][iSample] *= masterGain;
					outputs[1][iSample] *= masterGain;
				}
			}

			struct Maj7Voice : public Maj7SynthDevice::Voice
			{
				Maj7Voice(Maj7* owner) :
					mpOwner(owner),
					mAmpEnv(mModMatrix, ModDestination::AmpEnvDelayTime, owner->mParamCache, (int)ParamIndices::AmpEnvDelayTime),
					mModEnv1(mModMatrix, ModDestination::Env1DelayTime, owner->mParamCache, (int)ParamIndices::Env1DelayTime),
					mModEnv2(mModMatrix, ModDestination::Env2DelayTime, owner->mParamCache, (int)ParamIndices::Env2DelayTime),
					mModLFO1(OscillatorIntentionLFO{}, mModMatrix, ModDestination::LFO1Waveshape, owner->mParamCache, (int)ParamIndices::LFO1Waveform),
					mModLFO2(OscillatorIntentionLFO{}, mModMatrix, ModDestination::LFO2Waveshape, owner->mParamCache, (int)ParamIndices::LFO2Waveform),
					mOscillator1(OscillatorIntentionAudio{}, mModMatrix, ModDestination::Osc1Volume, owner->mParamCache, (int)ParamIndices::Osc1Enabled),
					mOscillator2(OscillatorIntentionAudio{}, mModMatrix, ModDestination::Osc2Volume, owner->mParamCache, (int)ParamIndices::Osc2Enabled),
					mOscillator3(OscillatorIntentionAudio{}, mModMatrix, ModDestination::Osc3Volume, owner->mParamCache, (int)ParamIndices::Osc3Enabled)
				{}

				Maj7* mpOwner;

				real_t mVelocity01 = 0;
				real_t mNoteValue01 = 0;
				real_t mTriggerRandom01 = 0;

				EnvelopeNode mAmpEnv;
				EnvelopeNode mModEnv1;
				EnvelopeNode mModEnv2;
				OscillatorNode mModLFO1;
				OscillatorNode mModLFO2;

				ModMatrixNode mModMatrix;

				OscillatorNode mOscillator1;
				OscillatorNode mOscillator2;
				OscillatorNode mOscillator3;
				FilterNode mFilterL;
				FilterNode mFilterR;

				ModulationBuffers mModSourceBuffers;

				virtual void ProcessAndMix(double songPosition, float* const* const outputs, int numSamples) override
				{
					if (!mAmpEnv.IsPlaying()) {
						return;
					}

					// parameters are stored in their respective objects already; no need to copy params everywhere.

					// at this point run the graph.
					// 1. run signals which produce A-Rate outputs, so we can use those buffers to modulate nodes which have A-Rate destinations
					// 2. mod matrix, which fills buffers with modulation signals. mod sources should be up-to-date
					// 3. run signals which are A-Rate destinations.
					// if we were really ambitious we could make the filter an A-Rate destination as well, with oscillator outputs being A-Rate.
					mAmpEnv.BeginBlock(); // tells envelopes to recalc state based on mod values and changed settings
					mModEnv1.BeginBlock();
					mModEnv2.BeginBlock();

					mModSourceBuffers.Reset((size_t)ModSource::Count, numSamples);// for our A-Rate mod source buffers, allocate the buffer.

					real_t midiNote = (real_t)mNoteInfo.MidiNoteValue;
					midiNote += mpOwner->mPitchBendRange.GetIntValue() * mpOwner->mPitchBendN11;

					real_t noteHz = MIDINoteToFreq(midiNote);

					// process a-rate mod source buffers.
					for (size_t iSample = 0; iSample < numSamples; ++iSample)
					{
						mModSourceBuffers.GetARateBuffer((size_t)ModSource::AmpEnv)[iSample] = mAmpEnv.ProcessSample();
						mModSourceBuffers.GetARateBuffer((size_t)ModSource::ModEnv1)[iSample] = mModEnv1.ProcessSample();
						mModSourceBuffers.GetARateBuffer((size_t)ModSource::ModEnv2)[iSample] = mModEnv2.ProcessSample();
						mModSourceBuffers.GetARateBuffer((size_t)ModSource::LFO1)[iSample] = mModLFO1.ProcessSample(noteHz, iSample, 0, 0, 0, 0);
						mModSourceBuffers.GetARateBuffer((size_t)ModSource::LFO2)[iSample] = mModLFO2.ProcessSample(noteHz, iSample, 0, 0, 0, 0);
					}

					mModMatrix.SetKRateSourceValue(ModSource::PitchBend, mpOwner->mPitchBendN11); // krate, N11
					mModMatrix.SetKRateSourceValue(ModSource::Velocity, mVelocity01);  // krate, 01
					mModMatrix.SetKRateSourceValue(ModSource::NoteValue, mNoteValue01); // krate, 01
					mModMatrix.SetKRateSourceValue(ModSource::RandomTrigger, mTriggerRandom01); // krate, 01
					mModMatrix.SetKRateSourceValue(ModSource::SustainPedal, real_t(mpOwner->mIsPedalDown ? 0 : 1)); // krate, 01
					mModMatrix.SetKRateSourceValue(ModSource::Macro1, mpOwner->mMacro1.Get01Value());  // krate, 01
					mModMatrix.SetKRateSourceValue(ModSource::Macro2, mpOwner->mMacro2.Get01Value());  // krate, 01
					mModMatrix.SetKRateSourceValue(ModSource::Macro3, mpOwner->mMacro3.Get01Value());  // krate, 01
					mModMatrix.SetKRateSourceValue(ModSource::Macro4, mpOwner->mMacro4.Get01Value());  // krate, 01

					mModMatrix.ProcessBlock(mModSourceBuffers);
					real_t baseVol = 0;
					VolumeParam hiddenVolume{ baseVol, 0, 0 };

					for (int iSample = 0; iSample < numSamples; ++iSample) {

						real_t s1 = mOscillator1.ProcessSample(noteHz, iSample,
							mOscillator2.GetLastSample(), mpOwner->mFMAmt2to1.Get01Value(),
							mOscillator3.GetLastSample(), mpOwner->mFMAmt3to1.Get01Value()
						);
						real_t s2 = mOscillator2.ProcessSample(noteHz, iSample,
							s1, mpOwner->mFMAmt1to2.Get01Value(),
							mOscillator3.GetLastSample(), mpOwner->mFMAmt3to2.Get01Value()
						);
						real_t s3 = mOscillator3.ProcessSample(noteHz, iSample,
							s1, mpOwner->mFMAmt1to3.Get01Value(),
							s2, mpOwner->mFMAmt2to3.Get01Value()
						);

						// TODO: apply panning to oscillator outputs
						real_t sl = s1 + s2 + s3;

						// apply amplitude envelope. it should be configured as a modulation tbh.
						float ampEnvVal = mModSourceBuffers.GetARateBuffer((size_t)ModSource::AmpEnv)[iSample];
						sl *= hiddenVolume.GetLinearGain(ampEnvVal);

						outputs[0][iSample] += mFilterL.ProcessSample(sl);
						outputs[1][iSample] += mFilterR.ProcessSample(sl);
					}

				}

				virtual void NoteOn() override {
					mVelocity01 = mNoteInfo.Velocity / 127.0f;
					mNoteValue01 = mNoteInfo.MidiNoteValue / 127.0f;
					mTriggerRandom01 = 0; // TODO
					mAmpEnv.noteOn(mLegato);
					mModEnv1.noteOn(mLegato);
					mModEnv2.noteOn(mLegato);
				}

				virtual void NoteOff() override {
					mAmpEnv.noteOff();
					mModEnv1.noteOff();
					mModEnv2.noteOff();
				}

				virtual void Kill() override {
					mAmpEnv.kill();
					mModEnv1.kill();
					mModEnv2.kill();
				}

				virtual bool IsPlaying() override {
					return mAmpEnv.IsPlaying();
				}
			};


		};
	} // namespace M7

	using Maj7 = M7::Maj7;

} // namespace WaveSabreCore




