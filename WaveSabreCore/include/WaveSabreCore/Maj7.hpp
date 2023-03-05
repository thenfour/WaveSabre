// TODO: multiple filters? or add a 1pole lowcut/highcut to each oscillator
// TODO: S&H to each oscillator as well. i think an "insert" stage before filter could be appropriate, with balances for all osc.
// adding params:
// - S&H freq
// - LCut
// - Highcut
// - osc1 balance
// - osc2 balance
// - osc3 balance
// TODO: time sync for LFOs? it's more complex than i expected, because of the difference between live play & playback.
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
		struct AuxNode
		{
			AuxLink mLinkToSelf;
			BoolParam mEnabledParam;
			EnumParam<AuxLink> mLink;
			EnumParam<AuxEffectType> mEffectType;
			float* mParamCache_Offset;
			//int mParamIndexOffset;
			int mModDestParam2ID;

			AuxLink mCurrentEffectLink;
			AuxEffectType mCurrentEffectType = AuxEffectType::None;
			IAuxEffect* mpCurrentEffect = nullptr;

			AuxNode(AuxLink selfLink, float* paramCache_Offset, int modDestParam2ID) :
				mLinkToSelf(selfLink),
				mEnabledParam(paramCache_Offset[(int)AuxParamIndexOffsets::Enabled]),
				mLink(paramCache_Offset[(int)AuxParamIndexOffsets::Link], AuxLink::Count),
				mEffectType(paramCache_Offset[(int)AuxParamIndexOffsets::Type], AuxEffectType::Count),
				mCurrentEffectLink(selfLink),
				mParamCache_Offset(paramCache_Offset),
				mModDestParam2ID(modDestParam2ID)
			{
			}

			virtual ~AuxNode()
			{
				delete mpCurrentEffect;
			}

			// nullptr is a valid return val for safe null effect.
			IAuxEffect* CreateEffect() const
			{
				// ASSERT: not linked.
				switch (mEffectType.GetEnumValue())
				{
				case AuxEffectType::None:
					break;
				case AuxEffectType::BigFilter:
					return new FilterAuxNode(mParamCache_Offset, mModDestParam2ID);
				}
				return nullptr;
			}

			bool IsLinkedExternally() const
			{
				return (mLinkToSelf != mLink.GetEnumValue());
			}

			// pass in aux nodes due to linking.
			// for simplicity to avoid circular refs, disable if you link to a linked aux
			void BeginBlock(int nSamples, AuxNode* allAuxNodes[], float noteHz, ModMatrixNode& modMatrix) {
				if (!mEnabledParam.GetBoolValue()) return;
				auto link = mLink.GetEnumValue();
				AuxNode* srcNode = this;
				if (IsLinkedExternally())
				{
					srcNode = allAuxNodes[(int)link];
					if (srcNode->IsLinkedExternally())
					{
						return; // no chaining; needlessly vexing
					}
				}
				auto effectType = srcNode->mEffectType.GetEnumValue();

				// ensure correct engine set up.
				if (mCurrentEffectLink != link || mCurrentEffectType != effectType) {
					delete mpCurrentEffect;
					mpCurrentEffect = srcNode->CreateEffect();
					mCurrentEffectLink = link;
					mCurrentEffectType = effectType;
				}
				if (!mpCurrentEffect) return;

				return mpCurrentEffect->AuxBeginBlock(noteHz, nSamples, modMatrix);
			}

			float ProcessSample(float inp) {
				if (!mEnabledParam.GetBoolValue()) return inp;
				if (!mpCurrentEffect) return inp;
				return mpCurrentEffect->AuxProcessSample(inp);
			}
		};


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
			static constexpr size_t gAuxNodeCount = 4;
			static constexpr real_t gMasterVolumeMaxDb = 6;

			static constexpr real_t gLFOLPCenterFrequency = 20.0f;
			static constexpr real_t gLFOLPFrequencyScale = 7.0f;

			// BASE PARAMS & state
			float mAlways0 = 0;
			real_t mPitchBendN11 = 0;
			bool mOscEnabled[gOscillatorCount] = { false, false, false };
			ModSource mOscAmpEnvSources[gOscillatorCount];
			bool mDeviceModSourceValuesSetThisFrame = false;
			float mLFO1LPCutoff = 0.0f;
			float mLFO2LPCutoff = 0.0f;

			float mOscDetuneAmts[gOscillatorCount] = { 0 };
			float mUnisonoDetuneAmts[gUnisonoVoiceMax] = { 0 };

			float mAuxOutputGains[2] = {0}; // precalc'd L/R gains based on aux panning/width [outputchannel]

			float mOscAuxPanAmts[gOscillatorCount] = { 0 };
			float mUnisonoPanAmts[gUnisonoVoiceMax] { 0 };

			float mOscShapeAmts[gOscillatorCount] = { 0 };
			float mUnisonoShapeAmts[gUnisonoVoiceMax] = { 0 };

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

			FrequencyParam mLFO1FilterLP { mParamCache[(int)ParamIndices::LFO1Sharpness], mAlways0, gLFOLPCenterFrequency, gLFOLPFrequencyScale };
			FrequencyParam mLFO2FilterLP { mParamCache[(int)ParamIndices::LFO2Sharpness], mAlways0, gLFOLPCenterFrequency, gLFOLPFrequencyScale };

			IntParam mPitchBendRange{ mParamCache[(int)ParamIndices::PitchBendRange], -gPitchBendMaxRange, gPitchBendMaxRange };

			EnumParam<AuxRoute> mAuxRoutingParam{ mParamCache[(int)ParamIndices::AuxRouting], AuxRoute::Count, AuxRoute::TwoTwo };
			FloatN11Param mAuxWidth{ mParamCache[(int)ParamIndices::AuxWidth], 1 };

			ModulationSpec mModulations[gModulationCount] {
				{ mParamCache, (int)ParamIndices::Mod1Enabled, ModulationSpecType::General },
				{ mParamCache, (int)ParamIndices::Mod2Enabled, ModulationSpecType::General },
				{ mParamCache, (int)ParamIndices::Mod3Enabled, ModulationSpecType::General },
				{ mParamCache, (int)ParamIndices::Mod4Enabled, ModulationSpecType::General },
				{ mParamCache, (int)ParamIndices::Mod5Enabled, ModulationSpecType::General },
				{ mParamCache, (int)ParamIndices::Mod6Enabled, ModulationSpecType::OscAmp },
				{ mParamCache, (int)ParamIndices::Mod7Enabled, ModulationSpecType::OscAmp },
				{ mParamCache, (int)ParamIndices::Mod8Enabled, ModulationSpecType::OscAmp },
			};

			ModulationSpec* mOscPreFMAmpModulations[gOscillatorCount] = {
				&mModulations[gModulationCount - 3],
				&mModulations[gModulationCount - 2],
				&mModulations[gModulationCount - 1],
			};

			real_t mParamCache[(int)ParamIndices::NumParams] = {0};

			// these are not REALLY lfos, they are used to track phase at the device level, for when an LFO's phase should be synced between all voices.
			// that would happen only when all of the following are satisfied:
			// 1. the LFO is not triggered by notes.
			// 2. the LFO has no modulations on its phase or frequency
			ModMatrixNode mNullModMatrix;
			OscillatorNode mModLFO1Phase{ OscillatorIntentionLFO{}, mNullModMatrix, ModDestination::LFO1Waveshape, mParamCache, (int)ParamIndices::LFO1Waveform };
			OscillatorNode mModLFO2Phase{ OscillatorIntentionLFO{}, mNullModMatrix, ModDestination::LFO2Waveshape, mParamCache, (int)ParamIndices::LFO2Waveform };

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

			float mAuxWidthMod = 0;
			float mOsc1AuxMixMod = 0;
			float mOsc2AuxMixMod = 0;
			float mOsc3AuxMixMod = 0;

			Maj7() :
				Maj7SynthDevice((int)ParamIndices::NumParams)
			{
				for (size_t i = 0; i < std::size(mVoices); ++ i) {
					mVoices[i] = mMaj7Voice[i] = new Maj7Voice(this);
				}

				mLFO1FilterLP.mValue.SetParamValue(0.5f);
				mLFO2FilterLP.mValue.SetParamValue(0.5f);

				mFMBrightness.SetParamValue(0.5f);

				mParamCache[(int)ParamIndices::Osc1Volume] = 1;
				mParamCache[(int)ParamIndices::Osc2Volume] = 1;
				mParamCache[(int)ParamIndices::Osc3Volume] = 1;

				// make by default aux nodes link mostly to self; set up filters as default
				AuxNode a1{ AuxLink::Aux1, mParamCache + (int)ParamIndices::Aux1Enabled, 0 };
				a1.mLink.SetEnumValue(a1.mLinkToSelf);
				AuxNode a2{ AuxLink::Aux2, mParamCache + (int)ParamIndices::Aux2Enabled, 0 };
				a2.mLink.SetEnumValue(a2.mLinkToSelf);
				a2.mEffectType.SetEnumValue(AuxEffectType::BigFilter);
				a2.mEnabledParam.SetBoolValue(true);
				AuxNode a3{ AuxLink::Aux3, mParamCache + (int)ParamIndices::Aux3Enabled, 0 };
				a3.mLink.SetEnumValue(a3.mLinkToSelf);
				AuxNode a4{ AuxLink::Aux4, mParamCache + (int)ParamIndices::Aux4Enabled, 0 };
				a4.mLink.SetEnumValue(AuxLink::Aux2);
				a4.mEnabledParam.SetBoolValue(true);
				//a4.mEffectType.SetEnumValue(AuxEffectType::BigFilter);

				FilterAuxNode f2{ mParamCache + (int)ParamIndices::Aux2Enabled, 0 };
				f2.mFilterFreqParam.mValue.SetParamValue(0.4f);
				f2.mFilterFreqParam.mKTValue.SetParamValue(1.0f);
				f2.mFilterQParam.SetParamValue(0.15f);
				f2.mFilterSaturationParam.SetParamValue(0.15f);
				//FilterAuxNode f4{ mParamCache + (int)ParamIndices::Aux2Enabled };

				mParamCache[(int)ParamIndices::AuxWidth] = 1.0f;
				mParamCache[(int)ParamIndices::Osc1AuxMix] = 0.5f;
				mParamCache[(int)ParamIndices::Osc2AuxMix] = 0.5f;
				mParamCache[(int)ParamIndices::Osc3AuxMix] = 0.5f;

				mOscPreFMAmpModulations[0]->mEnabled.SetBoolValue(true);
				mOscPreFMAmpModulations[0]->mSource.SetEnumValue(ModSource::Osc1AmpEnv);
				mOscPreFMAmpModulations[0]->mDestination.SetEnumValue(ModDestination::Osc1PreFMVolume);
				mOscPreFMAmpModulations[0]->mScale.SetN11Value(1);

				mOscPreFMAmpModulations[1]->mEnabled.SetBoolValue(true);
				mOscPreFMAmpModulations[1]->mSource.SetEnumValue(ModSource::Osc2AmpEnv);
				mOscPreFMAmpModulations[1]->mDestination.SetEnumValue(ModDestination::Osc2PreFMVolume);
				mOscPreFMAmpModulations[1]->mScale.SetN11Value(1);

				mOscPreFMAmpModulations[2]->mEnabled.SetBoolValue(true);
				mOscPreFMAmpModulations[2]->mSource.SetEnumValue(ModSource::Osc3AmpEnv);
				mOscPreFMAmpModulations[2]->mDestination.SetEnumValue(ModDestination::Osc3PreFMVolume);
				mOscPreFMAmpModulations[2]->mScale.SetN11Value(1);

				mPitchBendRange.SetIntValue(2);

				for (auto& m : mModulations)
				{
					m.mCurve.SetN11Value(0);
					m.mScale.SetN11Value(1);
					m.mAuxCurve.SetN11Value(0);
					m.mAuxAttenuation.SetParamValue(1);
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

				mLFO1LPCutoff = mLFO1FilterLP.GetFrequency(0, 0);
				mLFO2LPCutoff = mLFO2FilterLP.GetFrequency(0, 0);

				// scale down per param & mod
				float unisonoDetuneAmt = mUnisonoDetune.Get01Value(mUnisonoDetuneMod);
				float unisonoStereoSpreadAmt = mUnisonoStereoSpread.Get01Value(mUnisonoStereoSpreadMod);
				float unisonoShapeSpreadAmt = mUnisonoShapeSpread.Get01Value(mUnisonoShapeSpreadMod);
				for (size_t i = 0; i < mVoicesUnisono; ++i) {
					mUnisonoPanAmts[i] = mUnisonoDetuneAmts[i] * unisonoStereoSpreadAmt;
					mUnisonoShapeAmts[i] = mUnisonoDetuneAmts[i] * unisonoShapeSpreadAmt;
					mUnisonoDetuneAmts[i] *= unisonoDetuneAmt;
				}

				// when aux width is 0, they are both mono. it means pan of 0 for both aux routes.
				// when aux width is +1, auxroute 0 becomes -1 and auxroute 1 becomes +1, fully separating the channels
				auto auxGains = PanToFactor(mAuxWidth.GetN11Value(mAuxWidthMod));
				mAuxOutputGains[1] = auxGains.first;// auxPanVolParam.GetLinearGain();
				mAuxOutputGains[0] = auxGains.second;// auxPanVolParam.GetLinearGain();

				float oscDetuneAmt = mOscillatorDetune.Get01Value(mOscillatorDetuneMod);
				float oscStereoSpreadAmt = mOscillatorStereoSpread.Get01Value(mOscillatorStereoSpreadMod);
				float oscShapeSpreadAmt = mOscillatorShapeSpread.Get01Value(mOscillatorShapeSpreadMod);

				ParamIndices auxPanParams[gOscillatorCount] = {
					ParamIndices::Osc1AuxMix,
					ParamIndices::Osc2AuxMix,
					ParamIndices::Osc3AuxMix,
				};

				float auxPanMods[gOscillatorCount] = {
					mOsc1AuxMixMod,
					mOsc2AuxMixMod,
					mOsc3AuxMixMod,
				};

				for (size_t i = 0; i < gOscillatorCount; ++ i) {
					// for the moment mOscDetuneAmts[i] is just a generic spread value.
					mOscShapeAmts[i] = mOscDetuneAmts[i] * oscShapeSpreadAmt;
					mOscAuxPanAmts[i] = FloatN11Param{mParamCache[(int)auxPanParams[i]]}.GetN11Value(auxPanMods[i]) + mOscDetuneAmts[i] * oscStereoSpreadAmt;
					mOscDetuneAmts[i] *= oscDetuneAmt;
				}

				for (auto* v : mMaj7Voice)
				{
					v->ProcessAndMix(songPosition, outputs, numSamples);
				}

				// apply post FX.
				real_t masterGain = mMasterVolume.GetLinearGain();
				mModLFO1Phase.BeginBlock(0, 0, 1, 0, numSamples);
				mModLFO2Phase.BeginBlock(0, 0, 1, 0, numSamples);
				for (size_t iSample = 0; iSample < numSamples; ++iSample)
				{
					outputs[0][iSample] *= masterGain;
					outputs[1][iSample] *= masterGain;

					mModLFO1Phase.ProcessSample(iSample, 0, 0, 0, 0, true);
					mModLFO2Phase.ProcessSample(iSample, 0, 0, 0, 0, true);
				}
			}

			struct Maj7Voice : public Maj7SynthDevice::Voice
			{
				Maj7Voice(Maj7* owner) :
					mpOwner(owner),
					mPortamento(owner->mParamCache[(int)ParamIndices::PortamentoTime], owner->mParamCache[(int)ParamIndices::PortamentoCurve]),
					mOsc1AmpEnv(mModMatrix, ModDestination::Osc1AmpEnvDelayTime, owner->mParamCache, (int)ParamIndices::Osc1AmpEnvDelayTime),
					mOsc2AmpEnv(mModMatrix, ModDestination::Osc2AmpEnvDelayTime, owner->mParamCache, (int)ParamIndices::Osc2AmpEnvDelayTime),
					mOsc3AmpEnv(mModMatrix, ModDestination::Osc3AmpEnvDelayTime, owner->mParamCache, (int)ParamIndices::Osc3AmpEnvDelayTime),
					mModEnv1(mModMatrix, ModDestination::Env1DelayTime, owner->mParamCache, (int)ParamIndices::Env1DelayTime),
					mModEnv2(mModMatrix, ModDestination::Env2DelayTime, owner->mParamCache, (int)ParamIndices::Env2DelayTime),
					mModLFO1(OscillatorIntentionLFO{}, mModMatrix, ModDestination::LFO1Waveshape, owner->mParamCache, (int)ParamIndices::LFO1Waveform),
					mModLFO2(OscillatorIntentionLFO{}, mModMatrix, ModDestination::LFO2Waveshape, owner->mParamCache, (int)ParamIndices::LFO2Waveform),
					mOscillator1(OscillatorIntentionAudio{}, mModMatrix, ModDestination::Osc1Volume, owner->mParamCache, (int)ParamIndices::Osc1Enabled),
					mOscillator2(OscillatorIntentionAudio{}, mModMatrix, ModDestination::Osc2Volume, owner->mParamCache, (int)ParamIndices::Osc2Enabled),
					mOscillator3(OscillatorIntentionAudio{}, mModMatrix, ModDestination::Osc3Volume, owner->mParamCache, (int)ParamIndices::Osc3Enabled),
					mAux1(AuxLink::Aux1, owner->mParamCache + (int)ParamIndices::Aux1Enabled, (int)ModDestination::Aux1Param2),
					mAux2(AuxLink::Aux2, owner->mParamCache + (int)ParamIndices::Aux2Enabled, (int)ModDestination::Aux2Param2),
					mAux3(AuxLink::Aux3, owner->mParamCache + (int)ParamIndices::Aux3Enabled, (int)ModDestination::Aux3Param2),
					mAux4(AuxLink::Aux4, owner->mParamCache + (int)ParamIndices::Aux4Enabled, (int)ModDestination::Aux4Param2)
				{
				}

				Maj7* mpOwner;

				real_t mVelocity01 = 0;
				real_t mTriggerRandom01 = 0;
				PortamentoCalc mPortamento;

				EnvelopeNode mModEnv1;
				EnvelopeNode mModEnv2;

				OscillatorNode mModLFO1;
				OscillatorNode mModLFO2;
				FilterNode mLFOFilter1;
				FilterNode mLFOFilter2;

				ModMatrixNode mModMatrix;

				OscillatorNode mOscillator1;
				OscillatorNode mOscillator2;
				OscillatorNode mOscillator3;

				EnvelopeNode mOsc1AmpEnv;
				EnvelopeNode mOsc2AmpEnv;
				EnvelopeNode mOsc3AmpEnv;

				AuxNode mAux1;
				AuxNode mAux2;
				AuxNode mAux3;
				AuxNode mAux4;
				AuxNode* mAllAuxNodes[gAuxNodeCount] = {
					&mAux1,
					&mAux2,
					&mAux3,
					&mAux4,
				};

				void ProcessAndMix(double songPosition, float* const* const outputs, int numSamples)
				{
					if (!this->IsPlaying()) {
						return;
					}

					// at this point run the graph.
					// 1. run signals which produce A-Rate outputs, so we can use those buffers to modulate nodes which have A-Rate destinations
					// 2. mod matrix, which fills buffers with modulation signals. mod sources should be up-to-date
					// 3. run signals which are A-Rate destinations.
					// if we were really ambitious we could make the filter an A-Rate destination as well, with oscillator outputs being A-Rate.
					mOsc1AmpEnv.BeginBlock(); // tells envelopes to recalc state based on mod values and changed settings
					mOsc2AmpEnv.BeginBlock();
					mOsc3AmpEnv.BeginBlock();
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

					if (!mModLFO1.mPhaseRestart.GetBoolValue()) {
						// sync phase with device-level. TODO: also check that modulations aren't creating per-voice variation?
						mModLFO1.SetPhase(mpOwner->mModLFO1Phase.mPhase);
					}

					if (!mModLFO2.mPhaseRestart.GetBoolValue()) {
						// sync phase with device-level
						mModLFO2.SetPhase(mpOwner->mModLFO2Phase.mPhase);
					}

					mModLFO1.BeginBlock(0, 0, 1.0f, 0, numSamples);
					mModLFO2.BeginBlock(0, 0, 1.0f, 0, numSamples);

					mLFOFilter1.SetParams(FilterModel::LP_OnePole, mpOwner->mLFO1LPCutoff, 0, 0);
					mLFOFilter2.SetParams(FilterModel::LP_OnePole, mpOwner->mLFO2LPCutoff, 0, 0);

					// process a-rate mod source buffers.
					for (size_t iSample = 0; iSample < numSamples; ++iSample)
					{
						mModMatrix.SetSourceARateValue(ModSource::Osc1AmpEnv, iSample, mOsc1AmpEnv.ProcessSample(iSample));
						mModMatrix.SetSourceARateValue(ModSource::Osc2AmpEnv, iSample, mOsc2AmpEnv.ProcessSample(iSample));
						mModMatrix.SetSourceARateValue(ModSource::Osc3AmpEnv, iSample, mOsc3AmpEnv.ProcessSample(iSample));
						mModMatrix.SetSourceARateValue(ModSource::ModEnv1, iSample, mModEnv1.ProcessSample(iSample));
						mModMatrix.SetSourceARateValue(ModSource::ModEnv2, iSample, mModEnv2.ProcessSample(iSample));
						float l1 = mModLFO1.ProcessSample(iSample, 0, 0, 0, 0, false);
						l1 = mLFOFilter1.ProcessSample(l1);
						mModMatrix.SetSourceARateValue(ModSource::LFO1, iSample, l1);
						float l2 = mModLFO2.ProcessSample(iSample, 0, 0, 0, 0, false);
						l2 = mLFOFilter2.ProcessSample(l2);
						mModMatrix.SetSourceARateValue(ModSource::LFO2, iSample, l2);
						mModMatrix.ProcessSample(mpOwner->mModulations, iSample);
					}

					// set device-level modded values.
					if (!mpOwner->mDeviceModSourceValuesSetThisFrame) {
						mpOwner->mOscillatorDetuneMod = mModMatrix.GetDestinationValue(ModDestination::OscillatorDetune, 0);
						mpOwner->mUnisonoDetuneMod = mModMatrix.GetDestinationValue(ModDestination::UnisonoDetune, 0);
						mpOwner->mOscillatorStereoSpreadMod = mModMatrix.GetDestinationValue(ModDestination::OscillatorStereoSpread, 0);
						mpOwner->mAuxWidthMod = mModMatrix.GetDestinationValue(ModDestination::AuxWidth, 0);
						mpOwner->mOsc1AuxMixMod = mModMatrix.GetDestinationValue(ModDestination::Osc1AuxMix, 0);
						mpOwner->mOsc2AuxMixMod = mModMatrix.GetDestinationValue(ModDestination::Osc2AuxMix, 0);
						mpOwner->mOsc3AuxMixMod = mModMatrix.GetDestinationValue(ModDestination::Osc3AuxMix, 0);
						mpOwner->mUnisonoStereoSpreadMod = mModMatrix.GetDestinationValue(ModDestination::UnisonoStereoSpread, 0);
						mpOwner->mOscillatorShapeSpreadMod = mModMatrix.GetDestinationValue(ModDestination::OscillatorShapeSpread, 0);
						mpOwner->mUnisonoShapeSpreadMod = mModMatrix.GetDestinationValue(ModDestination::UnisonoShapeSpread, 0);
						mpOwner->mFMBrightnessMod = mModMatrix.GetDestinationValue(ModDestination::FMBrightness, 0);
						mpOwner->mPortamentoTimeMod = mModMatrix.GetDestinationValue(ModDestination::PortamentoTime, 0);
						mpOwner->mPortamentoCurveMod = mModMatrix.GetDestinationValue(ModDestination::PortamentoCurve, 0);
						mpOwner->mDeviceModSourceValuesSetThisFrame = true;
					}

					float globalFMScale = mpOwner->mFMBrightness.Get01Value(mpOwner->mFMBrightnessMod) * 2;
					float FMScales[] = {
						mpOwner->mFMAmt2to1.Get01Value(mModMatrix.GetDestinationValue(ModDestination::FMAmt2to1, 0)) * globalFMScale,
						mpOwner->mFMAmt3to1.Get01Value(mModMatrix.GetDestinationValue(ModDestination::FMAmt3to1, 0)) * globalFMScale,
						mpOwner->mFMAmt1to2.Get01Value(mModMatrix.GetDestinationValue(ModDestination::FMAmt1to2, 0)) * globalFMScale,
						mpOwner->mFMAmt3to2.Get01Value(mModMatrix.GetDestinationValue(ModDestination::FMAmt3to2, 0)) * globalFMScale,
						mpOwner->mFMAmt1to3.Get01Value(mModMatrix.GetDestinationValue(ModDestination::FMAmt1to3, 0)) * globalFMScale,
						mpOwner->mFMAmt2to3.Get01Value(mModMatrix.GetDestinationValue(ModDestination::FMAmt2to3, 0)) * globalFMScale,
					};

					float myUnisonoDetune = mpOwner->mUnisonoDetuneAmts[this->mUnisonVoice];
					float myUnisonoPan = mpOwner->mUnisonoPanAmts[this->mUnisonVoice];
					float myUnisonoShape = mpOwner->mUnisonoShapeAmts[this->mUnisonVoice];

					struct OscInfo
					{
						OscillatorNode& mNode;
						ModSource mAmpEnvSource;
						ModDestination mModDestVolume;
						ModDestination mModDestVolumePreFM;
						ParamIndices mOutputVolumeParamID;

						// calculated
						float mOutputGain[2] = { 0 }; // linear output volume gain calculated from output VolumeParam + panning
						float mAmpEnvGain = { 0 }; // linear gain calculated frequently from osc ampenv
					};
					OscInfo oscInfo[gOscillatorCount] = {
						{ mOscillator1, mpOwner->mOscAmpEnvSources[0], ModDestination::Osc1Volume, ModDestination::Osc1PreFMVolume, ParamIndices::Osc1Volume },
						{ mOscillator2, mpOwner->mOscAmpEnvSources[1], ModDestination::Osc2Volume, ModDestination::Osc2PreFMVolume, ParamIndices::Osc2Volume },
						{ mOscillator3, mpOwner->mOscAmpEnvSources[2], ModDestination::Osc3Volume, ModDestination::Osc3PreFMVolume, ParamIndices::Osc3Volume },
					};

					// begin oscillator blocks
					// and precalc some panning
					for (size_t i = 0; i < gOscillatorCount; ++i)
					{
						if (!mpOwner->mOscEnabled[i]) {
							continue;
						}
						auto& info = oscInfo[i];
						float semis = myUnisonoDetune + mpOwner->mOscDetuneAmts[i];
						info.mNode.BeginBlock(midiNote, myUnisonoShape + mpOwner->mOscShapeAmts[i], SemisToFrequencyMul(semis), globalFMScale, numSamples);

						float volumeMod = mModMatrix.GetDestinationValue(info.mModDestVolume, 0);
						VolumeParam outputVolParam{ mpOwner->mParamCache[(int)info.mOutputVolumeParamID], OscillatorNode::gVolumeMaxDb };

						// treat panning as added to modulation value
						float panParam = myUnisonoPan + mpOwner->mOscAuxPanAmts[i]; // -1 would mean full Left, 1 is full Right.
						auto panGains = PanToFactor(panParam);
						float outputVolLin = outputVolParam.GetLinearGain(0);
						info.mOutputGain[0] = outputVolLin * panGains.first;
						info.mOutputGain[1] = outputVolLin * panGains.second;
					}

					auto auxRouting = mpOwner->mAuxRoutingParam.GetEnumValue();
					for (size_t i = 0; i < gAuxNodeCount; ++i) {
						mAllAuxNodes[i]->BeginBlock(numSamples, mAllAuxNodes, noteHz, mModMatrix);
					}

					for (int iSample = 0; iSample < numSamples; ++iSample)
					{
						for (size_t i = 0; i < gOscillatorCount; ++i)
						{
							if (!mpOwner->mOscEnabled[i]) {
								continue;
							}
							auto& info = oscInfo[i];
							float hiddenVolumeBacking = mModMatrix.GetDestinationValue(info.mModDestVolumePreFM, iSample);
							VolumeParam hiddenAmpParam{ hiddenVolumeBacking, 0 };
							info.mAmpEnvGain = hiddenAmpParam.GetLinearGain(0);
						}

						float osc2LastSample = mOscillator2.GetSample() * oscInfo[1].mAmpEnvGain;
						float osc3LastSample = mOscillator3.GetSample() * oscInfo[2].mAmpEnvGain;

						real_t s1 = mOscillator1.ProcessSample(iSample,
							osc2LastSample, FMScales[0],
							osc3LastSample, FMScales[1], false
						);
						s1 *= oscInfo[0].mAmpEnvGain;

						real_t s2 = mOscillator2.ProcessSample(iSample,
							s1, FMScales[2],
							osc3LastSample, FMScales[3], false
						);
						s2 *= oscInfo[1].mAmpEnvGain;

						real_t s3 = mOscillator3.ProcessSample(iSample,
							s1, FMScales[4],
							s2, FMScales[5], false
						);
						s3 *= oscInfo[2].mAmpEnvGain;

						real_t sl = s1 * oscInfo[0].mOutputGain[0] + s2 * oscInfo[1].mOutputGain[0] + s3 * oscInfo[2].mOutputGain[0];
						real_t sr = s1 * oscInfo[0].mOutputGain[1] + s2 * oscInfo[1].mOutputGain[1] + s3 * oscInfo[2].mOutputGain[1];

						// send through aux
						switch (auxRouting) {
						case AuxRoute::FourZero:
							// L = aux1 -> aux2 -> aux3 -> aux4 -> *
							// R = *
							sl = mAux1.ProcessSample(sl);
							sl = mAux2.ProcessSample(sl);
							sl = mAux3.ProcessSample(sl);
							sl = mAux4.ProcessSample(sl);
							break;
						case AuxRoute::ThreeOne:
							// L = aux1 -> aux2 -> aux3 -> *
							// R = aux4 -> *
							sl = mAux1.ProcessSample(sl);
							sl = mAux2.ProcessSample(sl);
							sl = mAux3.ProcessSample(sl);
							sr = mAux4.ProcessSample(sr);
							break;
						case AuxRoute::TwoTwo:
							// L = aux1 -> aux2 -> *
							// R = aux3 -> aux4 -> *
							sl = mAux1.ProcessSample(sl);
							sl = mAux2.ProcessSample(sl);
							sr = mAux3.ProcessSample(sr);
							sr = mAux4.ProcessSample(sr);
							break;
						}

						outputs[0][iSample] += sl * mpOwner->mAuxOutputGains[0] + sr * mpOwner->mAuxOutputGains[1];
						outputs[1][iSample] += sl * mpOwner->mAuxOutputGains[1] + sr * mpOwner->mAuxOutputGains[0];
					}

					mPortamento.Advance(numSamples,
						mModMatrix.GetDestinationValue(ModDestination::PortamentoTime, 0),
						mModMatrix.GetDestinationValue(ModDestination::PortamentoCurve, 0)
					);
				}

				virtual void NoteOn() override {
					mVelocity01 = mNoteInfo.Velocity / 127.0f;
					mTriggerRandom01 = math::rand01();
					mOsc1AmpEnv.noteOn(mLegato);
					mOsc2AmpEnv.noteOn(mLegato);
					mOsc3AmpEnv.noteOn(mLegato);
					mModEnv1.noteOn(mLegato);
					mModEnv2.noteOn(mLegato);
					mModLFO1.NoteOn(mLegato);
					mModLFO2.NoteOn(mLegato);
					mOscillator1.NoteOn(mLegato);
					mOscillator2.NoteOn(mLegato);
					mOscillator3.NoteOn(mLegato);
					mPortamento.NoteOn((float)mNoteInfo.MidiNoteValue, !mLegato);
				}

				virtual void NoteOff() override {
					mOsc1AmpEnv.noteOff();
					mOsc2AmpEnv.noteOff();
					mOsc3AmpEnv.noteOff();
					mModEnv1.noteOff();
					mModEnv2.noteOff();
				}

				virtual void Kill() override {
					mOsc1AmpEnv.kill();
					mOsc2AmpEnv.kill();
					mOsc3AmpEnv.kill();
					mModEnv1.kill();
					mModEnv2.kill();
				}

				const EnvelopeNode* GetEnvelopeForModulation(ModulationSpec* m) const
				{
					switch (m->mSource.GetEnumValue())
					{
					case ModSource::Osc1AmpEnv:
						return &mOsc1AmpEnv;
					case ModSource::Osc2AmpEnv:
						return &mOsc2AmpEnv;
					case ModSource::Osc3AmpEnv:
						return &mOsc3AmpEnv;
					case ModSource::ModEnv1:
						return &mModEnv1;
					case ModSource::ModEnv2:
						return &mModEnv2;
					}
					return nullptr;
				}

				bool OscillatorIsPlaying(int iOsc, ParamIndices enabledParam, ModulationSpec* preFMAmpMod) const {
					bool enabled = BoolParam{ mpOwner->mParamCache[(int)enabledParam] }.GetBoolValue();
					if (!enabled) return false;

					auto penv = GetEnvelopeForModulation(preFMAmpMod);
					if (!penv) return false;
					//int ampEnvSrcIndex = IntParam{ mpOwner->mParamCache[(int)ampSrcParam], 0, gOscillatorCount - 1 }.GetIntValue();
					//const EnvelopeNode* ampEnvs[gOscillatorCount] = {
					//	&mOsc1AmpEnv,
					//	&mOsc2AmpEnv,
					//	&mOsc3AmpEnv,
					//};
					//auto ampEnvSrc = ampEnvs[ampEnvSrcIndex];
					return penv->IsPlaying();
				}

				virtual bool IsPlaying() override {
					return
						OscillatorIsPlaying(0, ParamIndices::Osc1Enabled, mpOwner->mOscPreFMAmpModulations[0])
						|| OscillatorIsPlaying(1, ParamIndices::Osc2Enabled, mpOwner->mOscPreFMAmpModulations[1])
						|| OscillatorIsPlaying(2, ParamIndices::Osc3Enabled, mpOwner->mOscPreFMAmpModulations[2]);
				}
			};


			struct Maj7Voice* mMaj7Voice[maxVoices] = { 0 };

		};
	} // namespace M7

	using Maj7 = M7::Maj7;

} // namespace WaveSabreCore




