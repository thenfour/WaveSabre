// TODO: multiple filters? or add a 1pole lowcut/highcut to each oscillator
// TODO: S&H to each oscillator as well. i think an "insert" stage before filter could be appropriate, with balances for all osc.
// adding params:
// - S&H freq
// - LCut
// - Highcut
// - osc1 balance
// - osc2 balance
// - osc3 balance
// TODO: time sync for LFOs? envelope times?
// - lfo1 time basis
// - lfo2 time basis
// TODO: sampler engine & GM.DLS
// the idea: if we are able to optimize unused params, then we benefit by including a sampler directly in Maj7.
// because then all the routing and processing uses shared code and can even interoperate with the synths, D-50 style.
// and if we discard data we don't use, this comes for free.
// 
// speed optimizations
// (use profiler)
// - some things may be calculated at the synth level, not voice level. think portamento time/curve params?
// - recalc some a-rate things less frequently / reduce some things to k-rate. frequencies are calculated every sample...
// - cache some values like mod matrix arate source buffers
// - precalc curve in like 1024 x 1024 steps
// - mod matrix currently processes every mod spec each sample. K-Rate should be done per buffer init.
//   for example a K-rate to K-rate (macro to filter freq) does not need to process every sample.
// 
// size-optimizations:
// (profile w sizebench!)
// - modulation spec array initialization takes a lot of space (it's not compacted; basically an unrolled loop)
// - dont inline (use cpp)
// - use fixed-point in oscillators. there's absolutely no reason to be using chonky floating point instructions there.
// - find a way to exclude filters & oscillator waveforms selectively
// - chunk optimization:
//   - skip params which are disabled (modulation specs!) - only for wavesabre binary export; not for VST chunk.
//   - group params which are very likely to be the same value or rarely used
//   - enum and bool params can be packed tight

// Pitch params in order of processing:
// x                                  Units        Level
// --------------------------------------------------------
// - midi note                        note         voice @ note on
// - portamento time / curve          note         voice @ block process                           
// - pitch bend / pitch bend range    note         voice       <-- used for modulation source                          
// - osc pitch semis                  note         oscillator                  
// - osc fine (semis)                 note         oscillator                   
// - osc sync freq / kt (Hz)          hz           oscillator                        
// - osc freq / kt (Hz)               hz           oscillator                   
// - osc mul (Hz)                     hz           oscillator             
// - osc detune (semis)               hz+semis     oscillator                         
// - unisono detune (semis)           hz+semis     oscillator                             



#pragma once

#include <WaveSabreCore/Helpers.h>
#include <WaveSabreCore/Maj7SynthDevice.hpp>
#include <WaveSabreCore/Maj7Basic.hpp>
#include <WaveSabreCore/Maj7Envelope.hpp>
#include <WaveSabreCore/Maj7Filter.hpp>
#include <WaveSabreCore/Maj7Oscillator.hpp>
#include <WaveSabreCore/Maj7ModMatrix.hpp>

//#include <Windows.h>
//#undef min
//#undef max

namespace WaveSabreCore
{
	namespace M7
	{
		// this does not operate on frequencies, it operates on target midi note value (0-127).
		struct PortamentoCalc
		{
		private:
			EnvTimeParam mTime;
			CurveParam mCurve;

			bool mEngaged = false;

			float mSourceNote = 0; // float because you portamento can initiate between notes
			float mTargetNote = 0;
			float mCurrentNote = 0;
			float mSlideCursorSamples = 0;

		public:

			PortamentoCalc(float& timeParamBacking, float& curveParamBacking) :
				mTime(timeParamBacking, 0.2f),
				mCurve(curveParamBacking, 0)
			{}

			// advances cursor; call this at the end of buffer processing. next buffer will call
			// GetCurrentMidiNote() then.
			void Advance(size_t nSamples, float timeMod, float curveMod) {
				if (!mEngaged) {
					return;
				}
				mSlideCursorSamples += nSamples;
				float durationSamples = MillisecondsToSamples(mTime.GetMilliseconds(timeMod));
				float t = mSlideCursorSamples / durationSamples;
				if (t >= 1) {
					NoteOn(mTargetNote, true); // disengages
					return;
				}
				t = mCurve.ApplyToValue(t, curveMod);
				mCurrentNote = Lerp(mSourceNote, mTargetNote, t);
			}

			float GetCurrentMidiNote() const
			{
				return mCurrentNote;
			}

