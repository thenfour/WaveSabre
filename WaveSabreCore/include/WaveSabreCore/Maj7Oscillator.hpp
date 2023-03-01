#pragma once

#include <WaveSabreCore/Maj7Basic.hpp>
#include <WaveSabreCore/Maj7ModMatrix.hpp>

namespace WaveSabreCore
{
	namespace M7
	{
		enum class OscillatorWaveform : uint8_t
		{
			SawClip,
			Sine,
			WhiteNoiseSH,
			Count,
		};
		// size-optimize using macro
		#define OSCILLATOR_WAVEFORM_CAPTIONS { \
            "Disabled", \
            "LP_OnePole", \
            "LP_SEM12", \
            "LP_Diode", \
            "LP_K35", \
            "LP_Moog2", \
            "LP_Moog4", \
            "BP_Moog2", \
            "BP_Moog4", \
            "HP_OnePole", \
            "HP_K35", \
            "HP_Moog2", \
            "HP_Moog4", \
        }

		/////////////////////////////////////////////////////////////////////////////
		struct IOscillatorWaveform
		{
			float mShape = 0.5; // wave shape
			float mPhaseOffset = 0;

			float mDCOffset = 0; // while we could try to bake our wave generator with correct DC, it's very inconvenient. This is just way easier.
			float mScale = 1; // same here; way easier to just scale in post than to try and get everything scaled correctly during generation.

			float mFrequency = 0;
			float mPhase = 0; // phase cursor 0-1
			float mPhaseIncrement = 0;
			//float mSyncMasterFrequency = 0;

			virtual float NaiveSample(float phase01) = 0; // return amplitude at phase
			virtual float NaiveSampleSlope(float phase01) = 0; // return slope at phase			
			virtual std::pair<float, float> OSC_ADVANCE(float samples, float samplesTillNextSample) = 0;// returns blep before and blep after discontinuity.

			// override if you need to adjust things
			virtual void SetParams(float freq, float phaseOffset, float waveshape/*, float syncmasterFreq*/)
			{
				mFrequency = freq;
				mPhaseOffset = Fract(phaseOffset);
				mShape = waveshape;
				//mSyncMasterFrequency = syncmasterFreq;
				mPhaseIncrement = mFrequency * Helpers::CurrentSampleRateRecipF;
			}

