#pragma once

#include <WaveSabreCore/Maj7Basic.hpp>
#include <WaveSabreCore/Maj7ModMatrix.hpp>

namespace WaveSabreCore
{
	namespace M7
	{
		enum class OscillatorWaveform : uint8_t
		{
			Sine,

			// white noise

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
			static constexpr real_t gVolumeMaxDb = 0;
			static constexpr real_t gSyncFrequencyCenterHz = 1000;
			static constexpr real_t gFrequencyCenterHz = 1000;
			static constexpr int gPitchSemisRange = 24;
			static constexpr real_t gFrequencyMulMax = 64;

			// BASE PARAMS
			BoolParam mEnabled;// Osc2Enabled,
			VolumeParam mVolume; //	Osc2Volume,
			EnumParam<OscillatorWaveform> mWaveform;
			FloatN11Param mWaveshape;//	Osc2Waveshape,
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
			float mLFOConst0 = 1.0f;
			float mLFOVolumeParamValue = 1.0f;
			float mLFOSyncFrequencyParamValue = 1.0f;
			float mLFOPitchSemisParamValue = 0.5f;
			float mLFOPitchFineParamValue = 0.5f;
			float mLFOFrequencyMulParamValue = 0.5f;
			float mLFOFMFeedbackParamValue = 0.0f;

			// for Audio
			explicit OscillatorNode(OscillatorIntentionAudio, ModMatrixNode& modMatrix, ModDestination modDestBase, real_t* paramCache, int paramBaseID) :
				mEnabled(paramCache[paramBaseID + (int)OscParamIndexOffsets::Enabled], false),
				mVolume(paramCache[paramBaseID + (int)OscParamIndexOffsets::Volume], gVolumeMaxDb, 1),
				mWaveform(paramCache[paramBaseID + (int)OscParamIndexOffsets::Waveform], OscillatorWaveform::Count, OscillatorWaveform::Sine),
				mWaveshape(paramCache[paramBaseID + (int)OscParamIndexOffsets::Waveshape], 0),
				mPhaseRestart(paramCache[paramBaseID + (int)OscParamIndexOffsets::PhaseRestart], false),
				mPhaseOffset(paramCache[paramBaseID + (int)OscParamIndexOffsets::PhaseOffset], 0),
				mSyncEnable(paramCache[paramBaseID + (int)OscParamIndexOffsets::SyncEnable], false),
				mSyncFrequency(paramCache[paramBaseID + (int)OscParamIndexOffsets::SyncFrequency], paramCache[paramBaseID + (int)OscParamIndexOffsets::SyncFrequencyKT], gSyncFrequencyCenterHz, 0.4f, 1.0f),
				mFrequency(paramCache[paramBaseID + (int)OscParamIndexOffsets::FrequencyParam], paramCache[paramBaseID + (int)OscParamIndexOffsets::FrequencyParamKT], gFrequencyCenterHz, 0.4f, 1.0f),
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
				mVolume(mLFOVolumeParamValue, 0, 1),
				mWaveform(paramCache[paramBaseID + (int)LFOParamIndexOffsets::Waveform], OscillatorWaveform::Count, OscillatorWaveform::Sine),
				mWaveshape(paramCache[paramBaseID + (int)LFOParamIndexOffsets::Waveshape], 0),
				mPhaseRestart(paramCache[paramBaseID + (int)LFOParamIndexOffsets::Restart], false),
				mPhaseOffset(paramCache[paramBaseID + (int)LFOParamIndexOffsets::PhaseOffset], 0),
				mSyncEnable(mLFOConst0, false),
				mSyncFrequency(mLFOSyncFrequencyParamValue, mLFOConst1, 1000.0f, 0.4f, 1.0f),
				mFrequency(paramCache[paramBaseID + (int)LFOParamIndexOffsets::FrequencyParam], mLFOConst0, 4.0f, 0.1f, 1.0f),
				mPitchSemis(mLFOPitchSemisParamValue, -24, 24, 0),
				mPitchFine(mLFOPitchFineParamValue, 0),
				mFrequencyMul(mLFOFrequencyMulParamValue, 0.0f, 64.0f, 1.0f),
				mFMFeedback01(mLFOFMFeedbackParamValue, 0),

				mIntention(OscillatorIntention::LFO),
				mModMatrix(modMatrix),
				mModDestBase((int)modDestBase)
			{}

			// state
			real_t mPhase = 0;
			real_t mLastSample = 0;

			OscillatorIntention mIntention;
			ModMatrixNode& mModMatrix;
			int mModDestBase; // ModDestination enum value representing the 1st mod value. for LFOs that's waveshape, for audio that's volume.

			void BeginBlock(real_t* params)
			{
			}

			real_t GetLastSample() const {
				return mLastSample;
			}

			real_t ProcessSample(real_t noteHz, size_t bufferPos, real_t signal1, real_t signal1PMAmount, real_t signal2, real_t signal2PMAmount)
			{
				if (!mEnabled.GetBoolValue()) {
					mLastSample = 0;
					return 0;
				}
				//return Helpers::RandFloat();
				float volumeModVal = 0;
				float waveShapeModVal = 0;
				float syncFreqModVal = 0;
				float freqModVal = 0;
				float fmFeedbackModVal = 0;
				switch (mIntention) {
				case OscillatorIntention::LFO:
					waveShapeModVal = mModMatrix.GetDestinationValue(mModDestBase + (int)LFOModParamIndexOffsets::Waveshape, bufferPos);
					freqModVal = mModMatrix.GetDestinationValue(mModDestBase + (int)LFOModParamIndexOffsets::FrequencyParam, bufferPos);
					break;
				case OscillatorIntention::Audio:
					volumeModVal = mModMatrix.GetDestinationValue(mModDestBase + (int)OscModParamIndexOffsets::Volume, bufferPos);
					waveShapeModVal = mModMatrix.GetDestinationValue(mModDestBase + (int)OscModParamIndexOffsets::Waveshape, bufferPos);
					syncFreqModVal = mModMatrix.GetDestinationValue(mModDestBase + (int)OscModParamIndexOffsets::SyncFrequency, bufferPos);
					freqModVal = mModMatrix.GetDestinationValue(mModDestBase + (int)OscModParamIndexOffsets::FrequencyParam, bufferPos);
					fmFeedbackModVal = mModMatrix.GetDestinationValue(mModDestBase + (int)OscModParamIndexOffsets::FMFeedback, bufferPos);
					break;
				}

				float freq = mFrequency.GetFrequency(noteHz, freqModVal);
				float dt = freq / (real_t)Helpers::CurrentSampleRate;
				mPhase += dt;
				mPhase -= math::floor(mPhase);

				mLastSample = math::sin((mPhase + (mLastSample * mFMFeedback01.Get01Value())) * 2 * math::gPI);

				return mLastSample * mVolume.GetLinearGain(volumeModVal);
			}
		};
	} // namespace M7


} // namespace WaveSabreCore




