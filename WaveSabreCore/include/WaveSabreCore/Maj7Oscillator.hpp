#pragma once

#include <WaveSabreCore/Maj7Basic.hpp>
#include <WaveSabreCore/Maj7ModMatrix.hpp>

namespace WaveSabreCore
{
	namespace M7
	{
		static constexpr real_t gSourceFrequencyCenterHz = 1000;
		static constexpr real_t gSourceFrequencyScale = 10;
		static constexpr int gSourcePitchSemisRange = 36;
		static constexpr float gSourcePitchFineRangeSemis = 2;
		static constexpr real_t gLFOFrequencyCenterHz = 3.0f;
		static constexpr real_t gLFOFrequencyScale = 6;

		// sampler, oscillator, LFO @ device level
		struct ISoundSourceDevice
		{
			ParamIndices mBaseParamID;
			ParamIndices mEnabledParamID;
			ParamIndices mVolumeParamID;
			ParamIndices mAuxPanParamID;

			ParamIndices mTuneSemisParamID;
			ParamIndices mTuneFineParamID;
			ParamIndices mFreqParamID;
			ParamIndices mFreqKTParamID;
			ParamIndices mKeyRangeMinParamID;
			ParamIndices mKeyRangeMaxParamID;

			ModSource mAmpEnvModSourceID;
			ModDestination mModDestBaseID;
			ModDestination mVolumeModDestID;
			ModDestination mAuxPanModDestID;
			ModDestination mHiddenVolumeModDestID;

			BoolParam mEnabledParam;
			VolumeParam mVolumeParam;
			FloatN11Param mAuxPanParam;
			FrequencyParam mFrequencyParam;
			IntParam mPitchSemisParam;
			FloatN11Param mPitchFineParam;

			IntParam mKeyRangeMin;
			IntParam mKeyRangeMax;

			ModulationSpec* mAmpEnvModulation;

			// device-level values set at the beginning of block processing
			float mDetuneDeviceModAmt = 0;
			float mAuxPanDeviceModAmt = 0;
			float mShapeDeviceModAmt = 0;

			// LFO backing values (they don't have params)
			float mLFOVolumeParamValue = 1.0f;
			float mLFOAuxPanBacking = 0;
			float mLFOPitchSemisParamValue = 0.5f;
			float mLFOPitchFineParamValue = 0.5f;
			float mLFOEnabledBacking = 1;
			float mLFOVolumeBacking = 1;
			float mLFOFreqKTBacking = 0;
			float mLFOKeyRangeMinBacking = 0;
			float mLFOKeyRangeMaxBacking = 0;

			virtual ~ISoundSourceDevice()
			{}

			ISoundSourceDevice(float* paramCache, ModulationSpec* ampEnvModulation,
				ParamIndices baseParamID, ParamIndices enabledParamID, ParamIndices volumeParamID, ParamIndices auxPanParamID,
				ParamIndices tuneSemisParamID, ParamIndices tuneFineParamID, ParamIndices freqParamID, ParamIndices freqKTParamID, ParamIndices keyRangeMinParamID, ParamIndices keyRangeMaxParamID,
				ModSource ampEnvModSourceID, ModDestination modDestBaseID, ModDestination volumeModDestID, ModDestination auxPanModDestID, ModDestination hiddenVolumeModDestID
			) :

				mAmpEnvModulation(ampEnvModulation),
				mBaseParamID(baseParamID),
				mEnabledParamID(enabledParamID),
				mVolumeParamID(volumeParamID),
				mAuxPanParamID(auxPanParamID),
				mTuneSemisParamID(tuneSemisParamID),
				mTuneFineParamID(tuneFineParamID), mFreqParamID(freqParamID), mFreqKTParamID(freqKTParamID),
				mKeyRangeMinParamID(keyRangeMinParamID),
				mKeyRangeMaxParamID(keyRangeMaxParamID),
				mAmpEnvModSourceID(ampEnvModSourceID),
				mVolumeModDestID(volumeModDestID),
				mAuxPanModDestID(auxPanModDestID),
				mModDestBaseID(modDestBaseID),
				mHiddenVolumeModDestID(hiddenVolumeModDestID),

				mEnabledParam(paramCache[(int)enabledParamID], false),
				mVolumeParam(paramCache[(int)volumeParamID], 0, 1),
				mAuxPanParam(paramCache[(int)auxPanParamID], 0),
				mFrequencyParam(paramCache[(int)freqParamID], paramCache[(int)freqKTParamID], gSourceFrequencyCenterHz, gSourceFrequencyScale, 0.4f, 1.0f),
				mPitchSemisParam(paramCache[(int)tuneSemisParamID], -gSourcePitchSemisRange, gSourcePitchSemisRange, 0),
				mPitchFineParam(paramCache[(int)tuneFineParamID], 0),
				mKeyRangeMin(paramCache[(int)keyRangeMinParamID], 0, 127, 0),
				mKeyRangeMax(paramCache[(int)keyRangeMaxParamID], 0, 127, 127)
			{
			}