			// process discontinuity due to restarting phase right now.
			// returns blep before and blep after discontinuity.
			virtual std::pair<float, float> OSC_RESTART(float samplesBeforeNext)
			{
				float sampleBefore = this->NaiveSample(this->mPhase);
				float newPhase = this->mPhaseOffset;

				// fix bleps
				float sampleAfter = NaiveSample(newPhase);
				float blepScale = (sampleAfter - sampleBefore) * .5f; // full sample scale is 2; *.5 to bring 0-1

				float blepBefore = blepScale * BlepBefore(samplesBeforeNext); // blep the phase restart.
				float blepAfter = blepScale * BlepAfter(samplesBeforeNext);

				// fix blamps.
				float slopeBefore = NaiveSampleSlope(mPhase);
				float slopeAfter = NaiveSampleSlope(newPhase);
				float blampScale = this->mPhaseIncrement * (slopeAfter - slopeBefore);
				blepBefore += blampScale * BlampBefore(samplesBeforeNext);
				blepAfter += blampScale * BlampAfter(samplesBeforeNext);

				mPhase = newPhase;

				return std::make_pair(blepBefore, blepAfter);
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

			// returns Y value at specified phase. instance / stateless.
			virtual float NaiveSampleSlope(float phase01) override
			{
				if (phase01 < mShape) {
					return 0;
				}
				return 1;
			}

			virtual void SetParams(float freq, float phaseOffset, float waveshape) override
			{
				IOscillatorWaveform::SetParams(freq, phaseOffset, waveshape);

				waveshape = 1 - waveshape;
				mShape = Clamp(1 - (waveshape * waveshape), 0, 0.9f);

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
				float phaseToAdvance = samples * mPhaseIncrement;
				float newPhase = Fract(mPhase + phaseToAdvance); // advance slave; doing it here helps us calculate discontinuity.

				std::pair<float, float> bleps{ 0.0f,0.0f };
				float blampScale = mPhaseIncrement;
				float blepScale = -(1.0f - mShape);

				OSC_ACCUMULATE_BLEP(bleps, newPhase, 0/*edge*/, blepScale, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLAMP(bleps, newPhase, 0/*edge*/, -blampScale, samples, samplesTillNextSample);
				OSC_ACCUMULATE_BLAMP(bleps, newPhase, this->mShape/*edge*/, blampScale, samples, samplesTillNextSample);

				this->mPhase = newPhase;
				return bleps;
			}

			void OSC_ACCUMULATE_BLEP(std::pair<float, float>& bleps, float newPhase, float edge, float blepScale, float samples, float samplesFromNewPositionUntilNextSample)
			{
				if (!DoesEncounter(mPhase, newPhase, edge))
					return;
				float samplesSinceEdge = Fract(newPhase - edge) / this->mPhaseIncrement;
				float samplesFromEdgeToNextSample = Fract(samplesSinceEdge + samplesFromNewPositionUntilNextSample);
				bleps.first = blepScale * BlepBefore(samplesFromEdgeToNextSample);
				bleps.second = blepScale * BlepAfter(samplesFromEdgeToNextSample);
			}

			void OSC_ACCUMULATE_BLAMP(std::pair<float, float>& bleps, float newPhase, float edge, float blampScale, float samples, float samplesFromNewPositionUntilNextSample)
			{
				if (!DoesEncounter(mPhase, newPhase, edge))
					return;

				float samplesSinceEdge = Fract(newPhase - edge) / this->mPhaseIncrement;
				float samplesFromEdgeToNextSample = Fract(samplesSinceEdge + samplesFromNewPositionUntilNextSample);

				bleps.first += blampScale * BlampBefore(samplesFromEdgeToNextSample);
				bleps.second += blampScale * BlampAfter(samplesFromEdgeToNextSample);
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

		struct OscillatorNode
		{
			static constexpr real_t gVolumeMaxDb = 0; // shared by amp env max db and must be 0 anyway for optimized calcs to work.
			static constexpr real_t gSyncFrequencyCenterHz = 1000;
			static constexpr real_t gSyncFrequencyScale = 10;
			static constexpr real_t gFrequencyCenterHz = 1000;
			static constexpr real_t gFrequencyScale = 10;
			static constexpr real_t gLFOFrequencyCenterHz = 1.0f;
			static constexpr real_t gLFOFrequencyScale = 8;
			static constexpr int gPitchSemisRange = 24;
			static constexpr real_t gFrequencyMulMax = 64;

			// BASE PARAMS
			BoolParam mEnabled;// Osc2Enabled,
			//VolumeParam mVolume; //	Osc2Volume,
			EnumParam<OscillatorWaveform> mWaveform;
			Float01Param mWaveshape;//	Osc2Waveshape,
			BoolParam mPhaseRestart;//	Osc2PhaseRestart,
			FloatN11Param mPhaseOffset;//	Osc2PhaseOffset,
			BoolParam mSyncEnable;//	Osc2SyncEnable,
			FrequencyParam mSyncFrequency;//	Osc2SyncFrequency,//	Osc2SyncFrequencyKT,
			FrequencyParam mFrequency;//	Osc2FrequencyParam,//Osc2FrequencyParamKT,
			IntParam mPitchSemis;//	Osc2PitchSemis,
			FloatN11Param mPitchFine;//	Osc2PitchFine,
			ScaledRealParam mFrequencyMul;//	Osc2FreqMul,. FM8 allows 0-64
			Float01Param mFMFeedback01;// Osc2FMFeedback,

			// LFO backing values
			float mLFOConst1 = 1.0f;
			float mLFOConst0 = 0.0f;
			float mLFOConstN1 = -1.0f;
			float mLFOVolumeParamValue = 1.0f;
			float mLFOSyncFrequencyParamValue = 1.0f;
			float mLFOPitchSemisParamValue = 0.5f;
			float mLFOPitchFineParamValue = 0.5f;
			float mLFOFrequencyMulParamValue = 0.5f;
			float mLFOFMFeedbackParamValue = 0.0f;

			// for Audio
			explicit OscillatorNode(OscillatorIntentionAudio, ModMatrixNode& modMatrix, ModDestination modDestBase, real_t* paramCache, int paramBaseID) :
				mEnabled(paramCache[paramBaseID + (int)OscParamIndexOffsets::Enabled], false),
				//mVolume(paramCache[paramBaseID + (int)OscParamIndexOffsets::Volume], gVolumeMaxDb, 1),
				mWaveform(paramCache[paramBaseID + (int)OscParamIndexOffsets::Waveform], OscillatorWaveform::Count, OscillatorWaveform::Sine),
				mWaveshape(paramCache[paramBaseID + (int)OscParamIndexOffsets::Waveshape], 0),
				mPhaseRestart(paramCache[paramBaseID + (int)OscParamIndexOffsets::PhaseRestart], false),
				mPhaseOffset(paramCache[paramBaseID + (int)OscParamIndexOffsets::PhaseOffset], 0),
				mSyncEnable(paramCache[paramBaseID + (int)OscParamIndexOffsets::SyncEnable], false),
				mSyncFrequency(paramCache[paramBaseID + (int)OscParamIndexOffsets::SyncFrequency], paramCache[paramBaseID + (int)OscParamIndexOffsets::SyncFrequencyKT], gSyncFrequencyCenterHz, gSyncFrequencyScale, 0.4f, 1.0f),
				mFrequency(paramCache[paramBaseID + (int)OscParamIndexOffsets::FrequencyParam], paramCache[paramBaseID + (int)OscParamIndexOffsets::FrequencyParamKT], gFrequencyCenterHz, gFrequencyScale, 0.4f, 1.0f),
				mPitchSemis(paramCache[paramBaseID + (int)OscParamIndexOffsets::PitchSemis], -gPitchSemisRange, gPitchSemisRange, 0),
				mPitchFine(paramCache[paramBaseID + (int)OscParamIndexOffsets::PitchFine], 0),
				mFrequencyMul(paramCache[paramBaseID + (int)OscParamIndexOffsets::FreqMul], 0.0f, gFrequencyMulMax, 1.0f),
				mFMFeedback01(paramCache[paramBaseID + (int)OscParamIndexOffsets::FMFeedback], 0),

				mIntention(OscillatorIntention::Audio),
				mModMatrix(modMatrix),
				mModDestBase((int)modDestBase)
			{}

			// for LFO, we internalize many params
			explicit OscillatorNode(OscillatorIntentionLFO, ModMatrixNode& modMatrix, ModDestination modDestBase, real_t* paramCache, int paramBaseID) :
				mEnabled(mLFOConst1, false),
				//mVolume(mLFOVolumeParamValue, 0, 1),
				mWaveform(paramCache[paramBaseID + (int)LFOParamIndexOffsets::Waveform], OscillatorWaveform::Count, OscillatorWaveform::Sine),
				mWaveshape(paramCache[paramBaseID + (int)LFOParamIndexOffsets::Waveshape], 0),
				mPhaseRestart(paramCache[paramBaseID + (int)LFOParamIndexOffsets::Restart], false),
				mPhaseOffset(paramCache[paramBaseID + (int)LFOParamIndexOffsets::PhaseOffset], 0),
				mSyncEnable(mLFOConst0, false),
				mSyncFrequency(mLFOSyncFrequencyParamValue, mLFOConst1, gSyncFrequencyCenterHz, gSyncFrequencyScale, 0.4f, 0),
				mFrequency(paramCache[paramBaseID + (int)LFOParamIndexOffsets::FrequencyParam], mLFOConst0, gLFOFrequencyCenterHz, gLFOFrequencyScale, 0.1f, 0),
				mPitchSemis(mLFOPitchSemisParamValue, -24, 24, 0),
				mPitchFine(mLFOPitchFineParamValue, 0),
				mFrequencyMul(mLFOFrequencyMulParamValue, 0.0f, 64.0f, 1.0f),
				mFMFeedback01(mLFOFMFeedbackParamValue, 0),

				mIntention(OscillatorIntention::LFO),
				mModMatrix(modMatrix),
				mModDestBase((int)modDestBase)
			{}

			// state
			float mPhase = 0;
			float mPhaseIncrement = 0;
			float mCurrentSample = 0;
			float mOutSample = 0;
			float mPrevSample = 0;

			SawClipWaveform mSawClip;
			IOscillatorWaveform* mpSlaveWave = &mSawClip;

			OscillatorIntention mIntention;
			ModMatrixNode& mModMatrix;
			int mModDestBase; // ModDestination enum value representing the 1st mod value. for LFOs that's waveshape, for audio that's volume.

			float mFreqModVal = 0;
			float mWaveShapeModVal = 0;
			float mSyncFreqModVal = 0;
			float mFMFeedbackModVal = 0;
			float mPhaseModVal = 0;

			real_t GetSample() const {
				return mOutSample;
			}

			void BeginBlock(real_t midiNote, float detuneFreqMul)
			{
				if (!mEnabled.GetBoolValue()) {
					return;
				}
				switch (mIntention) {
				case OscillatorIntention::LFO:
					mFreqModVal = mModMatrix.GetDestinationValue(mModDestBase + (int)LFOModParamIndexOffsets::FrequencyParam, 0);
					mWaveShapeModVal = mModMatrix.GetDestinationValue(mModDestBase + (int)LFOModParamIndexOffsets::Waveshape, 0);
					break;
				case OscillatorIntention::Audio:
					//mVolumeModVal = mModMatrix.GetDestinationValue(mModDestBase + (int)OscModParamIndexOffsets::Volume, 0);
					mSyncFreqModVal = mModMatrix.GetDestinationValue(mModDestBase + (int)OscModParamIndexOffsets::SyncFrequency, 0);
					mFreqModVal = mModMatrix.GetDestinationValue(mModDestBase + (int)OscModParamIndexOffsets::FrequencyParam, 0);
					mFMFeedbackModVal = mModMatrix.GetDestinationValue(mModDestBase + (int)OscModParamIndexOffsets::FMFeedback, 0);
					mWaveShapeModVal = mModMatrix.GetDestinationValue(mModDestBase + (int)OscModParamIndexOffsets::Waveshape, 0);
					mPhaseModVal = mModMatrix.GetDestinationValue(mModDestBase + (int)OscModParamIndexOffsets::Phase, 0);
					break;
				}

				// - osc pitch semis                  note         oscillator                  
				// - osc fine (semis)                 note         oscillator                   
				// - osc sync freq / kt (Hz)          hz           oscillator                        
				// - osc freq / kt (Hz)               hz           oscillator                   
				// - osc mul (Hz)                     hz           oscillator             
				// - osc detune (semis)               hz+semis     oscillator                         
				// - unisono detune (semis)           hz+semis     oscillator                             
				midiNote += mPitchSemis.GetIntValue() + mPitchFine.GetN11Value();
				float noteHz = MIDINoteToFreq(midiNote);
				float freq = mFrequency.GetFrequency(noteHz, mFreqModVal);
				freq *= mFrequencyMul.GetRangedValue();
				freq *= detuneFreqMul;
				mPhaseIncrement = freq / (real_t)Helpers::CurrentSampleRate;
				float slaveFreq = mSyncEnable.GetBoolValue() ? mSyncFrequency.GetFrequency(noteHz, mSyncFreqModVal) : freq;
				mpSlaveWave->SetParams(slaveFreq, mPhaseOffset.GetN11Value(mPhaseModVal), mWaveshape.Get01Value(mWaveShapeModVal));// , mSyncFrequency.GetFrequency(noteHz, mSyncFreqModVal));
			}

			void NoteOn(bool legato)
			{
				if (legato) return;
				if (mPhaseRestart.GetBoolValue()) {
					mPhase = Fract(mPhaseOffset.GetN11Value(mPhaseModVal));
				}
			}

			real_t ProcessSample(size_t bufferPos, real_t signal1, real_t signal1PMAmount, real_t signal2, real_t signal2PMAmount)
			{
				if (!mEnabled.GetBoolValue()) {
					mCurrentSample = 0;
					return 0;
				}

				mPrevSample = mCurrentSample;// THIS sample.
				mCurrentSample = 0; // a value that gets added to the next sample

				// why do we have 2 phase accumulators? because of hard sync support, and because they have different functions
				// though when hard sync is not enabled that difference is not very obvious.
				// mPhase / mPhaseIncrement are the master phase, bound by frequency.
				// mSlaveWave holds the slave phase, bound by sync frequency.

				// Push master phase forward by full sample.
				mPhase = Fract(mPhase + mPhaseIncrement);

				if (mPhase >= mPhaseIncrement || !mSyncEnable.GetBoolValue()) // did not cross cycle. advance 1 sample
				{
					auto bleps = mpSlaveWave->OSC_ADVANCE(1, 0);
					mPrevSample += bleps.first;
					mCurrentSample += bleps.second;
				}
				else {
					float x = mPhase / mPhaseIncrement; // sample overshoot, in samples.

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
				mCurrentSample += mpSlaveWave->NaiveSample(mpSlaveWave->mPhase);
				mOutSample = (mPrevSample + mpSlaveWave->mDCOffset) * mpSlaveWave->mScale;

				return mOutSample;
			}
		};
	} // namespace M7


} // namespace WaveSabreCore




