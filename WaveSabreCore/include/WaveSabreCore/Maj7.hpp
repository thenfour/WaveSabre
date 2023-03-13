// Time sync for LFOs? use Helpers::CurrentTempo

// speed optimizations
// (use profiler)
// - some things may be calculated at the synth level, not voice level. think portamento time/curve params?
// - do not process LFO & ENV when not in use
// - recalc some a-rate things less frequently / reduce some things to k-rate. frequencies are calculated every sample...
// - cache some values like mod matrix arate source buffers
// - mod matrix currently processes every mod spec each sample. K-Rate should be done per buffer init.
//   for example a K-rate to K-rate (macro to filter freq) does not need to process every sample.
// 
// size-optimizations:
// (profile w sizebench!)
// - dont inline (use cpp)
// - don't put GetChunk() or serialization code in size-optimized code. it's not used in WS runtime.
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


//  maj7 is 8kb bigger than the other stuff.

// 4kb for adultery+slaughter+falcon
// maj7 is about 12.5 kb by itself
//case SongRenderer::DeviceId::Adultery: return new WaveSabreCore::Adultery(); // ~ 0.7 kb
//case SongRenderer::DeviceId::Specimen: return new WaveSabreCore::Specimen(); // ~ 0.9 kb
//case SongRenderer::DeviceId::Slaughter: return new WaveSabreCore::Slaughter(); // ~ 2.5 kb
//case SongRenderer::DeviceId::Falcon: return new WaveSabreCore::Falcon(); // ~ 0.8 kb
//case SongRenderer::DeviceId::Cathedral: return new WaveSabreCore::Cathedral();
//case SongRenderer::DeviceId::Chamber: return new WaveSabreCore::Chamber();
//case SongRenderer::DeviceId::Crusher: return new WaveSabreCore::Crusher();
//case SongRenderer::DeviceId::Echo: return new WaveSabreCore::Echo(); // ~ 0.7 kb (with maj7)
//case SongRenderer::DeviceId::Leveller: return new WaveSabreCore::Leveller(); // ~ 0.8 kb
// scissor is about 450 bytes

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
			const AuxLink mLinkToSelf;
			BoolParam mEnabledParam;
			EnumParam<AuxLink> mLink;
			EnumParam<AuxEffectType> mEffectType;
			float* const mParamCache_Offset;
			const int mModDestParam2ID;
			const int mBaseParamID;

			AuxLink mCurrentEffectLink;
			AuxEffectType mCurrentEffectType = AuxEffectType::None;
			IAuxEffect* mpCurrentEffect = nullptr;

			AuxNode(AuxLink selfLink, int baseParamID, float* paramCache_Offset, int modDestParam2ID) :
				mLinkToSelf(selfLink),
				mEnabledParam(paramCache_Offset[(int)AuxParamIndexOffsets::Enabled]),
				mLink(paramCache_Offset[(int)AuxParamIndexOffsets::Link], AuxLink::Count),
				mEffectType(paramCache_Offset[(int)AuxParamIndexOffsets::Type], AuxEffectType::Count),
				mCurrentEffectLink(selfLink),
				mParamCache_Offset(paramCache_Offset),
				mModDestParam2ID(modDestParam2ID),
				mBaseParamID(baseParamID)
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
				//case AuxEffectType::Distortion:
				//	return new DistortionAuxNode(mParamCache_Offset, mModDestParam2ID);
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
			static constexpr size_t gModLFOCount = 2;

			// BASE PARAMS & state
			DCFilter mDCFilters[2];

			float mAlways0 = 0;
			real_t mPitchBendN11 = 0;
			bool mDeviceModSourceValuesSetThisFrame = false;
			float mLFO1LPCutoff = 0.0f;
			float mLFO2LPCutoff = 0.0f;

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

			OscillatorDevice mOscillatorDevices[gOscillatorCount] = {
				OscillatorDevice { OscillatorIntentionAudio{}, mParamCache, &mModulations[gModulationCount - 8], ParamIndices::Osc1Enabled, ModSource::Osc1AmpEnv, ModDestination::Osc1Volume },
				OscillatorDevice { OscillatorIntentionAudio{}, mParamCache, &mModulations[gModulationCount - 7], ParamIndices::Osc2Enabled, ModSource::Osc2AmpEnv, ModDestination::Osc2Volume },
				OscillatorDevice { OscillatorIntentionAudio{}, mParamCache, &mModulations[gModulationCount - 6], ParamIndices::Osc3Enabled, ModSource::Osc3AmpEnv, ModDestination::Osc3Volume },
				OscillatorDevice { OscillatorIntentionAudio{}, mParamCache, &mModulations[gModulationCount - 5], ParamIndices::Osc4Enabled, ModSource::Osc4AmpEnv, ModDestination::Osc4Volume },
			};

			SamplerDevice mSamplerDevices[gSamplerCount] = {
				SamplerDevice{ mParamCache, &mModulations[gModulationCount - 4], ParamIndices::Sampler1Enabled, ModSource::Sampler1AmpEnv, ModDestination::Sampler1Volume },
				SamplerDevice{ mParamCache, &mModulations[gModulationCount - 3], ParamIndices::Sampler2Enabled, ModSource::Sampler2AmpEnv, ModDestination::Sampler2Volume },
				SamplerDevice{ mParamCache, &mModulations[gModulationCount - 2], ParamIndices::Sampler3Enabled, ModSource::Sampler3AmpEnv, ModDestination::Sampler3Volume },
				SamplerDevice{ mParamCache, &mModulations[gModulationCount - 1], ParamIndices::Sampler4Enabled, ModSource::Sampler4AmpEnv, ModDestination::Sampler4Volume },
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
			//real_t mOscillatorDetuneMod = 0;
			//real_t mUnisonoDetuneMod = 0;
			//real_t mOscillatorStereoSpreadMod = 0;
			//real_t mUnisonoStereoSpreadMod = 0;
			//real_t mOscillatorShapeSpreadMod = 0;
			//real_t mUnisonoShapeSpreadMod = 0;
			real_t mFMBrightnessMod = 0;
			real_t mPortamentoTimeMod = 0;
			//real_t mPortamentoCurveMod = 0;

			//float mAuxWidthMod = 0;

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

				memcpy(mParamCache + (int)ParamIndices::LFO1Waveform, gDefaultLFOParams, sizeof(gDefaultLFOParams));
				memcpy(mParamCache + (int)ParamIndices::LFO2Waveform, gDefaultLFOParams, sizeof(gDefaultLFOParams));

				for (auto& m : mOscillatorDevices) {
					memcpy(mParamCache + (int)m.mBaseParamID, gDefaultOscillatorParams, sizeof(gDefaultOscillatorParams));
				}
				for (auto& s : mSamplerDevices) {
					memcpy(mParamCache + (int)s.mBaseParamID, gDefaultSamplerParams, sizeof(gDefaultSamplerParams));
				}
				for (auto& m : mModulations) {
					memcpy(mParamCache + (int)m.mBaseParamID, gDefaultModSpecParams, sizeof(gDefaultModSpecParams));
				}
				for (auto& m : mMaj7Voice[0]->mAllAuxNodes) {
					memcpy(m->mParamCache_Offset, gDefaultAuxParams, sizeof(gDefaultAuxParams));
				}
				for (auto* p : mMaj7Voice[0]->mpAllEnvelopes) {
					memcpy(mParamCache + (int)p->mParamBaseID, gDefaultEnvelopeParams, sizeof(gDefaultEnvelopeParams));
				}

				// and correct stuff
				for (size_t i = 0; i < gSourceCount; ++i) {
					mSources[i]->InitDevice();
				}

				mMaj7Voice[0]->mAux1.mEffectType.SetEnumValue(AuxEffectType::Bitcrush);
				mMaj7Voice[0]->mAux2.mEnabledParam.SetBoolValue(true);
				mMaj7Voice[0]->mAux2.mEffectType.SetEnumValue(AuxEffectType::BigFilter);
				mMaj7Voice[0]->mAux2.mLink.SetEnumValue(AuxLink::Aux2);
				mMaj7Voice[0]->mAux3.mLink.SetEnumValue(AuxLink::Aux1);
				mMaj7Voice[0]->mAux4.mLink.SetEnumValue(AuxLink::Aux2);

				BitcrushAuxNode a1{ mParamCache + (int)ParamIndices::Aux1Enabled, 0};
				a1.mFreqParam.mKTValue.SetParamValue(1);
				a1.mFreqParam.mValue.SetParamValue(0.3f);

				FilterAuxNode f2{ mParamCache + (int)ParamIndices::Aux2Enabled, 0 };
				f2.mFilterFreqParam.mValue.SetParamValue(0.4f);
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

				s.WriteUInt32(gChunkTag);
				s.WriteUByte((uint8_t)ChunkFormat::Minified);
				s.WriteUByte(gChunkVersion);

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

				for (auto& sd : mSamplerDevices) {
					sd.Serialize(s);
				}

				auto ret = s.DetachBuffer();
				*data = ret.first;
				return (int)ret.second;
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

				// scale down per param & mod
				for (size_t i = 0; i < mVoicesUnisono; ++i) {
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
					src->BeginBlock(numSamples);
				}

				mLFO1LPCutoff = mLFO1Device.mLPFFrequency.GetFrequency(0, 0);
				mLFO2LPCutoff = mLFO2Device.mLPFFrequency.GetFrequency(0, 0);

				for (auto* v : mMaj7Voice)
				{
					v->BeginBlock(numSamples);
				}

				// apply post FX.
				mModLFO1Phase.BeginBlock(numSamples);
				mModLFO2Phase.BeginBlock(numSamples);

				// very inefficient to calculate all in the loops like that but it's for size-optimization
				for (size_t iSample = 0; iSample < numSamples; ++iSample)
				{
					float s[2] = { 0 };

					for (auto* v : mMaj7Voice)
					{
						v->ProcessAndMix(s);
					}

					for (size_t ioutput = 0; ioutput < 2; ++ioutput) {
						outputs[ioutput][iSample] = mDCFilters[ioutput].ProcessSample(s[ioutput] * mMasterVolume.GetLinearGain());
					}

					// advance phase of master LFOs
					mModLFO1Phase.ProcessSample(0, 1, 0, 0, 0, 0, 0, 0, 0, true);
					mModLFO2Phase.ProcessSample(0, 1, 0, 0, 0, 0, 0, 0, 0, true);
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
					mModEnv2(mModMatrix, ModDestination::Env2DelayTime, owner->mParamCache, (int)ParamIndices::Env2DelayTime, ModSource::ModEnv1),
					mModLFO1(&owner->mLFO1Device, mModMatrix, nullptr),
					mModLFO2(&owner->mLFO2Device, mModMatrix, nullptr),
					mOscillator1(&owner->mOscillatorDevices[0], mModMatrix, &mOsc1AmpEnv),
					mOscillator2(&owner->mOscillatorDevices[1], mModMatrix, &mOsc2AmpEnv),
					mOscillator3(&owner->mOscillatorDevices[2], mModMatrix, &mOsc3AmpEnv),
					mOscillator4(&owner->mOscillatorDevices[3], mModMatrix, &mOsc4AmpEnv),
					mSampler1(&owner->mSamplerDevices[0], mModMatrix, &mSampler1AmpEnv),
					mSampler2(&owner->mSamplerDevices[1], mModMatrix, &mSampler2AmpEnv),
					mSampler3(&owner->mSamplerDevices[2], mModMatrix, &mSampler3AmpEnv),
					mSampler4(&owner->mSamplerDevices[3], mModMatrix, &mSampler4AmpEnv),
					mAux1(AuxLink::Aux1, (int)ParamIndices::Aux1Enabled, owner->mParamCache + (int)ParamIndices::Aux1Enabled, (int)ModDestination::Aux1Param2),
					mAux2(AuxLink::Aux2, (int)ParamIndices::Aux2Enabled, owner->mParamCache + (int)ParamIndices::Aux2Enabled, (int)ModDestination::Aux2Param2),
					mAux3(AuxLink::Aux3, (int)ParamIndices::Aux3Enabled, owner->mParamCache + (int)ParamIndices::Aux3Enabled, (int)ModDestination::Aux3Param2),
					mAux4(AuxLink::Aux4, (int)ParamIndices::Aux4Enabled, owner->mParamCache + (int)ParamIndices::Aux4Enabled, (int)ModDestination::Aux4Param2)
				{
				}

				Maj7* mpOwner;

				real_t mVelocity01 = 0;
				real_t mTriggerRandom01 = 0;
				PortamentoCalc mPortamento;

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
				EnvelopeNode mModEnv1;
				EnvelopeNode mModEnv2;

				EnvelopeNode* mpAllEnvelopes[gSourceCount + gModEnvCount] {
					&mOsc1AmpEnv,
					&mOsc2AmpEnv,
					&mOsc3AmpEnv,
					&mOsc4AmpEnv,
					&mSampler1AmpEnv,
					&mSampler2AmpEnv,
					&mSampler3AmpEnv,
					&mSampler4AmpEnv,
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

				void BeginBlock(int numSamples)
				{
					if (!this->IsPlaying()) {
						return;
					}
					// at this point run the graph.
					// 1. run signals which produce A-Rate outputs, so we can use those buffers to modulate nodes which have A-Rate destinations
					// 2. mod matrix, which fills buffers with modulation signals. mod sources should be up-to-date
					// 3. run signals which are A-Rate destinations.
					// if we were really ambitious we could make the filter an A-Rate destination as well, with oscillator outputs being A-Rate.
					for (auto p : mpAllEnvelopes) {
						p->BeginBlock();
					}

					mModMatrix.InitBlock();

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

					if (!mModLFO1.mpOscDevice->mPhaseRestart.GetBoolValue()) {
						// sync phase with device-level. TODO: also check that modulations aren't creating per-voice variation?
						mModLFO1.SetPhase(mpOwner->mModLFO1Phase.mPhase);
					}

					if (!mModLFO2.mpOscDevice->mPhaseRestart.GetBoolValue()) {
						// sync phase with device-level
						mModLFO2.SetPhase(mpOwner->mModLFO2Phase.mPhase);
					}

					// set device-level modded values.
					if (!mpOwner->mDeviceModSourceValuesSetThisFrame) {
						//mpOwner->mOscillatorDetuneMod = mModMatrix.GetDestinationValue(ModDestination::OscillatorDetune);
						//mpOwner->mUnisonoDetuneMod = mModMatrix.GetDestinationValue(ModDestination::UnisonoDetune);
						//mpOwner->mOscillatorStereoSpreadMod = mModMatrix.GetDestinationValue(ModDestination::OscillatorStereoSpread);
						//mpOwner->mAuxWidthMod = mModMatrix.GetDestinationValue(ModDestination::AuxWidth);
						//mpOwner->mUnisonoStereoSpreadMod = mModMatrix.GetDestinationValue(ModDestination::UnisonoStereoSpread);
						mpOwner->mFMBrightnessMod = mModMatrix.GetDestinationValue(ModDestination::FMBrightness);
						mpOwner->mPortamentoTimeMod = mModMatrix.GetDestinationValue(ModDestination::PortamentoTime);
						//mpOwner->mPortamentoCurveMod = mModMatrix.GetDestinationValue(ModDestination::PortamentoCurve);

						mpOwner->mDeviceModSourceValuesSetThisFrame = true;
					}

					mModLFO1.BeginBlock(numSamples);
					mModLFO2.BeginBlock(numSamples);

					mLFOFilter1.SetParams(FilterModel::LP_OnePole, mpOwner->mLFO1LPCutoff, 0, 0);
					mLFOFilter2.SetParams(FilterModel::LP_OnePole, mpOwner->mLFO2LPCutoff, 0, 0);

					for (size_t i = 0; i < gAuxNodeCount; ++i) {
						mAllAuxNodes[i]->BeginBlock(numSamples, mAllAuxNodes, noteHz, mModMatrix);
					}

					for (size_t i = 0; i < gSourceCount; ++i)
					{
						mSourceVoices[i]->BeginBlock(numSamples);
					}
				}

				void ProcessAndMix(float* s)
				{
					if (!this->IsPlaying()) {
						return;
					}

					mModMatrix.ProcessSample(mpOwner->mModulations); // this sets dest values to 0.

					mModMatrix.SetSourceValue(ModSource::ModEnv1, mModEnv1.ProcessSample());
					mModMatrix.SetSourceValue(ModSource::ModEnv2, mModEnv2.ProcessSample());

					float l1 = mModLFO1.ProcessSample(0, 1, 0, 0, 0, 0, 0, 0, 0, false);
					l1 = mLFOFilter1.ProcessSample(l1);
					mModMatrix.SetSourceValue(ModSource::LFO1, l1);

					float l2 = mModLFO2.ProcessSample(0, 1, 0, 0, 0, 0, 0, 0, 0, false);
					l2 = mLFOFilter2.ProcessSample(l2);
					mModMatrix.SetSourceValue(ModSource::LFO2, l2);

					float myUnisonoDetune = mpOwner->mUnisonoDetuneAmts[this->mUnisonVoice];
					float myUnisonoPan = mpOwner->mUnisonoPanAmts[this->mUnisonVoice];

					float sourceValues[gSourceCount] = { 0 };
					float detuneMul[gSourceCount] = { 0 };

					for (size_t i = 0; i < gSourceCount; ++i)
					{
						auto* srcVoice = mSourceVoices[i];

						mModMatrix.SetSourceValue(srcVoice->mpSrcDevice->mAmpEnvModSourceID, srcVoice->mpAmpEnv->ProcessSample());

						if (!srcVoice->mpSrcDevice->mEnabledParam.GetBoolValue()) {
							continue;
						}

						float volumeMod = mModMatrix.GetDestinationValue(srcVoice->mpSrcDevice->mVolumeModDestID);

						// treat panning as added to modulation value
						float panParam = myUnisonoPan + srcVoice->mpSrcDevice->mAuxPanParam.GetN11Value(srcVoice->mpSrcDevice->mAuxPanDeviceModAmt + mModMatrix.GetDestinationValue(srcVoice->mpSrcDevice->mAuxPanModDestID)); // -1 would mean full Left, 1 is full Right.
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

						sourceValues[i] = po->ProcessSample(mMidiNote, detuneMul[i], globalFMScale,
							sourceValues[i < 1 ? 1 : 0], mpOwner->mFMMatrix.Get01Value(x, mModMatrix.GetDestinationValue((int)ModDestination::FMAmt2to1 + x)) * globalFMScale,
							sourceValues[i < 2 ? 2 : 1], mpOwner->mFMMatrix.Get01Value(1 + x, mModMatrix.GetDestinationValue((int)ModDestination::FMAmt3to1 + x)) * globalFMScale,
							sourceValues[i < 3 ? 3 : 2], mpOwner->mFMMatrix.Get01Value(2 + x, mModMatrix.GetDestinationValue((int)ModDestination::FMAmt4to1 + x)) * globalFMScale,
							false
						) * srcVoice->mAmpEnvGain;
					}

					float sl = 0;
					float sr = 0;

					for (size_t i = 0; i < gSourceCount; ++i) {
						sl += sourceValues[i] * mSourceVoices[i]->mOutputGain[0];
						sr += sourceValues[i] * mSourceVoices[i]->mOutputGain[1];
					}

					// send through aux
					sl = mAux1.ProcessSample(sl, mAllAuxNodes);
					sl = mAux2.ProcessSample(sl, mAllAuxNodes);

					switch (mpOwner->mAuxRoutingParam.GetEnumValue()) {
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

					s[0] += sl * mpOwner->mAuxOutputGains[0] + sr * mpOwner->mAuxOutputGains[1];
					s[1] += sl * mpOwner->mAuxOutputGains[1] + sr * mpOwner->mAuxOutputGains[0];

					mPortamento.Advance(1,
						mModMatrix.GetDestinationValue(ModDestination::PortamentoTime)
					);
				}

				virtual void NoteOn() override {
					mVelocity01 = mNoteInfo.Velocity / 127.0f;
					mTriggerRandom01 = math::rand01();

					for (auto* p : mpAllEnvelopes)
					{
						p->noteOn(mLegato);
					}

					for (auto& srcVoice : mSourceVoices)
					{
						if (srcVoice->mpSrcDevice->mKeyRangeMin.GetIntValue() > mNoteInfo.MidiNoteValue)
							continue;
						if (srcVoice->mpSrcDevice->mKeyRangeMax.GetIntValue() < mNoteInfo.MidiNoteValue)
							continue;
						//srcVoice->mpAmpEnv->noteOn(mLegato);
						srcVoice->NoteOn(mLegato);
					}
					//mModEnv1.noteOn(mLegato);
					//mModEnv2.noteOn(mLegato);
					//mModLFO1.NoteOn(mLegato);
					//mModLFO2.NoteOn(mLegato);
					mPortamento.NoteOn((float)mNoteInfo.MidiNoteValue, !mLegato);
				}

				virtual void NoteOff() override {
					for (auto& srcVoice : mSourceVoices)
					{
						//srcVoice->mpAmpEnv->noteOff();
						srcVoice->NoteOff();
					}
					//mModEnv1.noteOff();
					//mModEnv2.noteOff();

					for (auto* p : mpAllEnvelopes)
					{
						p->noteOff();
					}

				}

				virtual void Kill() override {
					for (auto* p : mpAllEnvelopes)
					{
						p->kill();
					}

					//for (auto& srcVoice : mSourceVoices)
					//{
					//	srcVoice->mpAmpEnv->kill();
					//}
					//mModEnv1.kill();
					//mModEnv2.kill();
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




