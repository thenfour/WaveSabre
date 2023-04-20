// Time sync for LFOs? use Helpers::CurrentTempo; but do i always know the song position?

// size-optimizations:
// (profile w sizebench!)
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
#include <WaveSabreCore/Maj7ModMatrix.hpp>

#include <WaveSabreCore/Maj7Oscillator.hpp>
#include <WaveSabreCore/Maj7Sampler.hpp>
#include <WaveSabreCore/Maj7GmDls.hpp>

#ifdef _DEBUG
#	define MAJ7_SELECTABLE_OUTPUT_STREAM_SUPPORT
#endif

//#include <Windows.h>
//#undef min
//#undef max

namespace WaveSabreCore
{
	namespace M7
	{
		// even if this doesn't strictly need to have #ifdef, better to make it clear to callers that this depends on built config.
#ifdef MAJ7_SELECTABLE_OUTPUT_STREAM_SUPPORT
		enum class OutputStream
		{
			Master,
			Osc1,
			Osc2,
			Osc3,
			Osc4,
			LFO1,
			LFO2,
			LFO3,
			LFO4,
			ModMatrix_NormRecalcSample,
			LFO1_NormRecalcSample,
			ModSource_LFO1,
			ModSource_LFO2,
			ModSource_LFO3,
			ModSource_LFO4,
			ModSource_Osc1AmpEnv,
			ModSource_Osc2AmpEnv,
			ModSource_Osc3AmpEnv,
			ModSource_Osc4AmpEnv,
			ModSource_ModEnv1,
			ModSource_ModEnv2,
			ModDest_Osc1PreFMVolume,
			ModDest_Osc2PreFMVolume,
			ModDest_Osc3PreFMVolume,
			ModDest_Osc4PreFMVolume,
			ModDest_FMFeedback1,
			ModDest_FMFeedback2,
			ModDest_FMFeedback3,
			ModDest_FMFeedback4,
			ModDest_Osc1Freq,
			ModDest_Osc2Freq,
			ModDest_Osc3Freq,
			ModDest_Osc4Freq,
			ModDest_Osc1SyncFreq,
			ModDest_Osc2SyncFreq,
			ModDest_Osc3SyncFreq,
			ModDest_Osc4SyncFreq,
			Count,
		};
#define MAJ7_OUTPUT_STREAM_CAPTIONS(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::M7::OutputStream::Count]{ \
                "Master", \
"Osc1", \
"Osc2", \
"Osc3", \
"Osc4", \
"LFO1", \
"LFO2", \
"LFO3", \
"LFO4", \
"ModMatrix_NormRecalcSample", \
"LFO1_NormRecalcSample", \
                "ModSource_LFO1", \
                "ModSource_LFO2", \
                "ModSource_LFO3", \
                "ModSource_LFO4", \
			"ModSource_Osc1AmpEnv", \
		"ModSource_Osc2AmpEnv", \
			"ModSource_Osc3AmpEnv", \
			"ModSource_Osc4AmpEnv", \
			"ModSource_ModEnv1", \
			"ModSource_ModEnv2", \
			"ModDest_Osc1PreFMVolume", \
                "ModDest_Osc2PreFMVolume", \
                "ModDest_Osc3PreFMVolume", \
                "ModDest_Osc4PreFMVolume", \
				"ModDest_FMFeedback1", \
				"ModDest_FMFeedback2", \
				"ModDest_FMFeedback3", \
				"ModDest_FMFeedback4", \
				"ModDest_Osc1Freq", \
				"ModDest_Osc2Freq", \
				"ModDest_Osc3Freq", \
				"ModDest_Osc4Freq", \
				"ModDest_Osc1SyncFreq", \
				"ModDest_Osc2SyncFreq", \
				"ModDest_Osc3SyncFreq", \
				"ModDest_Osc4SyncFreq", \
        };
#endif // MAJ7_SELECTABLE_OUTPUT_STREAM_SUPPORT

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
			ParamAccessor mParams;

			PortamentoCalc(float* paramCache) :
				mParams(paramCache, 0)
			{}

