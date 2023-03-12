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
		static constexpr real_t gLFOFrequencyCenterHz = 1.5f;
		static constexpr real_t gLFOFrequencyScale = 8;
		static constexpr float gVarTrapezoidSoftSlope = 0.7f;
		static constexpr float gVarTrapezoidHardSlope = 0.2f;
		static constexpr float gOscillatorHeadroomScalar = 0.75f; // scale oscillator outputs down to make room for blepping etc.

		// sampler, oscillator, LFO @ device level
		struct ISoundSourceDevice
		{
			const ParamIndices mBaseParamID;
			const ParamIndices mEnabledParamID;
			const ParamIndices mVolumeParamID;
			const ParamIndices mAuxPanParamID;

			const ParamIndices mTuneSemisParamID;
			const ParamIndices mTuneFineParamID;
			const ParamIndices mFreqParamID;
			const ParamIndices mFreqKTParamID;
			const ParamIndices mKeyRangeMinParamID;
			const ParamIndices mKeyRangeMaxParamID;

			const ModSource mAmpEnvModSourceID;
			const ModDestination mModDestBaseID;
			const ModDestination mVolumeModDestID;
			const ModDestination mAuxPanModDestID;
			const ModDestination mHiddenVolumeModDestID;

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

				mEnabledParam(paramCache[(int)enabledParamID]),
				mVolumeParam(paramCache[(int)volumeParamID], 0),
				mAuxPanParam(paramCache[(int)auxPanParamID]),
				mFrequencyParam(paramCache[(int)freqParamID], paramCache[(int)freqKTParamID], gSourceFrequencyCenterHz, gSourceFrequencyScale),
				mPitchSemisParam(paramCache[(int)tuneSemisParamID], -gSourcePitchSemisRange, gSourcePitchSemisRange),
				mPitchFineParam(paramCache[(int)tuneFineParamID]),
				mKeyRangeMin(paramCache[(int)keyRangeMinParamID], 0, 127),
				mKeyRangeMax(paramCache[(int)keyRangeMaxParamID], 0, 127)
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
				mEnabledParam(mLFOEnabledBacking),
				mVolumeParam(mLFOVolumeBacking, 0),
				mAuxPanParam(mLFOAuxPanBacking),
				mFrequencyParam(paramCache[(int)freqParamID], mLFOFreqKTBacking, gLFOFrequencyCenterHz, gLFOFrequencyScale),
				mPitchSemisParam(mLFOPitchSemisParamValue, -gSourcePitchSemisRange, gSourcePitchSemisRange),
				mPitchFineParam(mLFOPitchFineParamValue),
				mKeyRangeMin(mLFOKeyRangeMinBacking, 0, 127),
				mKeyRangeMax(mLFOKeyRangeMaxBacking, 0, 127)
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
			virtual void EndBlock() = 0;

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
				//virtual void EndBlock() = 0;
			};
		};

		enum class OscillatorWaveform : uint8_t
		{
			Pulse,
			PulseTristate,
			SawClip,
			SineAsym,
			SineClip,
			SineHarmTrunc,
			SineTrunc,
			TriClip, // aka one-pole tri trunc for some reason
			TriSquare,
			TriTrunc,
			VarTrapezoidHard,
			VarTrapezoidSoft,
			VarTriangle,
			WhiteNoiseSH,
			Count,
		};
		// size-optimize using macro
		#define OSCILLATOR_WAVEFORM_CAPTIONS(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::M7::OscillatorWaveform::Count] { \
			"Pulse",\
			"PulseTristate",\
			"SawClip",\
			"SineAsym",\
			"SineClip",\
			"SineHarmTrunc",\
			"SineTrunc",\
			"TriClip",\
			"TriSquare",\
			"TriTrunc",\
			"VarTrapezoidHard",\
			"VarTrapezoidSoft",\
			"VarTriangle",\
			"WhiteNoiseSH",\
		}

		/////////////////////////////////////////////////////////////////////////////
		struct IOscillatorWaveform
		{
			float mShape = 0.5; // wave shape
			float mPhaseOffset = 0;

			float mDCOffset = 0; // while we could try to bake our wave generator with correct DC, it's very inconvenient. This is just way easier.
			float mScale = gOscillatorHeadroomScalar; // same here; way easier to just scale in post than to try and get everything scaled correctly during generation.

			float mFrequency = 0;
			double mPhase = 0; // phase cursor 0-1
			double mDTDT = 0; // to glide to a new frequency smoothly over a block.
			double mPhaseIncrement = 0; // dt
			float mSampleRate = 0;

			virtual float NaiveSample(float phase01) = 0; // return amplitude at phase
			virtual float NaiveSampleSlope(float phase01) = 0; // return slope at phase			
			//virtual std::pair<float, float> OSC_ADVANCE(float samples, float samplesTillNextSample) = 0;// returns blep before and blep after discontinuity.

			// override if you need to adjust things
			virtual void SetParams(float freq, float phaseOffsetN11, float waveshape, float sampleRate, int samplesInBlock)
			{
				mFrequency = freq;
				mPhaseOffset = math::fract(phaseOffsetN11);
				mShape = waveshape;
				mSampleRate = sampleRate;

				double newPhaseInc = (double)mFrequency / sampleRate;
				mDTDT = (newPhaseInc - mPhaseIncrement) / samplesInBlock;

				//mDCOffset = 0;
				//mScale = 1.0f;

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
				if (!math::DoesEncounter(mPhase, newPhase, edge))
					return;
				float samplesSinceEdge = float(math::fract(newPhase - edge) / this->mPhaseIncrement);
				float samplesFromEdgeToNextSample = math::fract(samplesSinceEdge + samplesFromNewPositionUntilNextSample);
				bleps.first = blepScale * BlepBefore(samplesFromEdgeToNextSample);
				bleps.second = blepScale * BlepAfter(samplesFromEdgeToNextSample);
			}

			void OSC_ACCUMULATE_BLAMP(std::pair<float, float>& bleps, double newPhase, float edge, float blampScale, float samples, float samplesFromNewPositionUntilNextSample)
			{
				if (!math::DoesEncounter((mPhase), (newPhase), edge))
					return;

				float samplesSinceEdge = float(math::fract(newPhase - edge) / float(this->mPhaseIncrement));
				float samplesFromEdgeToNextSample = math::fract(samplesSinceEdge + samplesFromNewPositionUntilNextSample);

				bleps.first += blampScale * BlampBefore(samplesFromEdgeToNextSample);
				bleps.second += blampScale * BlampAfter(samplesFromEdgeToNextSample);
			}

			// offers waveforms the opportunity to accumulate bleps along the advancement.
			virtual void Visit(std::pair<float, float>& bleps, double newPhase, float samples, float samplesTillNextSample) {}

			// samples is 0<samples<1
			// assume this.phase is currently 0<t<1
			// this.phase may not be on a sample boundary.
			// returns blep before and blep after discontinuity.
			virtual std::pair<float, float> OSC_ADVANCE(float samples, float samplesTillNextSample)
			{
				mPhaseIncrement += mDTDT * samples;
				double phaseToAdvance = samples * mPhaseIncrement;
				double newPhase = math::fract(mPhase + phaseToAdvance); // advance slave; doing it here helps us calculate discontinuity.
				std::pair<float, float> bleps{ 0.0f,0.0f };

				Visit(bleps, newPhase, samples, samplesTillNextSample);

				this->mPhase = newPhase;
				return bleps;
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

				waveshape = std::abs(waveshape * 2 - 1); // create reflection to make bipolar
				waveshape = 1 - waveshape;
				waveshape = 1 - (waveshape * waveshape);
				mShape = math::lerp(0, 0.95f, waveshape);
				//mShape = Clamp(waveshape * 0.9f, 0, 1);

				mFlatValue = mShape;

				mDCOffset = -mShape; // offset so we can scale to fill both axes
				mScale = 1 / (1.0f - mShape); // scale it up so it fills both axes
				mScale *= gOscillatorHeadroomScalar;
			}

			// samples is 0<samples<1
			// assume this.phase is currently 0<t<1
			// this.phase may not be on a sample boundary.
			// returns blep before and blep after discontinuity.
			//virtual std::pair<float, float> OSC_ADVANCE(float samples, float samplesTillNextSample) override
			virtual void Visit(std::pair<float, float>& bleps, double newPhase, float samples, float samplesTillNextSample) override
			{
				//mPhaseIncrement += mDTDT * samples;
				//double phaseToAdvance = samples * mPhaseIncrement;
				//double newPhase = Fract(mPhase + phaseToAdvance); // advance slave; doing it here helps us calculate discontinuity.

				//std::pair<float, float> bleps{ 0.0f,0.0f };
				float blampScale = float(mPhaseIncrement);
				float blepScale = -(1.0f - mShape);

				OSC_ACCUMULATE_BLEP(bleps, newPhase, 0/*edge*/, blepScale, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLAMP(bleps, newPhase, 0/*edge*/, -blampScale, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLAMP(bleps, newPhase, this->mShape/*edge*/, blampScale, samples, samplesTillNextSample);

				//this->mPhase = newPhase;
				//return bleps;
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

				mShape = std::abs(waveshape * 2 - 1); // create reflection to make bipolar

				mEdge1 = (0.75f - mShape * .25f);
				mEdge2 = (0.75f + mShape * .25f);
				mFlatValue = math::sin(mEdge1 * 2 * math::gPI); // the Y value that gets sustained

				mDCOffset = -(.5f + .5f * mFlatValue); // offset so we can scale to fill both axes
				mScale = 1.0f / (.5f - .5f * mFlatValue); // scale it up so it fills both axes
				mScale *= gOscillatorHeadroomScalar;
			}

			virtual void Visit(std::pair<float, float>& bleps, double newPhase, float samples, float samplesTillNextSample) override
//				virtual std::pair<float, float> OSC_ADVANCE(float samples, float samplesTillNextSample) override
			{
				//mPhaseIncrement += mDTDT * samples;
				//double phaseToAdvance = samples * mPhaseIncrement;
				//double newPhase = Fract(mPhase + phaseToAdvance); // advance slave; doing it here helps us calculate discontinuity.

				//std::pair<float, float> bleps{ 0.0f,0.0f };

				float scale = float(mPhaseIncrement * math::gPI * -math::cos(math::gPITimes2 * mEdge1));
				OSC_ACCUMULATE_BLAMP(bleps, newPhase, mEdge1, scale, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLAMP(bleps, newPhase, mEdge2, scale, samples, samplesTillNextSample);

				//this->mPhase = newPhase;
				//return bleps;
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
				this->mShape = math::lerp(0.98f, .02f, waveshape);
			}

			virtual void Visit(std::pair<float, float>& bleps, double newPhase, float samples, float samplesTillNextSample) override
				//virtual std::pair<float, float> OSC_ADVANCE(float samples, float samplesTillNextSample) override
			{
				//mPhaseIncrement += mDTDT * samples;
				//double phaseToAdvance = samples * mPhaseIncrement;
				//double newPhase = Fract(mPhase + phaseToAdvance); // advance slave; doing it here helps us calculate discontinuity.

				//std::pair<float, float> bleps{ 0.0f,0.0f };

				OSC_ACCUMULATE_BLEP(bleps, newPhase, 0, -1, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLEP(bleps, newPhase, mShape, 1, samples, samplesTillNextSample);

				//this->mPhase = newPhase;
				//return bleps;
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
				waveshape = std::abs(waveshape * 2 - 1); // create reflection to make bipolar
				mShape = math::lerp(1, 0.1f, waveshape);
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

				mShape = math::lerp(1, 0.1f, waveshape);
			}

			virtual void Visit(std::pair<float, float>& bleps, double newPhase, float samples, float samplesTillNextSample) override
//				virtual std::pair<float, float> OSC_ADVANCE(float samples, float samplesTillNextSample) override
			{
				//mPhaseIncrement += mDTDT * samples;
				//double phaseToAdvance = samples * mPhaseIncrement;
				//double newPhase = Fract(mPhase + phaseToAdvance); // advance slave; doing it here helps us calculate discontinuity.

				if (newPhase < this->mPhase) {
					mCurrentLevel = math::randN11();
				}

				// this effectively doubles the frequency which feels more comfy both for LFO and audio osc.
				if (newPhase > 0.5f && this->mPhase <= 0.5f) {
					mCurrentLevel = math::randN11();
				}

				//std::pair<float, float> bleps{ 0.0f,0.0f };
				//this->mPhase = newPhase;
				//return bleps;
			}
		};




		/////////////////////////////////////////////////////////////////////////////
		struct VarTriWaveform :IOscillatorWaveform
		{
			virtual void SetParams(float freq, float phaseOffset, float waveshape, float sampleRate, int samplesInBlock) override
			{
				IOscillatorWaveform::SetParams(freq, phaseOffset, waveshape, sampleRate, samplesInBlock);
				this->mShape = math::lerp(0.99f, .01f, waveshape); // just prevent div0
			}

			// returns Y value at specified phase. instance / stateless.
			virtual float NaiveSample(float phase01) override
			{
				if (phase01 < mShape) {
					return phase01 / mShape * 2 - 1;
				}
				return (1 - phase01) / (1 - mShape) * 2 - 1;
			}

			// this is not improved by returing correct slope. blepping curves is too hard 4 me.
			virtual float NaiveSampleSlope(float phase01) override
			{
				if (phase01 < mShape) {
					return 2 * mShape;
				}
				return -2 * (1 - mShape);
			}

			virtual void Visit(std::pair<float, float>& bleps, double newPhase, float samples, float samplesTillNextSample) override
			{
				//       1-|            ,-',        |
				//         |         ,-'  | ',      |
				//       0-|      ,-'         ',    |
				//         |   ,-'        |     ',  |
				//      -1-|,-'                   ',|
				// pw       0-------------|---------1
				// slope:
				//         |--------------          | = 1/pw
				//         |                        | = 0
				//         |              ----------| = -1/(1-pw)
				float pw = mShape;
				// this is
				// dt / (1/pw - 1/(1-pw))
				float scale = (float)(mPhaseIncrement / (pw - pw * pw));
				OSC_ACCUMULATE_BLAMP(bleps, newPhase, 0/*edge*/, scale, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLAMP(bleps, newPhase, pw/*edge*/, -scale, samples, samplesTillNextSample);
			}
		};

		/////////////////////////////////////////////////////////////////////////////
		struct PulseTristateWaveform :IOscillatorWaveform
		{
			float mT2 = 0;
			float mT3 = 0;
			float mT4 = 0;

			virtual void SetParams(float freq, float phaseOffset, float waveshape, float sampleRate, int samplesInBlock) override
			{
				IOscillatorWaveform::SetParams(freq, phaseOffset, waveshape, sampleRate, samplesInBlock);
				waveshape = std::abs(waveshape * 2 - 1); // create reflection to make bipolar
				this->mShape = math::lerp(0.95f, .05f, waveshape);
				mT2 = mShape / 2;
				mT3 = .5f;
				mT4 = .5f + mT2;
			}

			virtual float NaiveSample(float phase01) override
			{
				if (phase01 < mT2) return 1;
				if (phase01 < mT3) return 0;
				if (phase01 < mT4) return -1;
				return 0;
			}

			virtual float NaiveSampleSlope(float phase01) override
			{
				return 0;
			}

			virtual void Visit(std::pair<float, float>& bleps, double newPhase, float samples, float samplesTillNextSample) override
			{
				OSC_ACCUMULATE_BLEP(bleps, newPhase, 0, .5f, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLEP(bleps, newPhase, mT2, -.5f, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLEP(bleps, newPhase, mT3, -.5f, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLEP(bleps, newPhase, mT4, .5f, samples, samplesTillNextSample);
			}
		};

		/////////////////////////////////////////////////////////////////////////////
		struct SineAsymWaveform :IOscillatorWaveform
		{
			virtual void SetParams(float freq, float phaseOffset, float waveshape, float sampleRate, int samplesInBlock) override
			{
				IOscillatorWaveform::SetParams(freq, phaseOffset, waveshape, sampleRate, samplesInBlock);
				mShape = std::abs(waveshape * 2 - 1); // create reflection to make bipolar
				mShape = math::lerp(.5f, 0.03f, mShape * mShape); // prevent div0
			}

			virtual float NaiveSample(float phase01) override
			{
				if (phase01 < mShape) {
					return math::cos(math::gPI * phase01 / mShape);
				}
				return -math::cos(math::gPI * (phase01 - mShape) / (1 - mShape));
			}

			virtual float NaiveSampleSlope(float phase01) override
			{
				if (phase01 < mShape) {
					return math::sin(math::gPI * phase01 / mShape);
				}
				return -math::sin(math::gPI * (phase01 - mShape) / (1 - mShape));
			}
		};

		/////////////////////////////////////////////////////////////////////////////
		struct SineTruncWaveform :IOscillatorWaveform
		{
			virtual void SetParams(float freq, float phaseOffset, float waveshape, float sampleRate, int samplesInBlock) override
			{
				IOscillatorWaveform::SetParams(freq, phaseOffset, waveshape, sampleRate, samplesInBlock);
				waveshape = std::abs(waveshape * 2 - 1); // create reflection to make bipolar
				mShape = math::lerp(0.97f, 0.03f, waveshape); // prevent div0
			}

			virtual float NaiveSample(float phase01) override
			{
				if (phase01 < mShape) {
					return math::sin(math::gPITimes2 * phase01 / mShape);
				}
				return 0;
			}

			virtual float NaiveSampleSlope(float phase01) override
			{
				if (phase01 < mShape) {
					return math::gPITimes2 * math::cos(math::gPITimes2 * phase01 / mShape) / mShape;
				}
				return 0;
			}

			virtual void Visit(std::pair<float, float>& bleps, double newPhase, float samples, float samplesTillNextSample) override
			{
				float scale = float(math::gPI * mPhaseIncrement / mShape);
				OSC_ACCUMULATE_BLAMP(bleps, newPhase, 0/*edge*/, scale, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLAMP(bleps, newPhase, mShape/*edge*/, -scale, samples, samplesTillNextSample);
			}
		};


		/////////////////////////////////////////////////////////////////////////////
		struct TriClipWaveform :IOscillatorWaveform
		{
			virtual void SetParams(float freq, float phaseOffset, float waveshape, float sampleRate, int samplesInBlock) override
			{
				IOscillatorWaveform::SetParams(freq, phaseOffset, waveshape, sampleRate, samplesInBlock);
				waveshape = std::abs(waveshape * 2 - 1); // create reflection to make bipolar
				mShape = math::lerp(0.97f, 0.03f, waveshape);
				mDCOffset = -.5;//-.5 * this.shape;
				mScale = 2;//1/(.5+this.DCOffset);
				mScale *= gOscillatorHeadroomScalar;
			}

			virtual float NaiveSample(float phase01) override
			{
				if (phase01 >= mShape) {
					return 0;
				}
				float y = phase01 / (mShape * 0.5f);
				if (y < 1) {
					return y;
				}
				return 2 - y;
				/*
  (phase01 >= shape) ? (
	y = 0;
  ) : (
	y = phase01 / (shape * .5);
	(y < 1) ? (
	  y;// 0,1 =>
	):(
	  2-y;//y = y; // 0<t<.5pw. therefore 0<y<.5
	);
  );
				*/
			}

			virtual float NaiveSampleSlope(float phase01) override
			{
				if (phase01 >= mShape) {
					return 0;
				}
				float y = phase01 / (mShape * 0.5f);
				if (y < 1) {
					return 1 / mShape;
				}
				return -1 / mShape;
				/*
  (phase01 >= shape) ? (
	0
  ) : (
	y = phase01 / (shape * .5);
	(y < 1) ? (
	  1/shape
	):(
	  -1/shape
	);
  );
				*/
				return 0;
			}

			virtual void Visit(std::pair<float, float>& bleps, double newPhase, float samples, float samplesTillNextSample) override
			{
				float scale = float(mPhaseIncrement / mShape);
				OSC_ACCUMULATE_BLAMP(bleps, newPhase, 0/*edge*/, scale, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLAMP(bleps, newPhase, mShape * .5f/*edge*/, -2 * scale, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLAMP(bleps, newPhase, mShape/*edge*/, scale, samples, samplesTillNextSample);
			}
		};


		/////////////////////////////////////////////////////////////////////////////
		struct TriSquareWaveform :IOscillatorWaveform
		{
			float mT1 = 0;
			float mT2 = 0;
			float mT3 = 0;
			virtual void SetParams(float freq, float phaseOffset, float waveshape, float sampleRate, int samplesInBlock) override
			{
				IOscillatorWaveform::SetParams(freq, phaseOffset, waveshape, sampleRate, samplesInBlock);

				waveshape = std::abs(waveshape * 2 - 1); // create reflection to make bipolar
				mShape = math::lerp(0, .49f, waveshape * waveshape);

				mT1 = .5f - mShape;
				mT2 = .5f;
				mT3 = 1 - mShape;
			}

			virtual float NaiveSample(float t) override
			{
				/*
				  // my original impl created a tall triangle wave, and then clipped it.
				  // that is elegant, but when dealing with fixed point arith, those amplitudes will definitely get out of bounds.
				  // and i don't want to rely on saturating arith.
				  // so as with the others, let's REMAKE it.

				  //         shape describes the width of the flat areas.
				  //         0<shape<0.5. shape cannot be 0.5 exactly; it must be minus at least 1 sample length
				  //       1-|    /````\
				  //         |   /      \
				  //       0-|  /        \
				  //         | /          \
				  //      -1-|/            \____
				  //          ^    <==>     <==>
				  //          ^   t1  t2    t3  ^
				  //          0       .5        1
				  //          t2-t1 = 1-t3 = shape
				  //          t1-t0 = (1-shape*2)/2 = .5-shape
				  //          slope = 2 / shape
				  (t<this.t1) ? (
					(t/this.t1)*2-1
				  ):(t<this.t2)?(
					1
				  ):(t<this.t3)?(
					1-(t-.5)/this.t1*2
				  ):(
					-1
				  );
				*/
				if (t < mT1) {
					return (t / mT1) * 2 - 1;
				}
				if (t < mT2) {
					return 1;
				}
				if (t < mT3) {
					return 1 - (t - 0.5f) / mT1 * 2;
				}
				return -1;
			}

			virtual float NaiveSampleSlope(float t) override
			{
				/*
				  (t<this.t1) ? (
					1/this.t1;
				  ):(t<this.t2)?(
					0
				  ):(t<this.t3)?(
					-1/this.t1;
				  ):(
					0
				  );
				*/
				if (t < mT1) {
					return 1 / mT1;
				}
				if (t < mT2) {
					return 0;
				}
				if (t < mT3) {
					return -1 / mT1;
				}
				return 0;
			}

			virtual void Visit(std::pair<float, float>& bleps, double newPhase, float samples, float samplesTillNextSample) override
			{
				float scale = float(mPhaseIncrement / mT1);
				OSC_ACCUMULATE_BLAMP(bleps, newPhase, 0/*edge*/, scale, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLAMP(bleps, newPhase, mT1/*edge*/, -scale, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLAMP(bleps, newPhase, mT2/*edge*/, -scale, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLAMP(bleps, newPhase, mT3/*edge*/, scale, samples, samplesTillNextSample);
			}

		};


		/////////////////////////////////////////////////////////////////////////////
		struct TriTruncWaveform :IOscillatorWaveform
		{
			virtual void SetParams(float freq, float phaseOffset, float waveshape, float sampleRate, int samplesInBlock) override
			{
				IOscillatorWaveform::SetParams(freq, phaseOffset, waveshape, sampleRate, samplesInBlock);

				waveshape = std::abs(waveshape * 2 - 1); // create reflection to make bipolar
				mShape = math::lerp(0.97f, .03f, waveshape);
			}

			virtual float NaiveSample(float phase01) override
			{
				if (phase01 >= mShape) {
					return 0;
				}
				float tx = (phase01 - mShape) / mShape;
				tx -= 0.25f;
				tx = math::fract(tx);
				tx -= 0.5f;
				tx = math::abs(tx) * 4 - 1;
/*
  (phase01 >= shape) ? (
	0
  ) : (
	// within pulsewidth, do a full tri, but starting at 0, ascending, ending at 0
	tx = (phase01-shape)/ shape;
	tx -= .25;
	tx = fract(tx);
	tx -= .5;
	tx = abs(tx)*4-1;
  );
*/
				return tx;
			}

			virtual float NaiveSampleSlope(float phase01) override
			{
				if (phase01 < mShape * 0.25f) {
					return 2 / mShape;
				}
				if (phase01 < mShape * 0.75f) {
					return -2 / mShape;
				}
				if (phase01 < mShape) {
					return 2 / mShape;
				}
				return 0;
				/*
  (phase01 < shape*.25) ? (
	OSC_GENERAL_SLOPE(shape)
  ) : (phase01 < shape*.75) ? (
	-OSC_GENERAL_SLOPE(shape)
  ) : (phase01 < shape) ?(
	OSC_GENERAL_SLOPE(shape)
  ) : (
	0
  );
				*/
			}

			virtual void Visit(std::pair<float, float>& bleps, double newPhase, float samples, float samplesTillNextSample) override
			{
				float scale = float(mPhaseIncrement * 2 / mShape);//OSC_GENERAL_SLOPE(this.shape);
				OSC_ACCUMULATE_BLAMP(bleps, newPhase, 0/*edge*/, scale, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLAMP(bleps, newPhase, mShape * .25f/*edge*/, -2 * scale, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLAMP(bleps, newPhase, mShape * .75f/*edge*/, 2 * scale, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLAMP(bleps, newPhase, mShape/*edge*/, -scale, samples, samplesTillNextSample);
			}
		};

		/////////////////////////////////////////////////////////////////////////////
		struct VarTrapezoidWaveform :IOscillatorWaveform
		{
			float mT1 = 0;
			float mT2 = 0;
			float mT3 = 0;
			const float mSlope = 0.1f; // 0, .5 range., avoid div0!
			float mWidth = 0;

			VarTrapezoidWaveform(float slope01) :
				mSlope(math::lerp(0.02f, 0.47f, slope01 / 2))
			{
			}

			virtual void SetParams(float freq, float phaseOffset, float width, float sampleRate, int samplesInBlock) override
			{
				IOscillatorWaveform::SetParams(freq, phaseOffset, width, sampleRate, samplesInBlock);

				float remainingSpace = 1 - 2 * mSlope;
				mWidth = width * remainingSpace;

				mT1 = mWidth;
				mT2 = mWidth + mSlope;
				mT3 = 1 - mSlope;
			}

			virtual float NaiveSample(float t) override
			{
/*
  //--------------------------------------------------------
  // edge    0   t1  t2        t3  1
  //         |    |  |          |  |
  //         -----,                , +1
  //               \              /
  //                \            /
  //                 `----------`    -1
  // width   |----|
  // slope        |--|          |--|

  // derivative
  // edge    0   t1  t2        t3  1
  //         |    |  |          |  |
  //                            ---- ((slope*2))
  //         -----   -----------     0
  //              ---                -((slope*2))

  (t < this.t1) ? (
	1
  ) : (t < this.t2) ? (
	1-2*(t - this.t1) / this.slope
  ) : (t < this.t3) ? (
	-1
  ) : (
	-1+2*(t - this.t3) / this.slope
  );

*/
				if (t < mT1) {
					return 1;
				}
				if (t < mT2) {
					return 1 - 2 * (t - mT1) / mSlope;
				}
				if (t < mT3) {
					return -1;
				}
				return -1 + 2 * (t - mT3) / mSlope;
			}

			virtual float NaiveSampleSlope(float t) override
			{
/*
  (t < this.t1) ? (
    0
  ) : (t < this.t2) ? (
    -1/this.slope
  ) : (t < this.t3) ? (
    0
  ) : (
    1/this.slope
  );
*/
				if (t < mT1) {
					return 0;
				}
				if (t < mT2) {
					return -1 / mSlope;
				}
				if (t < mT3) {
					return 0;
				}
				return 1 / mSlope;
			}

			virtual void Visit(std::pair<float, float>& bleps, double newPhase, float samples, float samplesTillNextSample) override
			{
				float scale = float(mPhaseIncrement / (mSlope));
				OSC_ACCUMULATE_BLAMP(bleps, newPhase, 0/*edge*/, -scale, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLAMP(bleps, newPhase, mT1/*edge*/, -scale, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLAMP(bleps, newPhase, mT2/*edge*/, scale, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLAMP(bleps, newPhase, mT3/*edge*/, scale, samples, samplesTillNextSample);
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

			FrequencyParam mLPFFrequency;// { mParamCache[(int)ParamIndices::LFO1Sharpness], mAlways0, gLFOLPCenterFrequency, gLFOLPFrequencyScale };

			// backing values for LFO which don't have these params in main cache
			float mLFOSyncFrequencyParamValue = 1.0f;
			float mLFOSyncFrequencyKTBacking = 1.0f;
			float mLFOFrequencyMulParamValue = 0.5f;
			float mLFOFMFeedbackParamValue = 0.0f;
			float mLFOSyncEnableBacking = 0;
			float mLFPFrequencyBacking = 0;
			float mLFPFrequencyKTBacking = 0;

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
				mLPFFrequency(mLFPFrequencyBacking, mLFPFrequencyKTBacking, gLFOLPCenterFrequency, gLFOLPFrequencyScale),

				mIntention(OscillatorIntention::Audio)
			{
			}

			// for LFO, we internalize many params
			explicit OscillatorDevice(OscillatorIntentionLFO, float* paramCache, ParamIndices paramBaseID, ModDestination modDestBaseID) :
				ISoundSourceDevice(paramCache, paramBaseID,
					(ParamIndices)(int(paramBaseID) + int(LFOParamIndexOffsets::FrequencyParam)),
					modDestBaseID
				),
				mWaveform(paramCache[int(paramBaseID) + (int)LFOParamIndexOffsets::Waveform], OscillatorWaveform::Count),
				mWaveshape(paramCache[int(paramBaseID) + (int)LFOParamIndexOffsets::Waveshape]),
				mPhaseRestart(paramCache[int(paramBaseID) + (int)LFOParamIndexOffsets::Restart]),
				mPhaseOffset(paramCache[int(paramBaseID) + (int)LFOParamIndexOffsets::PhaseOffset]),
				mSyncEnable(mLFOSyncEnableBacking),
				mSyncFrequency(mLFOSyncFrequencyParamValue, mLFOSyncFrequencyKTBacking, gSyncFrequencyCenterHz, gSyncFrequencyScale),
				mFrequencyMul(mLFOFrequencyMulParamValue, 0.0f, 64.0f),
				mFMFeedback01(mLFOFMFeedbackParamValue),
				mLPFFrequency(paramCache[int(paramBaseID) + (int)LFOParamIndexOffsets::Sharpness], mLFPFrequencyKTBacking, gLFOLPCenterFrequency, gLFOLPFrequencyScale),
				mIntention(OscillatorIntention::LFO)
			{
				mFrequencyMul.SetRangedValue(1);
			}

			virtual void BeginBlock(int samplesInBlock) override {}
			virtual void EndBlock() override {}

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

			PulsePWMWaveform mPulsePWMWaveform;
			PulseTristateWaveform mPulseTristateWaveform;
			SawClipWaveform mSawClipWaveform;
			SineAsymWaveform mSineAsymWaveform;
			SineClipWaveform mSineClipWaveform;
			SineHarmTruncWaveform mSineHarmTruncWaveform;
			SineTruncWaveform mSineTruncWaveform;
			TriClipWaveform mTriClipWaveform;
			TriSquareWaveform mTriSquareWaveform;
			TriTruncWaveform mTriTruncWaveform;
			VarTrapezoidWaveform mVarTrapezoidHardWaveform { gVarTrapezoidHardSlope };
			VarTrapezoidWaveform mVarTrapezoidSoftWaveform { gVarTrapezoidSoftSlope };
			VarTriWaveform mVarTriWaveform;
			WhiteNoiseWaveform mWhiteNoiseWaveform;
			IOscillatorWaveform* mpSlaveWave = &mSawClipWaveform;

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
				case OscillatorWaveform::Pulse:
					mpSlaveWave = &mPulsePWMWaveform;
					break;
				case OscillatorWaveform::PulseTristate:
					mpSlaveWave = &mPulseTristateWaveform;
					break;
				case OscillatorWaveform::SawClip:
					mpSlaveWave = &mSawClipWaveform;
					break;
				case OscillatorWaveform::SineAsym:
					mpSlaveWave = &mSineAsymWaveform;
					break;
				case OscillatorWaveform::SineClip:
					mpSlaveWave = &mSineClipWaveform;
					break;
				case OscillatorWaveform::SineHarmTrunc:
					mpSlaveWave = &mSineHarmTruncWaveform;
					break;
				case OscillatorWaveform::SineTrunc:
					mpSlaveWave = &mSineTruncWaveform;
					break;
				case OscillatorWaveform::TriClip:
					mpSlaveWave = &mTriClipWaveform;
					break;
				case OscillatorWaveform::TriSquare:
					mpSlaveWave = &mTriSquareWaveform;
					break;
				case OscillatorWaveform::TriTrunc:
					mpSlaveWave = &mTriTruncWaveform;
					break;
				case OscillatorWaveform::VarTrapezoidSoft:
					mpSlaveWave = &mVarTrapezoidSoftWaveform;
					break;
				case OscillatorWaveform::VarTrapezoidHard:
					mpSlaveWave = &mVarTrapezoidHardWaveform;
					break;
				case OscillatorWaveform::VarTriangle:
					mpSlaveWave = &mVarTriWaveform;
					break;
				default:
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
				float noteHz = math::MIDINoteToFreq(midiNote);
				float freq = mpSrcDevice->mFrequencyParam.GetFrequency(noteHz, mFreqModVal);
				freq *= mpOscDevice->mFrequencyMul.GetRangedValue();
				freq *= detuneFreqMul;
				freq *= 0.5f; // WHY? because it corresponds more naturally to other synth octave ranges.
				// 0 frequencies would cause math problems, denormals, infinites... but fortunately they're inaudible so...
				freq = std::max(freq, 0.001f);
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
					mPhase = math::fract(mpOscDevice->mPhaseOffset.GetN11Value(mPhaseModVal));
				}
			}

			virtual void NoteOff() override {}

			real_t ProcessSample(size_t bufferPos, real_t signal1, real_t signal1PMAmount, real_t signal2, real_t signal2PMAmount, real_t signal3, real_t signal3PMAmount, bool forceSilence)
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
				mPhase = math::fract(mPhase + mPhaseIncrement);

				if (forceSilence) {
					mOutSample = mCurrentSample = 0;
					return 0;
				}
				
				float phaseMod =
					mPrevSample * mFMFeedbackAmt
					+ signal1 * signal1PMAmount
					+ signal2 * signal2PMAmount
					+ signal3 * signal3PMAmount
					;

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
				mCurrentSample = math::clamp(mCurrentSample, -1, 1);
				mOutSample = (mPrevSample + mpSlaveWave->mDCOffset) * mpSlaveWave->mScale;

				return mOutSample;
			}
		};
	} // namespace M7


} // namespace WaveSabreCore




