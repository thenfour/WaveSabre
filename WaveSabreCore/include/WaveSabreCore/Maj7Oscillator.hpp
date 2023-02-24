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

			COUNT,
		};

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

			// LFO
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
				mEnabled(paramCache[paramBaseID + gAudioParamEnabled], false),
				mVolume(paramCache[paramBaseID + gAudioParamVolume], gVolumeMaxDb, 1),
				mWaveform(paramCache[paramBaseID + gAudioParamWaveform], OscillatorWaveform::COUNT, OscillatorWaveform::Sine),
				mWaveshape(paramCache[paramBaseID + gAudioParamWaveshape], 0),
				mPhaseRestart(paramCache[paramBaseID + gAudioParamPhaseRestart], false),
				mPhaseOffset(paramCache[paramBaseID + gAudioParamPhaseOffset], 0),
				mSyncEnable(paramCache[paramBaseID + gAudioParamSyncEnable], false),
				mSyncFrequency(paramCache[paramBaseID + gAudioParamSyncFrequency], paramCache[paramBaseID + gAudioParamSyncFrequencyKT], gSyncFrequencyCenterHz, 0.4f, 1.0f),
				mFrequency(paramCache[paramBaseID + gAudioParamFrequencyParam], paramCache[paramBaseID + gAudioParamFrequencyParamKT], gFrequencyCenterHz, 0.4f, 1.0f),
				mPitchSemis(paramCache[paramBaseID + gAudioParamPitchSemis], -gPitchSemisRange, gPitchSemisRange, 0),
				mPitchFine(paramCache[paramBaseID + gAudioParamPitchFine], 0),
				mFrequencyMul(paramCache[paramBaseID + gAudioParamFreqMul], 0.0f, gFrequencyMulMax, 1.0f),
				mFMFeedback01(paramCache[paramBaseID + gAudioParamFMFeedback], 0),

				mIntention(OscillatorIntention::Audio),
				mModMatrix(modMatrix),
				mModDestBase((int)modDestBase)
			{}

			// for LFO, we internalize many params
			explicit OscillatorNode(OscillatorIntentionLFO, ModMatrixNode& modMatrix, ModDestination modDestBase, real_t* paramCache, int paramBaseID) :
				mEnabled(mLFOConst1, false),
				mVolume(mLFOVolumeParamValue, 0, 1),
				mWaveform(paramCache[paramBaseID + gLFOParamWaveform], OscillatorWaveform::COUNT, OscillatorWaveform::Sine),
				mWaveshape(paramCache[paramBaseID + gLFOParamWaveshape], 0),
				mPhaseRestart(paramCache[paramBaseID + gLFOParamPhaseRestart], false),
				mPhaseOffset(paramCache[paramBaseID + gLFOParamPhaseOffset], 0),
				mSyncEnable(mLFOConst0, false),
				mSyncFrequency(mLFOSyncFrequencyParamValue, mLFOConst1, 1000.0f, 0.4f, 1.0f),
				mFrequency(paramCache[paramBaseID + gLFOParamFrequencyParam], mLFOConst0, 4.0f, 0.1f, 1.0f),
				mPitchSemis(mLFOPitchSemisParamValue, -24, 24, 0),
				mPitchFine(mLFOPitchFineParamValue, 0),
				mFrequencyMul(mLFOFrequencyMulParamValue, 0.0f, 64.0f, 1.0f),
				mFMFeedback01(mLFOFMFeedbackParamValue, 0),

				mIntention(OscillatorIntention::LFO),
				mModMatrix(modMatrix),
				mModDestBase((int)modDestBase)
			{}

			static constexpr int gAudioModDestVolume = 0;
			static constexpr int gAudioModDestWaveshape = 1;
			static constexpr int gAudioModDestSyncFrequency = 2;
			static constexpr int gAudioModDestFrequencyParam = 3;
			static constexpr int gAudioModDestFMFeedback = 4;

			static constexpr int gLFOModDestWaveshape = 0;
			static constexpr int gLFOModDestFrequencyParam = 1;

			static constexpr int gAudioParamEnabled = 0;
			static constexpr int gAudioParamVolume = 1;
			static constexpr int gAudioParamWaveform = 2;
			static constexpr int gAudioParamWaveshape = 3;
			static constexpr int gAudioParamPhaseRestart = 4;
			static constexpr int gAudioParamPhaseOffset = 5;
			static constexpr int gAudioParamSyncEnable = 6;
			static constexpr int gAudioParamSyncFrequency = 7;
			static constexpr int gAudioParamSyncFrequencyKT = 8;
			static constexpr int gAudioParamFrequencyParam = 9;
			static constexpr int gAudioParamFrequencyParamKT = 10;
			static constexpr int gAudioParamPitchSemis = 11;
			static constexpr int gAudioParamPitchFine = 12;
			static constexpr int gAudioParamFreqMul = 13;
			static constexpr int gAudioParamFMFeedback = 14;

			static constexpr int gLFOParamWaveform = 0;
			static constexpr int gLFOParamWaveshape = 1;
			static constexpr int gLFOParamPhaseRestart = 2;
			static constexpr int gLFOParamPhaseOffset = 3;
			static constexpr int gLFOParamFrequencyParam = 4;

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
				float volumeModVal = 0;
				float waveShapeModVal = 0;
				float syncFreqModVal = 0;
				float freqModVal = 0;
				float fmFeedbackModVal = 0;
				switch (mIntention) {
				case OscillatorIntention::LFO:
					waveShapeModVal = mModMatrix.GetKRateDestinationValue(mModDestBase + gLFOModDestWaveshape);
					freqModVal = mModMatrix.GetKRateDestinationValue(mModDestBase + gLFOModDestWaveshape);
					break;
				case OscillatorIntention::Audio:
					volumeModVal = mModMatrix.GetARateDestinationBuffer(mModDestBase + gAudioModDestVolume)[bufferPos];
					waveShapeModVal = mModMatrix.GetARateDestinationBuffer(mModDestBase + gAudioModDestWaveshape)[bufferPos];
					syncFreqModVal = mModMatrix.GetARateDestinationBuffer(mModDestBase + gAudioModDestSyncFrequency)[bufferPos];
					freqModVal = mModMatrix.GetARateDestinationBuffer(mModDestBase + gAudioModDestFrequencyParam)[bufferPos];
					fmFeedbackModVal = mModMatrix.GetARateDestinationBuffer(mModDestBase + gAudioModDestFMFeedback)[bufferPos];
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