			// for LFOs
			ISoundSourceDevice(float* paramCache,
				ParamIndices baseParamID, ParamIndices freqParamID,
				ModDestination modDestBaseID
			) :
				mAmpEnvModulation(nullptr),
				mBaseParamID(baseParamID),
				mEnabledParamID(ParamIndices::Invalid),
				mVolumeParamID(ParamIndices::Invalid),
				mAuxPanParamID(ParamIndices::Invalid),
				mTuneSemisParamID(ParamIndices::Invalid),
				mTuneFineParamID(ParamIndices::Invalid),
				mFreqParamID(freqParamID),
				mFreqKTParamID(ParamIndices::Invalid),
				mKeyRangeMinParamID(ParamIndices::Invalid),
				mKeyRangeMaxParamID(ParamIndices::Invalid),
				mAmpEnvModSourceID(ModSource::Invalid),
				mVolumeModDestID(ModDestination::Invalid),
				mAuxPanModDestID(ModDestination::Invalid),
				mModDestBaseID(modDestBaseID),
				mHiddenVolumeModDestID(ModDestination::Invalid),
				mEnabledParam(mLFOEnabledBacking, true),
				mVolumeParam(mLFOVolumeBacking, 0, 1),
				mAuxPanParam(mLFOAuxPanBacking, 0),
				mFrequencyParam(paramCache[(int)freqParamID], mLFOFreqKTBacking, gLFOFrequencyCenterHz, gLFOFrequencyScale, 0.4f, 0),
				mPitchSemisParam(mLFOPitchSemisParamValue, -gSourcePitchSemisRange, gSourcePitchSemisRange, 0),
				mPitchFineParam(mLFOPitchFineParamValue, 0),
				mKeyRangeMin(mLFOKeyRangeMinBacking, 0, 127, 0),
				mKeyRangeMax(mLFOKeyRangeMaxBacking, 0, 127, 127)
			{
			}

			void InitDevice() {
				mAmpEnvModulation->mEnabled.SetBoolValue(true);
				mAmpEnvModulation->mSource.SetEnumValue(mAmpEnvModSourceID);
				mAmpEnvModulation->mDestination.SetEnumValue(mHiddenVolumeModDestID);
				mAmpEnvModulation->mScale.SetN11Value(1);
				mAmpEnvModulation->mType = ModulationSpecType::SourceAmp;
			}

			virtual void BeginBlock(int samplesInBlock) = 0;

			struct Voice
			{
				ISoundSourceDevice* mpSrcDevice;
				ModMatrixNode& mModMatrix;
				EnvelopeNode* mpAmpEnv;

				Voice(ISoundSourceDevice* psrcDevice, ModMatrixNode& modMatrix, EnvelopeNode* pAmpEnv) :
					mpSrcDevice(psrcDevice),
					mModMatrix(modMatrix),
					mpAmpEnv(pAmpEnv)
				{}

				virtual ~Voice()
				{}

				// used as temporary values during block processing.
				float mOutputGain[2] = { 0 }; // linear output volume gain calculated from output VolumeParam + panning
				float mAmpEnvGain = { 0 }; // linear gain calculated frequently from osc ampenv

				virtual void NoteOn(bool legato) = 0;
				virtual void NoteOff() = 0;
				virtual void BeginBlock(real_t midiNote, float voiceShapeMod, float detuneFreqMul, float fmScale, int samplesInBlock) = 0;
			};
		};

		enum class OscillatorWaveform : uint8_t
		{
			SawClip,
			Pulse,
			SineClip,
			SineHarmTrunc,
			WhiteNoiseSH,
			Count,
		};
		// size-optimize using macro
		#define OSCILLATOR_WAVEFORM_CAPTIONS(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::M7::OscillatorWaveform::Count] { \
            "SawClip", \
			"Pulse", \
			"SineClip", \
            "SineHarmTrunc", \
            "WhiteNoiseSH", \
        }

		/////////////////////////////////////////////////////////////////////////////
		struct IOscillatorWaveform
		{
			float mShape = 0.5; // wave shape
			float mPhaseOffset = 0;

			float mDCOffset = 0; // while we could try to bake our wave generator with correct DC, it's very inconvenient. This is just way easier.
			float mScale = 1; // same here; way easier to just scale in post than to try and get everything scaled correctly during generation.

			float mFrequency = 0;
			double mPhase = 0; // phase cursor 0-1
			double mDTDT = 0; // to glide to a new frequency smoothly over a block.
			double mPhaseIncrement = 0; // dt
			float mSampleRate = 0;

