#pragma once

#include <WaveSabreCore/Maj7Basic.hpp>
#include <WaveSabreCore/Maj7Envelope.hpp>
#include <WaveSabreCore/Maj7ModMatrix.hpp>
#include <WaveSabreCore/Filters/FilterOnePole.hpp>

namespace WaveSabreCore
{
	namespace M7
	{
		static constexpr float gVarTrapezoidSoftSlope = 0.7f;
		static constexpr float gVarTrapezoidHardSlope = 0.15f;
		static constexpr float gOscillatorHeadroomScalar = 0.75f; // scale oscillator outputs down to make room for blepping etc.

		// sampler, oscillator, LFO @ device level
		struct ISoundSourceDevice
		{
			ParamAccessor mParams;
			bool mEnabledCache;

			const ModSource mAmpEnvModSourceID;
			const ModDestination mModDestBaseID;

			ModulationSpec* mAmpEnvModulation;

			virtual bool IsEnabled() const = 0;
			virtual bool MatchesKeyRange(int midiNote) const = 0;

			virtual ~ISoundSourceDevice()
			{}

			ISoundSourceDevice(float* paramCache, ModulationSpec* ampEnvModulation, ParamIndices baseParamID,
				ModSource ampEnvModSourceID, ModDestination modDestBaseID
			) :
				mParams(paramCache, baseParamID),
				mAmpEnvModulation(ampEnvModulation),
				mAmpEnvModSourceID(ampEnvModSourceID),
				mModDestBaseID(modDestBaseID)
			{
			}

			void InitDevice() {
				mAmpEnvModulation->SetSourceAmp(mAmpEnvModSourceID, AddEnum(mModDestBaseID, SourceModParamIndexOffsets::HiddenVolume), &mEnabledCache);
			}

			virtual void BeginBlock()
			{
				mEnabledCache = this->IsEnabled();
			}
			virtual void EndBlock() = 0;

			struct Voice
			{
				ISoundSourceDevice* mpSrcDevice;
				ModMatrixNode* mpModMatrix;
				EnvelopeNode* mpAmpEnv;

				Voice(ISoundSourceDevice* psrcDevice, ModMatrixNode* pModMatrix, EnvelopeNode* pAmpEnv) :
					mpSrcDevice(psrcDevice),
					mpModMatrix(pModMatrix),
					mpAmpEnv(pAmpEnv)
				{}

				virtual ~Voice()
				{}

				virtual void NoteOn(bool legato) = 0;
				virtual void NoteOff() = 0;
				virtual void BeginBlock() = 0;

				void SetModMatrix(ModMatrixNode* pModMatrix)
				{
					mpModMatrix = pModMatrix;
				}
			};
		};

		enum class OscillatorWaveform : uint8_t
		{
			Pulse,
			PulseTristate,
			SawClip,
			SineClip,
			SineHarmTrunc,
			TriSquare,
			TriTrunc,
			VarTrapezoidHard,
			VarTrapezoidSoft,
			WhiteNoiseSH,
			SineRectified,
			SinePhaseDist,
			StaircaseSaw,
			TriFold,
			DoublePulse,
			Count,
		};
		// size-optimize using macro
		#define OSCILLATOR_WAVEFORM_CAPTIONS(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::M7::OscillatorWaveform::Count] { \
			"Pulse",\
			"PulseTristate",\
			"Saw",\
			"Sine",\
			"SineHarmTrunc",\
			"TriSquare",\
			"TriTrunc",\
			"VarTrapezoidHard",\
			"VarTrapezoidSoft",\
			"WhiteNoiseSH",\
			"SineRectified", \
			"SinePhaseDist", \
			"StaircaseSaw", \
			"TriFold", \
			"DoublePulse", \
		}


		/////////////////////////////////////////////////////////////////////////////
		enum class OscillatorIntention : uint8_t
		{
			LFO,
			Audio,
		};


		/////////////////////////////////////////////////////////////////////////////
		struct IOscillatorWaveform
		{
			float mShape = 0.5; // wave shape
			float mPhaseOffset = 0;

			float mDCOffset = 0; // while we could try to bake our wave generator with correct DC, it's very inconvenient. This is just way easier.
			float mScale = 1; // same here; way easier to just scale in post than to try and get everything scaled correctly during generation.

			float mFrequency = 0;
			double mPhase = 0; // phase cursor 0-1
			double mPhaseIncrement = 0; // dt

			float mBlepBefore;
			float mBlepAfter;

			OscillatorIntention mIntention = OscillatorIntention::LFO;

			//IOscillatorWaveform() = default;
			//virtual ~IOscillatorWaveform() = default;

			virtual float NaiveSample(float phase01) = 0; // return amplitude at phase
			virtual float NaiveSampleSlope(float phase01) = 0; // return slope at phase			
			virtual void AfterSetParams() = 0;
			// offers waveforms the opportunity to accumulate bleps along the advancement.
			virtual void Visit(double newPhase, float samples, float samplesTillNextSample) {}

			// override if you need to adjust things
			virtual void SetParams(float freq, float phaseOffsetN11, float waveshape, double sampleRate /*required for VST to display a graphic*/, OscillatorIntention intention)
			{
				mIntention = intention;
				mFrequency = freq;
				mPhaseOffset = math::fract(phaseOffsetN11);
				mShape = waveshape;

				double newPhaseInc = (double)mFrequency / sampleRate;

				mPhaseIncrement = newPhaseInc;

				AfterSetParams();
			}

			// process discontinuity due to restarting phase right now.
			// updates blep before and blep after discontinuity.
			void OSC_RESTART(float samplesBeforeNext)
			{
				float sampleBefore = this->NaiveSample((float)this->mPhase);
				double newPhase = this->mPhaseOffset;

				// fix bleps
				float sampleAfter = NaiveSample((float)newPhase);
				float blepScale = (sampleAfter - sampleBefore) * .5f; // full sample scale is 2; *.5 to bring 0-1

				mBlepBefore += blepScale * BlepBefore(samplesBeforeNext); // blep the phase restart.
				mBlepAfter += blepScale * BlepAfter(samplesBeforeNext);

				// fix blamps.
				float slopeBefore = NaiveSampleSlope((float)mPhase);
				float slopeAfter = NaiveSampleSlope((float)newPhase);
				float blampScale = float(this->mPhaseIncrement * (slopeAfter - slopeBefore));
				mBlepBefore += blampScale * BlampBefore(samplesBeforeNext);
				mBlepAfter += blampScale * BlampAfter(samplesBeforeNext);

				mPhase = newPhase;

				//return { blepBefore, blepAfter };
			}

			float samplesSinceEdge;
			float samplesFromEdgeToNextSample;

			void WeirdPreBlepBlampCode(double newPhase, float edge, float blepScale, float samplesFromNewPositionUntilNextSample, decltype(BlepBefore) pProcBefore, decltype(BlepBefore) pProcAfter)
			{
				if (!math::DoesEncounter(mPhase, newPhase, edge))
					return;
				samplesSinceEdge = float(math::fract(newPhase - edge) / this->mPhaseIncrement);
				samplesFromEdgeToNextSample = math::fract(samplesSinceEdge + samplesFromNewPositionUntilNextSample);
				mBlepBefore += blepScale * pProcBefore(samplesFromEdgeToNextSample);
				mBlepAfter += blepScale * pProcAfter(samplesFromEdgeToNextSample);
			}