			// advances cursor; call this at the end of buffer processing. next buffer will call
			// GetCurrentMidiNote() then.
			void Advance(size_t nSamples, float timeMod) {
				if (!mEngaged) {
					return;
				}
				mSlideCursorSamples += nSamples;
				float durationSamples = math::MillisecondsToSamples(mParams.GetEnvTimeMilliseconds(ParamIndices::PortamentoTime, timeMod));// mTime.GetMilliseconds(timeMod));
				float t = mSlideCursorSamples / durationSamples;
				if (t >= 1) {
					NoteOn(mTargetNote, true); // disengages
					return;
				}
				t = mParams.ApplyCurveToValue(ParamIndices::PortamentoCurve, t, 0);// mCurve.ApplyToValue(t, 0);
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

		extern const int16_t gDefaultMasterParams[(int)MainParamIndices::Count];
		extern const int16_t gDefaultSamplerParams[(int)SamplerParamIndexOffsets::Count];
		extern const int16_t gDefaultModSpecParams[(int)ModParamIndexOffsets::Count];
		extern const int16_t gDefaultLFOParams[(int)LFOParamIndexOffsets::Count];
		extern const int16_t gDefaultEnvelopeParams[(int)EnvParamIndexOffsets::Count];
		extern const int16_t gDefaultOscillatorParams[(int)OscParamIndexOffsets::Count];
		extern const int16_t gDefaultFilterParams[(int)FilterParamIndexOffsets::Count];

		struct Maj7 : public Maj7SynthDevice
		{
			static constexpr size_t gOscillatorCount = 4;
			static constexpr size_t gFMMatrixSize = gOscillatorCount * (gOscillatorCount - 1);
			static constexpr size_t gSamplerCount = 4;
			static constexpr size_t gSourceCount = gOscillatorCount + gSamplerCount;
			//static constexpr size_t gAuxNodeCount = 4;
			static constexpr size_t gMacroCount = 7;

			static constexpr size_t gModEnvCount = 2;
			static constexpr size_t gModLFOCount = 4;

#ifdef MAJ7_SELECTABLE_OUTPUT_STREAM_SUPPORT
			OutputStream mOutputStreams[2] = {OutputStream::Master, OutputStream::Master };
#endif // MAJ7_SELECTABLE_OUTPUT_STREAM_SUPPORT

			// BASE PARAMS & state
			DCFilter mDCFilters[2];

			float mAlways0 = 0;
			real_t mPitchBendN11 = 0;

			float mUnisonoDetuneAmts[gUnisonoVoiceMax] = { 0 };

			//float mAuxOutputGains[2] = {0}; // precalc'd L/R gains based on aux panning/width [outputchannel]

			float mUnisonoPanAmts[gUnisonoVoiceMax] { 0 };

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
			ParamAccessor mParams{ mParamCache, 0 };

			// these are not REALLY lfos, they are used to track phase at the device level, for when an LFO's phase should be synced between all voices.
			// that would happen only when all of the following are satisfied:
			// 1. the LFO is not triggered by notes.
			// 2. the LFO has no modulations on its phase or frequency

			struct LFODevice
			{
				explicit LFODevice(float* paramCache, ParamIndices paramBaseID, ModDestination modBaseID) :
					mDevice{ OscillatorIntentionLFO{}, paramCache, paramBaseID, modBaseID }
				{}
				//float mLPCutoff = 0.0f;
				OscillatorDevice mDevice;
				ModMatrixNode mNullModMatrix;
				OscillatorNode mPhase{ &mDevice, &mNullModMatrix, nullptr };
			};

			LFODevice mLFOs[gModLFOCount] = {
				LFODevice{mParamCache, ParamIndices::LFO1Waveform, ModDestination::LFO1Waveshape },
				LFODevice{mParamCache, ParamIndices::LFO2Waveform, ModDestination::LFO2Waveshape },
				LFODevice{mParamCache, ParamIndices::LFO3Waveform, ModDestination::LFO3Waveshape },
				LFODevice{mParamCache, ParamIndices::LFO4Waveform, ModDestination::LFO4Waveshape },
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

			virtual void LoadDefaults() override
			{
				// samplers reset
				for (auto& s : mSamplerDevices) {
					s.Reset();
				}

				// now load in all default params
				ImportDefaultsArray(std::size(gDefaultMasterParams), gDefaultMasterParams, mParamCache);

				for (auto& m : mLFOs) {
					ImportDefaultsArray(std::size(gDefaultLFOParams), gDefaultLFOParams, m.mDevice.mParams.GetOffsetParamCache());
				}
				for (auto& m : mOscillatorDevices) {
					ImportDefaultsArray(std::size(gDefaultOscillatorParams), gDefaultOscillatorParams, m.mParams.GetOffsetParamCache());// mParamCache + (int)m.mBaseParamID);
				}
				for (auto& s : mSamplerDevices) {
					ImportDefaultsArray(std::size(gDefaultSamplerParams), gDefaultSamplerParams, s.mParams.GetOffsetParamCache());// mParamCache + (int)s.mBaseParamID);
				}
				for (auto& m : mModulations) {
					ImportDefaultsArray(std::size(gDefaultModSpecParams), gDefaultModSpecParams, m.mParams.GetOffsetParamCache());// mParamCache + (int)m.mBaseParamID);
				}
				for (auto& m : mMaj7Voice[0]->mFilters) {
					ImportDefaultsArray(std::size(gDefaultFilterParams), gDefaultFilterParams, m[0].mParams.GetOffsetParamCache());
				}
				for (auto& m : mMaj7Voice[0]->mAllEnvelopes) {
					ImportDefaultsArray(std::size(gDefaultEnvelopeParams), gDefaultEnvelopeParams, m.mParams.GetOffsetParamCache());// mParamCache + (int)p.mParams.mBaseParamID);
				}
				for (auto* p : mSources) {
					p->InitDevice();
				}

				mParamCache[(int)ParamIndices::Osc1Enabled] = 1.0f;

				// Apply dynamic state
				this->SetVoiceMode(mParams.GetEnumValue<VoiceMode>(ParamIndices::VoicingMode));// mVoicingModeParam.GetEnumValue());
				this->SetUnisonoVoices(mParams.GetIntValue(ParamIndices::Unisono, gUnisonoVoiceCfg));// mUnisonoVoicesParam.GetIntValue());
				// NOTE: samplers will always be empty here

				SetVoiceInitialStates();
			}

			// when you load params or defaults, you need to seed voice initial states so that
			// when for example a NoteOn happens, the voice will have correct values in its mod matrix.
			void SetVoiceInitialStates()
			{
				int numSamples = GetModulationRecalcSampleMask() + 1;
				float x[gModulationRecalcSampleMaskValues[0] * 2];
				memset(x, 0, sizeof(x));
				float* outputs[2] = { x, x };
				ProcessBlock(0, outputs, numSamples, true);
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
				Deserializer ds{ (const uint8_t*)data };
				SetMaj7StyleChunk(ds);
				for (auto& s : mSamplerDevices) {
					s.Deserialize(ds);
				}
				SetVoiceInitialStates();
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
						this->SetVoiceMode(mParams.GetEnumValue<VoiceMode>(ParamIndices::VoicingMode));
						break;
					}
					case (int)ParamIndices::Unisono:
					{
						this->SetUnisonoVoices(mParams.GetIntValue(ParamIndices::Unisono, gUnisonoVoiceCfg));
						break;
					}
					case (int)ParamIndices::MaxVoices:
					{
						this->SetMaxVoices(mParams.GetIntValue(ParamIndices::MaxVoices, gMaxVoicesCfg));
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
				ProcessBlock(songPosition, outputs, numSamples, false);
			}

			void ProcessBlock(double songPosition, float* const* const outputs, int numSamples, bool forceAllVoicesToProcess)
			{
				bool sourceEnabled[gSourceCount];

				for (size_t i = 0; i < gSourceCount; ++i) {
					auto* src = mSources[i];
					sourceEnabled[i] = src->IsEnabled();// mEnabledParam.GetBoolValue();
					src->BeginBlock();
				}

				for (auto& m : mModulations) {
					m.BeginBlock();
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
				//auto auxGains = math::PanToFactor(mAuxWidth.GetN11Value(/*mAuxWidthMod*/));
				//auto auxGains = math::PanToFactor(mParams.GetN11Value(ParamIndices::AuxWidth, 0));
				//mAuxOutputGains[1] = auxGains.first;
				//mAuxOutputGains[0] = auxGains.second;

				for (size_t i = 0; i < gSourceCount; ++i) {
					auto* src = mSources[i];
					// for the moment mOscDetuneAmts[i] is just a generic spread value.
					src->mAuxPanDeviceModAmt = sourceModDistribution[i] * (mParamCache[(int)ParamIndices::OscillatorSpread] /*+ mOscillatorStereoSpreadMod*/);
					src->mDetuneDeviceModAmt = sourceModDistribution[i] * (mParamCache[(int)ParamIndices::OscillatorDetune]/* + mOscillatorDetuneMod*/);
				}

				for (size_t i = 0; i < gModLFOCount; ++i) {
					auto& lfo = mLFOs[i];
					lfo.mDevice.BeginBlock();
					lfo.mPhase.BeginBlock();
				}

				for (size_t iv = 0; iv < (size_t)mMaxVoices; ++ iv)
				{
					mMaj7Voice[iv]->BeginBlock(forceAllVoicesToProcess);
				}

				// very inefficient to calculate all in the loops like that but it's for size-optimization
				float masterGain = mParams.GetLinearVolume(ParamIndices::MasterVolume, gMasterVolumeCfg, 0);// mMasterVolume.GetLinearGain();
				for (size_t iSample = 0; iSample < (size_t)numSamples; ++iSample)
				{
					float s[2] = { 0 };

					for (size_t iv = 0; iv < (size_t)mMaxVoices; ++iv)
					{
						mMaj7Voice[iv]->ProcessAndMix(s, forceAllVoicesToProcess);
					}

					for (size_t ioutput = 0; ioutput < 2; ++ioutput) {
						float o = s[ioutput];
						o *= masterGain;
						o = mDCFilters[ioutput].ProcessSample(o);
						o = math::clampN11(o);
#ifdef MAJ7_SELECTABLE_OUTPUT_STREAM_SUPPORT
						outputs[ioutput][iSample] = SelectStreamValue(mOutputStreams[ioutput], o);
#else
						outputs[ioutput][iSample] = o;
#endif // MAJ7_SELECTABLE_OUTPUT_STREAM_SUPPORT
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

#ifdef MAJ7_SELECTABLE_OUTPUT_STREAM_SUPPORT
			float SelectStreamValue(OutputStream s, float masterValue)
			{
				switch (s) {
				default:
				case OutputStream::Master:
					return masterValue;

				case OutputStream::LFO1:
					return this->mMaj7Voice[0]->mLFOs[0].mNode.GetLastSample();
				case OutputStream::LFO2:
					return this->mMaj7Voice[0]->mLFOs[1].mNode.GetLastSample();
				case OutputStream::LFO3:
					return this->mMaj7Voice[0]->mLFOs[2].mNode.GetLastSample();
				case OutputStream::LFO4:
					return this->mMaj7Voice[0]->mLFOs[3].mNode.GetLastSample();
				case OutputStream::ModMatrix_NormRecalcSample:
					return float(this->mMaj7Voice[0]->mModMatrix.mnSampleCount) / GetModulationRecalcSampleMask();
				case OutputStream::LFO1_NormRecalcSample:
					return float(this->mMaj7Voice[0]->mLFOs[0].mNode.mnSamples) / GetAudioOscillatorRecalcSampleMask();
				case OutputStream::ModSource_LFO1:
					return this->mMaj7Voice[0]->mModMatrix.GetSourceValue(ModSource::LFO1);
				case OutputStream::ModSource_LFO2:
					return this->mMaj7Voice[0]->mModMatrix.GetSourceValue(ModSource::LFO2);
				case OutputStream::ModSource_LFO3:
					return this->mMaj7Voice[0]->mModMatrix.GetSourceValue(ModSource::LFO3);
				case OutputStream::ModSource_LFO4:
					return this->mMaj7Voice[0]->mModMatrix.GetSourceValue(ModSource::LFO4);
				case OutputStream::ModDest_Osc1PreFMVolume:
					return this->mMaj7Voice[0]->mModMatrix.GetDestinationValue(ModDestination::Osc1PreFMVolume);
				case OutputStream::ModDest_Osc2PreFMVolume:
					return this->mMaj7Voice[0]->mModMatrix.GetDestinationValue(ModDestination::Osc2PreFMVolume);
				case OutputStream::ModDest_Osc3PreFMVolume:
					return this->mMaj7Voice[0]->mModMatrix.GetDestinationValue(ModDestination::Osc3PreFMVolume);
				case OutputStream::ModDest_Osc4PreFMVolume:
					return this->mMaj7Voice[0]->mModMatrix.GetDestinationValue(ModDestination::Osc4PreFMVolume);
				case OutputStream::ModDest_FMFeedback1:
					return this->mMaj7Voice[0]->mModMatrix.GetDestinationValue(ModDestination::Osc1FMFeedback);
				case OutputStream::ModDest_FMFeedback2:
					return this->mMaj7Voice[0]->mModMatrix.GetDestinationValue(ModDestination::Osc2FMFeedback);
				case OutputStream::ModDest_FMFeedback3:
					return this->mMaj7Voice[0]->mModMatrix.GetDestinationValue(ModDestination::Osc3FMFeedback);
				case OutputStream::ModDest_FMFeedback4:
					return this->mMaj7Voice[0]->mModMatrix.GetDestinationValue(ModDestination::Osc4FMFeedback);
				case OutputStream::ModDest_Osc1Freq:
					return this->mMaj7Voice[0]->mModMatrix.GetDestinationValue(ModDestination::Osc1FrequencyParam);
				case OutputStream::ModDest_Osc2Freq:
					return this->mMaj7Voice[0]->mModMatrix.GetDestinationValue(ModDestination::Osc1FrequencyParam);
				case OutputStream::ModDest_Osc3Freq:
					return this->mMaj7Voice[0]->mModMatrix.GetDestinationValue(ModDestination::Osc3FrequencyParam);
				case OutputStream::ModDest_Osc4Freq:
					return this->mMaj7Voice[0]->mModMatrix.GetDestinationValue(ModDestination::Osc4FrequencyParam);
				case OutputStream::ModDest_Osc1SyncFreq:
					return this->mMaj7Voice[0]->mModMatrix.GetDestinationValue(ModDestination::Osc1SyncFrequency);
				case OutputStream::ModDest_Osc2SyncFreq:
					return this->mMaj7Voice[0]->mModMatrix.GetDestinationValue(ModDestination::Osc2SyncFrequency);
				case OutputStream::ModDest_Osc3SyncFreq:
					return this->mMaj7Voice[0]->mModMatrix.GetDestinationValue(ModDestination::Osc3SyncFrequency);
				case OutputStream::ModDest_Osc4SyncFreq:
					return this->mMaj7Voice[0]->mModMatrix.GetDestinationValue(ModDestination::Osc4SyncFrequency);

				case OutputStream::ModSource_Osc1AmpEnv:
					return this->mMaj7Voice[0]->mModMatrix.GetSourceValue(ModSource::Osc1AmpEnv);
				case OutputStream::ModSource_Osc2AmpEnv:
					return this->mMaj7Voice[0]->mModMatrix.GetSourceValue(ModSource::Osc2AmpEnv);
				case OutputStream::ModSource_Osc3AmpEnv:
					return this->mMaj7Voice[0]->mModMatrix.GetSourceValue(ModSource::Osc3AmpEnv);
				case OutputStream::ModSource_Osc4AmpEnv:
					return this->mMaj7Voice[0]->mModMatrix.GetSourceValue(ModSource::Osc4AmpEnv);
				case OutputStream::ModSource_ModEnv1:
					return this->mMaj7Voice[0]->mModMatrix.GetSourceValue(ModSource::ModEnv1);
				case OutputStream::ModSource_ModEnv2:
					return this->mMaj7Voice[0]->mModMatrix.GetSourceValue(ModSource::ModEnv2);

				case OutputStream::Osc1:
					return this->mMaj7Voice[0]->mOscillator1.GetLastSample();
				case OutputStream::Osc2:
					return this->mMaj7Voice[0]->mOscillator2.GetLastSample();
				case OutputStream::Osc3:
					return this->mMaj7Voice[0]->mOscillator3.GetLastSample();
				case OutputStream::Osc4:
					return this->mMaj7Voice[0]->mOscillator4.GetLastSample();

				}
			}
#endif // MAJ7_SELECTABLE_OUTPUT_STREAM_SUPPORT


			struct Maj7Voice : public Maj7SynthDevice::Voice
			{
				Maj7Voice(Maj7* owner) :
					mpOwner(owner),
					mPortamento(owner->mParamCache),
					mAllEnvelopes{
						/*mOsc1AmpEnv*/EnvelopeNode(mModMatrix, ModDestination::Osc1AmpEnvDelayTime, owner->mParamCache, (int)ParamIndices::Osc1AmpEnvDelayTime, ModSource::Osc1AmpEnv),
						/*mOsc2AmpEnv*/EnvelopeNode(mModMatrix, ModDestination::Osc2AmpEnvDelayTime, owner->mParamCache, (int)ParamIndices::Osc2AmpEnvDelayTime, ModSource::Osc2AmpEnv),
						/*mOsc3AmpEnv*/EnvelopeNode(mModMatrix, ModDestination::Osc3AmpEnvDelayTime, owner->mParamCache, (int)ParamIndices::Osc3AmpEnvDelayTime, ModSource::Osc3AmpEnv),
						/*mOsc4AmpEnv*/EnvelopeNode(mModMatrix, ModDestination::Osc4AmpEnvDelayTime, owner->mParamCache, (int)ParamIndices::Osc4AmpEnvDelayTime, ModSource::Osc4AmpEnv),
						/*mSampler1AmpEnv*/EnvelopeNode(mModMatrix, ModDestination::Sampler1AmpEnvDelayTime, owner->mParamCache, (int)ParamIndices::Sampler1AmpEnvDelayTime, ModSource::Sampler1AmpEnv),
						/*mSampler2AmpEnv*/EnvelopeNode(mModMatrix, ModDestination::Sampler2AmpEnvDelayTime, owner->mParamCache, (int)ParamIndices::Sampler2AmpEnvDelayTime, ModSource::Sampler2AmpEnv),
						/*mSampler3AmpEnv*/EnvelopeNode(mModMatrix, ModDestination::Sampler3AmpEnvDelayTime, owner->mParamCache, (int)ParamIndices::Sampler3AmpEnvDelayTime, ModSource::Sampler3AmpEnv),
						/*mSampler4AmpEnv*/EnvelopeNode(mModMatrix, ModDestination::Sampler4AmpEnvDelayTime, owner->mParamCache, (int)ParamIndices::Sampler4AmpEnvDelayTime, ModSource::Sampler4AmpEnv),
						/*mModEnv1*/EnvelopeNode(mModMatrix, ModDestination::Env1DelayTime, owner->mParamCache, (int)ParamIndices::Env1DelayTime, ModSource::ModEnv1),
						/*mModEnv2*/EnvelopeNode(mModMatrix, ModDestination::Env2DelayTime, owner->mParamCache, (int)ParamIndices::Env2DelayTime, ModSource::ModEnv2),
					},
					mOscillator1(&owner->mOscillatorDevices[0], &mModMatrix, &mAllEnvelopes[0]),
					mOscillator2(&owner->mOscillatorDevices[1], &mModMatrix, &mAllEnvelopes[1]),
					mOscillator3(&owner->mOscillatorDevices[2], &mModMatrix, &mAllEnvelopes[2]),
					mOscillator4(&owner->mOscillatorDevices[3], &mModMatrix, &mAllEnvelopes[3]),
					mSampler1(&owner->mSamplerDevices[0], mModMatrix, &mAllEnvelopes[4]),
					mSampler2(&owner->mSamplerDevices[1], mModMatrix, &mAllEnvelopes[5]),
					mSampler3(&owner->mSamplerDevices[2], mModMatrix, &mAllEnvelopes[6]),
					mSampler4(&owner->mSamplerDevices[3], mModMatrix, &mAllEnvelopes[7]),
					mFilters{
						{FilterAuxNode(owner->mParamCache, ParamIndices::Filter1Enabled, ModDestination::Filter1Q),
						FilterAuxNode(owner->mParamCache, ParamIndices::Filter1Enabled, ModDestination::Filter1Q),
						},
						{
						FilterAuxNode(owner->mParamCache, ParamIndices::Filter2Enabled, ModDestination::Filter2Q),
						FilterAuxNode(owner->mParamCache, ParamIndices::Filter2Enabled, ModDestination::Filter2Q),
						}
					}
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
						mNode(&device.mDevice, &modMatrix, nullptr)
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

				ModMatrixNode mModMatrix;

				OscillatorNode mOscillator1;
				OscillatorNode mOscillator2;
				OscillatorNode mOscillator3;
				OscillatorNode mOscillator4;
				SamplerVoice mSampler1;
				SamplerVoice mSampler2;
				SamplerVoice mSampler3;
				SamplerVoice mSampler4;

				EnvelopeNode mAllEnvelopes[gSourceCount + gModEnvCount];
				static constexpr size_t Osc1AmpEnvIndex = 0;
				static constexpr size_t Osc2AmpEnvIndex = 1;
				static constexpr size_t Osc3AmpEnvIndex = 2;
				static constexpr size_t Osc4AmpEnvIndex = 3;
				static constexpr size_t Sampler1AmpEnvIndex = 4;
				static constexpr size_t Sampler2AmpEnvIndex = 5;
				static constexpr size_t Sampler3AmpEnvIndex = 6;
				static constexpr size_t Sampler4AmpEnvIndex = 7;
				static constexpr size_t ModEnv1Index = 8;
				static constexpr size_t ModEnv2Index = 9;

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

				FilterAuxNode mFilters[2][2]; // [filtercount][channel]

				float mMidiNote = 0;

				void BeginBlock(bool forceProcessing)
				{
					for (auto& p : mAllEnvelopes) {
						p.BeginBlock();
					}

					if (!forceProcessing && !this->IsPlaying()) {
						return;
					}

					// we know we are a valid and playing voice; set master LFO to use this mod matrix.
					for (auto& lfo : this->mpOwner->mLFOs) {
						lfo.mPhase.SetModMatrix(&this->mModMatrix);
					}

					mMidiNote = mPortamento.GetCurrentMidiNote() + mpOwner->mParams.GetIntValue(ParamIndices::PitchBendRange, gPitchBendCfg) * mpOwner->mPitchBendN11;

					real_t noteHz = math::MIDINoteToFreq(mMidiNote);

					mModMatrix.BeginBlock();
					mModMatrix.SetSourceValue(ModSource::PitchBend, mpOwner->mPitchBendN11);
					mModMatrix.SetSourceValue(ModSource::Velocity, mVelocity01);
					mModMatrix.SetSourceValue(ModSource::NoteValue, mMidiNote / 127.0f);
					mModMatrix.SetSourceValue(ModSource::RandomTrigger, mTriggerRandom01);
					float iuv = 1;
					if (mpOwner->mVoicesUnisono > 1) {
						iuv = float(mUnisonVoice) / (mpOwner->mVoicesUnisono - 1);
					}
					mModMatrix.SetSourceValue(ModSource::UnisonoVoice, iuv);
					mModMatrix.SetSourceValue(ModSource::SustainPedal, real_t(mpOwner->mIsPedalDown ? 0 : 1)); // krate, 01

					mModMatrix.SetSourceValue(ModSource::Const_1, 1);
					mModMatrix.SetSourceValue(ModSource::Const_0_5, 0.5f);
					mModMatrix.SetSourceValue(ModSource::Const_0, 0);
					mModMatrix.SetSourceValue(ModSource::Const_N0_5, -0.5f);
					mModMatrix.SetSourceValue(ModSource::Const_N1, -1);

					for (size_t iMacro = 0; iMacro < gMacroCount; ++iMacro)
					{
						mModMatrix.SetSourceValue((int)ModSource::Macro1 + iMacro, mpOwner->mParams.Get01Value((int)ParamIndices::Macro1 + iMacro, 0));  // krate, 01
					}

					for (size_t i = 0; i < gModLFOCount; ++i)
					{
						auto& lfo = mLFOs[i];
						if (!lfo.mNode.mpOscDevice->GetPhaseRestart()) {
							// sync phase with device-level. TODO: also check that modulations aren't creating per-voice variation?
							lfo.mNode.SetPhase(lfo.mDevice.mPhase.mPhase);
						}
						lfo.mNode.BeginBlock();

						auto freq = lfo.mDevice.mDevice.mParams.GetFrequency(LFOParamIndexOffsets::Sharpness, -1, gLFOLPFreqConfig, 0,
							mModMatrix.GetDestinationValue((int)lfo.mDevice.mDevice.mModDestBaseID + (int)LFOModParamIndexOffsets::Sharpness));

						lfo.mFilter.SetParams(FilterModel::LP_OnePole, freq, 0);
					}

					// set device-level modded values.
					mpOwner->mFMBrightnessMod = mModMatrix.GetDestinationValue(ModDestination::FMBrightness);
					mpOwner->mPortamentoTimeMod = mModMatrix.GetDestinationValue(ModDestination::PortamentoTime);

					for (auto& a : mFilters)
					{
						a[0].AuxBeginBlock(noteHz, mModMatrix);
						a[1].AuxBeginBlock(noteHz, mModMatrix);
					}

					float myUnisonoPan = mpOwner->mUnisonoPanAmts[this->mUnisonVoice];

					for (size_t i = 0; i < gSourceCount; ++i)
					{
						auto* srcVoice = mSourceVoices[i];

						float volumeMod = mModMatrix.GetDestinationValue(srcVoice->mpSrcDevice->mVolumeModDestID);

						// treat panning as added to modulation value
						float panParam = myUnisonoPan +
							srcVoice->mpSrcDevice->GetAuxPan() + //>mAuxPanParam.mCachedVal +
							srcVoice->mpSrcDevice->mAuxPanDeviceModAmt +
							mModMatrix.GetDestinationValue(srcVoice->mpSrcDevice->mAuxPanModDestID); // -1 would mean full Left, 1 is full Right.
						auto panGains = math::PanToFactor(panParam);
						float outputVolLin = srcVoice->mpSrcDevice->GetLinearVolume(volumeMod);
						srcVoice->mOutputGain[0] = outputVolLin * panGains.first;
						srcVoice->mOutputGain[1] = outputVolLin * panGains.second;

						srcVoice->BeginBlock();
						srcVoice->mpAmpEnv->BeginBlock();
					}
				}

				void ProcessAndMix(float* s, bool forceProcessing)
				{
					// NB: process envelopes before short-circuiting due to being not playing.
					// this is for issue#31; mod envelopes need to be able to release down to 0 even when the source envs are not playing.
					// if mod envs get suspended rudely, then they'll "wake up" at the wrong value.
					for (auto& env : mAllEnvelopes)
					{
						float l = env.ProcessSample();
						mModMatrix.SetSourceValue(env.mMyModSource, l);
					}

					if (!forceProcessing && !this->IsPlaying()) {
						return;
					}

					for (size_t i = 0; i < gModLFOCount; ++i) {
						auto& lfo = mLFOs[i];
						float s = lfo.mNode.ProcessSampleForLFO(false);
						s = lfo.mFilter.ProcessSample(s);
						mModMatrix.SetSourceValue(lfo.mModSourceID, s);
					}

					for (size_t i = 0; i < gSourceCount; ++i)
					{
						auto* srcVoice = mSourceVoices[i];

						if (!srcVoice->mpSrcDevice->IsEnabled()) {
							srcVoice->mpAmpEnv->kill();
							continue;
						}
					}

					// process modulations here. sources have just been set, and past here we're getting many destination values.
					// processing here ensures fairly up-to-date accurate values.
					// one area where this is quite sensitive is envelopes with instant attacks/releases
					mModMatrix.ProcessSample(mpOwner->mModulations); // this sets dest values to 0.

					float myUnisonoDetune = mpOwner->mUnisonoDetuneAmts[this->mUnisonVoice];

					float sourceValues[gSourceCount] = { 0 };
					float detuneMul[gSourceCount] = { 0 };

					for (size_t i = 0; i < gSourceCount; ++i)
					{
						auto* srcVoice = mSourceVoices[i];

						float hiddenVolumeBacking = mModMatrix.GetDestinationValue(srcVoice->mpSrcDevice->mHiddenVolumeModDestID);
						ParamAccessor hiddenAmpParam{ &hiddenVolumeBacking, 0 };

						float ampEnvGain = hiddenAmpParam.GetLinearVolume(0, gUnityVolumeCfg);
						srcVoice->mAmpEnvGain = ampEnvGain;
						sourceValues[i] = srcVoice->GetLastSample() * ampEnvGain;

						float semis = myUnisonoDetune + srcVoice->mpSrcDevice->mDetuneDeviceModAmt * 2;
						float det = detuneMul[i] = math::SemisToFrequencyMul(semis);

						if (i >= gOscillatorCount)
						{
							auto ps = static_cast<SamplerVoice*>(srcVoice);
							sourceValues[i] = ps->ProcessSample(mMidiNote, det, 0) * ampEnvGain;
						}
					}

					float globalFMScale = 3 * mpOwner->mParams.Get01Value(ParamIndices::FMBrightness, mpOwner->mFMBrightnessMod);

					for (size_t i = 0; i < gOscillatorCount; ++i) {
						auto* srcVoice = mSourceVoices[i];
						auto po = static_cast<OscillatorNode*>(srcVoice);
						const size_t x = i * (gOscillatorCount - 1);

						auto matrixVal1 = mpOwner->mParams.Get01Value((int)ParamIndices::FMAmt2to1 + x, mModMatrix.GetDestinationValue((int)ModDestination::FMAmt2to1 + x));
						auto matrixVal2 = mpOwner->mParams.Get01Value((int)ParamIndices::FMAmt2to1 + x + 1, mModMatrix.GetDestinationValue((int)ModDestination::FMAmt3to1 + x));
						auto matrixVal3 = mpOwner->mParams.Get01Value((int)ParamIndices::FMAmt2to1 + x + 2, mModMatrix.GetDestinationValue((int)ModDestination::FMAmt4to1 + x));

						sourceValues[i] = po->ProcessSampleForAudio(mMidiNote, detuneMul[i], globalFMScale,
							sourceValues[i < 1 ? 1 : 0], matrixVal1 * globalFMScale,
							sourceValues[i < 2 ? 2 : 1], matrixVal2 * globalFMScale,
							sourceValues[i < 3 ? 3 : 2], matrixVal3 * globalFMScale
						) * srcVoice->mAmpEnvGain;

					}

					float q[2] = { 0 };

					for (size_t i = 0; i < gSourceCount; ++i)
					{
						for (size_t ich = 0; ich < 2; ++ich)
						{
							q[ich] += sourceValues[i] * mSourceVoices[i]->mOutputGain[ich];
						}
					}

					// send through serial filter
					for (size_t ich = 0; ich < 2; ++ich)
					{
						q[ich] = mFilters[0][ich].AuxProcessSample(q[ich]);
						q[ich] = mFilters[1][ich].AuxProcessSample(q[ich]);
					}

					s[0] += q[0];// *mpOwner->mAuxOutputGains[0] + q[1] * mpOwner->mAuxOutputGains[1];
					s[1] += q[1];// *mpOwner->mAuxOutputGains[1] + q[1] * mpOwner->mAuxOutputGains[0];

					mPortamento.Advance(1,
						mModMatrix.GetDestinationValue(ModDestination::PortamentoTime)
					);
				}

				virtual void NoteOn() override
				{
					mVelocity01 = mNoteInfo.Velocity / 127.0f;
					mTriggerRandom01 = math::rand01();

					mAllEnvelopes[ModEnv1Index].noteOn(mLegato);
					mAllEnvelopes[ModEnv2Index].noteOn(mLegato);

					for (auto& p : mLFOs) {
						p.mNode.NoteOn(mLegato);
					}

					for (auto& srcVoice : mSourceVoices)
					{
						if (!srcVoice->mpSrcDevice->MatchesKeyRange(mNoteInfo.MidiNoteValue))
							continue;
						srcVoice->NoteOn(mLegato);
						srcVoice->mpAmpEnv->noteOn(mLegato);
					}
					mPortamento.NoteOn((float)mNoteInfo.MidiNoteValue, !mLegato);
				}

				virtual void NoteOff() override {
					for (auto& srcVoice : mSourceVoices)
					{
						srcVoice->NoteOff();
					}

					for (auto& p : mAllEnvelopes)
					{
						p.noteOff();
					}
				}

				virtual void Kill() override {
					for (auto& p : mAllEnvelopes)
					{
						p.kill();
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

			struct Maj7Voice* mMaj7Voice[gMaxMaxVoices] = { 0 };

		};
	} // namespace M7

	using Maj7 = M7::Maj7;

} // namespace WaveSabreCore