			void NoteOn(float targetNote, bool instant)
			{
				if (instant) {
					mCurrentNote = mSourceNote = mTargetNote = targetNote;
					mEngaged = false;
					return;
				}
				mEngaged = true;
				mSourceNote = mCurrentNote;
				mTargetNote = targetNote;
				mSlideCursorSamples = 0;
				return;
			}
		};

		struct Maj7 : public Maj7SynthDevice
		{
			static constexpr size_t gModulationCount = 8;
			static constexpr int gPitchBendMaxRange = 24;
			static constexpr int gUnisonoVoiceMax = 12;
			static constexpr size_t gOscillatorCount = 3;
			static constexpr real_t gMasterVolumeMaxDb = 6;
			static constexpr real_t gFilterCenterFrequency = 1000.0f;
			static constexpr real_t gFilterFrequencyScale = 10.0f;

			// BASE PARAMS & state
			real_t mPitchBendN11 = 0;
			bool mOscEnabled[gOscillatorCount] = { false, false, false };
			bool mDeviceModSourceValuesSetThisFrame = false;

			float mOscDetuneAmts[gOscillatorCount] = { 0 };
			float mUnisonoDetuneAmts[gUnisonoVoiceMax] = { 0 };

			float mOscPanAmts[gOscillatorCount] = { 0 };
			float mUnisonoPanAmts[gUnisonoVoiceMax];

			float mOscShapeAmts[gOscillatorCount] = { 0 };
			float mUnisonoShapeAmts[gUnisonoVoiceMax];

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
			Float01Param mUnisonoStereoSpread{ mParamCache[(int)ParamIndices::UnisonoStereoSpread], 0.0f };
			Float01Param mOscillatorDetune{ mParamCache[(int)ParamIndices::OscillatorDetune], 0.0f };
			Float01Param mOscillatorStereoSpread{ mParamCache[(int)ParamIndices::OscillatorSpread], 0.0f };

			Float01Param mUnisonoShapeSpread{ mParamCache[(int)ParamIndices::UnisonoShapeSpread], 0.0f };
			Float01Param mOscillatorShapeSpread{ mParamCache[(int)ParamIndices::OscillatorShapeSpread], 0.0f };

			Float01Param mFMBrightness{ mParamCache[(int)ParamIndices::FMBrightness], 0.5f };

			IntParam mPitchBendRange{ mParamCache[(int)ParamIndices::PitchBendRange], -gPitchBendMaxRange, gPitchBendMaxRange, 2 };
			
			ModulationSpec mModulations[gModulationCount] {
				{ mParamCache, (int)ParamIndices::Mod1Enabled },
				{ mParamCache, (int)ParamIndices::Mod2Enabled },
				{ mParamCache, (int)ParamIndices::Mod3Enabled },
				{ mParamCache, (int)ParamIndices::Mod4Enabled },
				{ mParamCache, (int)ParamIndices::Mod5Enabled },
				{ mParamCache, (int)ParamIndices::Mod6Enabled },
				{ mParamCache, (int)ParamIndices::Mod7Enabled },
				{ mParamCache, (int)ParamIndices::Mod8Enabled },
			};

			real_t mParamCache[(int)ParamIndices::NumParams] = {0};

			// these values are at the device level (not voice level), but yet they can be modulated by voice-level things.
			// for exmaple you could map Velocity to unisono detune. Well it should be clear that this doesn't make much sense,
			// except for maybe monophonic mode? So they are evaluated at the voice level but ultimately stored here.
			real_t mOscillatorDetuneMod = 0;
			real_t mUnisonoDetuneMod = 0;
			real_t mOscillatorStereoSpreadMod = 0;
			real_t mUnisonoStereoSpreadMod = 0;
			real_t mOscillatorShapeSpreadMod = 0;
			real_t mUnisonoShapeSpreadMod = 0;
			real_t mFMBrightnessMod = 0;
			real_t mPortamentoTimeMod = 0;
			real_t mPortamentoCurveMod = 0;