			virtual float NaiveSample(float phase01) = 0; // return amplitude at phase
			virtual float NaiveSampleSlope(float phase01) = 0; // return slope at phase			
			virtual std::pair<float, float> OSC_ADVANCE(float samples, float samplesTillNextSample) = 0;// returns blep before and blep after discontinuity.

			// override if you need to adjust things
			virtual void SetParams(float freq, float phaseOffsetN11, float waveshape, float sampleRate, int samplesInBlock)
			{
				mFrequency = freq;
				mPhaseOffset = Fract(phaseOffsetN11);
				mShape = waveshape;
				mSampleRate = sampleRate;

				double newPhaseInc = (double)mFrequency / sampleRate;
				mDTDT = (newPhaseInc - mPhaseIncrement) / samplesInBlock;
				//mPhaseIncrement = newPhaseInc;
				//mDTDT = 0;
			}

			// process discontinuity due to restarting phase right now.
			// returns blep before and blep after discontinuity.
			virtual std::pair<float, float> OSC_RESTART(float samplesBeforeNext)
			{
				float sampleBefore = this->NaiveSample((float)this->mPhase);
				double newPhase = this->mPhaseOffset;

				// fix bleps
				float sampleAfter = NaiveSample((float)newPhase);
				float blepScale = (sampleAfter - sampleBefore) * .5f; // full sample scale is 2; *.5 to bring 0-1

				float blepBefore = blepScale * BlepBefore(samplesBeforeNext); // blep the phase restart.
				float blepAfter = blepScale * BlepAfter(samplesBeforeNext);

				// fix blamps.
				float slopeBefore = NaiveSampleSlope((float)mPhase);
				float slopeAfter = NaiveSampleSlope((float)newPhase);
				float blampScale = float(this->mPhaseIncrement * (slopeAfter - slopeBefore));
				blepBefore += blampScale * BlampBefore(samplesBeforeNext);
				blepAfter += blampScale * BlampAfter(samplesBeforeNext);

				mPhase = newPhase;

				return std::make_pair(blepBefore, blepAfter);
			}

			void OSC_ACCUMULATE_BLEP(std::pair<float, float>& bleps, double newPhase, float edge, float blepScale, float samples, float samplesFromNewPositionUntilNextSample)
			{
				if (!DoesEncounter(mPhase, newPhase, edge))
					return;
				float samplesSinceEdge = float(Fract(newPhase - edge) / this->mPhaseIncrement);
				float samplesFromEdgeToNextSample = Fract(samplesSinceEdge + samplesFromNewPositionUntilNextSample);
				bleps.first = blepScale * BlepBefore(samplesFromEdgeToNextSample);
				bleps.second = blepScale * BlepAfter(samplesFromEdgeToNextSample);
			}

			void OSC_ACCUMULATE_BLAMP(std::pair<float, float>& bleps, double newPhase, float edge, float blampScale, float samples, float samplesFromNewPositionUntilNextSample)
			{
				if (!DoesEncounter((mPhase), (newPhase), edge))
					return;

				float samplesSinceEdge = float(Fract(newPhase - edge) / float(this->mPhaseIncrement));
				float samplesFromEdgeToNextSample = Fract(samplesSinceEdge + samplesFromNewPositionUntilNextSample);

				bleps.first += blampScale * BlampBefore(samplesFromEdgeToNextSample);
				bleps.second += blampScale * BlampAfter(samplesFromEdgeToNextSample);
			}
		};

		/////////////////////////////////////////////////////////////////////////////
		struct SawClipWaveform :IOscillatorWaveform
		{
			float mFlatValue = 0;

			// returns Y value at specified phase. instance / stateless.
			virtual float NaiveSample(float phase01) override
			{
				if (phase01 < mShape) {
					return mShape * 2 - 1;
				}
				return phase01 * 2 - 1;
			}

			virtual float NaiveSampleSlope(float phase01) override
			{
				if (phase01 < mShape) {
					return 0;
				}
				return 1;
			}

			virtual void SetParams(float freq, float phaseOffset, float waveshape, float sampleRate, int samplesInBlock) override
			{
				IOscillatorWaveform::SetParams(freq, phaseOffset, waveshape, sampleRate, samplesInBlock);

				waveshape = 1 - waveshape;
				waveshape = 1 - (waveshape * waveshape);
				mShape = Clamp(waveshape * 0.9f, 0, 1);

				mFlatValue = mShape;

				mDCOffset = -mShape; // offset so we can scale to fill both axes
				mScale = 1 / (1.0f - mShape); // scale it up so it fills both axes
			}