			inline void OSC_ACCUMULATE_BLEP(double newPhase, float edge, float blepScale, float samples, float samplesFromNewPositionUntilNextSample)
			{
				//if (!math::DoesEncounter(mPhase, newPhase, edge))
				//	return;
				//float samplesSinceEdge = float(math::fract(newPhase - edge) / this->mPhaseIncrement);
				//float samplesFromEdgeToNextSample = math::fract(samplesSinceEdge + samplesFromNewPositionUntilNextSample);
				WeirdPreBlepBlampCode(newPhase, edge, blepScale, samplesFromNewPositionUntilNextSample, BlepBefore, BlepAfter);
				//mBlepBefore += blepScale * BlepBefore(samplesFromEdgeToNextSample);
				//mBlepAfter += blepScale * BlepAfter(samplesFromEdgeToNextSample);
			}

			inline void OSC_ACCUMULATE_BLAMP(double newPhase, float edge, float blampScale, float samples, float samplesFromNewPositionUntilNextSample)
			{
				//if (!math::DoesEncounter((mPhase), (newPhase), edge))
				//	return;

				//float samplesSinceEdge = float(math::fract(newPhase - edge) / float(this->mPhaseIncrement));
				//float samplesFromEdgeToNextSample = math::fract(samplesSinceEdge + samplesFromNewPositionUntilNextSample);
				WeirdPreBlepBlampCode(newPhase, edge, blampScale, samplesFromNewPositionUntilNextSample, BlampBefore, BlampAfter);

				//mBlepBefore += blampScale * BlampBefore(samplesFromEdgeToNextSample);
				//mBlepAfter += blampScale * BlampAfter(samplesFromEdgeToNextSample);
			}


			// samples is 0<samples<1
			// assume this.phase is currently 0<t<1
			// this.phase may not be on a sample boundary.
			// accumulates blep before and blep after discontinuity.
			virtual void OSC_ADVANCE(float samples, float samplesTillNextSample)
			{
				//mPhaseIncrement += mDTDT * samples;
				double phaseToAdvance = samples * mPhaseIncrement;
				//double newPhase = math::fract(mPhase + phaseToAdvance); // advance slave; doing it here helps us calculate discontinuity.
				double newPhase = mPhase + phaseToAdvance;
				if (newPhase > 1) newPhase -= 1; // slightly faster t han fract()
				//FloatPair bleps{ 0.0f,0.0f };
				if (mIntention == OscillatorIntention::Audio) {
					Visit(newPhase, samples, samplesTillNextSample);
				}
				this->mPhase = newPhase;
				//return bleps;
			}
		};

		///////////////////////////////////////////////////////////////////////////////
		//struct SawClipWaveform :IOscillatorWaveform
		//{
		//	float mFlatValue = 0;

		//	// returns Y value at specified phase. instance / stateless.
		//	virtual float NaiveSample(float phase01) override
		//	{
		//		if (phase01 < mShape) {
		//			return mShape * 2 - 1;
		//		}
		//		return phase01 * 2 - 1;
		//	}

		//	virtual float NaiveSampleSlope(float phase01) override
		//	{
		//		if (phase01 < mShape) {
		//			return 0;
		//		}
		//		return 1;
		//	}

		//	virtual void AfterSetParams() override
		//	{
		//		float waveshape = std::abs(mShape * 2 - 1); // create reflection to make bipolar
		//		waveshape = 1 - waveshape;
		//		waveshape = 1 - (waveshape * waveshape);
		//		mShape = math::lerp(0, 0.95f, waveshape);
		//		//mShape = Clamp(waveshape * 0.9f, 0, 1);

		//		mFlatValue = mShape;

		//		mDCOffset = -mShape; // offset so we can scale to fill both axes
		//		mScale = 1 / (1.0f - mShape); // scale it up so it fills both axes
		//		//mScale *= gOscillatorHeadroomScalar;
		//	}

		//	// samples is 0<samples<1
		//	// assume this.phase is currently 0<t<1
		//	// this.phase may not be on a sample boundary.
		//	// returns blep before and blep after discontinuity.
		//	virtual void Visit(double newPhase, float samples, float samplesTillNextSample) override
		//	{
		//		//mPhaseIncrement += mDTDT * samples;
		//		//double phaseToAdvance = samples * mPhaseIncrement;
		//		//double newPhase = Fract(mPhase + phaseToAdvance); // advance slave; doing it here helps us calculate discontinuity.

		//		float blampScale = float(mPhaseIncrement);
		//		float blepScale = -(1.0f - mShape);

		//		OSC_ACCUMULATE_BLEP(newPhase, 0/*edge*/, blepScale, samples, samplesTillNextSample);
		//		OSC_ACCUMULATE_BLAMP(newPhase, 0/*edge*/, -blampScale, samples, samplesTillNextSample);
		//		OSC_ACCUMULATE_BLAMP(newPhase, this->mShape/*edge*/, blampScale, samples, samplesTillNextSample);

		//		//this->mPhase = newPhase;
		//		//return bleps;
		//	}
		//};

		/////////////////////////////////////////////////////////////////////////////
		//struct SawClipWaveform :IOscillatorWaveform
		//{
		//	// returns Y value at specified phase. instance / stateless.
		//	virtual float NaiveSample(float phase01) override
		//	{
		//		return phase01 * 2 - 1;
		//	}

		//	virtual float NaiveSampleSlope(float phase01) override
		//	{
		//		return 1;
		//	}

		//	virtual void AfterSetParams() override
		//	{
		//	}
		//	virtual void Visit(double newPhase, float samples, float samplesTillNextSample) override
		//	{
		//		float blepScale = -1;
		//		OSC_ACCUMULATE_BLEP(newPhase, 0/*edge*/, blepScale, samples, samplesTillNextSample);
		//	}
		//};



		///////////////////////////////////////////////////////////////////////////////
		//struct SineClipWaveform :IOscillatorWaveform
		//{
		//	float mEdge1;
		//	float mEdge2;
		//	float mFlatValue;
		//	// returns Y value at specified phase. instance / stateless.
		//	virtual float NaiveSample(float phase01) override
		//	{
		//		if ((phase01 >= mEdge1) && (phase01 < mEdge2)) {
		//			return mFlatValue;
		//		}
		//		return math::sin(math::gPITimes2 * phase01);
		//	}

		//	// this is not improved by returing correct slope. blepping curves is too hard 4 me.
		//	virtual float NaiveSampleSlope(float phase01) override
		//	{
		//		if ((phase01 >= mEdge1) && (phase01 < mEdge2)) {
		//			return 0;
		//		}
		//		return math::cos(math::gPITimes2 * phase01);
		//	}

		//	virtual void AfterSetParams() override
		//	{
		//		mShape = std::abs(mShape * 2 - 1); // create reflection to make bipolar

		//		mEdge1 = (0.75f - mShape * .25f);
		//		mEdge2 = (0.75f + mShape * .25f);
		//		mFlatValue = math::sin(mEdge1 * 2 * math::gPI); // the Y value that gets sustained

		//		mDCOffset = -(.5f + .5f * mFlatValue); // offset so we can scale to fill both axes
		//		mScale = 1.0f / (.5f - .5f * mFlatValue); // scale it up so it fills both axes
		//	}

		//	virtual void Visit(double newPhase, float samples, float samplesTillNextSample) override
		//	{
		//		float scale = float(mPhaseIncrement * math::gPI * -math::cos(math::gPITimes2 * mEdge1));
		//		OSC_ACCUMULATE_BLAMP(newPhase, mEdge1, scale, samples, samplesTillNextSample);
		//		OSC_ACCUMULATE_BLAMP(newPhase, mEdge2, scale, samples, samplesTillNextSample);
		//	}
		//};


