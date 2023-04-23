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
			//SourceInfo& mSourceInfo;
			ParamAccessor mParams;
			bool mEnabledCache;

			const ModSource mAmpEnvModSourceID;
			const ModDestination mModDestBaseID;
			const ModDestination mVolumeModDestID;
			const ModDestination mAuxPanModDestID;
			const ModDestination mHiddenVolumeModDestID;

			ModulationSpec* mAmpEnvModulation;

			virtual bool IsEnabled() const = 0;
			virtual bool MatchesKeyRange(int midiNote) const = 0;
			virtual float GetAuxPan() const = 0; // does not account for modulation
			virtual float GetLinearVolume(float mod) const = 0;
			// device-level values set at the beginning of block processing
			float mDetuneDeviceModAmt = 0;
			float mAuxPanDeviceModAmt = 0;

			virtual ~ISoundSourceDevice()
			{}

			ISoundSourceDevice(float* paramCache, ModulationSpec* ampEnvModulation, ParamIndices baseParamID,
				ModSource ampEnvModSourceID, ModDestination modDestBaseID, ModDestination volumeModDestID, ModDestination auxPanModDestID, ModDestination hiddenVolumeModDestID
			) :
				mParams(paramCache, baseParamID),
				mAmpEnvModulation(ampEnvModulation),
				mAmpEnvModSourceID(ampEnvModSourceID),
				mVolumeModDestID(volumeModDestID),
				mAuxPanModDestID(auxPanModDestID),
				mModDestBaseID(modDestBaseID),
				mHiddenVolumeModDestID(hiddenVolumeModDestID)
			{
			}

			void InitDevice() {
				mAmpEnvModulation->SetSourceAmp(mAmpEnvModSourceID, mHiddenVolumeModDestID, &mEnabledCache);
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

				// used as temporary values during block processing.
				float mOutputGain[2];// = { 0 }; // linear output volume gain calculated from output VolumeParam + panning
				float mAmpEnvGain;// = { 0 }; // linear gain calculated frequently from osc ampenv

				virtual void NoteOn(bool legato) = 0;
				virtual void NoteOff() = 0;
				virtual void BeginBlock() = 0;
				virtual float GetLastSample() const = 0;

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
			"SineClip",\
			"SineHarmTrunc",\
			"TriClip",\
			"TriSquare",\
			"TriTrunc",\
			"VarTrapezoidHard",\
			"VarTrapezoidSoft",\
			"VarTriangle",\
			"WhiteNoiseSH",\
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
			OscillatorIntention mIntention = OscillatorIntention::LFO;

			virtual float NaiveSample(float phase01) = 0; // return amplitude at phase
			virtual float NaiveSampleSlope(float phase01) = 0; // return slope at phase			
			virtual void AfterSetParams() = 0;
			// offers waveforms the opportunity to accumulate bleps along the advancement.
			virtual void Visit(FloatPair& bleps, double newPhase, float samples, float samplesTillNextSample) {}

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
			// returns blep before and blep after discontinuity.
			FloatPair OSC_RESTART(float samplesBeforeNext)
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

				return { blepBefore, blepAfter };
			}

			void OSC_ACCUMULATE_BLEP(FloatPair& bleps, double newPhase, float edge, float blepScale, float samples, float samplesFromNewPositionUntilNextSample)
			{
				if (!math::DoesEncounter(mPhase, newPhase, edge))
					return;
				float samplesSinceEdge = float(math::fract(newPhase - edge) / this->mPhaseIncrement);
				float samplesFromEdgeToNextSample = math::fract(samplesSinceEdge + samplesFromNewPositionUntilNextSample);
				bleps.first = blepScale * BlepBefore(samplesFromEdgeToNextSample);
				bleps.second = blepScale * BlepAfter(samplesFromEdgeToNextSample);
			}

			void OSC_ACCUMULATE_BLAMP(FloatPair& bleps, double newPhase, float edge, float blampScale, float samples, float samplesFromNewPositionUntilNextSample)
			{
				if (!math::DoesEncounter((mPhase), (newPhase), edge))
					return;

				float samplesSinceEdge = float(math::fract(newPhase - edge) / float(this->mPhaseIncrement));
				float samplesFromEdgeToNextSample = math::fract(samplesSinceEdge + samplesFromNewPositionUntilNextSample);

				bleps.first += blampScale * BlampBefore(samplesFromEdgeToNextSample);
				bleps.second += blampScale * BlampAfter(samplesFromEdgeToNextSample);
			}


			// samples is 0<samples<1
			// assume this.phase is currently 0<t<1
			// this.phase may not be on a sample boundary.
			// returns blep before and blep after discontinuity.
			virtual FloatPair OSC_ADVANCE(float samples, float samplesTillNextSample)
			{
				//mPhaseIncrement += mDTDT * samples;
				double phaseToAdvance = samples * mPhaseIncrement;
				//double newPhase = math::fract(mPhase + phaseToAdvance); // advance slave; doing it here helps us calculate discontinuity.
				double newPhase = mPhase + phaseToAdvance;
				if (newPhase > 1) newPhase -= 1; // slightly faster t han fract()
				FloatPair bleps{ 0.0f,0.0f };
				if (mIntention == OscillatorIntention::Audio) {
					Visit(bleps, newPhase, samples, samplesTillNextSample);
				}
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

			virtual void AfterSetParams() override
			{
				float waveshape = std::abs(mShape * 2 - 1); // create reflection to make bipolar
				waveshape = 1 - waveshape;
				waveshape = 1 - (waveshape * waveshape);
				mShape = math::lerp(0, 0.95f, waveshape);
				//mShape = Clamp(waveshape * 0.9f, 0, 1);

				mFlatValue = mShape;

				mDCOffset = -mShape; // offset so we can scale to fill both axes
				mScale = 1 / (1.0f - mShape); // scale it up so it fills both axes
				//mScale *= gOscillatorHeadroomScalar;
			}

			// samples is 0<samples<1
			// assume this.phase is currently 0<t<1
			// this.phase may not be on a sample boundary.
			// returns blep before and blep after discontinuity.
			virtual void Visit(FloatPair& bleps, double newPhase, float samples, float samplesTillNextSample) override
			{
				//mPhaseIncrement += mDTDT * samples;
				//double phaseToAdvance = samples * mPhaseIncrement;
				//double newPhase = Fract(mPhase + phaseToAdvance); // advance slave; doing it here helps us calculate discontinuity.

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
			}

			// this is not improved by returing correct slope. blepping curves is too hard 4 me.
			virtual float NaiveSampleSlope(float phase01) override
			{
				if ((phase01 >= mEdge1) && (phase01 < mEdge2)) {
					return 0;
				}
				return math::cos(math::gPITimes2 * phase01);
			}

			virtual void AfterSetParams() override
			{
				mShape = std::abs(mShape * 2 - 1); // create reflection to make bipolar

				mEdge1 = (0.75f - mShape * .25f);
				mEdge2 = (0.75f + mShape * .25f);
				mFlatValue = math::sin(mEdge1 * 2 * math::gPI); // the Y value that gets sustained

				mDCOffset = -(.5f + .5f * mFlatValue); // offset so we can scale to fill both axes
				mScale = 1.0f / (.5f - .5f * mFlatValue); // scale it up so it fills both axes
			}

			virtual void Visit(FloatPair& bleps, double newPhase, float samples, float samplesTillNextSample) override
			{
				float scale = float(mPhaseIncrement * math::gPI * -math::cos(math::gPITimes2 * mEdge1));
				OSC_ACCUMULATE_BLAMP(bleps, newPhase, mEdge1, scale, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLAMP(bleps, newPhase, mEdge2, scale, samples, samplesTillNextSample);
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

			virtual void Visit(FloatPair& bleps, double newPhase, float samples, float samplesTillNextSample) override
			{
				OSC_ACCUMULATE_BLEP(bleps, newPhase, 0, -1, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLEP(bleps, newPhase, mShape, 1, samples, samplesTillNextSample);
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
		struct WhiteNoiseWaveform :IOscillatorWaveform
		{
			OnePoleFilter mHPFilter;
			float mCurrentLevel = 0;
			float mCurrentSample = 0;

			// returns Y value at specified phase. instance / stateless.
			virtual float NaiveSample(float phase01) override
			{
				return mCurrentSample;// math::randN11();// Helpers::RandFloat() * 2 - 1;
			}

			virtual float NaiveSampleSlope(float phase01) override
			{
				return 0;
			}

			virtual FloatPair OSC_ADVANCE(float samples, float samplesTillNextSample)
			{
				// assume we're always advancing by 1 exact sample. it theoretically breaks hard sync but like, who the f is hard-syncing a white noise channel?
				
				mPhase += mPhaseIncrement;
				if (mPhase > 1) {
					mCurrentLevel = math::randN11();
					mPhase = math::fract(mPhase);
				}

				mCurrentSample = mHPFilter.InlineProcessSample(mCurrentLevel);

				return { 0,0 };
			}


			virtual void AfterSetParams() override
			{
				float kt = 0;
				FrequencyParam fp{ mShape, kt, { mFrequency, 6, 0 /*assume never used*/}};
				float lfoFreqShape = mFrequency * mShape * mShape;// *0.25f;

				mHPFilter.SetParams(FilterType::HP, (mIntention == OscillatorIntention::LFO) ? lfoFreqShape : fp.GetFrequency(0, 0), 0);
			}

		};




		/////////////////////////////////////////////////////////////////////////////
		struct VarTriWaveform :IOscillatorWaveform
		{
			virtual void AfterSetParams() override
			{
				mShape = math::lerp(0.99f, .01f, mShape); // just prevent div0
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

			virtual void Visit(FloatPair& bleps, double newPhase, float samples, float samplesTillNextSample) override
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

			virtual void Visit(FloatPair& bleps, double newPhase, float samples, float samplesTillNextSample) override
			{
				OSC_ACCUMULATE_BLEP(bleps, newPhase, 0, .5f, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLEP(bleps, newPhase, mT2, -.5f, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLEP(bleps, newPhase, mT3, -.5f, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLEP(bleps, newPhase, mT4, .5f, samples, samplesTillNextSample);
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


		/////////////////////////////////////////////////////////////////////////////
		struct TriClipWaveform :IOscillatorWaveform
		{
			virtual void AfterSetParams() override
			{
				mShape = std::abs(mShape * 2 - 1); // create reflection to make bipolar
				mShape = math::lerp(0.97f, 0.03f, mShape);
				mDCOffset = -.5;//-.5 * this.shape;
				mScale = 2;//1/(.5+this.DCOffset);
				//mScale *= gOscillatorHeadroomScalar;
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
			}

			virtual void Visit(FloatPair& bleps, double newPhase, float samples, float samplesTillNextSample) override
			{
				float scale = float(mPhaseIncrement / mShape);
				OSC_ACCUMULATE_BLAMP(bleps, newPhase, 0/*edge*/, scale, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLAMP(bleps, newPhase, mShape * .5f/*edge*/, -2 * scale, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLAMP(bleps, newPhase, mShape/*edge*/, scale, samples, samplesTillNextSample);
			}
		};


		///////////////////////////////////////////////////////////////////////////////
		//struct TriSquareWaveform :IOscillatorWaveform
		//{
		//	float mT1 = 0;
		//	float mT2 = 0;
		//	float mT3 = 0;
		//	virtual void SetParams(float freq, float phaseOffset, float waveshape, double sampleRate) override
		//	{
		//		IOscillatorWaveform::SetParams(freq, phaseOffset, waveshape, sampleRate);
		//		waveshape = std::abs(waveshape * 2 - 1); // create reflection to make bipolar
		//		mShape = math::lerp(0, .49f, waveshape * waveshape);

		//		mT1 = .5f - mShape;
		//		mT2 = .5f;
		//		mT3 = 1 - mShape;
		//	}

		//	virtual float NaiveSample(float t) override
		//	{
		//		/*
		//		  // my original impl created a tall triangle wave, and then clipped it.
		//		  // that is elegant, but when dealing with fixed point arith, those amplitudes will definitely get out of bounds.
		//		  // and i don't want to rely on saturating arith.
		//		  // so as with the others, let's REMAKE it.

		//		  //         shape describes the width of the flat areas.
		//		  //         0<shape<0.5. shape cannot be 0.5 exactly; it must be minus at least 1 sample length
		//		  //       1-|    /````\
		//		  //         |   /      \
		//		  //       0-|  /        \
		//		  //         | /          \
		//		  //      -1-|/            \____
		//		  //          ^    <==>     <==>
		//		  //          ^   t1  t2    t3  ^
		//		  //          0       .5        1
		//		  //          t2-t1 = 1-t3 = shape
		//		  //          t1-t0 = (1-shape*2)/2 = .5-shape
		//		  //          slope = 2 / shape
		//		  (t<this.t1) ? (
		//			(t/this.t1)*2-1
		//		  ):(t<this.t2)?(
		//			1
		//		  ):(t<this.t3)?(
		//			1-(t-.5)/this.t1*2
		//		  ):(
		//			-1
		//		  );
		//		*/
		//		if (t < mT1) {
		//			return (t / mT1) * 2 - 1;
		//		}
		//		if (t < mT2) {
		//			return 1;
		//		}
		//		if (t < mT3) {
		//			return 1 - (t - 0.5f) / mT1 * 2;
		//		}
		//		return -1;
		//	}

		//	virtual float NaiveSampleSlope(float t) override
		//	{
		//		/*
		//		  (t<this.t1) ? (
		//			1/this.t1;
		//		  ):(t<this.t2)?(
		//			0
		//		  ):(t<this.t3)?(
		//			-1/this.t1;
		//		  ):(
		//			0
		//		  );
		//		*/
		//		if (t < mT1) {
		//			return 1 / mT1;
		//		}
		//		if (t < mT2) {
		//			return 0;
		//		}
		//		if (t < mT3) {
		//			return -1 / mT1;
		//		}
		//		return 0;
		//	}

		//	virtual void Visit(std::pair<float, float>& bleps, double newPhase, float samples, float samplesTillNextSample) override
		//	{
		//		float scale = float(mPhaseIncrement / mT1);
		//		OSC_ACCUMULATE_BLAMP(bleps, newPhase, 0/*edge*/, scale, samples, samplesTillNextSample);
		//		OSC_ACCUMULATE_BLAMP(bleps, newPhase, mT1/*edge*/, -scale, samples, samplesTillNextSample);
		//		OSC_ACCUMULATE_BLAMP(bleps, newPhase, mT2/*edge*/, -scale, samples, samplesTillNextSample);
		//		OSC_ACCUMULATE_BLAMP(bleps, newPhase, mT3/*edge*/, scale, samples, samplesTillNextSample);
		//	}

		//};


		/////////////////////////////////////////////////////////////////////////////
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
				tx = math::abs(tx) * 4 - 1;
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

			virtual void Visit(FloatPair& bleps, double newPhase, float samples, float samplesTillNextSample) override
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

			virtual void Visit(FloatPair& bleps, double newPhase, float samples, float samplesTillNextSample) override
			{
				float scale = float(mPhaseIncrement / (mSlope));
				OSC_ACCUMULATE_BLAMP(bleps, newPhase, 0/*edge*/, -scale, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLAMP(bleps, newPhase, mT1/*edge*/, -scale, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLAMP(bleps, newPhase, mT2/*edge*/, scale, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLAMP(bleps, newPhase, mT3/*edge*/, scale, samples, samplesTillNextSample);
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

			virtual float GetAuxPan() const override
			{
				// only valid for audio (no lfo)
				return mParams.GetN11Value(OscParamIndexOffsets::AuxMix, 0);
			}

			virtual float GetLinearVolume(float mod) const override
			{
				return mParams.GetLinearVolume(OscParamIndexOffsets::Volume, gUnityVolumeCfg, mod);
			}

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
			//double mDTDT = 0; // to smooth frequency changes without having expensive recalc frequency every sample, just linearly adjust phaseincrement (DT) every sample over the block.
			float mCurrentSample = 0;
			float mOutSample = 0;
			float mPrevSample = 0;
			float mFMFeedbackAmt = 0; // adjusted for global FM scale
			float mCurrentFreq = 0;

			float mFreqModVal = 0;
			float mPitchFineModVal = 0;
			float mWaveShapeModVal = 0;
			float mSyncFreqModVal = 0;
			float mFMFeedbackModVal = 0;
			float mPhaseModVal = 0;

			PulsePWMWaveform mPulsePWMWaveform;
			PulseTristateWaveform mPulseTristateWaveform;
			SawClipWaveform mSawClipWaveform;
			SineClipWaveform mSineClipWaveform;
			SineHarmTruncWaveform mSineHarmTruncWaveform;
			TriClipWaveform mTriClipWaveform;
			TriSquareWaveform mTriSquareWaveform;
			TriTruncWaveform mTriTruncWaveform;
			VarTrapezoidWaveform mVarTrapezoidHardWaveform { gVarTrapezoidHardSlope };
			VarTrapezoidWaveform mVarTrapezoidSoftWaveform { gVarTrapezoidSoftSlope };
			VarTriWaveform mVarTriWaveform;
			WhiteNoiseWaveform mWhiteNoiseWaveform;
			IOscillatorWaveform* mpSlaveWave = &mSawClipWaveform;

			OscillatorNode(ModMatrixNode* pModMatrix, OscillatorDevice* pOscDevice, EnvelopeNode* pAmpEnv) :
				ISoundSourceDevice::Voice(pOscDevice, pModMatrix, pAmpEnv),
				mpOscDevice(pOscDevice)
			{
			}

			virtual float GetLastSample() const override { return mOutSample; }

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
				//switch (mpOscDevice->mWaveform.GetEnumValue()) {
				auto w = mpOscDevice->mParams.GetEnumValue<OscillatorWaveform>(mpOscDevice->mIntention == OscillatorIntention::Audio ? (int)OscParamIndexOffsets::Waveform : (int)LFOParamIndexOffsets::Waveform);
				switch (w) {
				case OscillatorWaveform::Pulse:
					mpSlaveWave = &mPulsePWMWaveform;
					break;
				case OscillatorWaveform::PulseTristate:
					mpSlaveWave = &mPulseTristateWaveform;
					break;
				case OscillatorWaveform::SawClip:
					mpSlaveWave = &mSawClipWaveform;
					break;
				case OscillatorWaveform::SineClip:
					mpSlaveWave = &mSineClipWaveform;
					break;
				case OscillatorWaveform::SineHarmTrunc:
					mpSlaveWave = &mSineHarmTruncWaveform;
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
			}

			real_t ProcessSampleForAudio(real_t midiNote, float detuneFreqMul, float fmScale, real_t signal1, real_t signal1PMAmount, real_t signal2, real_t signal2PMAmount, real_t signal3, real_t signal3PMAmount)
			{
				if (!this->mpSrcDevice->mEnabledCache) {
					mOutSample = mCurrentSample = 0;
					return 0;
				}

				bool syncEnable = mpOscDevice->mParams.GetBoolValue(OscParamIndexOffsets::SyncEnable);

				if (0 == mnSamples) // NOTE: important that this is designed to be 0 the first run to force initial calculation.
				{
					mSyncFreqModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID + (int)OscModParamIndexOffsets::SyncFrequency);
					mFreqModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID + (int)OscModParamIndexOffsets::FrequencyParam);
					mFMFeedbackModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID + (int)OscModParamIndexOffsets::FMFeedback);
					mWaveShapeModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID + (int)OscModParamIndexOffsets::Waveshape);
					mPhaseModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID + (int)OscModParamIndexOffsets::Phase);
					mPitchFineModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID + (int)OscModParamIndexOffsets::PitchFine);

					mFMFeedbackAmt = mpOscDevice->mParams.Get01Value(OscParamIndexOffsets::FMFeedback, mFMFeedbackModVal) * fmScale * 0.5f;
					//mFMFeedbackAmt = mpOscDevice->mFMFeedback01.Get01Value(mFMFeedbackModVal) * fmScale * 0.5f;

					// - osc pitch semis                  note         oscillator                  
					// - osc fine (semis)                 note         oscillator                   
					// - osc sync freq / kt (Hz)          hz           oscillator                        
					// - osc freq / kt (Hz)               hz           oscillator                   
					// - osc mul (Hz)                     hz           oscillator             
					// - osc detune (semis)               hz+semis     oscillator                         
					// - unisono detune (semis)           hz+semis     oscillator                             
					int pitchSemis = mpSrcDevice->mParams.GetIntValue(OscParamIndexOffsets::PitchSemis, gSourcePitchSemisRange);
					float pitchFine = mpSrcDevice->mParams.GetN11Value(OscParamIndexOffsets::PitchFine, mPitchFineModVal) * gSourcePitchFineRangeSemis;
					midiNote += pitchSemis + pitchFine;// (mpSrcDevice->mPitchFineParam.mCachedVal + mPitchFineModVal)* gSourcePitchFineRangeSemis;
					float noteHz = math::MIDINoteToFreq(midiNote);
					float freq = mpSrcDevice->mParams.GetFrequency(OscParamIndexOffsets::FrequencyParam, OscParamIndexOffsets::FrequencyParamKT, gSourceFreqConfig, noteHz, mFreqModVal);//mpSrcDevice->mFrequencyParam.GetFrequency(noteHz, mFreqModVal);
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
				//mPhaseIncrement += mDTDT;
				mPhase = math::fract(mPhase + mPhaseIncrement);

				float phaseMod =
					mPrevSample * mFMFeedbackAmt
					+ signal1 * signal1PMAmount
					+ signal2 * signal2PMAmount
					+ signal3 * signal3PMAmount
					//+ mpOscDevice->mPhaseOffset.GetN11Value(mPhaseModVal)
					;

				if (mPhase >= mPhaseIncrement || !syncEnable) // did not cross cycle. advance 1 sample
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
				mCurrentSample = math::clampN11(mCurrentSample); // prevent FM from going crazy.
				mOutSample = (mPrevSample + mpSlaveWave->mDCOffset) * mpSlaveWave->mScale * gOscillatorHeadroomScalar;

				return mOutSample;
			} // process sample for audio


			real_t ProcessSampleForLFO(bool forceSilence)
			{
				if (!mnSamples)// NOTE: important that this is designed to be 0 the first run to force initial calculation.
				{
					mFreqModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID + (int)LFOModParamIndexOffsets::FrequencyParam);
					mWaveShapeModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID + (int)LFOModParamIndexOffsets::Waveshape);
					mPhaseModVal = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID + (int)LFOModParamIndexOffsets::Phase);

					//float freq = mpSrcDevice->mFrequencyParam.GetFrequency(0, mFreqModVal);
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

				auto bleps = mpSlaveWave->OSC_ADVANCE(1, 0); // advance phase
				mOutSample = mCurrentSample = mpSlaveWave->NaiveSample(math::fract(float(mPhase + mPhaseModVal)));

				return mOutSample;
			} // process sample for lfo
		};
	} // namespace M7


} // namespace WaveSabreCore