			// samples is 0<samples<1
			// assume this.phase is currently 0<t<1
			// this.phase may not be on a sample boundary.
			// returns blep before and blep after discontinuity.
			virtual std::pair<float, float> OSC_ADVANCE(float samples, float samplesTillNextSample) override
			{
				mPhaseIncrement += mDTDT * samples;
				double phaseToAdvance = samples * mPhaseIncrement;
				double newPhase = Fract(mPhase + phaseToAdvance); // advance slave; doing it here helps us calculate discontinuity.

				std::pair<float, float> bleps{ 0.0f,0.0f };
				float blampScale = float(mPhaseIncrement);
				float blepScale = -(1.0f - mShape);

				OSC_ACCUMULATE_BLEP(bleps, newPhase, 0/*edge*/, blepScale, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLAMP(bleps, newPhase, 0/*edge*/, -blampScale, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLAMP(bleps, newPhase, this->mShape/*edge*/, blampScale, samples, samplesTillNextSample);

				this->mPhase = newPhase;
				return bleps;
			}
		};

		/////////////////////////////////////////////////////////////////////////////
		struct SineClipWaveform :IOscillatorWaveform
		{
			float mEdge1;
			float mEdge2;
			float mFlatValue;
			// returns Y value at specified phase. instance / stateless.
			virtual float NaiveSample(float phase01) override
			{
				if ((phase01 >= mEdge1) && (phase01 < mEdge2)) {
					return mFlatValue;
				}
				return math::sin(math::gPITimes2 * phase01);
				//(phase01 < this.edge1) ? (
				//	  sin($pi * 2 * phase01)
				//	) : (phase01 < this.edge2) ? (
				//		this.flatValue; // the Y value that gets sustained
				//  ) : (
				//	sin($pi * 2 * phase01)
				//	);
			}

			// this is not improved by returing correct slope. blepping curves is too hard 4 me.
			virtual float NaiveSampleSlope(float phase01) override
			{
				if ((phase01 >= mEdge1) && (phase01 < mEdge2)) {
					return 0;
				}
				return math::cos(math::gPITimes2 * phase01);
				//(phase01 < this.edge1) ? (
				//  cos($pi * 2 * phase01) * $pi * 2
				//) : (phase01 < this.edge2) ? (
				//  0; // the Y value that gets sustained
				//) : (
				//  cos($pi * 2 * phase01) * $pi * 2
				//);
			}

			virtual void SetParams(float freq, float phaseOffset, float waveshape, float sampleRate, int samplesInBlock) override
			{
				IOscillatorWaveform::SetParams(freq, phaseOffset, waveshape, sampleRate, samplesInBlock);

				mEdge1 = (0.75f - mShape * .25f);
				mEdge2 = (0.75f + mShape * .25f);
				mFlatValue = math::sin(mEdge1 * 2 * math::gPI); // the Y value that gets sustained

				mDCOffset = -(.5f + .5f * mFlatValue); // offset so we can scale to fill both axes
				mScale = 1.0f / (.5f - .5f * mFlatValue); // scale it up so it fills both axes
			}

			virtual std::pair<float, float> OSC_ADVANCE(float samples, float samplesTillNextSample) override
			{
				mPhaseIncrement += mDTDT * samples;
				double phaseToAdvance = samples * mPhaseIncrement;
				double newPhase = Fract(mPhase + phaseToAdvance); // advance slave; doing it here helps us calculate discontinuity.

				std::pair<float, float> bleps{ 0.0f,0.0f };

				float scale = float(mPhaseIncrement * math::gPI * -math::cos(math::gPITimes2 * mEdge1));
				OSC_ACCUMULATE_BLAMP(bleps, newPhase, mEdge1, scale, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLAMP(bleps, newPhase, mEdge2, scale, samples, samplesTillNextSample);

				this->mPhase = newPhase;
				return bleps;
			}
		};


		/////////////////////////////////////////////////////////////////////////////
		struct PulsePWMWaveform :IOscillatorWaveform
		{
			virtual float NaiveSample(float phase01) override
			{
				//   (phase01 < shape) ? -1 : 1;
				return phase01 < mShape ? -1.0f : 1.0f;
			}

			virtual float NaiveSampleSlope(float phase01) override
			{
				return 0;
			}

			virtual void SetParams(float freq, float phaseOffset, float waveshape, float sampleRate, int samplesInBlock) override
			{
				IOscillatorWaveform::SetParams(freq, phaseOffset, waveshape, sampleRate, samplesInBlock);
			}

			virtual std::pair<float, float> OSC_ADVANCE(float samples, float samplesTillNextSample) override
			{
				mPhaseIncrement += mDTDT * samples;
				double phaseToAdvance = samples * mPhaseIncrement;
				double newPhase = Fract(mPhase + phaseToAdvance); // advance slave; doing it here helps us calculate discontinuity.

				std::pair<float, float> bleps{ 0.0f,0.0f };

				OSC_ACCUMULATE_BLEP(bleps, newPhase, 0, -1, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLEP(bleps, newPhase, mShape, 1, samples, samplesTillNextSample);

				this->mPhase = newPhase;
				return bleps;
			}
		};


		/////////////////////////////////////////////////////////////////////////////
		struct SineHarmTruncWaveform :IOscillatorWaveform
		{
			// returns Y value at specified phase. instance / stateless.
			virtual float NaiveSample(float phase01) override
			{
				if (phase01 < this->mShape) {
					float rad = phase01 / mShape * math::gPITimes2;
					float s = math::sin(rad);
					float b = .5f - math::cos(rad) * .5f;
					s = s * b * 1.5f;
					return s;
				}
				else {
					return 0;
				}
			}

			// this is not improved by returing correct slope. blepping curves is too hard 4 me.
			virtual float NaiveSampleSlope(float phase01) override
			{
				return 0;
			}

			virtual void SetParams(float freq, float phaseOffset, float waveshape, float sampleRate, int samplesInBlock) override
			{
				IOscillatorWaveform::SetParams(freq, phaseOffset, waveshape, sampleRate, samplesInBlock);

				mShape = Lerp(1, 0.1f, waveshape);

				mDCOffset = 0;
				mScale = 1.0f;
			}

			// samples is 0<samples<1
			// assume this.phase is currently 0<t<1
			// this.phase may not be on a sample boundary.
			// returns blep before and blep after discontinuity.
			virtual std::pair<float, float> OSC_ADVANCE(float samples, float samplesTillNextSample) override
			{
				mPhaseIncrement += mDTDT * samples;
				double phaseToAdvance = samples * mPhaseIncrement;
				double newPhase = Fract(mPhase + phaseToAdvance); // advance slave; doing it here helps us calculate discontinuity.

				std::pair<float, float> bleps{ 0.0f,0.0f };
				this->mPhase = newPhase;
				return bleps;
			}
		};

		/////////////////////////////////////////////////////////////////////////////
		struct WhiteNoiseWaveform :IOscillatorWaveform
		{
			float mCurrentLevel = 0;
			// returns Y value at specified phase. instance / stateless.
			virtual float NaiveSample(float phase01) override
			{
				return mCurrentLevel;// math::randN11();// Helpers::RandFloat() * 2 - 1;
			}

			// this is not improved by returing correct slope. blepping curves is too hard 4 me.
			virtual float NaiveSampleSlope(float phase01) override
			{
				return 0;
			}

			virtual void SetParams(float freq, float phaseOffset, float waveshape, float sampleRate, int samplesInBlock) override
			{
				IOscillatorWaveform::SetParams(freq, phaseOffset, waveshape, sampleRate, samplesInBlock);

				mShape = Lerp(1, 0.1f, waveshape);
			}

			virtual std::pair<float, float> OSC_ADVANCE(float samples, float samplesTillNextSample) override
			{
				mPhaseIncrement += mDTDT * samples;
				double phaseToAdvance = samples * mPhaseIncrement;
				double newPhase = Fract(mPhase + phaseToAdvance); // advance slave; doing it here helps us calculate discontinuity.

				if (newPhase < this->mPhase) {
					mCurrentLevel = math::randN11();
				}

				// this effectively doubles the frequency which feels more comfy both for LFO and audio osc.
				if (newPhase > 0.5f && this->mPhase <= 0.5f) {
					mCurrentLevel = math::randN11();
				}

				std::pair<float, float> bleps{ 0.0f,0.0f };
				this->mPhase = newPhase;
				return bleps;
			}
		};

		/////////////////////////////////////////////////////////////////////////////
		enum class OscillatorIntention
		{
			LFO,
			Audio,
		};

		// helps select constructors.
		// Some differences in LFO and Audio behaviors:
		// - LFOs don't have any anti-aliasing measures.
		// - LFOs have fewer vst-controlled params; so many of their params have backing values as members
		// - LFO modulations are K-Rate rather than A-Rate.
		struct OscillatorIntentionLFO {};
		struct OscillatorIntentionAudio {};

		// nothing additional to add
		struct OscillatorDevice : ISoundSourceDevice
		{
			static constexpr real_t gSyncFrequencyCenterHz = 1000;
			static constexpr real_t gSyncFrequencyScale = 10;
			static constexpr real_t gFrequencyMulMax = 64;

			// BASE PARAMS
			EnumParam<OscillatorWaveform> mWaveform;
			Float01Param mWaveshape;//	Osc2Waveshape,
			BoolParam mPhaseRestart;//	Osc2PhaseRestart,
			FloatN11Param mPhaseOffset;//	Osc2PhaseOffset,
			BoolParam mSyncEnable;//	Osc2SyncEnable,
			FrequencyParam mSyncFrequency;//	Osc2SyncFrequency,//	Osc2SyncFrequencyKT,
			ScaledRealParam mFrequencyMul;//	Osc2FreqMul,. FM8 allows 0-64
			Float01Param mFMFeedback01;// Osc2FMFeedback,

			OscillatorIntention mIntention;

			// backing values for LFO which don't have these params in main cache
			float mLFOSyncFrequencyParamValue = 1.0f;
			float mLFOSyncFrequencyKTBacking = 1.0f;
			float mLFOFrequencyMulParamValue = 0.5f;
			float mLFOFMFeedbackParamValue = 0.0f;
			float mLFOSyncEnableBacking = 0;

			// for Audio
			explicit OscillatorDevice(OscillatorIntentionAudio, float* paramCache, ModulationSpec* ampEnvModulation,
				ParamIndices baseParamID, ModSource ampEnvModSourceID, ModDestination modDestBaseID
			) :
				ISoundSourceDevice(paramCache, ampEnvModulation, baseParamID,
					(ParamIndices)(int(baseParamID) + int(OscParamIndexOffsets::Enabled)),
					(ParamIndices)(int(baseParamID) + int(OscParamIndexOffsets::Volume)),
					(ParamIndices)(int(baseParamID) + int(OscParamIndexOffsets::AuxMix)),
					(ParamIndices)(int(baseParamID) + int(OscParamIndexOffsets::PitchSemis)),
					(ParamIndices)(int(baseParamID) + int(OscParamIndexOffsets::PitchFine)),
					(ParamIndices)(int(baseParamID) + int(OscParamIndexOffsets::FrequencyParam)),
					(ParamIndices)(int(baseParamID) + int(OscParamIndexOffsets::FrequencyParamKT)),
					(ParamIndices)(int(baseParamID) + int(OscParamIndexOffsets::KeyRangeMin)),
					(ParamIndices)(int(baseParamID) + int(OscParamIndexOffsets::KeyRangeMax)),
					ampEnvModSourceID,
					modDestBaseID,
					(ModDestination)(int(modDestBaseID) + int(OscModParamIndexOffsets::Volume)),
					(ModDestination)(int(modDestBaseID) + int(OscModParamIndexOffsets::AuxMix)),
					(ModDestination)(int(modDestBaseID) + int(OscModParamIndexOffsets::PreFMVolume))
					),
				mWaveform(paramCache[(int)baseParamID + (int)OscParamIndexOffsets::Waveform], OscillatorWaveform::Count, OscillatorWaveform::SineClip),
				mWaveshape(paramCache[(int)baseParamID + (int)OscParamIndexOffsets::Waveshape], 0),
				mPhaseRestart(paramCache[(int)baseParamID + (int)OscParamIndexOffsets::PhaseRestart], false),
				mPhaseOffset(paramCache[(int)baseParamID + (int)OscParamIndexOffsets::PhaseOffset], 0),
				mSyncEnable(paramCache[(int)baseParamID + (int)OscParamIndexOffsets::SyncEnable], false),
				mSyncFrequency(paramCache[(int)baseParamID + (int)OscParamIndexOffsets::SyncFrequency], paramCache[(int)baseParamID + (int)OscParamIndexOffsets::SyncFrequencyKT], gSyncFrequencyCenterHz, gSyncFrequencyScale, 0.4f, 1.0f),
				mFrequencyMul(paramCache[(int)baseParamID + (int)OscParamIndexOffsets::FreqMul], 0.0f, gFrequencyMulMax, 1.0f),
				mFMFeedback01(paramCache[(int)baseParamID + (int)OscParamIndexOffsets::FMFeedback], 0),

				mIntention(OscillatorIntention::Audio)
			{}

			// for LFO, we internalize many params
			explicit OscillatorDevice(OscillatorIntentionLFO, float* paramCache, ParamIndices paramBaseID, ModDestination modDestBaseID) :
				ISoundSourceDevice(paramCache, paramBaseID,
					(ParamIndices)(int(paramBaseID) + int(LFOParamIndexOffsets::FrequencyParam)),
					modDestBaseID
				),
				mWaveform(paramCache[int(paramBaseID) + (int)LFOParamIndexOffsets::Waveform], OscillatorWaveform::Count, OscillatorWaveform::SineClip),
				mWaveshape(paramCache[int(paramBaseID) + (int)LFOParamIndexOffsets::Waveshape], 0),
				mPhaseRestart(paramCache[int(paramBaseID) + (int)LFOParamIndexOffsets::Restart], false),
				mPhaseOffset(paramCache[int(paramBaseID) + (int)LFOParamIndexOffsets::PhaseOffset], 0),
				mSyncEnable(mLFOSyncEnableBacking, false),
				mSyncFrequency(mLFOSyncFrequencyParamValue, mLFOSyncFrequencyKTBacking, gSyncFrequencyCenterHz, gSyncFrequencyScale, 0.4f, 0),
				mFrequencyMul(mLFOFrequencyMulParamValue, 0.0f, 64.0f),
				mFMFeedback01(mLFOFMFeedbackParamValue, 0),

				mIntention(OscillatorIntention::LFO)
			{
				mFrequencyMul.SetRangedValue(1.0f);
			}

			virtual void BeginBlock(int samplesInBlock) override
			{
				//
			}

		};


		struct OscillatorNode : ISoundSourceDevice::Voice
		{
			OscillatorDevice* mpOscDevice = nullptr;

			// voice-level state
			double mPhase = 0;
			double mPhaseIncrement = 0; // DT
			double mDTDT = 0; // to smooth frequency changes without having expensive recalc frequency every sample, just linearly adjust phaseincrement (DT) every sample over the block.
			float mCurrentSample = 0;
			float mOutSample = 0;
			float mPrevSample = 0;
			float mFMFeedbackAmt = 0; // adjusted for global FM scale
			float mCurrentFreq = 0;

			SawClipWaveform mSawClipWaveform;
			SineClipWaveform mSineClipWaveform;
			PulsePWMWaveform mPulsePWMWaveform;
			SineHarmTruncWaveform mSineHarmTruncWaveform;
			WhiteNoiseWaveform mWhiteNoiseWaveform;

			IOscillatorWaveform* mpSlaveWave = &mSawClipWaveform;

			//ModMatrixNode& mModMatrix;
			//int mModDestBase; // ModDestination enum value representing the 1st mod value. for LFOs that's waveshape, for audio that's volume.

			float mFreqModVal = 0;
			float mPitchFineModVal = 0;
			float mWaveShapeModVal = 0;
			float mSyncFreqModVal = 0;
			float mFMFeedbackModVal = 0;
			float mPhaseModVal = 0;

			OscillatorNode(OscillatorDevice* pOscDevice, ModMatrixNode& modMatrix, EnvelopeNode* pAmpEnv) :
				ISoundSourceDevice::Voice(pOscDevice, modMatrix, pAmpEnv),
				mpOscDevice(pOscDevice)
			{
			}

			real_t GetSample() const {
				return mOutSample;
			}

			// used by LFOs to just hard-set the phase. nothing fancy.
			void SetPhase(double phase01)
			{
				mPhase = phase01;
				mpSlaveWave->mPhase = phase01;
			}

			void BeginBlock(real_t midiNote, float voiceShapeMod, float detuneFreqMul, float fmScale, int samplesInBlock)
			{
				if (!this->mpSrcDevice->mEnabledParam.GetBoolValue()) {
					return;
				}
				switch (mpOscDevice->mIntention) {
				case OscillatorIntention::LFO:
					mFreqModVal = mModMatrix.GetDestinationValue((int)mpSrcDevice->mModDestBaseID + (int)LFOModParamIndexOffsets::FrequencyParam, 0);
					mWaveShapeModVal = mModMatrix.GetDestinationValue((int)mpSrcDevice->mModDestBaseID + (int)LFOModParamIndexOffsets::Waveshape, 0);
					break;
				case OscillatorIntention::Audio:
					mSyncFreqModVal = mModMatrix.GetDestinationValue((int)mpSrcDevice->mModDestBaseID + (int)OscModParamIndexOffsets::SyncFrequency, 0);
					mFreqModVal = mModMatrix.GetDestinationValue((int)mpSrcDevice->mModDestBaseID + (int)OscModParamIndexOffsets::FrequencyParam, 0);
					mFMFeedbackModVal = mModMatrix.GetDestinationValue((int)mpSrcDevice->mModDestBaseID + (int)OscModParamIndexOffsets::FMFeedback, 0);
					mWaveShapeModVal = mModMatrix.GetDestinationValue((int)mpSrcDevice->mModDestBaseID + (int)OscModParamIndexOffsets::Waveshape, 0);
					mPhaseModVal = mModMatrix.GetDestinationValue((int)mpSrcDevice->mModDestBaseID + (int)OscModParamIndexOffsets::Phase, 0);
					mPitchFineModVal = mModMatrix.GetDestinationValue((int)mpSrcDevice->mModDestBaseID + (int)OscModParamIndexOffsets::PitchFine, 0);

					mFMFeedbackAmt = mpOscDevice->mFMFeedback01.Get01Value(mFMFeedbackModVal) * fmScale * 0.5f;
					break;
				}

				switch (mpOscDevice->mWaveform.GetEnumValue()) {
				case OscillatorWaveform::SawClip:
					mpSlaveWave = &mSawClipWaveform;
					break;
				case OscillatorWaveform::Pulse:
					mpSlaveWave = &mPulsePWMWaveform;
					break;
				case OscillatorWaveform::SineClip:
					mpSlaveWave = &mSineClipWaveform;
					break;
				case OscillatorWaveform::SineHarmTrunc:
					mpSlaveWave = &mSineHarmTruncWaveform;
					break;
				case OscillatorWaveform::WhiteNoiseSH:
					mpSlaveWave = &mWhiteNoiseWaveform;
					break;
				}

				// - osc pitch semis                  note         oscillator                  
				// - osc fine (semis)                 note         oscillator                   
				// - osc sync freq / kt (Hz)          hz           oscillator                        
				// - osc freq / kt (Hz)               hz           oscillator                   
				// - osc mul (Hz)                     hz           oscillator             
				// - osc detune (semis)               hz+semis     oscillator                         
				// - unisono detune (semis)           hz+semis     oscillator                             
				midiNote += mpSrcDevice->mPitchSemisParam.GetIntValue() + mpSrcDevice->mPitchFineParam.GetN11Value(mPitchFineModVal) * gSourcePitchFineRangeSemis;
				float noteHz = MIDINoteToFreq(midiNote);
				float freq = mpSrcDevice->mFrequencyParam.GetFrequency(noteHz, mFreqModVal);
				freq *= mpOscDevice->mFrequencyMul.GetRangedValue();
				freq *= detuneFreqMul;
				freq *= 0.5f; // WHY? because it corresponds more naturally to other synth octave ranges.
				mCurrentFreq = freq;

				double newDT = (double)freq / Helpers::CurrentSampleRate;
				mDTDT = (newDT - mPhaseIncrement) / samplesInBlock;
				//mDTDT = 0;
				//mPhaseIncrement = newDT;

				float slaveFreq = mpOscDevice->mSyncEnable.GetBoolValue() ? mpOscDevice->mSyncFrequency.GetFrequency(noteHz, mSyncFreqModVal) : freq;
				mpSlaveWave->SetParams(slaveFreq, mpOscDevice->mPhaseOffset.GetN11Value(mPhaseModVal), mpOscDevice->mWaveshape.Get01Value(voiceShapeMod + mWaveShapeModVal), Helpers::CurrentSampleRateF, samplesInBlock);
			}

			virtual void NoteOn(bool legato) override
			{
				if (legato) return;
				if (mpOscDevice->mPhaseRestart.GetBoolValue()) {
					mpSlaveWave->OSC_RESTART(0);
					mPhase = Fract(mpOscDevice->mPhaseOffset.GetN11Value(mPhaseModVal));
				}
			}

			virtual void NoteOff() override {}

			real_t ProcessSample(size_t bufferPos, real_t signal1, real_t signal1PMAmount, real_t signal2, real_t signal2PMAmount, bool forceSilence)
			{
				if (!this->mpSrcDevice->mEnabledParam.GetBoolValue()) {
					mOutSample = mCurrentSample = 0;
					return 0;
				}

				mPrevSample = mCurrentSample;// THIS sample.
				mCurrentSample = 0; // a value that gets added to the next sample

				// why do we have 2 phase accumulators? because of hard sync support, and because they have different functions
				// though when hard sync is not enabled that difference is not very obvious.
				// mPhase / mPhaseIncrement are the master phase, bound by frequency.
				// mSlaveWave holds the slave phase, bound by sync frequency.

				// Push master phase forward by full sample.
				mPhaseIncrement += mDTDT;
				mPhase = Fract(mPhase + mPhaseIncrement);

				if (forceSilence) {
					mOutSample = mCurrentSample = 0;
					return 0;
				}
				
				float phaseMod = mPrevSample * mFMFeedbackAmt + signal1 * signal1PMAmount + signal2 * signal2PMAmount;

				if (mPhase >= mPhaseIncrement || !mpOscDevice->mSyncEnable.GetBoolValue()) // did not cross cycle. advance 1 sample
				{
					auto bleps = mpSlaveWave->OSC_ADVANCE(1, 0);
					mPrevSample += bleps.first;
					mCurrentSample += bleps.second;
				}
				else {
					float x = float(mPhase / mPhaseIncrement); // sample overshoot, in samples.

					auto bleps = mpSlaveWave->OSC_ADVANCE(1 - x, x); // the amount before the cycle boundary
					mPrevSample += bleps.first;
					mCurrentSample += bleps.second;

					bleps = mpSlaveWave->OSC_RESTART(x); // notify of cycle crossing
					mPrevSample += bleps.first;
					mCurrentSample += bleps.second;

					bleps = mpSlaveWave->OSC_ADVANCE(x, 0); // and advance after the cycle begin
					mPrevSample += bleps.first;
					mCurrentSample += bleps.second;
				}

				// current sample will be used on next sample (this is the 1-sample delay)
				mCurrentSample += mpSlaveWave->NaiveSample(float(mpSlaveWave->mPhase + phaseMod));
				mOutSample = (mPrevSample + mpSlaveWave->mDCOffset) * mpSlaveWave->mScale;

				return mOutSample;
			}
		};
	} // namespace M7


} // namespace WaveSabreCore