			Maj7() :
				Maj7SynthDevice((int)ParamIndices::NumParams)
			{
				for (size_t i = 0; i < std::size(mVoices); ++ i) {
					mVoices[i] = mMaj7Voice[i] = new Maj7Voice(this);
				}

				static constexpr ModDestination gOscVolumeModDests[gOscillatorCount] = {
					ModDestination::Osc1Volume,
					ModDestination::Osc2Volume,
					ModDestination::Osc3Volume,
				};

				for (size_t i = 0; i < gOscillatorCount; ++i) {
					auto& m = mModulations[gModulationCount - gOscillatorCount + i];
					m.mEnabled.SetBoolValue(true);
					m.mCurve.SetN11Value(0);
					m.mSource.SetEnumValue(ModSource::AmpEnv);
					m.mDestination.SetEnumValue(gOscVolumeModDests[i]);
					m.mScale.SetN11Value(1.0f);
				}

				mMasterVolume.SetLinearValue(1.0f);
				mParamCache[(int)ParamIndices::Osc1Enabled] = 1.0f;
			}

			virtual void HandlePitchBend(float pbN11) override
			{
				mPitchBendN11 = pbN11;
			}
			virtual void HandleMidiCC(int ccN, int val) override {
				// we don't care about this for the moment.
			}

			// TODO: size optimize chunk
			virtual void SetChunk(void* data, int size) override
			{
				auto params = (float*)data;
				// This may be different than our internal numParams value if this chunk was
				//  saved with an earlier version of the plug for example. It's important we
				//  don't read past the chunk data, so we set as many parameters as the
				//  chunk contains, not the amount of parameters we have available. The
				//  remaining parameters will retain their default values in that case, which
				//  if we've done our job right, shouldn't change the sound with respect to
				//  the parameters we read here.
				auto numChunkParams = (int)((size - sizeof(int)) / sizeof(float));
				for (int i = 0; i < numChunkParams; i++)
					SetParam(i, params[i]);
			}

