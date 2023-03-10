// Time sync for LFOs? it's more complex than i expected, because of the difference between live play & playback.

// speed optimizations
// (use profiler)
// - some things may be calculated at the synth level, not voice level. think portamento time/curve params?
// - do not process LFO & ENV when not in use
// - recalc some a-rate things less frequently / reduce some things to k-rate. frequencies are calculated every sample...
// - cache some values like mod matrix arate source buffers
// - LUTs: curves (bilinear), sin, cos, tanh
// - mod matrix currently processes every mod spec each sample. K-Rate should be done per buffer init.
//   for example a K-rate to K-rate (macro to filter freq) does not need to process every sample.
// 
// size-optimizations:
// (profile w sizebench!)
// - dont inline (use cpp)
// - don't put GetChunk() in size-optimized code. it's not used in WS runtime.
// - use fixed-point in oscillators. there's absolutely no reason to be using chonky floating point instructions there.

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

#include <type_traits>

#include <WaveSabreCore/Helpers.h>
#include <WaveSabreCore/Maj7SynthDevice.hpp>
#include <WaveSabreCore/Maj7Basic.hpp>
#include <WaveSabreCore/Maj7Envelope.hpp>
#include <WaveSabreCore/Maj7Filter.hpp>
#include <WaveSabreCore/Maj7Distortion.hpp>
#include <WaveSabreCore/Maj7Bitcrush.hpp>
#include <WaveSabreCore/Maj7ModMatrix.hpp>