		/////////////////////////////////////////////////////////////////////////////
		struct SineClipWaveform :IOscillatorWaveform
		{
			// returns Y value at specified phase. instance / stateless.
			virtual float NaiveSample(float phase01) override
			{
				return math::sin(math::gPITimes2 * phase01);
			}

			// this is not improved by returing correct slope. blepping curves is too hard 4 me.
			virtual float NaiveSampleSlope(float phase01) override
			{
				return 1;// math::cos(math::gPITimes2 * phase01);
			}

			virtual void AfterSetParams() override
			{
				//mShape = std::abs(mShape * 2 - 1); // create reflection to make bipolar

				//mEdge1 = (0.75f - mShape * .25f);
				//mEdge2 = (0.75f + mShape * .25f);
				//mFlatValue = math::sin(mEdge1 * 2 * math::gPI); // the Y value that gets sustained

				//mDCOffset = -(.5f + .5f * mFlatValue); // offset so we can scale to fill both axes
				//mScale = 1.0f / (.5f - .5f * mFlatValue); // scale it up so it fills both axes
			}

			virtual void Visit(double newPhase, float samples, float samplesTillNextSample) override
			{
				//float scale = float(mPhaseIncrement * math::gPI * -math::cos(math::gPITimes2 * mEdge1));
				//OSC_ACCUMULATE_BLAMP(newPhase, mEdge1, scale, samples, samplesTillNextSample);
				//OSC_ACCUMULATE_BLAMP(newPhase, mEdge2, scale, samples, samplesTillNextSample);
			}
		};

		/////////////////////////////////////////////////////////////////////////////
		struct PulsePWMWaveform :IOscillatorWaveform
		{
			virtual float NaiveSample(float phase01) override
			{
				return phase01 < mShape ? -1.0f : 1.0f;
			}

			virtual float NaiveSampleSlope(float phase01) override
			{
				return 0;
			}

			virtual void AfterSetParams() override
			{
				mShape = math::lerp(0.98f, .02f, mShape);
			}