			// TODO: size optimize chunk. first output bools, then based on bools skip params.
			// another way to size-optimize is that ALL our params are positive-valued floats.
			// that leaves 1 free bit to specify if the value is the default value, then either 7 or 31 bits to complete the field.
			// something like if (param.isbool)
			virtual int GetChunk(void** data) override
			{
				int chunkSize = numParams * sizeof(float) + sizeof(int);
				if (!chunkData) chunkData = new char[chunkSize];

				for (int i = 0; i < numParams; i++)
					((float*)chunkData)[i] = GetParam(i);
				*(int*)((char*)chunkData + chunkSize - sizeof(int)) = chunkSize;

				*data = chunkData;
				return chunkSize;
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

			virtual void ProcessBlock(double songPosition, float* const* const outputs, int numSamples) override
			{
				mDeviceModSourceValuesSetThisFrame = false;
				mOscEnabled[0] = BoolParam{ mParamCache[(int)ParamIndices::Osc1Enabled] }.GetBoolValue();
				mOscEnabled[1] = BoolParam{ mParamCache[(int)ParamIndices::Osc2Enabled] }.GetBoolValue();
				mOscEnabled[2] = BoolParam{ mParamCache[(int)ParamIndices::Osc3Enabled] }.GetBoolValue();

				BipolarDistribute(gOscillatorCount, mOscEnabled, mOscDetuneAmts);

				bool unisonoEnabled[gUnisonoVoiceMax] = { false };
				for (size_t i = 0; i < mVoicesUnisono; ++i) {
					unisonoEnabled[i] = true;
				}
				BipolarDistribute(gUnisonoVoiceMax, unisonoEnabled, mUnisonoDetuneAmts);

				// scale down per param & mod
				float unisonoDetuneAmt = mUnisonoDetune.Get01Value(mUnisonoDetuneMod);
				float unisonoStereoSpreadAmt = mUnisonoStereoSpread.Get01Value(mUnisonoStereoSpreadMod);
				float unisonoShapeSpreadAmt = mUnisonoShapeSpread.Get01Value(mUnisonoShapeSpreadMod);
				for (size_t i = 0; i < mVoicesUnisono; ++i) {
					mUnisonoPanAmts[i] = mUnisonoDetuneAmts[i] * unisonoStereoSpreadAmt;
					mUnisonoShapeAmts[i] = mUnisonoDetuneAmts[i] * unisonoShapeSpreadAmt;
					mUnisonoDetuneAmts[i] *= unisonoDetuneAmt;
				}

				float oscDetuneAmt = mOscillatorDetune.Get01Value(mOscillatorDetuneMod);
				float oscStereoSpreadAmt = mOscillatorStereoSpread.Get01Value(mOscillatorStereoSpreadMod);
				float oscShapeSpreadAmt = mOscillatorShapeSpread.Get01Value(mOscillatorShapeSpreadMod);
				for (size_t i = 0; i < gOscillatorCount; ++ i) {
					mOscShapeAmts[i] = mOscDetuneAmts[i] * oscShapeSpreadAmt;
					mOscPanAmts[i] = mOscDetuneAmts[i] * oscStereoSpreadAmt;
					mOscDetuneAmts[i] *= oscDetuneAmt;
				}

				for (auto* v : mMaj7Voice)
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
					mPortamento(owner->mParamCache[(int)ParamIndices::PortamentoTime], owner->mParamCache[(int)ParamIndices::PortamentoCurve]),
					mAmpEnv(mModMatrix, ModDestination::AmpEnvDelayTime, owner->mParamCache, (int)ParamIndices::AmpEnvDelayTime),
					mModEnv1(mModMatrix, ModDestination::Env1DelayTime, owner->mParamCache, (int)ParamIndices::Env1DelayTime),
					mModEnv2(mModMatrix, ModDestination::Env2DelayTime, owner->mParamCache, (int)ParamIndices::Env2DelayTime),
					mModLFO1(OscillatorIntentionLFO{}, mModMatrix, ModDestination::LFO1Waveshape, owner->mParamCache, (int)ParamIndices::LFO1Waveform),
					mModLFO2(OscillatorIntentionLFO{}, mModMatrix, ModDestination::LFO2Waveshape, owner->mParamCache, (int)ParamIndices::LFO2Waveform),
					mOscillator1(OscillatorIntentionAudio{}, mModMatrix, ModDestination::Osc1Volume, owner->mParamCache, (int)ParamIndices::Osc1Enabled),
					mOscillator2(OscillatorIntentionAudio{}, mModMatrix, ModDestination::Osc2Volume, owner->mParamCache, (int)ParamIndices::Osc2Enabled),
					mOscillator3(OscillatorIntentionAudio{}, mModMatrix, ModDestination::Osc3Volume, owner->mParamCache, (int)ParamIndices::Osc3Enabled),
					mFilterType(owner->mParamCache[(int)ParamIndices::FilterType], FilterModel::Count, FilterModel::Disabled),
					mFilterQ(owner->mParamCache[(int)ParamIndices::FilterQ], 0.35f),
					mFilterSaturation(owner->mParamCache[(int)ParamIndices::FilterSaturation], 0.06f),
					mFilterFreq(owner->mParamCache[(int)ParamIndices::FilterFrequency], owner->mParamCache[(int)ParamIndices::FilterFrequencyKT], Maj7::gFilterCenterFrequency, Maj7::gFilterFrequencyScale, 0.4f, 1.0f)
				{}

				Maj7* mpOwner;

				real_t mVelocity01 = 0;
				real_t mTriggerRandom01 = 0;
				PortamentoCalc mPortamento;

				EnvelopeNode mAmpEnv;
				EnvelopeNode mModEnv1;
				EnvelopeNode mModEnv2;
				OscillatorNode mModLFO1;
				OscillatorNode mModLFO2;

				ModMatrixNode mModMatrix;

				OscillatorNode mOscillator1;
				OscillatorNode mOscillator2;
				OscillatorNode mOscillator3;

				EnumParam<FilterModel> mFilterType; // FilterType,
				Float01Param mFilterQ;// FilterQ,
				Float01Param mFilterSaturation;// FilterSaturation,
				FrequencyParam mFilterFreq;// FilterFrequency,// FilterFrequencyKT,
				

				FilterNode mFilterL;
				FilterNode mFilterR;

				void ProcessAndMix(double songPosition, float* const* const outputs, int numSamples)
				{
					if (!mAmpEnv.IsPlaying()) {
						return;
					}

					// at this point run the graph.
					// 1. run signals which produce A-Rate outputs, so we can use those buffers to modulate nodes which have A-Rate destinations
					// 2. mod matrix, which fills buffers with modulation signals. mod sources should be up-to-date
					// 3. run signals which are A-Rate destinations.
					// if we were really ambitious we could make the filter an A-Rate destination as well, with oscillator outputs being A-Rate.
					mAmpEnv.BeginBlock(); // tells envelopes to recalc state based on mod values and changed settings
					mModEnv1.BeginBlock();
					mModEnv2.BeginBlock();

					mModMatrix.InitBlock(numSamples);

					real_t midiNote = mPortamento.GetCurrentMidiNote();
					midiNote += mpOwner->mPitchBendRange.GetIntValue() * mpOwner->mPitchBendN11;

					real_t noteHz = MIDINoteToFreq(midiNote);

					mModMatrix.SetSourceKRateValue(ModSource::PitchBend, mpOwner->mPitchBendN11); // krate, N11
					mModMatrix.SetSourceKRateValue(ModSource::Velocity, mVelocity01);  // krate, 01
					mModMatrix.SetSourceKRateValue(ModSource::NoteValue, midiNote / 127.0f); // krate, 01
					mModMatrix.SetSourceKRateValue(ModSource::RandomTrigger, mTriggerRandom01); // krate, 01
					mModMatrix.SetSourceKRateValue(ModSource::UnisonoVoice, float(mUnisonVoice + 1) / mpOwner->mVoicesUnisono); // krate, 01
					mModMatrix.SetSourceKRateValue(ModSource::SustainPedal, real_t(mpOwner->mIsPedalDown ? 0 : 1)); // krate, 01
					mModMatrix.SetSourceKRateValue(ModSource::Macro1, mpOwner->mMacro1.Get01Value());  // krate, 01
					mModMatrix.SetSourceKRateValue(ModSource::Macro2, mpOwner->mMacro2.Get01Value());  // krate, 01
					mModMatrix.SetSourceKRateValue(ModSource::Macro3, mpOwner->mMacro3.Get01Value());  // krate, 01
					mModMatrix.SetSourceKRateValue(ModSource::Macro4, mpOwner->mMacro4.Get01Value());  // krate, 01

					mModLFO1.BeginBlock(0, 1.0f);
					mModLFO2.BeginBlock(0, 1.0f);

					// process a-rate mod source buffers.
					for (size_t iSample = 0; iSample < numSamples; ++iSample)
					{
						mModMatrix.SetSourceARateValue(ModSource::AmpEnv, iSample, mAmpEnv.ProcessSample(iSample));
						mModMatrix.SetSourceARateValue(ModSource::ModEnv1, iSample, mModEnv1.ProcessSample(iSample));
						mModMatrix.SetSourceARateValue(ModSource::ModEnv2, iSample, mModEnv2.ProcessSample(iSample));
						mModMatrix.SetSourceARateValue(ModSource::LFO1, iSample, mModLFO1.ProcessSample(iSample, 0, 0, 0, 0));
						mModMatrix.SetSourceARateValue(ModSource::LFO2, iSample, mModLFO2.ProcessSample(iSample, 0, 0, 0, 0));
						mModMatrix.ProcessSample(mpOwner->mModulations, iSample);
					}

					auto filterType = this->mFilterType.GetEnumValue();
					auto filterFreq = mFilterFreq.GetFrequency(noteHz, mModMatrix.GetDestinationValue(ModDestination::FilterFrequency, 0));
					filterFreq = ClampInclusive(filterFreq, 0.0f, 20000.0f);
					auto filterQ = mFilterQ.Get01Value(mModMatrix.GetDestinationValue(ModDestination::FilterQ, 0));
					auto filterSaturation = mFilterSaturation.Get01Value(mModMatrix.GetDestinationValue(ModDestination::FilterSaturation, 0));
					mFilterL.SetParams(
						filterType,
						filterFreq,
						filterQ,
						filterSaturation
					);
					mFilterR.SetParams(
						filterType,
						filterFreq,
						filterQ,
						filterSaturation
					);

					// set device-level modded values.
					if (!mpOwner->mDeviceModSourceValuesSetThisFrame) {
						mpOwner->mOscillatorDetuneMod = mModMatrix.GetDestinationValue(ModDestination::OscillatorDetune, 0);
						mpOwner->mUnisonoDetuneMod = mModMatrix.GetDestinationValue(ModDestination::UnisonoDetune, 0);
						mpOwner->mOscillatorStereoSpreadMod = mModMatrix.GetDestinationValue(ModDestination::OscillatorStereoSpread, 0);
						mpOwner->mUnisonoStereoSpreadMod = mModMatrix.GetDestinationValue(ModDestination::UnisonoStereoSpread, 0);
						mpOwner->mOscillatorShapeSpreadMod = mModMatrix.GetDestinationValue(ModDestination::OscillatorShapeSpread, 0);
						mpOwner->mUnisonoShapeSpreadMod = mModMatrix.GetDestinationValue(ModDestination::UnisonoShapeSpread, 0);
						mpOwner->mFMBrightnessMod = mModMatrix.GetDestinationValue(ModDestination::FMBrightness, 0);
						mpOwner->mPortamentoTimeMod = mModMatrix.GetDestinationValue(ModDestination::PortamentoTime, 0);
						mpOwner->mPortamentoCurveMod = mModMatrix.GetDestinationValue(ModDestination::PortamentoCurve, 0);
						mpOwner->mDeviceModSourceValuesSetThisFrame = true;
					}

					float myUnisonoDetune = mpOwner->mUnisonoDetuneAmts[this->mUnisonVoice];
					float myUnisonoPan = mpOwner->mUnisonoPanAmts[this->mUnisonVoice];
					float ampEnvParam = mModMatrix.GetSourceValue(ModSource::AmpEnv, 0);

					float oscGains[gOscillatorCount][2] = { 0 };// [osc index, channel]

					// first just set left channel to gain, right channel to pan
					static const ModDestination gModDestOscVolumes[gOscillatorCount] = {
						ModDestination::Osc1Volume,
						ModDestination::Osc2Volume,
						ModDestination::Osc3Volume,
					};
					static const ParamIndices gOscVolumeParams[gOscillatorCount] = {
						ParamIndices::Osc1Volume,
						ParamIndices::Osc2Volume,
						ParamIndices::Osc3Volume,
					};

					OscillatorNode* posc[gOscillatorCount] = {
						&mOscillator1, &mOscillator2, &mOscillator3
					};
					for (size_t i = 0; i < std::size(posc); ++i)
					{
						if (!mpOwner->mOscEnabled[i]) {
							continue;
						}
						float semis = myUnisonoDetune + mpOwner->mOscDetuneAmts[i];
						posc[i]->BeginBlock(midiNote, SemisToFrequencyMul(semis));
					}

					// process volume every 16 samples
					static const int gQuasaiARateSampleMask = 0x7;

					for (int iSample = 0; iSample < numSamples; ++iSample) {
						if ((iSample & gQuasaiARateSampleMask) == 0) {
							for (size_t i = 0; i < std::size(posc); ++i)
							{
								if (!mpOwner->mOscEnabled[i]) {
									continue;
								}
								VolumeParam ampParam{ mpOwner->mParamCache[(int)gOscVolumeParams[i]], OscillatorNode::gVolumeMaxDb };
								float ampLinear = ampParam.GetLinearGain(mModMatrix.GetDestinationValue(gModDestOscVolumes[i], iSample));
								float pan = myUnisonoPan + mpOwner->mOscPanAmts[i];
								auto gains = PanToFactor(pan);
								oscGains[i][0] = gains.first * ampLinear;
								oscGains[i][1] = gains.second * ampLinear;
							}

						}

						real_t s1 = mOscillator1.ProcessSample(iSample,
							mOscillator2.GetLastSample(), mpOwner->mFMAmt2to1.Get01Value(),
							mOscillator3.GetLastSample(), mpOwner->mFMAmt3to1.Get01Value()
						);
						real_t s2 = mOscillator2.ProcessSample(iSample,
							s1, mpOwner->mFMAmt1to2.Get01Value(),
							mOscillator3.GetLastSample(), mpOwner->mFMAmt3to2.Get01Value()
						);
						real_t s3 = mOscillator3.ProcessSample(iSample,
							s1, mpOwner->mFMAmt1to3.Get01Value(),
							s2, mpOwner->mFMAmt2to3.Get01Value()
						);

						real_t sl = s1 * oscGains[0][0] + s2 * oscGains[1][0] + s3 * oscGains[2][0];
						real_t sr = s1 * oscGains[0][1] + s2 * oscGains[1][1] + s3 * oscGains[2][1];

						// apply filter
						sl = mFilterL.ProcessSample(sl);
						sr = mFilterR.ProcessSample(sr);

						outputs[0][iSample] += sl;
						outputs[1][iSample] += sr;
					}

					mPortamento.Advance(numSamples,
						mModMatrix.GetDestinationValue(ModDestination::PortamentoTime, 0),
						mModMatrix.GetDestinationValue(ModDestination::PortamentoCurve, 0)
						);
				}

				virtual void NoteOn() override {
					mVelocity01 = mNoteInfo.Velocity / 127.0f;
					mTriggerRandom01 = Helpers::RandFloat();
					mAmpEnv.noteOn(mLegato);
					mModEnv1.noteOn(mLegato);
					mModEnv2.noteOn(mLegato);
					mPortamento.NoteOn((float)mNoteInfo.MidiNoteValue, !mLegato);
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


			struct Maj7Voice* mMaj7Voice[maxVoices] = { 0 };

		};
	} // namespace M7

	using Maj7 = M7::Maj7;

} // namespace WaveSabreCore