#include <WaveSabreCore/Maj7Oscillator.hpp>
#include <WaveSabreCore/Maj7Sampler.hpp>
#include <WaveSabreCore/Maj7GmDls.hpp>

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
			float* const mParamCache_Offset;
			const int mModDestParam2ID;

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

			void LoadDefaults()
			{
				//mEnabledParam(paramCache_Offset[(int)AuxParamIndexOffsets::Enabled]),
				mLink.SetEnumValue(mLinkToSelf);// (paramCache_Offset[(int)AuxParamIndexOffsets::Link], AuxLink::Count),
				mEffectType.SetEnumValue(AuxEffectType::None);// (paramCache_Offset[(int)AuxParamIndexOffsets::Type], AuxEffectType::Count),
				delete mpCurrentEffect;
				mpCurrentEffect = nullptr;
				mCurrentEffectType = AuxEffectType::None;
			}

			virtual ~AuxNode()
			{
				delete mpCurrentEffect;
			}

			// nullptr is a valid return val for safe null effect.
			IAuxEffect* CreateEffect(AuxNode* allAuxNodes[]) const
			{
				if (!IsEnabled(allAuxNodes)) {
					return nullptr;
				}
				// ASSERT: not linked.
				switch (mEffectType.GetEnumValue())
				{
				case AuxEffectType::None:
					break;
				case AuxEffectType::BigFilter:
					return new FilterAuxNode(mParamCache_Offset, mModDestParam2ID);
				case AuxEffectType::Distortion:
					return new DistortionAuxNode(mParamCache_Offset, mModDestParam2ID);
				case AuxEffectType::Bitcrush:
					return new BitcrushAuxNode(mParamCache_Offset, mModDestParam2ID);
				}
				return nullptr;
			}

			bool IsLinkedExternally() const
			{
				return (mLinkToSelf != mLink.GetEnumValue());
			}

			bool IsEnabled(AuxNode* allAuxNodes[]) const
			{
				auto link = mLink.GetEnumValue();
				const AuxNode* srcNode = this;
				if (IsLinkedExternally())
				{
					if (!allAuxNodes) return false;
					srcNode = allAuxNodes[(int)link];
					if (srcNode->IsLinkedExternally())
					{
						return false; // no chaining; needlessly vexing
					}
				}
				return srcNode->mEnabledParam.GetBoolValue();
			}

			// pass in aux nodes due to linking.
			// for simplicity to avoid circular refs, disable if you link to a linked aux
			void BeginBlock(int nSamples, AuxNode* allAuxNodes[], float noteHz, ModMatrixNode& modMatrix) {
				if (!IsEnabled(allAuxNodes)) return;
				auto link = mLink.GetEnumValue();
				AuxNode* srcNode = this;
				srcNode = allAuxNodes[(int)link];
				auto effectType = srcNode->mEffectType.GetEnumValue();

				// ensure correct engine set up.
				if (mCurrentEffectLink != link || mCurrentEffectType != effectType) {
					delete mpCurrentEffect;
					mpCurrentEffect = srcNode->CreateEffect(allAuxNodes);
					mCurrentEffectLink = link;
					mCurrentEffectType = effectType;
				}
				if (!mpCurrentEffect) return;

				return mpCurrentEffect->AuxBeginBlock(noteHz, nSamples, modMatrix);
			}

			float ProcessSample(float inp, AuxNode* allAuxNodes[]) {
				if (!IsEnabled(allAuxNodes)) return inp;
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
			static constexpr size_t gModulationCount = 16;
			static constexpr int gPitchBendMaxRange = 24;
			static constexpr int gUnisonoVoiceMax = 12;
			static constexpr size_t gOscillatorCount = 4;
			static constexpr size_t gSamplerCount = 4;
			static constexpr size_t gSourceCount = gOscillatorCount + gSamplerCount;
			static constexpr size_t gAuxNodeCount = 4;
			static constexpr real_t gMasterVolumeMaxDb = 6;

			static constexpr real_t gLFOLPCenterFrequency = 20.0f;
			static constexpr real_t gLFOLPFrequencyScale = 7.0f;

			// BASE PARAMS & state
			float mAlways0 = 0;
			real_t mPitchBendN11 = 0;
			bool mDeviceModSourceValuesSetThisFrame = false;
			float mLFO1LPCutoff = 0.0f;
			float mLFO2LPCutoff = 0.0f;

			float mUnisonoDetuneAmts[gUnisonoVoiceMax] = { 0 };

			float mAuxOutputGains[2] = {0}; // precalc'd L/R gains based on aux panning/width [outputchannel]

			float mUnisonoPanAmts[gUnisonoVoiceMax] { 0 };

			float mUnisonoShapeAmts[gUnisonoVoiceMax] = { 0 };

			VolumeParam mMasterVolume{ mParamCache[(int)ParamIndices::MasterVolume], gMasterVolumeMaxDb };
			Float01Param mMacro1{ mParamCache[(int)ParamIndices::Macro1] };
			Float01Param mMacro2{ mParamCache[(int)ParamIndices::Macro2] };
			Float01Param mMacro3{ mParamCache[(int)ParamIndices::Macro3] };
			Float01Param mMacro4{ mParamCache[(int)ParamIndices::Macro4] };
			Float01Param mMacro5{ mParamCache[(int)ParamIndices::Macro5] };
			Float01Param mMacro6{ mParamCache[(int)ParamIndices::Macro6] };
			Float01Param mMacro7{ mParamCache[(int)ParamIndices::Macro7] };

			Float01Param mFMAmt1to2{ mParamCache[(int)ParamIndices::FMAmt1to2] };
			Float01Param mFMAmt1to3{ mParamCache[(int)ParamIndices::FMAmt1to3] };
			Float01Param mFMAmt1to4{ mParamCache[(int)ParamIndices::FMAmt1to4] };
			Float01Param mFMAmt2to1{ mParamCache[(int)ParamIndices::FMAmt2to1] };
			Float01Param mFMAmt2to3{ mParamCache[(int)ParamIndices::FMAmt2to3] };
			Float01Param mFMAmt2to4{ mParamCache[(int)ParamIndices::FMAmt2to4] };
			Float01Param mFMAmt3to1{ mParamCache[(int)ParamIndices::FMAmt3to1] };
			Float01Param mFMAmt3to2{ mParamCache[(int)ParamIndices::FMAmt3to2] };
			Float01Param mFMAmt3to4{ mParamCache[(int)ParamIndices::FMAmt3to4] };
			Float01Param mFMAmt4to1{ mParamCache[(int)ParamIndices::FMAmt4to1] };
			Float01Param mFMAmt4to2{ mParamCache[(int)ParamIndices::FMAmt4to2] };
			Float01Param mFMAmt4to3{ mParamCache[(int)ParamIndices::FMAmt4to3] };

			Float01Param mUnisonoDetune{ mParamCache[(int)ParamIndices::UnisonoDetune] };
			Float01Param mUnisonoStereoSpread{ mParamCache[(int)ParamIndices::UnisonoStereoSpread] };
			Float01Param mOscillatorDetune{ mParamCache[(int)ParamIndices::OscillatorDetune] };
			Float01Param mOscillatorStereoSpread{ mParamCache[(int)ParamIndices::OscillatorSpread] };

			Float01Param mUnisonoShapeSpread{ mParamCache[(int)ParamIndices::UnisonoShapeSpread] };
			Float01Param mOscillatorShapeSpread{ mParamCache[(int)ParamIndices::OscillatorShapeSpread] };

			Float01Param mFMBrightness{ mParamCache[(int)ParamIndices::FMBrightness] };

			FrequencyParam mLFO1FilterLP { mParamCache[(int)ParamIndices::LFO1Sharpness], mAlways0, gLFOLPCenterFrequency, gLFOLPFrequencyScale };
			FrequencyParam mLFO2FilterLP { mParamCache[(int)ParamIndices::LFO2Sharpness], mAlways0, gLFOLPCenterFrequency, gLFOLPFrequencyScale };

			IntParam mPitchBendRange{ mParamCache[(int)ParamIndices::PitchBendRange], -gPitchBendMaxRange, gPitchBendMaxRange };

			EnumParam<AuxRoute> mAuxRoutingParam{ mParamCache[(int)ParamIndices::AuxRouting], AuxRoute::Count };
			FloatN11Param mAuxWidth{ mParamCache[(int)ParamIndices::AuxWidth] };

			ModulationSpec mModulations[gModulationCount] {
				{ mParamCache, (int)ParamIndices::Mod1Enabled },
				{ mParamCache, (int)ParamIndices::Mod2Enabled },
				{ mParamCache, (int)ParamIndices::Mod3Enabled },
				{ mParamCache, (int)ParamIndices::Mod4Enabled },
				{ mParamCache, (int)ParamIndices::Mod5Enabled },
				{ mParamCache, (int)ParamIndices::Mod6Enabled },
				{ mParamCache, (int)ParamIndices::Mod7Enabled },
				{ mParamCache, (int)ParamIndices::Mod8Enabled },
				{ mParamCache, (int)ParamIndices::Mod9Enabled },
				{ mParamCache, (int)ParamIndices::Mod10Enabled },
				{ mParamCache, (int)ParamIndices::Mod11Enabled },
				{ mParamCache, (int)ParamIndices::Mod12Enabled },
				{ mParamCache, (int)ParamIndices::Mod13Enabled },
				{ mParamCache, (int)ParamIndices::Mod14Enabled },
				{ mParamCache, (int)ParamIndices::Mod15Enabled },
				{ mParamCache, (int)ParamIndices::Mod16Enabled },
			};

			float mParamCache[(int)ParamIndices::NumParams];
			float mDefaultParamCache[(int)ParamIndices::NumParams];

			// these are not REALLY lfos, they are used to track phase at the device level, for when an LFO's phase should be synced between all voices.
			// that would happen only when all of the following are satisfied:
			// 1. the LFO is not triggered by notes.
			// 2. the LFO has no modulations on its phase or frequency
			ModMatrixNode mNullModMatrix;
			OscillatorDevice mLFO1Device{ OscillatorIntentionLFO{}, mParamCache, ParamIndices::LFO1Waveform, ModDestination::LFO1Waveshape };
			OscillatorDevice mLFO2Device{ OscillatorIntentionLFO{}, mParamCache, ParamIndices::LFO2Waveform, ModDestination::LFO2Waveshape };

			OscillatorNode mModLFO1Phase{ &mLFO1Device, mNullModMatrix, nullptr };
			OscillatorNode mModLFO2Phase{ &mLFO2Device, mNullModMatrix, nullptr };

			OscillatorDevice mOsc1Device{ OscillatorIntentionAudio{}, mParamCache, &mModulations[gModulationCount - 8], ParamIndices::Osc1Enabled, ModSource::Osc1AmpEnv, ModDestination::Osc1Volume };
			OscillatorDevice mOsc2Device{ OscillatorIntentionAudio{}, mParamCache, &mModulations[gModulationCount - 7], ParamIndices::Osc2Enabled, ModSource::Osc2AmpEnv, ModDestination::Osc2Volume };
			OscillatorDevice mOsc3Device{ OscillatorIntentionAudio{}, mParamCache, &mModulations[gModulationCount - 6], ParamIndices::Osc3Enabled, ModSource::Osc3AmpEnv, ModDestination::Osc3Volume };
			OscillatorDevice mOsc4Device{ OscillatorIntentionAudio{}, mParamCache, &mModulations[gModulationCount - 5], ParamIndices::Osc4Enabled, ModSource::Osc4AmpEnv, ModDestination::Osc4Volume };
			SamplerDevice mSampler1Device{ mParamCache, &mModulations[gModulationCount - 4], ParamIndices::Sampler1Enabled, ModSource::Sampler1AmpEnv, ModDestination::Sampler1Volume };
			SamplerDevice mSampler2Device{ mParamCache, &mModulations[gModulationCount - 3], ParamIndices::Sampler2Enabled, ModSource::Sampler2AmpEnv, ModDestination::Sampler2Volume };
			SamplerDevice mSampler3Device{ mParamCache, &mModulations[gModulationCount - 2], ParamIndices::Sampler3Enabled, ModSource::Sampler3AmpEnv, ModDestination::Sampler3Volume };
			SamplerDevice mSampler4Device{ mParamCache, &mModulations[gModulationCount - 1], ParamIndices::Sampler4Enabled, ModSource::Sampler4AmpEnv, ModDestination::Sampler4Volume };

			ISoundSourceDevice* mSources[gSourceCount] = {
				&mOsc1Device,
				&mOsc2Device,
				&mOsc3Device,
				&mOsc4Device,
				&mSampler1Device,
				&mSampler2Device,
				&mSampler3Device,
				&mSampler4Device,
			};

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

			Maj7() :
				Maj7SynthDevice((int)ParamIndices::NumParams)
			{
				for (size_t i = 0; i < std::size(mVoices); ++ i) {
					mVoices[i] = mMaj7Voice[i] = new Maj7Voice(this);
				}

				LoadDefaults();
			}

			void LoadDefaults()
			{
				for (int i = 0; i < numParams; ++i) {
					mParamCache[i] = 0;
				}

				mLFO1Device.LoadDefaults();
				mLFO2Device.LoadDefaults();
				mOsc1Device.LoadDefaults();
				mOsc2Device.LoadDefaults();
				mOsc3Device.LoadDefaults();
				mOsc4Device.LoadDefaults();
				mSampler1Device.LoadDefaults();
				mSampler2Device.LoadDefaults();
				mSampler3Device.LoadDefaults();
				mSampler4Device.LoadDefaults();

				for (auto& m : mModulations) {
					m.LoadDefaults();
				}

				//for (size_t i = 0; i < std::size(mVoices); ++i) {
				//	mMaj7Voice[i]->LoadDefaults();
				//}

				mLFO1FilterLP.mValue.SetParamValue(0.5f);
				mLFO2FilterLP.mValue.SetParamValue(0.5f);

				mFMBrightness.SetParamValue(0.5f);

				// make by default aux nodes link mostly to self; set up filters as default
				AuxNode a1{ AuxLink::Aux1, mParamCache + (int)ParamIndices::Aux1Enabled, 0 };
				AuxNode a2{ AuxLink::Aux2, mParamCache + (int)ParamIndices::Aux2Enabled, 0 };
				AuxNode a3{ AuxLink::Aux3, mParamCache + (int)ParamIndices::Aux3Enabled, 0 };
				AuxNode a4{ AuxLink::Aux4, mParamCache + (int)ParamIndices::Aux4Enabled, 0 };
				a1.LoadDefaults();
				a2.LoadDefaults();
				a3.LoadDefaults();
				a4.LoadDefaults();

				a1.mLink.SetEnumValue(a1.mLinkToSelf);
				a2.mLink.SetEnumValue(a2.mLinkToSelf);
				a2.mEffectType.SetEnumValue(AuxEffectType::BigFilter);
				a2.mEnabledParam.SetBoolValue(true);
				a3.mLink.SetEnumValue(a1.mLinkToSelf);
				a4.mLink.SetEnumValue(AuxLink::Aux2);
				a4.mEnabledParam.SetBoolValue(true);

				FilterAuxNode f2{ mParamCache + (int)ParamIndices::Aux2Enabled, 0 };
				f2.mFilterFreqParam.mValue.SetParamValue(0.4f);
				f2.mFilterFreqParam.mKTValue.SetParamValue(1.0f);
				f2.mFilterQParam.SetParamValue(0.15f);
				f2.mFilterSaturationParam.SetParamValue(0.15f);

				mAuxWidth.SetN11Value(1);
				mAuxRoutingParam.SetEnumValue(AuxRoute::TwoTwo);

				mPitchBendRange.SetIntValue(2);

				for (auto& m : mModulations)
				{
					m.mCurve.SetN11Value(0);
					m.mScale.SetN11Value(1);
					m.mAuxCurve.SetN11Value(0);
					m.mAuxAttenuation.SetParamValue(1);
				}

				for (size_t i = 0; i < gSourceCount; ++i) {
					mSources[i]->InitDevice();
				}

				EnvelopeNode mOsc1AmpEnv(mNullModMatrix, ModDestination::Osc1AmpEnvDelayTime, mParamCache, (int)ParamIndices::Osc1AmpEnvDelayTime);
				EnvelopeNode mOsc2AmpEnv(mNullModMatrix, ModDestination::Osc2AmpEnvDelayTime, mParamCache, (int)ParamIndices::Osc2AmpEnvDelayTime);
				EnvelopeNode mOsc3AmpEnv(mNullModMatrix, ModDestination::Osc3AmpEnvDelayTime, mParamCache, (int)ParamIndices::Osc3AmpEnvDelayTime);
				EnvelopeNode mOsc4AmpEnv(mNullModMatrix, ModDestination::Osc4AmpEnvDelayTime, mParamCache, (int)ParamIndices::Osc4AmpEnvDelayTime);
				EnvelopeNode mSampler1AmpEnv(mNullModMatrix, ModDestination::Sampler1AmpEnvDelayTime, mParamCache, (int)ParamIndices::Sampler1AmpEnvDelayTime);
				EnvelopeNode mSampler2AmpEnv(mNullModMatrix, ModDestination::Sampler2AmpEnvDelayTime, mParamCache, (int)ParamIndices::Sampler2AmpEnvDelayTime);
				EnvelopeNode mSampler3AmpEnv(mNullModMatrix, ModDestination::Sampler3AmpEnvDelayTime, mParamCache, (int)ParamIndices::Sampler3AmpEnvDelayTime);
				EnvelopeNode mSampler4AmpEnv(mNullModMatrix, ModDestination::Sampler4AmpEnvDelayTime, mParamCache, (int)ParamIndices::Sampler4AmpEnvDelayTime);
				EnvelopeNode mModEnv1(mNullModMatrix, ModDestination::Env1DelayTime, mParamCache, (int)ParamIndices::Env1DelayTime);
				EnvelopeNode mModEnv2(mNullModMatrix, ModDestination::Env2DelayTime, mParamCache, (int)ParamIndices::Env2DelayTime);

				mOsc1AmpEnv.LoadDefaults();
				mOsc2AmpEnv.LoadDefaults();
				mOsc3AmpEnv.LoadDefaults();
				mOsc4AmpEnv.LoadDefaults();
				mSampler1AmpEnv.LoadDefaults();
				mSampler2AmpEnv.LoadDefaults();
				mSampler3AmpEnv.LoadDefaults();
				mSampler4AmpEnv.LoadDefaults();
				mModEnv1.LoadDefaults();
				mModEnv2.LoadDefaults();

				mMasterVolume.SetLinearValue(1.0f);
				mParamCache[(int)ParamIndices::Osc1Enabled] = 1.0f;

				memcpy(mDefaultParamCache, mParamCache, sizeof(mParamCache));
			}

			virtual void HandlePitchBend(float pbN11) override
			{
				mPitchBendN11 = pbN11;
			}

			virtual void HandleMidiCC(int ccN, int val) override {
				// we don't care about this for the moment.
			}

			// minified
			virtual void SetChunk(void* data, int size) override
			{
				LoadDefaults();
				Deserializer ds{ (const uint8_t*)data, (size_t)size };
				for (int i = 0; i < numParams; ++i) {
					float f = mDefaultParamCache[i] + ds.ReadFloat();
					SetParam(i, f);
				}
				static_assert(gSamplerCount == 4, "sampler count");
				mSampler1Device.Deserialize(ds);
				mSampler2Device.Deserialize(ds);
				mSampler3Device.Deserialize(ds);
				mSampler4Device.Deserialize(ds);
			}

			// minified
			virtual int GetChunk(void** data) override
			{
				// the base chunk is our big param dump, with sampler data appended. each sampler will append its data; we won't think too much about that data here.
				// the base chunk is a diff of default params, because that's way more compressible. the alternative would be to meticulously
				// serialize some optimal fields based on whether an object is in use or not.
				// if we just do diffs against defaults, then it's going to be almost all zeroes.
				// the DAW can also attempt to optimize by finding unused elements and setting them to defaults.
				//     0.20000000298023223877, // PortTm
				//     0.40000000596046447754, // O1ScFq
				//     0.0000001
				Serializer s;
				for (int i = 0; i < numParams; ++i) {
					double f = mParamCache[i];
					f -= mDefaultParamCache[i];
					static constexpr double eps = 0.0000001;
					double af = f < 0 ? -f : f;
					if (af < eps) {
						f = 0;
					}
					s.WriteFloat((float)f);
				}

				static_assert(gSamplerCount == 4, "sampler count");
				mSampler1Device.Serialize(s);
				mSampler2Device.Serialize(s);
				mSampler3Device.Serialize(s);
				mSampler4Device.Serialize(s);

				auto ret = s.DetachBuffer();
				*data = ret.first;
				return (int)ret.second;
			}

			bool IsEnvelopeInUse(/*EnvelopeNode& env,*/ ModSource modSource) const
			{
				// an envelope is in use if it's used as a modulation source, AND the modulation is enabled,
				// AND the modulation destination is not some disabled thing. The VAST majority of these "disabled destinations" would just be the hidden volume / pre-fm volume source param,
				// so to simplify the logic just check for that.
				for (auto& m : mModulations) {
					if (!m.mEnabled.GetBoolValue()) {
						continue;
					}
					if ((m.mSource.GetEnumValue() == modSource) || (m.mAuxEnabled.GetBoolValue() && (m.mAuxSource.GetEnumValue() == modSource))) {
						// it's referenced by aux or main source. check that the destination is enabled.
						auto dest = m.mDestination.GetEnumValue();
						bool isHiddenVolumeDest = false;
						for (auto& src : mSources)
						{
							if (src->mHiddenVolumeModDestID == dest) {
								isHiddenVolumeDest = true;
								// the mod spec is modulating from this env to a hidden volume control; likely a built-in modulationspec for osc vol.
								// if that source is enabled, then consider this used.
								// otherwise keep looking.
								if (src->mEnabledParam.GetBoolValue()) {
									return true;
								}
							}
						}

						if (!isHiddenVolumeDest) {
							// it's modulating something other than a built-in volume control; assume the use cares about this.
							// here's where ideally we'd check deeper but rather just assume it's in use. after all the user explicitly set this mod.
							return true;
						}
					}
				}
				return false;
			}

			bool IsLFOInUse(ModSource modSource) const
			{
				// an LFO is in use if it's used as a modulation source, AND the modulation is enabled.
				// that's it. we don't nede to be any more specific than that because there are no default modspecs that contain lfos.
				// therefore if it's enabled we assume the user means it.
				for (auto& m : mModulations) {
					if (!m.mEnabled.GetBoolValue()) {
						continue;
					}
					if ((m.mSource.GetEnumValue() == modSource) || (m.mAuxEnabled.GetBoolValue() && (m.mAuxSource.GetEnumValue() == modSource))) {
						// it's referenced by aux or main source.
						return true;
					}
				}
				return false;
			}


			void SetParam(int index, float value)
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

			float GetParam(int index) const
			{
				if (index < 0) return 0;
				if (index >= (int)ParamIndices::NumParams) return 0;
				return mParamCache[index];
			}

			virtual void ProcessBlock(double songPosition, float* const* const outputs, int numSamples) override
			{
				mDeviceModSourceValuesSetThisFrame = false;

				bool sourceEnabled[gSourceCount];

				for (size_t i = 0; i < gSourceCount; ++i) {
					sourceEnabled[i] = mSources[i]->mEnabledParam.GetBoolValue();
				}

				float sourceModDistribution[gSourceCount];
				BipolarDistribute(gSourceCount, sourceEnabled, sourceModDistribution);

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

				for (size_t i = 0; i < gSourceCount; ++i) {
					auto* src = mSources[i];
					// for the moment mOscDetuneAmts[i] is just a generic spread value.
					src->mShapeDeviceModAmt = sourceModDistribution[i] * oscShapeSpreadAmt;
					src->mAuxPanDeviceModAmt = sourceModDistribution[i] * oscStereoSpreadAmt;
					src->mDetuneDeviceModAmt = sourceModDistribution[i] * oscDetuneAmt;
					src->BeginBlock(numSamples);
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

					mModLFO1Phase.ProcessSample(iSample, 0, 0, 0, 0, 0, 0, true);
					mModLFO2Phase.ProcessSample(iSample, 0, 0, 0, 0, 0, 0, true);
				}

				for (size_t i = 0; i < gSourceCount; ++i) {
					auto* src = mSources[i];
					src->EndBlock();
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
					mOsc4AmpEnv(mModMatrix, ModDestination::Osc4AmpEnvDelayTime, owner->mParamCache, (int)ParamIndices::Osc4AmpEnvDelayTime),
					mSampler1AmpEnv(mModMatrix, ModDestination::Sampler1AmpEnvDelayTime, owner->mParamCache, (int)ParamIndices::Sampler1AmpEnvDelayTime),
					mSampler2AmpEnv(mModMatrix, ModDestination::Sampler2AmpEnvDelayTime, owner->mParamCache, (int)ParamIndices::Sampler2AmpEnvDelayTime),
					mSampler3AmpEnv(mModMatrix, ModDestination::Sampler3AmpEnvDelayTime, owner->mParamCache, (int)ParamIndices::Sampler3AmpEnvDelayTime),
					mSampler4AmpEnv(mModMatrix, ModDestination::Sampler4AmpEnvDelayTime, owner->mParamCache, (int)ParamIndices::Sampler4AmpEnvDelayTime),
					mModEnv1(mModMatrix, ModDestination::Env1DelayTime, owner->mParamCache, (int)ParamIndices::Env1DelayTime),
					mModEnv2(mModMatrix, ModDestination::Env2DelayTime, owner->mParamCache, (int)ParamIndices::Env2DelayTime),
					mModLFO1(&owner->mLFO1Device, mModMatrix, nullptr),
					mModLFO2(&owner->mLFO2Device, mModMatrix, nullptr),
					mOscillator1(&owner->mOsc1Device, mModMatrix, &mOsc1AmpEnv),
					mOscillator2(&owner->mOsc2Device, mModMatrix, &mOsc2AmpEnv),
					mOscillator3(&owner->mOsc3Device, mModMatrix, &mOsc3AmpEnv),
					mOscillator4(&owner->mOsc4Device, mModMatrix, &mOsc4AmpEnv),
					mSampler1(&owner->mSampler1Device, mModMatrix, &mSampler1AmpEnv),
					mSampler2(&owner->mSampler2Device, mModMatrix, &mSampler2AmpEnv),
					mSampler3(&owner->mSampler3Device, mModMatrix, &mSampler3AmpEnv),
					mSampler4(&owner->mSampler4Device, mModMatrix, &mSampler4AmpEnv),
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
				OscillatorNode mOscillator4;
				SamplerVoice mSampler1;
				SamplerVoice mSampler2;
				SamplerVoice mSampler3;
				SamplerVoice mSampler4;

				EnvelopeNode mOsc1AmpEnv;
				EnvelopeNode mOsc2AmpEnv;
				EnvelopeNode mOsc3AmpEnv;
				EnvelopeNode mOsc4AmpEnv;
				EnvelopeNode mSampler1AmpEnv;
				EnvelopeNode mSampler2AmpEnv;
				EnvelopeNode mSampler3AmpEnv;
				EnvelopeNode mSampler4AmpEnv;

				ISoundSourceDevice::Voice* mSourceVoices[gSourceCount] = {
					&mOscillator1,
					&mOscillator2,
					&mOscillator3,
					&mOscillator4,
					&mSampler1,
					&mSampler2,
					&mSampler3,
					&mSampler4,
				};

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
					mOsc4AmpEnv.BeginBlock();
					mSampler1AmpEnv.BeginBlock();
					mSampler2AmpEnv.BeginBlock();
					mSampler3AmpEnv.BeginBlock();
					mSampler4AmpEnv.BeginBlock();
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
					mModMatrix.SetSourceKRateValue(ModSource::Macro5, mpOwner->mMacro5.Get01Value());  // krate, 01
					mModMatrix.SetSourceKRateValue(ModSource::Macro6, mpOwner->mMacro6.Get01Value());  // krate, 01
					mModMatrix.SetSourceKRateValue(ModSource::Macro7, mpOwner->mMacro7.Get01Value());  // krate, 01

					if (!mModLFO1.mpOscDevice->mPhaseRestart.GetBoolValue()) {
						// sync phase with device-level. TODO: also check that modulations aren't creating per-voice variation?
						mModLFO1.SetPhase(mpOwner->mModLFO1Phase.mPhase);
					}

					if (!mModLFO2.mpOscDevice->mPhaseRestart.GetBoolValue()) {
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
						for (auto& srcVoice : mSourceVoices)
						{
							mModMatrix.SetSourceARateValue(srcVoice->mpSrcDevice->mAmpEnvModSourceID, iSample, srcVoice->mpAmpEnv->ProcessSample(iSample));
						}
						mModMatrix.SetSourceARateValue(ModSource::ModEnv1, iSample, mModEnv1.ProcessSample(iSample));
						mModMatrix.SetSourceARateValue(ModSource::ModEnv2, iSample, mModEnv2.ProcessSample(iSample));
						float l1 = mModLFO1.ProcessSample(iSample, 0, 0, 0, 0, 0, 0, false);
						l1 = mLFOFilter1.ProcessSample(l1);
						mModMatrix.SetSourceARateValue(ModSource::LFO1, iSample, l1);
						float l2 = mModLFO2.ProcessSample(iSample, 0, 0, 0, 0, 0, 0, false);
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
						mpOwner->mFMAmt2to1.Get01Value(mModMatrix.GetDestinationValue(ModDestination::FMAmt2to1, 0))* globalFMScale,
						mpOwner->mFMAmt3to1.Get01Value(mModMatrix.GetDestinationValue(ModDestination::FMAmt3to1, 0))* globalFMScale,
						mpOwner->mFMAmt4to1.Get01Value(mModMatrix.GetDestinationValue(ModDestination::FMAmt4to1, 0))* globalFMScale,
						mpOwner->mFMAmt1to2.Get01Value(mModMatrix.GetDestinationValue(ModDestination::FMAmt1to2, 0))* globalFMScale,
						mpOwner->mFMAmt3to2.Get01Value(mModMatrix.GetDestinationValue(ModDestination::FMAmt3to2, 0))* globalFMScale,
						mpOwner->mFMAmt4to2.Get01Value(mModMatrix.GetDestinationValue(ModDestination::FMAmt4to2, 0))* globalFMScale,
						mpOwner->mFMAmt1to3.Get01Value(mModMatrix.GetDestinationValue(ModDestination::FMAmt1to3, 0))* globalFMScale,
						mpOwner->mFMAmt2to3.Get01Value(mModMatrix.GetDestinationValue(ModDestination::FMAmt2to3, 0))* globalFMScale,
						mpOwner->mFMAmt4to3.Get01Value(mModMatrix.GetDestinationValue(ModDestination::FMAmt4to3, 0))* globalFMScale,
						mpOwner->mFMAmt1to4.Get01Value(mModMatrix.GetDestinationValue(ModDestination::FMAmt1to4, 0))* globalFMScale,
						mpOwner->mFMAmt2to4.Get01Value(mModMatrix.GetDestinationValue(ModDestination::FMAmt2to4, 0))* globalFMScale,
						mpOwner->mFMAmt3to4.Get01Value(mModMatrix.GetDestinationValue(ModDestination::FMAmt3to4, 0)) * globalFMScale,
					};

					float myUnisonoDetune = mpOwner->mUnisonoDetuneAmts[this->mUnisonVoice];
					float myUnisonoPan = mpOwner->mUnisonoPanAmts[this->mUnisonVoice];
					float myUnisonoShape = mpOwner->mUnisonoShapeAmts[this->mUnisonVoice];

					for (size_t i = 0; i < gSourceCount; ++i)
					{
						auto* srcVoice = mSourceVoices[i];
						if (!srcVoice->mpSrcDevice->mEnabledParam.GetBoolValue()) {
							continue;
						}
						float semis = myUnisonoDetune + srcVoice->mpSrcDevice->mDetuneDeviceModAmt;// mpOwner->mOscDetuneAmts[i];
						
						srcVoice->BeginBlock(midiNote, myUnisonoShape + srcVoice->mpSrcDevice->mShapeDeviceModAmt, SemisToFrequencyMul(semis), globalFMScale, numSamples);

						float volumeMod = mModMatrix.GetDestinationValue(srcVoice->mpSrcDevice->mVolumeModDestID, 0);
						//VolumeParam outputVolParam{ mpOwner->mParamCache[(int)info.mOutputVolumeParamID], OscillatorNode::gVolumeMaxDb };

						// treat panning as added to modulation value
						float panParam = myUnisonoPan + srcVoice->mpSrcDevice->mAuxPanParam.GetN11Value(srcVoice->mpSrcDevice->mAuxPanDeviceModAmt + mModMatrix.GetDestinationValue(srcVoice->mpSrcDevice->mAuxPanModDestID, 0)); // -1 would mean full Left, 1 is full Right.
						auto panGains = PanToFactor(panParam);
						float outputVolLin = srcVoice->mpSrcDevice->mVolumeParam.GetLinearGain(volumeMod);
						srcVoice->mOutputGain[0] = outputVolLin * panGains.first;
						srcVoice->mOutputGain[1] = outputVolLin * panGains.second;
					}

					auto auxRouting = mpOwner->mAuxRoutingParam.GetEnumValue();
					for (size_t i = 0; i < gAuxNodeCount; ++i) {
						mAllAuxNodes[i]->BeginBlock(numSamples, mAllAuxNodes, noteHz, mModMatrix);
					}

					for (int iSample = 0; iSample < numSamples; ++iSample)
					{
						for (size_t i = 0; i < gSourceCount; ++i)
						{
							if (!mSourceVoices[i]->mpSrcDevice->mEnabledParam.GetBoolValue()) {
								continue;
							}
							auto srcVoice = mSourceVoices[i];
							float hiddenVolumeBacking = mModMatrix.GetDestinationValue(srcVoice->mpSrcDevice->mHiddenVolumeModDestID, iSample);
							VolumeParam hiddenAmpParam{ hiddenVolumeBacking, 0 };
							srcVoice->mAmpEnvGain = hiddenAmpParam.GetLinearGain(0);
						}

						float osc2LastSample = mOscillator2.GetSample() * mOscillator2.mAmpEnvGain;
						float osc3LastSample = mOscillator3.GetSample() * mOscillator3.mAmpEnvGain;
						float osc4LastSample = mOscillator4.GetSample() * mOscillator4.mAmpEnvGain;

						float sourceValues[gSourceCount] = { 0 };

						sourceValues[0] = mOscillator1.ProcessSample(iSample,
							osc2LastSample, FMScales[0],
							osc3LastSample, FMScales[1],
							osc4LastSample, FMScales[2],
							false
						);
						sourceValues[0] *= mOscillator1.mAmpEnvGain;

						sourceValues[1] = mOscillator2.ProcessSample(iSample,
							sourceValues[0], FMScales[3],
							osc3LastSample, FMScales[4],
							osc4LastSample, FMScales[5],
							false
						);
						sourceValues[1] *= mOscillator2.mAmpEnvGain;

						sourceValues[2] = mOscillator3.ProcessSample(iSample,
							sourceValues[0], FMScales[6],
							sourceValues[1], FMScales[7],
							osc4LastSample, FMScales[8],
							false
						);
						sourceValues[2] *= mOscillator3.mAmpEnvGain;

						sourceValues[3] = mOscillator4.ProcessSample(iSample,
							sourceValues[0], FMScales[9],
							sourceValues[1], FMScales[10],
							sourceValues[2], FMScales[11],
							false
						);
						sourceValues[3] *= mOscillator4.mAmpEnvGain;

						sourceValues[4] = mSampler1.ProcessSample(iSample) * mSampler1.mAmpEnvGain;
						sourceValues[5] = mSampler2.ProcessSample(iSample) * mSampler2.mAmpEnvGain;
						sourceValues[6] = mSampler3.ProcessSample(iSample) * mSampler3.mAmpEnvGain;
						sourceValues[7] = mSampler4.ProcessSample(iSample) * mSampler4.mAmpEnvGain;

						float sl = 0;
						float sr = 0;

						for (size_t i = 0; i < gSourceCount; ++i) {
							sl += sourceValues[i] * mSourceVoices[i]->mOutputGain[0];
							sr += sourceValues[i] * mSourceVoices[i]->mOutputGain[1];
						}

						// send through aux
						sl = mAux1.ProcessSample(sl, mAllAuxNodes);
						sl = mAux2.ProcessSample(sl, mAllAuxNodes);

						switch (auxRouting) {
						case AuxRoute::FourZero:
							// L = aux1 -> aux2 -> aux3 -> aux4 -> *
							// R = *
							sl = mAux3.ProcessSample(sl, mAllAuxNodes);
							sl = mAux4.ProcessSample(sl, mAllAuxNodes);
							break;
						case AuxRoute::SerialMono:
							// L = aux1 -> aux2 -> aux3 -> aux4 -> *
							// R = 
							// for the sake of being pleasant this swaps l/r channels as well for continuity with other settings
							sl = mAux3.ProcessSample(sl, mAllAuxNodes);
							sr = mAux4.ProcessSample(sl, mAllAuxNodes);
							sl = 0;
							break;
						case AuxRoute::ThreeOne:
							// L = aux1 -> aux2 -> aux3 -> *
							// R = aux4 -> *
							sl = mAux3.ProcessSample(sl, mAllAuxNodes);
							sr = mAux4.ProcessSample(sr, mAllAuxNodes);
							break;
						case AuxRoute::TwoTwo:
							// L = aux1 -> aux2 -> *
							// R = aux3 -> aux4 -> *
							sr = mAux3.ProcessSample(sr, mAllAuxNodes);
							sr = mAux4.ProcessSample(sr, mAllAuxNodes);
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

					for (auto& srcVoice : mSourceVoices)
					{
						if (srcVoice->mpSrcDevice->mKeyRangeMin.GetIntValue() > mNoteInfo.MidiNoteValue)
							continue;
						if (srcVoice->mpSrcDevice->mKeyRangeMax.GetIntValue() < mNoteInfo.MidiNoteValue)
							continue;
						srcVoice->mpAmpEnv->noteOn(mLegato);
						srcVoice->NoteOn(mLegato);
					}
					mModEnv1.noteOn(mLegato);
					mModEnv2.noteOn(mLegato);
					mModLFO1.NoteOn(mLegato);
					mModLFO2.NoteOn(mLegato);
					mPortamento.NoteOn((float)mNoteInfo.MidiNoteValue, !mLegato);
				}

				virtual void NoteOff() override {
					for (auto& srcVoice : mSourceVoices)
					{
						srcVoice->mpAmpEnv->noteOff();
						srcVoice->NoteOff();
					}
					mModEnv1.noteOff();
					mModEnv2.noteOff();
				}

				virtual void Kill() override {
					for (auto& srcVoice : mSourceVoices)
					{
						srcVoice->mpAmpEnv->kill();
					}
					mModEnv1.kill();
					mModEnv2.kill();
				}

				virtual bool IsPlaying() override {
					for (auto& srcVoice : mSourceVoices)
					{
						if (!srcVoice->mpSrcDevice->mEnabledParam.GetBoolValue())
							continue;
						if (srcVoice->mpAmpEnv->IsPlaying())
							return true;
					}
					return false;
				}
			};

			struct Maj7Voice* mMaj7Voice[maxVoices] = { 0 };

		};
	} // namespace M7

	using Maj7 = M7::Maj7;

} // namespace WaveSabreCore




