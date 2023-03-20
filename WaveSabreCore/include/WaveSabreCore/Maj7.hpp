// Time sync for LFOs? use Helpers::CurrentTempo; but do i always know the song position?

// size-optimizations:
// (profile w sizebench!)
// - use fixed-point in oscillators. there's absolutely no reason to be using chonky floating point instructions there.
// - re-order params so likely-zeros are grouped together.

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
		struct AuxDevice
		{
			const AuxLink mLinkToSelf;
			const AuxDevice* mpAllAuxNodes;

			BoolParam mEnabledParam;
			EnumParam<AuxLink> mLink;
			EnumParam<AuxEffectType> mEffectType;

			float* const mParamCache_Offset;
			const int mModDestParam2ID;
			const int mBaseParamID;

			AuxDevice(AuxDevice* allAuxNodes, AuxLink selfLink, int baseParamID, float* paramCache, int modDestParam2ID) :
				mpAllAuxNodes(allAuxNodes),
				mLinkToSelf(selfLink),
				mEnabledParam(paramCache[baseParamID + (int)AuxParamIndexOffsets::Enabled]),
				mLink(paramCache[baseParamID + (int)AuxParamIndexOffsets::Link], AuxLink::Count),
				mEffectType(paramCache[baseParamID + (int)AuxParamIndexOffsets::Type], AuxEffectType::Count),
				mParamCache_Offset(paramCache + baseParamID),
				mModDestParam2ID(modDestParam2ID),
				mBaseParamID(baseParamID)
			{
			}

			bool IsLinkedExternally() const
			{
				return mLinkToSelf != mLink.mCachedVal;
			}

			// nullptr is a valid return val for safe null effect.
			IAuxEffect* CreateEffect() const
			{
				if (!IsEnabled()) {
					return nullptr;
				}
				// ASSERT: not linked.
				switch (mEffectType.mCachedVal)
				{
				default:
				case AuxEffectType::None:
					break;
				case AuxEffectType::BigFilter:
					return new FilterAuxNode(mParamCache_Offset, mModDestParam2ID);
				case AuxEffectType::Bitcrush:
					return new BitcrushAuxNode(mParamCache_Offset, mModDestParam2ID);
				}
				return nullptr;
			}

			// returns whether this node will process audio. depends on external linking.
			bool IsEnabled() const
			{
				auto link = mLink.mCachedVal;// .GetEnumValue();
				const AuxDevice* srcNode = this;
				if (IsLinkedExternally())
				{
					if (!mpAllAuxNodes) return false;
					srcNode = &mpAllAuxNodes[(int)link];
					if (srcNode->IsLinkedExternally())
					{
						return false; // no chaining; needlessly vexing
					}
				}
				return srcNode->mEnabledParam.mCachedVal;// .GetBoolValue();
			}

			// pass in aux nodes due to linking.
			// for simplicity to avoid circular refs, disable if you link to a linked aux
			void BeginBlock()
			{
				mEnabledParam.CacheValue();
				mLink.CacheValue();
				mEffectType.CacheValue();
			}
		};


		struct AuxNode
		{
			AuxDevice* mpDevice = nullptr;
			AuxLink mCurrentEffectLink;
			AuxEffectType mCurrentEffectType = AuxEffectType::None;
			IAuxEffect* mpCurrentEffect = nullptr;

			AuxNode(AuxDevice* pDevice) :
				mpDevice(pDevice),
				mCurrentEffectLink(pDevice->mLinkToSelf)
			{
			}

			virtual ~AuxNode()
			{
				delete mpCurrentEffect;
			}

			void Reset()
			{
				delete mpCurrentEffect;
				mpCurrentEffect = nullptr;
			}

			// pass in aux nodes due to linking.
			// for simplicity to avoid circular refs, disable if you link to a linked aux
			void BeginBlock(float noteHz, ModMatrixNode& modMatrix) {
				if (!mpDevice->IsEnabled()) return;
				auto link = mpDevice->mLink.mCachedVal;// .GetEnumValue();
				const AuxDevice* const srcNode = &mpDevice->mpAllAuxNodes[(int)link];
				auto effectType = srcNode->mEffectType.mCachedVal;// .GetEnumValue();

				// ensure correct engine set up.
				if (mCurrentEffectLink != link || mCurrentEffectType != effectType || !mpCurrentEffect) {
					delete mpCurrentEffect;
					mpCurrentEffect = srcNode->CreateEffect();
					mCurrentEffectLink = link;
					mCurrentEffectType = effectType;
				}
				if (!mpCurrentEffect) return;

				return mpCurrentEffect->AuxBeginBlock(noteHz, modMatrix);
			}

			float ProcessSample(float inp) {
				if (!mpDevice->IsEnabled()) return inp;
				if (!mpCurrentEffect) return inp;
				return mpCurrentEffect->AuxProcessSample(inp);
			}
		};


		// this does not operate on frequencies, it operates on target midi note value (0-127).
		struct PortamentoCalc
		{
		private:
			bool mEngaged = false;

			float mSourceNote = 0; // float because you portamento can initiate between notes
			float mTargetNote = 0;
			float mCurrentNote = 0;
			float mSlideCursorSamples = 0;

		public:
			EnvTimeParam mTime;
			CurveParam mCurve;

			PortamentoCalc(float& timeParamBacking, float& curveParamBacking) :
				mTime(timeParamBacking),
				mCurve(curveParamBacking)
			{}

			// advances cursor; call this at the end of buffer processing. next buffer will call
			// GetCurrentMidiNote() then.
			void Advance(size_t nSamples, float timeMod) {
				if (!mEngaged) {
					return;
				}
				mSlideCursorSamples += nSamples;
				float durationSamples = math::MillisecondsToSamples(mTime.GetMilliseconds(timeMod));
				float t = mSlideCursorSamples / durationSamples;
				if (t >= 1) {
					NoteOn(mTargetNote, true); // disengages
					return;
				}
				t = mCurve.ApplyToValue(t, 0);
				mCurrentNote = math::lerp(mSourceNote, mTargetNote, t);
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

		extern const float gDefaultMasterParams[(int)MainParamIndices::Count];
		extern const float gDefaultSamplerParams[(int)SamplerParamIndexOffsets::Count];
		extern const float gDefaultModSpecParams[(int)ModParamIndexOffsets::Count];
		extern const float gDefaultLFOParams[(int)LFOParamIndexOffsets::Count];
		extern const float gDefaultEnvelopeParams[(int)EnvParamIndexOffsets::Count];
		extern const float gDefaultOscillatorParams[(int)OscParamIndexOffsets::Count];
		extern const float gDefaultAuxParams[(int)AuxParamIndexOffsets::Count];


		struct Maj7 : public Maj7SynthDevice
		{
			static constexpr DWORD gChunkTag = MAKEFOURCC('M', 'a', 'j', '7');
			static constexpr uint8_t gChunkVersion = 0;
			enum class ChunkFormat : uint8_t
			{
				Minified,
				Count,
			};

			static constexpr int gPitchBendMaxRange = 24;
			static constexpr int gUnisonoVoiceMax = 12;
			static constexpr size_t gOscillatorCount = 4;
			static constexpr size_t gFMMatrixSize = gOscillatorCount * (gOscillatorCount - 1);
			static constexpr size_t gSamplerCount = 4;
			static constexpr size_t gSourceCount = gOscillatorCount + gSamplerCount;
			static constexpr size_t gAuxNodeCount = 4;
			static constexpr real_t gMasterVolumeMaxDb = 6;
			static constexpr size_t gMacroCount = 7;

			static constexpr size_t gModEnvCount = 2;
			static constexpr size_t gModLFOCount = 4;

			// BASE PARAMS & state
			DCFilter mDCFilters[2];

			float mAlways0 = 0;
			real_t mPitchBendN11 = 0;

			float mUnisonoDetuneAmts[gUnisonoVoiceMax] = { 0 };

			float mAuxOutputGains[2] = {0}; // precalc'd L/R gains based on aux panning/width [outputchannel]

			float mUnisonoPanAmts[gUnisonoVoiceMax] { 0 };

			EnumParam<VoiceMode> mVoicingModeParam{ mParamCache[(int)ParamIndices::VoicingMode], WaveSabreCore::VoiceMode::Count};
			IntParam mUnisonoVoicesParam{ mParamCache[(int)ParamIndices::Unisono], 1, gUnisonoVoiceMax };

			VolumeParam mMasterVolume{ mParamCache[(int)ParamIndices::MasterVolume], gMasterVolumeMaxDb };

			Float01ParamArray mMacros { &mParamCache[(int)ParamIndices::Macro1], gMacroCount };

			Float01ParamArray mFMMatrix{ &mParamCache[(int)ParamIndices::FMAmt2to1], gFMMatrixSize };

			Float01Param mFMBrightness{ mParamCache[(int)ParamIndices::FMBrightness] };

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
				{ mParamCache, (int)ParamIndices::Mod17Enabled },
				{ mParamCache, (int)ParamIndices::Mod18Enabled },
			};

			float mParamCache[(int)ParamIndices::NumParams];
			float mDefaultParamCache[(int)ParamIndices::NumParams];

			// these are not REALLY lfos, they are used to track phase at the device level, for when an LFO's phase should be synced between all voices.
			// that would happen only when all of the following are satisfied:
			// 1. the LFO is not triggered by notes.
			// 2. the LFO has no modulations on its phase or frequency

			struct LFODevice
			{
				explicit LFODevice(float* paramCache, ParamIndices paramBaseID, ModDestination modBaseID) :
					mDevice{ OscillatorIntentionLFO{}, paramCache, paramBaseID, modBaseID }
				{}
				float mLPCutoff = 0.0f;
				ModMatrixNode mNullModMatrix;
				OscillatorDevice mDevice;
				OscillatorNode mPhase{ &mDevice, mNullModMatrix, nullptr };
			};

			LFODevice mLFOs[gModLFOCount] = {
				LFODevice{mParamCache, ParamIndices::LFO1Waveform, ModDestination::LFO1Waveshape},
				LFODevice{mParamCache, ParamIndices::LFO2Waveform, ModDestination::LFO2Waveshape},
				LFODevice{mParamCache, ParamIndices::LFO3Waveform, ModDestination::LFO3Waveshape},
				LFODevice{mParamCache, ParamIndices::LFO4Waveform, ModDestination::LFO4Waveshape},
			};

			OscillatorDevice mOscillatorDevices[gOscillatorCount] = {
				OscillatorDevice { OscillatorIntentionAudio{}, mParamCache, &mModulations[gModulationCount - 8], ParamIndices::Osc1Enabled, ParamIndices::Osc1AmpEnvDelayTime, ModSource::Osc1AmpEnv, ModDestination::Osc1Volume },
				OscillatorDevice { OscillatorIntentionAudio{}, mParamCache, &mModulations[gModulationCount - 7], ParamIndices::Osc2Enabled, ParamIndices::Osc2AmpEnvDelayTime, ModSource::Osc2AmpEnv, ModDestination::Osc2Volume },
				OscillatorDevice { OscillatorIntentionAudio{}, mParamCache, &mModulations[gModulationCount - 6], ParamIndices::Osc3Enabled, ParamIndices::Osc3AmpEnvDelayTime, ModSource::Osc3AmpEnv, ModDestination::Osc3Volume },
				OscillatorDevice { OscillatorIntentionAudio{}, mParamCache, &mModulations[gModulationCount - 5], ParamIndices::Osc4Enabled, ParamIndices::Osc4AmpEnvDelayTime, ModSource::Osc4AmpEnv, ModDestination::Osc4Volume },
			};

			SamplerDevice mSamplerDevices[gSamplerCount] = {
				SamplerDevice{ mParamCache, &mModulations[gModulationCount - 4], ParamIndices::Sampler1Enabled, ParamIndices::Sampler1AmpEnvDelayTime, ModSource::Sampler1AmpEnv, ModDestination::Sampler1Volume },
				SamplerDevice{ mParamCache, &mModulations[gModulationCount - 3], ParamIndices::Sampler2Enabled, ParamIndices::Sampler2AmpEnvDelayTime, ModSource::Sampler2AmpEnv, ModDestination::Sampler2Volume },
				SamplerDevice{ mParamCache, &mModulations[gModulationCount - 2], ParamIndices::Sampler3Enabled, ParamIndices::Sampler3AmpEnvDelayTime, ModSource::Sampler3AmpEnv, ModDestination::Sampler3Volume },
				SamplerDevice{ mParamCache, &mModulations[gModulationCount - 1], ParamIndices::Sampler4Enabled, ParamIndices::Sampler4AmpEnvDelayTime, ModSource::Sampler4AmpEnv, ModDestination::Sampler4Volume },
			};

			AuxDevice mAuxDevices[gAuxNodeCount] = {
				AuxDevice { mAuxDevices, AuxLink::Aux1, (int)ParamIndices::Aux1Enabled, mParamCache, (int)ModDestination::Aux1Param2 },
				AuxDevice { mAuxDevices, AuxLink::Aux2, (int)ParamIndices::Aux2Enabled, mParamCache, (int)ModDestination::Aux2Param2 },
				AuxDevice { mAuxDevices, AuxLink::Aux3, (int)ParamIndices::Aux3Enabled, mParamCache, (int)ModDestination::Aux3Param2 },
				AuxDevice { mAuxDevices, AuxLink::Aux4, (int)ParamIndices::Aux4Enabled, mParamCache, (int)ModDestination::Aux4Param2 },
			};

			ISoundSourceDevice* mSources[gSourceCount] = {
				&mOscillatorDevices[0],
				&mOscillatorDevices[1],
				&mOscillatorDevices[2],
				&mOscillatorDevices[3],
				&mSamplerDevices[0],
				&mSamplerDevices[1],
				&mSamplerDevices[2],
				&mSamplerDevices[3],
			};

			// these values are at the device level (not voice level), but yet they can be modulated by voice-level things.
			// for exmaple you could map Velocity to unisono detune. Well it should be clear that this doesn't make much sense,
			// except for maybe monophonic mode? So they are evaluated at the voice level but ultimately stored here.
			real_t mFMBrightnessMod = 0;
			real_t mPortamentoTimeMod = 0;

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
				// aux nodes reset
				for (size_t i = 0; i < std::size(mVoices); ++i) {
					for (auto& x : mMaj7Voice[i]->mAllAuxNodes) {
						x->Reset();
					}
				}
				// samplers reset
				for (auto& s : mSamplerDevices) {
					s.Reset();
				}

				// now load in all default params
				memcpy(mParamCache, gDefaultMasterParams, sizeof(gDefaultMasterParams));

				for (auto& m : mLFOs) {
					memcpy(mParamCache + (int)m.mDevice.mBaseParamID, gDefaultLFOParams, sizeof(gDefaultLFOParams));
				}
				for (auto& m : mOscillatorDevices) {
					memcpy(mParamCache + (int)m.mBaseParamID, gDefaultOscillatorParams, sizeof(gDefaultOscillatorParams));
				}
				for (auto& s : mSamplerDevices) {
					memcpy(mParamCache + (int)s.mBaseParamID, gDefaultSamplerParams, sizeof(gDefaultSamplerParams));
				}
				for (auto& m : mModulations) {
					memcpy(mParamCache + (int)m.mBaseParamID, gDefaultModSpecParams, sizeof(gDefaultModSpecParams));
				}
				for (auto& m : mAuxDevices) {
					memcpy(m.mParamCache_Offset, gDefaultAuxParams, sizeof(gDefaultAuxParams));
				}
				for (auto* p : mMaj7Voice[0]->mpAllModEnvelopes) {
					memcpy(mParamCache + (int)p->mParamBaseID, gDefaultEnvelopeParams, sizeof(gDefaultEnvelopeParams));
				}
				for (auto* p : mSources) {
					memcpy(mParamCache + (int)p->mAmpEnvBaseParamID, gDefaultEnvelopeParams, sizeof(gDefaultEnvelopeParams));
					p->InitDevice();
				}

				// and correct stuff

				mAuxDevices[0].mEffectType.SetEnumValue(AuxEffectType::Bitcrush);
				mAuxDevices[1].mEnabledParam.SetBoolValue(true);
				mAuxDevices[1].mEffectType.SetEnumValue(AuxEffectType::BigFilter);
				mAuxDevices[1].mLink.SetEnumValue(AuxLink::Aux2);
				mAuxDevices[2].mLink.SetEnumValue(AuxLink::Aux1);
				mAuxDevices[3].mLink.SetEnumValue(AuxLink::Aux2);

				BitcrushAuxNode a1{ mParamCache + (int)ParamIndices::Aux1Enabled, 0};
				a1.mFreqParam.mKTValue.SetParamValue(1);
				a1.mFreqParam.mValue.SetParamValue(0.3f);

				FilterAuxNode f2{ mParamCache + (int)ParamIndices::Aux2Enabled, 0 };
				f2.mFilterFreqParam.mValue.SetParamValue(M7::gFreqParamKTUnity);
				f2.mFilterFreqParam.mKTValue.SetParamValue(1.0f);
				f2.mFilterQParam.SetParamValue(0.15f);
				f2.mFilterSaturationParam.SetParamValue(0.15f);

				mParamCache[(int)ParamIndices::Osc1Enabled] = 1.0f;

				// Apply dynamic state
				this->SetVoiceMode(mVoicingModeParam.GetEnumValue());
				this->SetUnisonoVoices(mUnisonoVoicesParam.GetIntValue());
				// NOTE: samplers will always be empty here

				// store these values for later when we need to do diffs
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

				auto tag = ds.ReadUInt32();
				if (tag != gChunkTag) return;
				auto format = (ChunkFormat)ds.ReadUByte();
				if (format != ChunkFormat::Minified) return;
				auto version = ds.ReadUByte();
				if (version != gChunkVersion) return; // in the future maybe support other versions? but probably not in the minified code
				for (int i = 0; i < numParams; ++i) {
					float f = mDefaultParamCache[i] + ds.ReadFloat();
					SetParam(i, f);
				}
				for (auto& s : mSamplerDevices) {
					s.Deserialize(ds);
				}
			}

			void SetParam(int index, float value)
			{
				if (index < 0) return;
				if (index >= (int)ParamIndices::NumParams) return;

				mParamCache[index] = value;

				switch (index)
				{
					case (int)ParamIndices::VoicingMode:
					{
						this->SetVoiceMode(mVoicingModeParam.GetEnumValue());
						break;
					}
					case (int)ParamIndices::Unisono:
					{
						this->SetUnisonoVoices(mUnisonoVoicesParam.GetIntValue());
						break;
					}
				}

			}

			float GetParam(int index) const
			{
				if (index < 0) return 0;
				if (index >= (int)ParamIndices::NumParams) return 0;
				return mParamCache[index];
			}


			virtual void ProcessBlock(double songPosition, float* const* const outputs, int numSamples) override
			{
				bool sourceEnabled[gSourceCount];

				// cache values
				mAuxRoutingParam.CacheValue();

				for (size_t i = 0; i < gSourceCount; ++i) {
					auto* src = mSources[i];
					sourceEnabled[i] = src->mEnabledParam.GetBoolValue();
					src->BeginBlock();
				}

				for (auto& m : mModulations) {
					m.BeginBlock();
				}
				for (size_t i = 0; i < gAuxNodeCount; ++i) {
					mAuxDevices[i].BeginBlock();
				}

				float sourceModDistribution[gSourceCount];
				BipolarDistribute(gSourceCount, sourceEnabled, sourceModDistribution);

				bool unisonoEnabled[gUnisonoVoiceMax] = { false };
				for (size_t i = 0; i < (size_t)mVoicesUnisono; ++i) {
					unisonoEnabled[i] = true;
				}
				BipolarDistribute(gUnisonoVoiceMax, unisonoEnabled, mUnisonoDetuneAmts);

				// scale down per param & mod
				for (size_t i = 0; i < (size_t)mVoicesUnisono; ++i) {
					mUnisonoPanAmts[i] = mUnisonoDetuneAmts[i] * (mParamCache[(int)ParamIndices::UnisonoStereoSpread] /*+ mUnisonoStereoSpreadMod*/);
					mUnisonoDetuneAmts[i] *= mParamCache[(int)ParamIndices::UnisonoDetune] /*+ mUnisonoDetuneMod*/;
				}

				// when aux width is 0, they are both mono. it means pan of 0 for both aux routes.
				// when aux width is +1, auxroute 0 becomes -1 and auxroute 1 becomes +1, fully separating the channels
				auto auxGains = math::PanToFactor(mAuxWidth.GetN11Value(/*mAuxWidthMod*/));
				mAuxOutputGains[1] = auxGains.first;
				mAuxOutputGains[0] = auxGains.second;

				for (size_t i = 0; i < gSourceCount; ++i) {
					auto* src = mSources[i];
					// for the moment mOscDetuneAmts[i] is just a generic spread value.
					src->mAuxPanDeviceModAmt = sourceModDistribution[i] * (mParamCache[(int)ParamIndices::OscillatorSpread] /*+ mOscillatorStereoSpreadMod*/);
					src->mDetuneDeviceModAmt = sourceModDistribution[i] * (mParamCache[(int)ParamIndices::OscillatorDetune]/* + mOscillatorDetuneMod*/);
				}

				for (size_t i = 0; i < gModLFOCount; ++i) {
					auto& lfo = mLFOs[i];
					lfo.mDevice.BeginBlock();
					lfo.mLPCutoff = lfo.mDevice.mLPFFrequency.GetFrequency(0, 0);
					lfo.mPhase.BeginBlock();
				}

				for (auto* v : mMaj7Voice)
				{
					v->BeginBlock();
				}

				// very inefficient to calculate all in the loops like that but it's for size-optimization
				float masterGain = mMasterVolume.GetLinearGain();
				for (size_t iSample = 0; iSample < (size_t)numSamples; ++iSample)
				{
					float s[2] = { 0 };

					for (auto* v : mMaj7Voice)
					{
						v->ProcessAndMix(s);
					}

					for (size_t ioutput = 0; ioutput < 2; ++ioutput) {
						float o = s[ioutput];
						o *= masterGain;
						o = mDCFilters[ioutput].ProcessSample(o);
						o = math::clampN11(o);
						outputs[ioutput][iSample] = o;
					}

					// advance phase of master LFOs
					for (size_t i = 0; i < gModLFOCount; ++i) {
						auto& lfo = mLFOs[i];
						lfo.mPhase.ProcessSampleForLFO(true);
					}
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
					mOsc1AmpEnv(mModMatrix, ModDestination::Osc1AmpEnvDelayTime, owner->mParamCache, (int)ParamIndices::Osc1AmpEnvDelayTime, ModSource::Osc1AmpEnv),
					mOsc2AmpEnv(mModMatrix, ModDestination::Osc2AmpEnvDelayTime, owner->mParamCache, (int)ParamIndices::Osc2AmpEnvDelayTime, ModSource::Osc2AmpEnv),
					mOsc3AmpEnv(mModMatrix, ModDestination::Osc3AmpEnvDelayTime, owner->mParamCache, (int)ParamIndices::Osc3AmpEnvDelayTime, ModSource::Osc3AmpEnv),
					mOsc4AmpEnv(mModMatrix, ModDestination::Osc4AmpEnvDelayTime, owner->mParamCache, (int)ParamIndices::Osc4AmpEnvDelayTime, ModSource::Osc4AmpEnv),
					mSampler1AmpEnv(mModMatrix, ModDestination::Sampler1AmpEnvDelayTime, owner->mParamCache, (int)ParamIndices::Sampler1AmpEnvDelayTime, ModSource::Sampler1AmpEnv),
					mSampler2AmpEnv(mModMatrix, ModDestination::Sampler2AmpEnvDelayTime, owner->mParamCache, (int)ParamIndices::Sampler2AmpEnvDelayTime, ModSource::Sampler2AmpEnv),
					mSampler3AmpEnv(mModMatrix, ModDestination::Sampler3AmpEnvDelayTime, owner->mParamCache, (int)ParamIndices::Sampler3AmpEnvDelayTime, ModSource::Sampler3AmpEnv),
					mSampler4AmpEnv(mModMatrix, ModDestination::Sampler4AmpEnvDelayTime, owner->mParamCache, (int)ParamIndices::Sampler4AmpEnvDelayTime, ModSource::Sampler4AmpEnv),
					mModEnv1(mModMatrix, ModDestination::Env1DelayTime, owner->mParamCache, (int)ParamIndices::Env1DelayTime, ModSource::ModEnv1),
					mModEnv2(mModMatrix, ModDestination::Env2DelayTime, owner->mParamCache, (int)ParamIndices::Env2DelayTime, ModSource::ModEnv2),
					mOscillator1(&owner->mOscillatorDevices[0], mModMatrix, &mOsc1AmpEnv),
					mOscillator2(&owner->mOscillatorDevices[1], mModMatrix, &mOsc2AmpEnv),
					mOscillator3(&owner->mOscillatorDevices[2], mModMatrix, &mOsc3AmpEnv),
					mOscillator4(&owner->mOscillatorDevices[3], mModMatrix, &mOsc4AmpEnv),
					mSampler1(&owner->mSamplerDevices[0], mModMatrix, &mSampler1AmpEnv),
					mSampler2(&owner->mSamplerDevices[1], mModMatrix, &mSampler2AmpEnv),
					mSampler3(&owner->mSamplerDevices[2], mModMatrix, &mSampler3AmpEnv),
					mSampler4(&owner->mSamplerDevices[3], mModMatrix, &mSampler4AmpEnv),
					mAux1(&owner->mAuxDevices[0]),
					mAux2(&owner->mAuxDevices[1]),
					mAux3(&owner->mAuxDevices[2]),
					mAux4(&owner->mAuxDevices[3])
					//mModLFO1(&owner->mLFO1Device, mModMatrix, nullptr),
					//mModLFO2(&owner->mLFO2Device, mModMatrix, nullptr),
				{
				}

				Maj7* mpOwner;

				real_t mVelocity01 = 0;
				real_t mTriggerRandom01 = 0;
				PortamentoCalc mPortamento;

				struct LFOVoice
				{
					explicit LFOVoice(ModSource modSourceID, LFODevice& device, ModMatrixNode& modMatrix) :
						mModSourceID(modSourceID),
						mDevice(device),
						mNode(&device.mDevice, modMatrix, nullptr)
					{}
					ModSource mModSourceID;
					LFODevice& mDevice;
					OscillatorNode mNode;
					FilterNode mFilter;
				};
				LFOVoice mLFOs[gModLFOCount]{
					LFOVoice{ ModSource::LFO1, mpOwner->mLFOs[0], mModMatrix },
					LFOVoice{ ModSource::LFO2, mpOwner->mLFOs[1], mModMatrix },
					LFOVoice{ ModSource::LFO3, mpOwner->mLFOs[2], mModMatrix },
					LFOVoice{ ModSource::LFO4, mpOwner->mLFOs[3], mModMatrix }
				};

				//OscillatorNode mModLFO2;
				//FilterNode mLFOFilter2;

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
				EnvelopeNode mModEnv1;
				EnvelopeNode mModEnv2;

				EnvelopeNode* mpAllModEnvelopes[gModEnvCount] {
					&mModEnv1,
					&mModEnv2,
				};

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

				float mMidiNote = 0;

				void BeginBlock()
				{
					if (!this->IsPlaying()) {
						return;
					}
					// at this point run the graph.
					// 1. run signals which produce A-Rate outputs, so we can use those buffers to modulate nodes which have A-Rate destinations
					// 2. mod matrix, which fills buffers with modulation signals. mod sources should be up-to-date
					// 3. run signals which are A-Rate destinations.
					// if we were really ambitious we could make the filter an A-Rate destination as well, with oscillator outputs being A-Rate.
					for (auto p : mpAllModEnvelopes) {
						p->BeginBlock();
					}

					mModMatrix.BeginBlock();

					mMidiNote = mPortamento.GetCurrentMidiNote() + mpOwner->mPitchBendRange.GetIntValue() * mpOwner->mPitchBendN11;

					real_t noteHz = math::MIDINoteToFreq(mMidiNote);

					mModMatrix.SetSourceValue(ModSource::PitchBend, mpOwner->mPitchBendN11); // krate, N11
					mModMatrix.SetSourceValue(ModSource::Velocity, mVelocity01);  // krate, 01
					mModMatrix.SetSourceValue(ModSource::NoteValue, mMidiNote / 127.0f); // krate, 01
					mModMatrix.SetSourceValue(ModSource::RandomTrigger, mTriggerRandom01); // krate, 01
					mModMatrix.SetSourceValue(ModSource::UnisonoVoice, float(mUnisonVoice + 1) / mpOwner->mVoicesUnisono); // krate, 01
					mModMatrix.SetSourceValue(ModSource::SustainPedal, real_t(mpOwner->mIsPedalDown ? 0 : 1)); // krate, 01

					for (size_t iMacro = 0; iMacro < gMacroCount; ++iMacro)
					{
						mModMatrix.SetSourceValue((int)ModSource::Macro1 + iMacro, mpOwner->mMacros.Get01Value(iMacro));  // krate, 01
					}

					for (size_t i = 0; i < gModLFOCount; ++i) {
						auto& lfo = mLFOs[i];
						if (!lfo.mNode.mpOscDevice->mPhaseRestart.GetBoolValue()) {
							// sync phase with device-level. TODO: also check that modulations aren't creating per-voice variation?
							lfo.mNode.SetPhase(lfo.mDevice.mPhase.mPhase);
						}
						lfo.mNode.BeginBlock();
						lfo.mFilter.SetParams(FilterModel::LP_OnePole, lfo.mDevice.mLPCutoff, 0, 0);
					}

					// set device-level modded values.
					mpOwner->mFMBrightnessMod = mModMatrix.GetDestinationValue(ModDestination::FMBrightness);
					mpOwner->mPortamentoTimeMod = mModMatrix.GetDestinationValue(ModDestination::PortamentoTime);

					for (size_t i = 0; i < gAuxNodeCount; ++i) {
						mAllAuxNodes[i]->BeginBlock(noteHz, mModMatrix);
					}

					for (size_t i = 0; i < gSourceCount; ++i)
					{
						mSourceVoices[i]->BeginBlock();
						mSourceVoices[i]->mpAmpEnv->BeginBlock();
					}
				}

				void ProcessAndMix(float* s)
				{
					if (!this->IsPlaying()) {
						return;
					}

					float l = mModEnv1.ProcessSample();
					mModMatrix.SetSourceValue(ModSource::ModEnv1,l );

					l = mModEnv2.ProcessSample();
					mModMatrix.SetSourceValue(ModSource::ModEnv2, l);

					for (size_t i = 0; i < gModLFOCount; ++i) {
						auto& lfo = mLFOs[i];
						float s = lfo.mNode.ProcessSampleForLFO(false);
						s = lfo.mFilter.ProcessSample(s);
						mModMatrix.SetSourceValue(lfo.mModSourceID, s);
					}

					for (size_t i = 0; i < gSourceCount; ++i)
					{
						auto* srcVoice = mSourceVoices[i];

						if (!srcVoice->mpSrcDevice->mEnabledParam.mCachedVal) {
							srcVoice->mpAmpEnv->kill();
							continue;
						}

						mModMatrix.SetSourceValue(srcVoice->mpSrcDevice->mAmpEnvModSourceID, srcVoice->mpAmpEnv->ProcessSample());
					}

					// process modulations here. sources have just been set, and past here we're getting many destination values.
					// processing here ensures fairly up-to-date accurate values.
					// one area where this is quite sensitive is envelopes with instant attacks/releases
					mModMatrix.ProcessSample(mpOwner->mModulations); // this sets dest values to 0.

					float myUnisonoDetune = mpOwner->mUnisonoDetuneAmts[this->mUnisonVoice];
					float myUnisonoPan = mpOwner->mUnisonoPanAmts[this->mUnisonVoice];

					float sourceValues[gSourceCount] = { 0 };
					float detuneMul[gSourceCount] = { 0 };

					for (size_t i = 0; i < gSourceCount; ++i)
					{
						auto* srcVoice = mSourceVoices[i];

						float volumeMod = mModMatrix.GetDestinationValue(srcVoice->mpSrcDevice->mVolumeModDestID);

						// treat panning as added to modulation value
						float panParam = myUnisonoPan +
							srcVoice->mpSrcDevice->mAuxPanParam.mCachedVal +
							srcVoice->mpSrcDevice->mAuxPanDeviceModAmt +
							mModMatrix.GetDestinationValue(srcVoice->mpSrcDevice->mAuxPanModDestID); // -1 would mean full Left, 1 is full Right.
						auto panGains = math::PanToFactor(panParam);
						float outputVolLin = srcVoice->mpSrcDevice->mVolumeParam.GetLinearGain(volumeMod);
						srcVoice->mOutputGain[0] = outputVolLin * panGains.first;
						srcVoice->mOutputGain[1] = outputVolLin * panGains.second;

						float hiddenVolumeBacking = mModMatrix.GetDestinationValue(srcVoice->mpSrcDevice->mHiddenVolumeModDestID);
						VolumeParam hiddenAmpParam{ hiddenVolumeBacking, 0 };
						float ampEnvGain = hiddenAmpParam.GetLinearGain(0);
						srcVoice->mAmpEnvGain = ampEnvGain;
						sourceValues[i] = srcVoice->GetLastSample() * ampEnvGain;

						float semis = myUnisonoDetune + srcVoice->mpSrcDevice->mDetuneDeviceModAmt * 2;// mpOwner->mOscDetuneAmts[i];
						float det = detuneMul[i] = math::SemisToFrequencyMul(semis);

						if (i >= gOscillatorCount)
						{
							auto ps = static_cast<SamplerVoice*>(srcVoice);
							sourceValues[i] = ps->ProcessSample(mMidiNote, det, 0) * ampEnvGain;
						}
					}

					float globalFMScale = 3 * mpOwner->mFMBrightness.Get01Value(mpOwner->mFMBrightnessMod);

					for (size_t i = 0; i < gOscillatorCount; ++i) {
						auto* srcVoice = mSourceVoices[i];
						auto po = static_cast<OscillatorNode*>(srcVoice);
						const size_t x = i * (gOscillatorCount - 1);

						sourceValues[i] = po->ProcessSampleForAudio(mMidiNote, detuneMul[i], globalFMScale,
							sourceValues[i < 1 ? 1 : 0], mpOwner->mFMMatrix.Get01Value(x, mModMatrix.GetDestinationValue((int)ModDestination::FMAmt2to1 + x)) * globalFMScale,
							sourceValues[i < 2 ? 2 : 1], mpOwner->mFMMatrix.Get01Value(1 + x, mModMatrix.GetDestinationValue((int)ModDestination::FMAmt3to1 + x)) * globalFMScale,
							sourceValues[i < 3 ? 3 : 2], mpOwner->mFMMatrix.Get01Value(2 + x, mModMatrix.GetDestinationValue((int)ModDestination::FMAmt4to1 + x)) * globalFMScale
						) * srcVoice->mAmpEnvGain;
					}

					float sl = 0;
					float sr = 0;

					for (size_t i = 0; i < gSourceCount; ++i) {
						sl += sourceValues[i] * mSourceVoices[i]->mOutputGain[0];
						sr += sourceValues[i] * mSourceVoices[i]->mOutputGain[1];
					}

					// send through aux
					//AuxDevice* const allAuxDevices = &mpOwner->mAuxDevices[0];
					sl = mAux1.ProcessSample(sl);
					sl = mAux2.ProcessSample(sl);

					switch (mpOwner->mAuxRoutingParam.mCachedVal) {
					case AuxRoute::FourZero:
						// L = aux1 -> aux2 -> aux3 -> aux4 -> *
						// R = *
						sl = mAux3.ProcessSample(sl);
						sl = mAux4.ProcessSample(sl);
						break;
					case AuxRoute::SerialMono:
						// L = aux1 -> aux2 -> aux3 -> aux4 -> *
						// R = 
						// for the sake of being pleasant this swaps l/r channels as well for continuity with other settings
						sl = mAux3.ProcessSample(sl);
						sr = mAux4.ProcessSample(sl);
						sl = 0;
						break;
					case AuxRoute::ThreeOne:
						// L = aux1 -> aux2 -> aux3 -> *
						// R = aux4 -> *
						sl = mAux3.ProcessSample(sl);
						sr = mAux4.ProcessSample(sr);
						break;
					case AuxRoute::TwoTwo:
						// L = aux1 -> aux2 -> *
						// R = aux3 -> aux4 -> *
						sr = mAux3.ProcessSample(sr);
						sr = mAux4.ProcessSample(sr);
						break;
					}

					s[0] += sl * mpOwner->mAuxOutputGains[0] + sr * mpOwner->mAuxOutputGains[1];
					s[1] += sl * mpOwner->mAuxOutputGains[1] + sr * mpOwner->mAuxOutputGains[0];
					//s[1] += mModMatrix.mDestValueDeltas[8];

					mPortamento.Advance(1,
						mModMatrix.GetDestinationValue(ModDestination::PortamentoTime)
					);
				}

				virtual void NoteOn() override {
					//mModMatrix.OnRecalcEvent();
					mVelocity01 = mNoteInfo.Velocity / 127.0f;
					mTriggerRandom01 = math::rand01();

					for (auto* p : mpAllModEnvelopes)
					{
						p->noteOn(mLegato);
					}
					for (auto& p : mLFOs) {
						p.mNode.NoteOn(mLegato);
					}

					for (auto& srcVoice : mSourceVoices)
					{
						if (srcVoice->mpSrcDevice->mKeyRangeMin.GetIntValue() > mNoteInfo.MidiNoteValue)
							continue;
						if (srcVoice->mpSrcDevice->mKeyRangeMax.GetIntValue() < mNoteInfo.MidiNoteValue)
							continue;
						srcVoice->NoteOn(mLegato);
						srcVoice->mpAmpEnv->noteOn(mLegato);
					}
					mPortamento.NoteOn((float)mNoteInfo.MidiNoteValue, !mLegato);
				}

				virtual void NoteOff() override {
					//mModMatrix.OnRecalcEvent();
					for (auto& srcVoice : mSourceVoices)
					{
						srcVoice->NoteOff();
						srcVoice->mpAmpEnv->noteOff();
					}

					for (auto* p : mpAllModEnvelopes)
					{
						p->noteOff();
					}

				}

				virtual void Kill() override {
					for (auto* p : mpAllModEnvelopes)
					{
						p->kill();
					}
					for (auto& srcVoice : mSourceVoices)
					{
						srcVoice->mpAmpEnv->kill();
					}

				}

				virtual bool IsPlaying() override {
					for (auto& srcVoice : mSourceVoices)
					{
						// if the voice is not enabled, its env won't be playing.
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