			virtual void Visit(double newPhase, float samples, float samplesTillNextSample) override
			{
				OSC_ACCUMULATE_BLEP(newPhase, 0, -1, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLEP(newPhase, mShape, 1, samples, samplesTillNextSample);
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

			virtual void AfterSetParams() override
			{
				float waveshape = std::abs(mShape * 2 - 1); // create reflection to make bipolar
				mShape = math::lerp(1, 0.1f, waveshape);
			}

		};

		/////////////////////////////////////////////////////////////////////////////
		// frequency = sample and hold (a sort of sample-crush, in the spirit of a low pass)
		// shape = sigle pole high pass
		struct WhiteNoiseWaveform :IOscillatorWaveform
		{
			OnePoleFilter mHPFilter;
			float mCurrentLevel = 0;
			float mCurrentSample = 0;

			// returns Y value at specified phase. instance / stateless.
			virtual float NaiveSample(float phase01) override
			{
				return mCurrentSample;
			}

			virtual float NaiveSampleSlope(float phase01) override
			{
				return 0;
			}

			virtual void OSC_ADVANCE(float samples, float samplesTillNextSample)
			{
				// assume we're always advancing by 1 exact sample. it theoretically breaks hard sync but like, who the f is hard-syncing a white noise channel?
				
				mPhase += mPhaseIncrement;
				if (mPhase > 1) {
					mCurrentLevel = math::randN11();
					mPhase = math::fract(mPhase);
				}

				mCurrentSample = mHPFilter.ProcessSample(mCurrentLevel);
				//mCurrentSample = mCurrentLevel;
			}


			virtual void AfterSetParams() override
			{
				// shape determines the high pass frequency
				float cutoff;

				if (mIntention == OscillatorIntention::LFO) {
					cutoff = mFrequency * mShape * mShape;
				}
				else {
					ParamAccessor pa{ &mShape, 0 };
					cutoff = pa.GetFrequency(0, FreqParamConfig{ mFrequency, 6, 0 /*assume never used*/ });
				}


				mHPFilter.SetParams(FilterType::HP, cutoff, 0);
			}

		};




		///////////////////////////////////////////////////////////////////////////////
		//struct VarTriWaveform :IOscillatorWaveform
		//{
		//	virtual void AfterSetParams() override
		//	{
		//		mShape = math::lerp(0.99f, .01f, mShape); // just prevent div0
		//	}

		//	// returns Y value at specified phase. instance / stateless.
		//	virtual float NaiveSample(float phase01) override
		//	{
		//		if (phase01 < mShape) {
		//			return phase01 / mShape * 2 - 1;
		//		}
		//		return (1 - phase01) / (1 - mShape) * 2 - 1;
		//	}

		//	// this is not improved by returing correct slope. blepping curves is too hard 4 me.
		//	virtual float NaiveSampleSlope(float phase01) override
		//	{
		//		if (phase01 < mShape) {
		//			return 2 * mShape;
		//		}
		//		return -2 * (1 - mShape);
		//	}

		//	virtual void Visit(double newPhase, float samples, float samplesTillNextSample) override
		//	{
		//		//       1-|            ,-',        |
		//		//         |         ,-'  | ',      |
		//		//       0-|      ,-'         ',    |
		//		//         |   ,-'        |     ',  |
		//		//      -1-|,-'                   ',|
		//		// pw       0-------------|---------1
		//		// slope:
		//		//         |--------------          | = 1/pw
		//		//         |                        | = 0
		//		//         |              ----------| = -1/(1-pw)
		//		float pw = mShape;
		//		// this is
		//		// dt / (1/pw - 1/(1-pw))
		//		float scale = (float)(mPhaseIncrement / (pw - pw * pw));
		//		OSC_ACCUMULATE_BLAMP(newPhase, 0/*edge*/, scale, samples, samplesTillNextSample);
		//		OSC_ACCUMULATE_BLAMP(newPhase, pw/*edge*/, -scale, samples, samplesTillNextSample);
		//	}
		//};

		/////////////////////////////////////////////////////////////////////////////
		struct PulseTristateWaveform :IOscillatorWaveform
		{
			float mT2 = 0;
			static constexpr float mT3 = 0.5f;
			float mT4 = 0;

			virtual void AfterSetParams() override
			{
				mShape = std::abs(mShape * 2 - 1); // create reflection to make bipolar
				this->mShape = math::lerp(0.95f, .05f, mShape);
				mT2 = mShape / 2;
				//mT3 = .5f;
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

			virtual void Visit(double newPhase, float samples, float samplesTillNextSample) override
			{
				OSC_ACCUMULATE_BLEP(newPhase, 0, .5f, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLEP(newPhase, mT2, -.5f, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLEP(newPhase, mT3, -.5f, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLEP(newPhase, mT4, .5f, samples, samplesTillNextSample);
			}
		};

		///////////////////////////////////////////////////////////////////////////////
		//struct SineAsymWaveform :IOscillatorWaveform
		//{
		//	virtual void SetParams(float freq, float phaseOffset, float waveshape, double sampleRate) override
		//	{
		//		IOscillatorWaveform::SetParams(freq, phaseOffset, waveshape, sampleRate);
		//		mShape = std::abs(waveshape * 2 - 1); // create reflection to make bipolar
		//		mShape = math::lerp(.5f, 0.03f, mShape * mShape); // prevent div0
		//	}

		//	virtual float NaiveSample(float phase01) override
		//	{
		//		if (phase01 < mShape) {
		//			return math::cos(math::gPI * phase01 / mShape);
		//		}
		//		return -math::cos(math::gPI * (phase01 - mShape) / (1 - mShape));
		//	}

		//	virtual float NaiveSampleSlope(float phase01) override
		//	{
		//		if (phase01 < mShape) {
		//			return math::sin(math::gPI * phase01 / mShape);
		//		}
		//		return -math::sin(math::gPI * (phase01 - mShape) / (1 - mShape));
		//	}
		//};

		///////////////////////////////////////////////////////////////////////////////
		//struct SineTruncWaveform :IOscillatorWaveform
		//{
		//	virtual void SetParams(float freq, float phaseOffset, float waveshape, double sampleRate) override
		//	{
		//		IOscillatorWaveform::SetParams(freq, phaseOffset, waveshape, sampleRate);
		//		waveshape = std::abs(waveshape * 2 - 1); // create reflection to make bipolar
		//		mShape = math::lerp(0.97f, 0.03f, waveshape); // prevent div0
		//	}

		//	virtual float NaiveSample(float phase01) override
		//	{
		//		if (phase01 < mShape) {
		//			return math::sin(math::gPITimes2 * phase01 / mShape);
		//		}
		//		return 0;
		//	}

		//	virtual float NaiveSampleSlope(float phase01) override
		//	{
		//		if (phase01 < mShape) {
		//			return math::gPITimes2 * math::cos(math::gPITimes2 * phase01 / mShape) / mShape;
		//		}
		//		return 0;
		//	}

		//	virtual void Visit(std::pair<float, float>& bleps, double newPhase, float samples, float samplesTillNextSample) override
		//	{
		//		float scale = float(math::gPI * mPhaseIncrement / mShape);
		//		OSC_ACCUMULATE_BLAMP(bleps, newPhase, 0/*edge*/, scale, samples, samplesTillNextSample);
		//		OSC_ACCUMULATE_BLAMP(bleps, newPhase, mShape/*edge*/, -scale, samples, samplesTillNextSample);
		//	}
		//};


		///////////////////////////////////////////////////////////////////////////////
		//struct TriClipWaveform :IOscillatorWaveform
		//{
		//	virtual void AfterSetParams() override
		//	{
		//		mShape = std::abs(mShape * 2 - 1); // create reflection to make bipolar
		//		mShape = math::lerp(0.97f, 0.03f, mShape);
		//		mDCOffset = -.5;//-.5 * this.shape;
		//		mScale = 2;//1/(.5+this.DCOffset);
		//		//mScale *= gOscillatorHeadroomScalar;
		//	}

		//	virtual float NaiveSample(float phase01) override
		//	{
		//		if (phase01 >= mShape) {
		//			return 0;
		//		}
		//		float y = phase01 / (mShape * 0.5f);
		//		if (y < 1) {
		//			return y;
		//		}
		//		return 2 - y;
		//	}

		//	virtual float NaiveSampleSlope(float phase01) override
		//	{
		//		if (phase01 >= mShape) {
		//			return 0;
		//		}
		//		float y = phase01 / (mShape * 0.5f);
		//		if (y < 1) {
		//			return 1 / mShape;
		//		}
		//		return -1 / mShape;
		//	}

		//	virtual void Visit(std::pair<float, float>& bleps, double newPhase, float samples, float samplesTillNextSample) override
		//	{
		//		float scale = float(mPhaseIncrement / mShape);
		//		OSC_ACCUMULATE_BLAMP(bleps, newPhase, 0/*edge*/, scale, samples, samplesTillNextSample);
		//		OSC_ACCUMULATE_BLAMP(bleps, newPhase, mShape * .5f/*edge*/, -2 * scale, samples, samplesTillNextSample);
		//		OSC_ACCUMULATE_BLAMP(bleps, newPhase, mShape/*edge*/, scale, samples, samplesTillNextSample);
		//	}
		//};


		///////////////////////////////////////////////////////////////////////////////
		struct TriTruncWaveform :IOscillatorWaveform
		{
			virtual void AfterSetParams() override
			{
				mShape = std::abs(mShape * 2 - 1); // create reflection to make bipolar
				mShape = math::lerp(0.97f, .03f, mShape);
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
				tx = std::abs(tx) * 4 - 1;
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
			}

			virtual void Visit(double newPhase, float samples, float samplesTillNextSample) override
			{
				float scale = float(mPhaseIncrement * 2 / mShape);//OSC_GENERAL_SLOPE(this.shape);
				OSC_ACCUMULATE_BLAMP(newPhase, 0/*edge*/, scale, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLAMP(newPhase, mShape * .25f/*edge*/, -2 * scale, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLAMP(newPhase, mShape * .75f/*edge*/, 2 * scale, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLAMP(newPhase, mShape/*edge*/, -scale, samples, samplesTillNextSample);
			}
		};

		/////////////////////////////////////////////////////////////////////////////
		struct VarTrapezoidWaveform :IOscillatorWaveform
		{
			float mT1 = 0;
			float mT2 = 0;
			float mT3 = 0;
			float mSlope = 0.1f; // 0, .5 range., avoid div0!
			float mWidth = 0;

			VarTrapezoidWaveform(float slope01) :
				mSlope(math::lerp(0.02f, 0.47f, slope01 / 2))
			{
			}

			virtual void AfterSetParams() override
			{
				float remainingSpace = 1 - 2 * mSlope;
				mWidth = mShape * remainingSpace;

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

			virtual void Visit(double newPhase, float samples, float samplesTillNextSample) override
			{
				float scale = (float)(mPhaseIncrement / (mSlope));
				OSC_ACCUMULATE_BLAMP(newPhase, 0/*edge*/, -scale, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLAMP(newPhase, mT1/*edge*/, -scale, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLAMP(newPhase, mT2/*edge*/, scale, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLAMP(newPhase, mT3/*edge*/, scale, samples, samplesTillNextSample);
			}
		};

		struct TriSquareWaveform :VarTrapezoidWaveform
		{
			TriSquareWaveform() : VarTrapezoidWaveform(0.5f)
			{
			}

			virtual void SetParams(float freq, float phaseOffset, float slope, double sampleRate, OscillatorIntention intention) override
			{
				slope = std::abs(slope * 2 - 1); // create reflection to make bipolar
				mSlope = math::lerp(0.5f, 0.015f, math::sqrt01(slope));
				VarTrapezoidWaveform::SetParams(freq, phaseOffset, 0.5f, sampleRate, intention);
			}
		};


		struct NotchSawWaveform : IOscillatorWaveform
		{
			// fixed notch at 0.5 for simplicity; mShape controls depth [0..1]
			static constexpr float kNotchPos = 0.5f;
			float mDepth = 0.0f; // actual notch depth [0..1]

			virtual void AfterSetParams() override
			{
				// gentle curve for depth feel, and avoid extremes
				mDepth = math::clamp01(mShape);
				// DC: step is active for half the cycle -> avg shift = -depth * 0.5
				mDCOffset = +0.5f * mDepth;
				mScale = 1.0f;
			}

			virtual float NaiveSample(float phase01) override
			{
				// base saw plus a negative step at the notch
				float y = phase01 * 2.0f - 1.0f;
				if (phase01 >= kNotchPos) y -= mDepth;
				return y;
			}

			virtual float NaiveSampleSlope(float /*phase01*/) override
			{
				// piecewise linear everywhere except discontinuities (handled by BLEP)
				return 2.0f;
			}

			virtual void Visit(double newPhase, float samples, float samplesTillNextSample) override
			{
				// One value-step at wrap (0), one at notch.
				// For a value step of delta, blepScale should be delta/2.
				// Base saw has delta = -2 -> blepScale = -1 at 0.
				// The notch adds: step -mDepth at kNotchPos, and +mDepth at wrap (when step resets).
				OSC_ACCUMULATE_BLEP(newPhase, 0.0f, -1.0f, samples, samplesTillNextSample);                  // saw wrap
				OSC_ACCUMULATE_BLEP(newPhase, 0.0f, +0.5f * mDepth, samples, samplesTillNextSample);         // step resets at wrap
				OSC_ACCUMULATE_BLEP(newPhase, kNotchPos, -0.5f * mDepth, samples, samplesTillNextSample);    // notch step
			}
		};

				struct RectifiedSineWaveform : IOscillatorWaveform
		{
			// mShape is the morph amount 0 (pure sine) .. 1 (full rectified)
			virtual void AfterSetParams() override
			{
				// DC of (2|sin|-1) over a cycle is (4/pi - 1). We lerp toward it by mShape.
				float dcRect = 4.0f / math::gPI - 1.0f;
				mDCOffset = -dcRect * mShape;
				mScale = 1.0f;
			}

			virtual float NaiveSample(float phase01) override
			{
				float s = math::sin(math::gPITimes2 * phase01);
				float rect = 2.0f * std::abs(s) - 1.0f;
				return math::lerp(s, rect, mShape);
			}

			virtual float NaiveSampleSlope(float phase01) override
			{
				// derivative (not used for Visit kinks, but useful for sync)
				float s = math::sin(math::gPITimes2 * phase01);
				float c = math::cos(math::gPITimes2 * phase01);
				float ds = math::gPITimes2 * c; // sine derivative
				float drect = 4.0f * math::gPI * ((s >= 0.0f) ? 1.0f : -1.0f) * c; // derivative of 2|sin|-1
				return math::lerp(ds, drect, mShape);
			}

			virtual void Visit(double newPhase, float samples, float samplesTillNextSample) override
			{
				// Kinks at phase 0 and 0.5. Slope jump magnitude = 8*pi*mShape.
				if (mIntention != OscillatorIntention::Audio || mShape <= 1e-5f) return;
				float scale = float(mPhaseIncrement * (8.0 * math::gPI) * mShape);
				OSC_ACCUMULATE_BLAMP(newPhase, 0.0f, scale, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLAMP(newPhase, 0.5f, scale, samples, samplesTillNextSample);
			}
		};

		struct PhaseDistortedSineWaveform : IOscillatorWaveform
		{
			float mBreak = 0.5f; // breakpoint in [0,1], avoid extremes

			virtual void AfterSetParams() override
			{
				// map mShape -> breakpoint (keep away from 0/1 to prevent crazy slopes)
				float x = math::clamp01(mShape);
				mBreak = math::lerp(0.08f, 0.92f, x);
				mDCOffset = 0.0f;
				mScale = 1.0f;
			}

			inline float WarpPhase(float t) const
			{
				// compress [0..mBreak] into [0..0.5], [mBreak..1] into [0.5..1]
				if (t < mBreak) return (t / mBreak) * 0.5f;
				return 0.5f + (t - mBreak) / (1.0f - mBreak) * 0.5f;
			}

			virtual float NaiveSample(float phase01) override
			{
				float phi = WarpPhase(phase01);
				return math::sin(math::gPITimes2 * phi);
			}

			virtual float NaiveSampleSlope(float phase01) override
			{
				// dy/dt = 2π cos(2π phi) * dphi/dt
				float dphidt = (phase01 < mBreak) ? (0.5f / mBreak) : (0.5f / (1.0f - mBreak));
				float phi = WarpPhase(phase01);
				return math::gPITimes2 * math::cos(math::gPITimes2 * phi) * dphidt;
			}

			virtual void Visit(double newPhase, float samples, float samplesTillNextSample) override
			{
				// BLAMP at breakpoint (phi=0.5, cos(pi)=-1) and at wrap (0).
				float dphidtBefore = 0.5f / mBreak;
				float dphidtAfter  = 0.5f / (1.0f - mBreak);

				// slope jump at mBreak: -2π*(after - before)
				float deltaMid = -2.0f * float(math::gPI) * (dphidtAfter - dphidtBefore);
				OSC_ACCUMULATE_BLAMP(newPhase, mBreak, float(mPhaseIncrement) * deltaMid, samples, samplesTillNextSample);

				// slope jump at wrap (end -> start): 2π*(before - after)
				float deltaWrap =  2.0f * float(math::gPI) * (dphidtBefore - dphidtAfter);
				OSC_ACCUMULATE_BLAMP(newPhase, 0.0f, float(mPhaseIncrement) * deltaWrap, samples, samplesTillNextSample);
			}
		};

		// New: Staircase saw (quantized steps per cycle)
		struct StaircaseSawWaveform : IOscillatorWaveform
		{
			static constexpr int kMinSteps = 2;
			static constexpr int kMaxSteps = 32;
			int mSteps = 8;
			float mStepHeight = 0.0f; // 2/steps

			virtual void AfterSetParams() override
			{
				// map shape^2 -> steps in [2..32]
				float x = mShape; x = x * x;
				int steps = (int)(math::lerp((float)kMinSteps, (float)kMaxSteps, x) + 0.5f);
				mSteps = math::ClampI(steps, kMinSteps, kMaxSteps);
				mStepHeight = 2.0f / (float)mSteps;
				// Zero mean for our definition where top step is +1
				mDCOffset = -1.0f / (float)mSteps;
				mScale = 1.0f;
			}

			virtual float NaiveSample(float phase01) override
			{
				float idx = std::floor(phase01 * (float)mSteps);
				float y = -1.0f + mStepHeight * (idx + 1.0f);
				return y;
			}

			virtual float NaiveSampleSlope(float /*phase01*/) override { return 0.0f; }

			virtual void Visit(double newPhase, float samples, float samplesTillNextSample) override
			{
				// Wrap step at 0 has delta = -2 + 2/steps => blepScale = -(1 - 1/steps)
				float wrapScale = -(1.0f - 1.0f / (float)mSteps);
				OSC_ACCUMULATE_BLEP(newPhase, 0.0f, wrapScale, samples, samplesTillNextSample);
				// interior rising steps at n/steps with delta = +2/steps => blepScale = +1/steps
				float interiorScale = 1.0f / (float)mSteps;
				for (int n = 1; n < mSteps; ++n)
				{
					float edge = (float)n / (float)mSteps;
					OSC_ACCUMULATE_BLEP(newPhase, edge, interiorScale, samples, samplesTillNextSample);
				}
			}
		};

		// New: Folded triangle (single fold threshold)
		struct TriFoldWaveform : IOscillatorWaveform
		{
			float mThreshold = 1.0f; // 0..1

			virtual void AfterSetParams() override
			{
				// map shape^2 to threshold from 1 -> 0.05 (avoid 0)
				float x = mShape; x = x * x;
				mThreshold = math::lerp(1.0f, 0.05f, x);
				mDCOffset = 0.0f;
				mScale = 1.0f;
			}

			inline static float Tri(float t)
			{
				// triangle, range [-1,1], tri(0)=0
				float a = math::fract(t + 0.25f) - 0.5f;
				return (std::abs(a) * 4.0f - 1.0f);
			}

			inline static float TriSlope(float t)
			{
				// sign based on position inside the triangle cycle
				float b = math::fract(t + 0.25f) - 0.5f;
				return (b < 0.0f) ? -4.0f : 4.0f;
			}

			virtual float NaiveSample(float phase01) override
			{
				float x = Tri(phase01);
				float ax = std::abs(x);
				if (ax <= mThreshold) return x; // no fold region
				float y = (2.0f * mThreshold - ax);
				return (x >= 0.0f) ? y : -y;
			}

			virtual float NaiveSampleSlope(float phase01) override
			{
				float x = Tri(phase01);
				float s = TriSlope(phase01);
				// inside fold, dy/dt = -s, else dy/dt = s
				return (std::abs(x) <= mThreshold) ? s : -s;
			}

			virtual void Visit(double newPhase, float samples, float samplesTillNextSample) override
			{
				// Always handle base triangle corners and wrap
				float k = float(mPhaseIncrement) * 8.0f; // slope jump magnitude (±8)
				OSC_ACCUMULATE_BLAMP(newPhase, 0.0f, +k, samples, samplesTillNextSample);    // wrap: -4 -> +4
				OSC_ACCUMULATE_BLAMP(newPhase, 0.25f, -k, samples, samplesTillNextSample);  // corner: +4 -> -4 (after folding sign)
				OSC_ACCUMULATE_BLAMP(newPhase, 0.75f, +k, samples, samplesTillNextSample);  // corner: -4 -> +4 (after folding sign)

				// Additional knees introduced by folding where |tri| == threshold
				if (mThreshold < 0.999f)
				{
					float th = mThreshold;
					float t1 = th * 0.25f;
					float t2 = 0.5f - th * 0.25f;
					float t3 = 0.5f + th * 0.25f;
					float t4 = 1.0f - th * 0.25f;
					// Correct signs for slope jumps at knees:
					// t1: inside -> folded: -4 -> +4 => +k
					// t2: folded -> inside: -4 -> +4 => +k
					// t3: inside -> folded: +4 -> -4 => -k
					// t4: folded -> inside: +4 -> -4 => -k
					OSC_ACCUMULATE_BLAMP(newPhase, t1, +k, samples, samplesTillNextSample);
					OSC_ACCUMULATE_BLAMP(newPhase, t2, +k, samples, samplesTillNextSample);
					OSC_ACCUMULATE_BLAMP(newPhase, t3, -k, samples, samplesTillNextSample);
					OSC_ACCUMULATE_BLAMP(newPhase, t4, -k, samples, samplesTillNextSample);
				}
			}
		};

		// New: Double pulse (two pulses per cycle)
		struct DoublePulseWaveform : IOscillatorWaveform
		{
			float mCenter1 = 0.35f;
			float mCenter2 = 0.65f;
			float mWidth = 0.08f; // fixed width per pulse

			virtual void AfterSetParams() override
			{
				// shape controls separation between the two pulse centers, keep symmetric around 0.5
				float sep = math::lerp(0.10f, 0.45f, mShape); // half-distance from 0.5
				mCenter1 = 0.5f - sep * 0.5f;
				mCenter2 = 0.5f + sep * 0.5f;
				mWidth = 0.08f; // fixed for now
				float duty = math::clamp01(2.0f * mWidth);
				float mean = -1.0f + 2.0f * duty;
				mDCOffset = -mean; // center around zero
				mScale = 1.0f;
			}

			virtual float NaiveSample(float t) override
			{
				auto inPulse = [&](float c){
					float d = t - c;
					// wrap shortest distance in 0..1 domain
					d -= std::floor(d + 0.5f);
					return std::abs(d) < (mWidth * 0.5f);
				};
				bool on = inPulse(mCenter1) || inPulse(mCenter2);
				return on ? 1.0f : -1.0f;
			}

			virtual float NaiveSampleSlope(float /*t*/) override { return 0.0f; }

			virtual void Visit(double newPhase, float samples, float samplesTillNextSample) override
			{
				// Four edges per cycle (two pulses). Handle wrap by duplicating edges +/- 1.
				auto emitPulseEdges = [&](float c){
					float a = c - mWidth * 0.5f;
					float b = c + mWidth * 0.5f;
					// rising at a, falling at b
					float edges[4] = { a, b, a-1.0f, b-1.0f };
					for (int i=0;i<4;++i){
						float e = edges[i];
						if (e < 0.0f) e += 1.0f;
						// Decide sign: at 'a' we go -1 -> +1 => +1 blepScale; at 'b' we go +1 -> -1 => -1
						bool isRising = (i % 2) == 0; // a and a-1
						float s = isRising ? +1.0f : -1.0f;
						OSC_ACCUMULATE_BLEP(newPhase, e, s, samples, samplesTillNextSample);
					}
				};
				emitPulseEdges(mCenter1);
				emitPulseEdges(mCenter2);
			}
		};

		// helps select constructors.
		// Some differences in LFO and Audio behaviors:
		// - LFOs don't have any anti-aliasing measures.
		// - LFOs have fewer vst-controlled params; so many of their params have backing values as members
		// - LFO modulations are K-Rate rather than A-Rate.
		//struct OscillatorIntentionLFO {};
		//struct OscillatorIntentionAudio {};

		// nothing additional to add
		struct OscillatorDevice : ISoundSourceDevice
		{
			OscillatorIntention mIntention;

			virtual bool IsEnabled() const override {
				return (mIntention == OscillatorIntention::LFO) || mParams.GetBoolValue(OscParamIndexOffsets::Enabled);
			}
			virtual bool MatchesKeyRange(int midiNote) const override {
				if (mIntention == OscillatorIntention::LFO) return true;
				if (mParams.GetIntValue(OscParamIndexOffsets::KeyRangeMin, gKeyRangeCfg) > midiNote)
					return false;
				if (mParams.GetIntValue(OscParamIndexOffsets::KeyRangeMax, gKeyRangeCfg) < midiNote)
					return false;
				return true;
			}


			//virtual float GetLinearVolume(float mod) const override
			//{
			//	return mParams.GetLinearVolume(OscParamIndexOffsets::Volume, gUnityVolumeCfg, mod);
			//}

			// for Audio
			explicit OscillatorDevice(/*OscillatorIntentionAudio, */float* paramCache, ModulationList modulations, const SourceInfo& srcinfo/* ModulationSpec* pAmpModulation, size_t isrc*/);

			// for LFO, we internalize many params
			explicit OscillatorDevice(/*OscillatorIntentionLFO, */float* paramCache, size_t ilfo);

			bool GetPhaseRestart() const {
				return mParams.GetBoolValue((mIntention == OscillatorIntention::Audio) ? (int)OscParamIndexOffsets::PhaseRestart : (int)LFOParamIndexOffsets::Restart);
			}
			//// only for LFO
			//float GetLPFFrequency() const {
			//	return mParams.GetFrequency(LFOParamIndexOffsets::Sharpness, -1, gLFOLPFreqConfig, 0, );
			//}

			virtual void BeginBlock() override {
				ISoundSourceDevice::BeginBlock();
			}
			virtual void EndBlock() override {}

		};

		/////////////////////////////////////////////////////////////////////////////
		struct OscillatorNode : ISoundSourceDevice::Voice
		{
			OscillatorDevice* mpOscDevice = nullptr;

			size_t mnSamples = 0;

			// voice-level state
			double mPhase = 0;
			double mPhaseIncrement = 0; // DT
			float mCurrentSample = 0;
			float mOutSample = 0;
			float mPrevSample = 0;
			float mCurrentFreq = 0;

			float mPhaseModVal = 0;

			IOscillatorWaveform* mpWaveforms[(int)OscillatorWaveform::Count];

			IOscillatorWaveform* mpSlaveWave;// = &mSawClipWaveform;

			OscillatorNode(ModMatrixNode* pModMatrix, OscillatorDevice* pOscDevice, EnvelopeNode* pAmpEnv) :
				ISoundSourceDevice::Voice(pOscDevice, pModMatrix, pAmpEnv),
				mpOscDevice(pOscDevice)
			{
				mpSlaveWave = mpWaveforms[(int)OscillatorWaveform::Pulse] = new PulsePWMWaveform;
				mpWaveforms[(int)OscillatorWaveform::PulseTristate] = new PulseTristateWaveform;
				mpWaveforms[(int)OscillatorWaveform::SawClip] = new NotchSawWaveform;
				mpWaveforms[(int)OscillatorWaveform::SineClip] = new SineClipWaveform;
				mpWaveforms[(int)OscillatorWaveform::SineHarmTrunc] = new SineHarmTruncWaveform;
//				mpWaveforms[(int)OscillatorWaveform::TriClip__obsolete] = ;// new SineHarmTruncWaveform;
				mpWaveforms[(int)OscillatorWaveform::TriSquare] = new TriSquareWaveform;
				mpWaveforms[(int)OscillatorWaveform::TriTrunc] = new TriTruncWaveform;
				mpWaveforms[(int)OscillatorWaveform::VarTrapezoidHard] = new VarTrapezoidWaveform{ gVarTrapezoidHardSlope };
				mpWaveforms[(int)OscillatorWaveform::VarTrapezoidSoft] = new VarTrapezoidWaveform{ gVarTrapezoidSoftSlope };
				//mpWaveforms[(int)OscillatorWaveform::VarTriangle] = mpWaveforms[(int)OscillatorWaveform::TriClip__obsolete] = new VarTriWaveform;
				mpWaveforms[(int)OscillatorWaveform::WhiteNoiseSH] = new WhiteNoiseWaveform;
				mpWaveforms[(int)OscillatorWaveform::SineRectified] = new RectifiedSineWaveform;
				mpWaveforms[(int)OscillatorWaveform::SinePhaseDist] = new PhaseDistortedSineWaveform;
				mpWaveforms[(int)OscillatorWaveform::StaircaseSaw] = new StaircaseSawWaveform;
				mpWaveforms[(int)OscillatorWaveform::TriFold] = new TriFoldWaveform;
				mpWaveforms[(int)OscillatorWaveform::DoublePulse] = new DoublePulseWaveform;
			}

					~OscillatorNode()
			{
#ifdef MIN_SIZE_REL
#pragma message("OscillatorNode::~OscillatorNode() Leaking memory to save bits.")
#else
 			   // careful: some pointers are reused so care to avoid double delete.

				delete (PulsePWMWaveform*)mpWaveforms[(int)OscillatorWaveform::Pulse];
				delete (PulseTristateWaveform*)mpWaveforms[(int)OscillatorWaveform::PulseTristate];
				delete (NotchSawWaveform*)mpWaveforms[(int)OscillatorWaveform::SawClip];
				delete (SineClipWaveform*)mpWaveforms[(int)OscillatorWaveform::SineClip];
				delete (SineHarmTruncWaveform*)mpWaveforms[(int)OscillatorWaveform::SineHarmTrunc];
				delete (TriSquareWaveform*)mpWaveforms[(int)OscillatorWaveform::TriSquare];
				delete (TriTruncWaveform*)mpWaveforms[(int)OscillatorWaveform::TriTrunc];
				delete (VarTrapezoidWaveform*)mpWaveforms[(int)OscillatorWaveform::VarTrapezoidHard];
				delete (VarTrapezoidWaveform*)mpWaveforms[(int)OscillatorWaveform::VarTrapezoidSoft];
				//delete (VarTriWaveform*)mpWaveforms[(int)OscillatorWaveform::VarTriangle];
				delete (WhiteNoiseWaveform*)mpWaveforms[(int)OscillatorWaveform::WhiteNoiseSH];
				delete (RectifiedSineWaveform*)mpWaveforms[(int)OscillatorWaveform::SineRectified];
				delete (PhaseDistortedSineWaveform*)mpWaveforms[(int)OscillatorWaveform::SinePhaseDist];
				delete (StaircaseSawWaveform*)mpWaveforms[(int)OscillatorWaveform::StaircaseSaw];
				delete (TriFoldWaveform*)mpWaveforms[(int)OscillatorWaveform::TriFold];
				delete (DoublePulseWaveform*)mpWaveforms[(int)OscillatorWaveform::DoublePulse];

#endif // MIN_SIZE_REL

			}

			float GetLastSample() const { return mOutSample; }

			// used by LFOs to just hard-set the phase. nothing fancy.
			void SetPhase(double phase01)
			{
				mPhase = phase01;
				mpSlaveWave->mPhase = phase01;
			}


			virtual void NoteOn(bool legato) override
			{
				if (legato) return;
				if (mpOscDevice->GetPhaseRestart()) {
					mpSlaveWave->OSC_RESTART(0);
					mPhase = GetPhaseOffset();// math::fract(f);
				}
			}

			float GetPhaseOffset()
			{
				int po = mpOscDevice->mIntention == OscillatorIntention::Audio ? (int)OscParamIndexOffsets::PhaseOffset : (int)LFOParamIndexOffsets::PhaseOffset;
				float f = mpOscDevice->mParams.GetN11Value(po, mPhaseModVal);
				return math::fract(f);
			}

			virtual void NoteOff() override {}

			virtual void BeginBlock() override
			{
				if (!this->mpSrcDevice->IsEnabled()) {
					return;
				}
				mnSamples = 0; // ensure reprocessing after setting these params to avoid corrupt state.
				auto w = mpOscDevice->mParams.GetEnumValue<OscillatorWaveform>(mpOscDevice->mIntention == OscillatorIntention::Audio ? (int)OscParamIndexOffsets::Waveform : (int)LFOParamIndexOffsets::Waveform);
				mpSlaveWave = mpWaveforms[math::ClampI((int)w, 0, (int)OscillatorWaveform::Count - 1)];
			}

			real_t ProcessSampleForAudio(real_t midiNote, float detuneFreqMul, float fmScale,
				const ParamAccessor& globalParams, float const *otherSignals, int iosc, float ampEnvLin)
			{
				const ParamAccessor& params = mpSrcDevice->mParams;
				if (!this->mpSrcDevice->mEnabledCache) {
					mOutSample = mCurrentSample = 0;
					return 0;
				}

				bool syncEnable = mpOscDevice->mParams.GetBoolValue(OscParamIndexOffsets::SyncEnable);

				float mSyncFreqModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID + (int)OscModParamIndexOffsets::SyncFrequency);
				float mFreqModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID + (int)OscModParamIndexOffsets::FrequencyParam);
				float mFMFeedbackModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID + (int)OscModParamIndexOffsets::FMFeedback);
				float mWaveShapeModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID + (int)OscModParamIndexOffsets::Waveshape);
				mPhaseModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID + (int)OscModParamIndexOffsets::Phase);
				float mPitchFineModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID + (int)OscModParamIndexOffsets::PitchFine);
				float mFMFeedbackAmt = mpOscDevice->mParams.Get01Value(OscParamIndexOffsets::FMFeedback, mFMFeedbackModVal) * fmScale * 0.5f;

				if (0 == mnSamples) // NOTE: important that this is designed to be 0 the first run to force initial calculation.
				{
					// - osc pitch semis                  note         oscillator                  
					// - osc fine (semis)                 note         oscillator                   
					// - osc sync freq / kt (Hz)          hz           oscillator                        
					// - osc freq / kt (Hz)               hz           oscillator                   
					// - osc mul (Hz)                     hz           oscillator              
					// - osc detune (semis)               hz+semis     oscillator                         
					// - unisono detune (semis)           hz+semis     oscillator                             
					int pitchSemis = params.GetIntValue(OscParamIndexOffsets::PitchSemis, gSourcePitchSemisRange);
					float pitchFine = params.GetN11Value(OscParamIndexOffsets::PitchFine, mPitchFineModVal) * gSourcePitchFineRangeSemis;
					midiNote += pitchSemis + pitchFine;// (mpSrcDevice->mPitchFineParam.mCachedVal + mPitchFineModVal)* gSourcePitchFineRangeSemis;
					float noteHz = math::MIDINoteToFreq(midiNote);
					float freq = params.GetFrequency(OscParamIndexOffsets::FrequencyParam, OscParamIndexOffsets::FrequencyParamKT, gSourceFreqConfig, noteHz, mFreqModVal);//mpSrcDevice->mFrequencyParam.GetFrequency(noteHz, mFreqModVal);
					freq *= mpOscDevice->mParams.GetScaledRealValue(OscParamIndexOffsets::FreqMul, 0.0f, gFrequencyMulMax, 0); //mpOscDevice->mFrequencyMul.mCachedVal;// .GetRangedValue();
					freq *= detuneFreqMul;
					// 0 frequencies would cause math problems, denormals, infinites... but fortunately they're inaudible so...
					freq = std::max(freq, 0.001f);
					mCurrentFreq = freq;

					double newDT = (double)freq / Helpers::CurrentSampleRate;
					mPhaseIncrement = newDT;

					float slaveFreq = syncEnable ?
						mpOscDevice->mParams.GetFrequency(OscParamIndexOffsets::SyncFrequency, OscParamIndexOffsets::SyncFrequencyKT, gSyncFreqConfig, noteHz, mSyncFreqModVal)
						: freq; // mpOscDevice->mSyncFrequency.GetFrequency(noteHz, mSyncFreqModVal) : freq;
					float waveshape = mpOscDevice->mParams.Get01Value(OscParamIndexOffsets::Waveshape, mWaveShapeModVal);
					mpSlaveWave->SetParams(slaveFreq, GetPhaseOffset(), waveshape, Helpers::CurrentSampleRate, OscillatorIntention::Audio);
				}

				mnSamples = (mnSamples + 1) & GetAudioOscillatorRecalcSampleMask();

				mPrevSample = mCurrentSample;// THIS sample.
				mCurrentSample = 0; // a value that gets added to the next sample

				// why do we have 2 phase accumulators? because of hard sync support, and because they have different functions
				// though when hard sync is not enabled that difference is not very obvious.
				// mPhase / mPhaseIncrement are the master phase, bound by frequency.
				// mSlaveWave holds the slave phase, bound by sync frequency.

				// Push master phase forward by full sample.
				mPhase = math::fract(mPhase + mPhaseIncrement);

				float phaseMod = mPrevSample * mFMFeedbackAmt;
				int otherIndex = -1;
				for (int i = 0; i < gOscillatorCount - 1; ++i) {
					int off = iosc * (gOscillatorCount - 1) + i;
					ParamIndices amtParam = ParamIndices((int)ParamIndices::FMAmt2to1 + off);
					ModDestination amtMod = ModDestination((int)ModDestination::FMAmt2to1 + off);
					float amt = globalParams.Get01Value(amtParam, mpModMatrix->GetDestinationValue(amtMod));

					otherIndex += (i == iosc) ? 2 : 1;
					phaseMod += otherSignals[otherIndex] * amt * fmScale;
				}

				mpSlaveWave->mBlepAfter = 0;
				mpSlaveWave->mBlepBefore = 0;

				if (mPhase >= mPhaseIncrement || !syncEnable) // did not cross cycle. advance 1 sample
				{
					mpSlaveWave->OSC_ADVANCE(1, 0);
				}
				else {
					float x = float(mPhase / mPhaseIncrement); // sample overshoot, in samples.
					mpSlaveWave->OSC_ADVANCE(1 - x, x); // the amount before the cycle boundary
					mpSlaveWave->OSC_RESTART(x); // notify of cycle crossing
					mpSlaveWave->OSC_ADVANCE(x, 0); // and advance after the cycle begin
				}

				mPrevSample += mpSlaveWave->mBlepBefore;
				mCurrentSample += mpSlaveWave->mBlepAfter;
				// current sample will be used on next sample (this is the 1-sample delay)
				mCurrentSample += mpSlaveWave->NaiveSample(float(mpSlaveWave->mPhase + phaseMod));
				mCurrentSample = math::clampN11(mCurrentSample); // prevent FM from going crazy.
				mOutSample = (mPrevSample + mpSlaveWave->mDCOffset) * mpSlaveWave->mScale * gOscillatorHeadroomScalar * ampEnvLin;

				return mOutSample;
			} // process sample for audio


			real_t ProcessSampleForLFO(bool forceSilence)
			{
				float mFreqModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID + (int)LFOModParamIndexOffsets::FrequencyParam);
				float mWaveShapeModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID + (int)LFOModParamIndexOffsets::Waveshape);
				mPhaseModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID + (int)LFOModParamIndexOffsets::Phase);
				if (!mnSamples)// NOTE: important that this is designed to be 0 the first run to force initial calculation.
				{
					float freq = mpSrcDevice->mParams.GetFrequency(LFOParamIndexOffsets::FrequencyParam, -1, gLFOFreqConfig, 0, mFreqModVal);
					 // 0 frequencies would cause math problems, denormals, infinites... but fortunately they're inaudible so...
					freq = std::max(freq, 0.001f);
					mCurrentFreq = freq;

					float waveshape = mpOscDevice->mParams.Get01Value(LFOParamIndexOffsets::Waveshape, mWaveShapeModVal);
					mpSlaveWave->SetParams(freq, GetPhaseOffset(), waveshape, Helpers::CurrentSampleRate, OscillatorIntention::LFO);

					double newDT = (double)freq / Helpers::CurrentSampleRate;
					mPhaseIncrement = newDT;
				}
				mnSamples = (mnSamples + 1) & GetModulationRecalcSampleMask();

				mPhase = math::fract(mPhase + mPhaseIncrement);

				if (forceSilence) {
					mOutSample = mCurrentSample = 0;
					return 0;
				}

				mpSlaveWave->OSC_ADVANCE(1, 0); // advance phase
				mOutSample = mCurrentSample = mpSlaveWave->NaiveSample(math::fract(float(mPhase + mPhaseModVal)));

				return mOutSample;
			} // process sample for lfo
		};
	} // namespace M7


} // namespace WaveSabreCore







