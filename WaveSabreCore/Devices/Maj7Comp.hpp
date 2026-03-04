// note that envelope smoothing is NOT serving the same function as RMS detection.
// envelope smoothing still allows instant attack, and serves to mitigate weird gain properties drifting because of imbalances of attack/release.
// RMS detection is for the incoming signal and slows response time of the compressor, making a sluggish attack and release.

// hm, midside is not well implemented IMO.
// basically it's just hard to find it useful to use, because mid/side channels tend to not be equal loudness like stereo is.. so a compensation panning control is implied?
// plus you'd likely be applying the effect to balance mid/side, so ANOTHER panning control is implied (effect dry-wet panning)
// this is like 4 params dedicated to an obscure feature (msenable, compensationpan, effectpan, chanlink). best to remove.
// actually, i think to support MS, probably have a 3-way toggle: STEREO, MID, SIDE. and treat mid/side as mono, so no panning controls or complications.

// or we could look at how C 2 does it, by not getting into that mess but still providing some MS options like mid-only, side-only, etc. but it's too obscure.
// kotelnikov doesn't offer MS at all.
// ozone vintage compressor makes MS processing very accessible, but makes you choose different settings for mid or side. so yea.

#pragma once

#include "../WSCore/Device.h"
#include "../GigaSynth/Maj7Basic.hpp"
#include "../Analysis/RMS.hpp"
#include "../Filters/BiquadFilter.h"

namespace WaveSabreCore
{
	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct Maj7CompFollower {

		float e = 1.0f; // ongoing follower output
		float a; // attack coef
		float r; // release coef
		//const float s = 0; // smoothing coef
		static constexpr float s = 0.997735023f; // CalcFollowerCoef(smoothingMS) where smoothingMS is 10.0
		float tmp = 0; // a continuous value

		// the smoothing time ensures that attack/release changes don't cause weird gain compensation values.
		static constexpr float smoothingMS = 10.0f;

		//Maj7CompFollower() : s(CalcFollowerCoef(smoothingMS)) //
		//{
		//}

		void SetParams(float attackMS, float releaseMS) {
			this->a = CalcFollowerCoef(attackMS);
			this->r = CalcFollowerCoef(releaseMS);
		}

		// in must be rectified!
		float ProcessSample(float in) {
			// similar to the Joep (saike?) one, this looks at a previous value, to see whether it's attack or release env,
			// then does lerp using the attack or release-per-sample coef
			this->tmp = std::max(in + this->s * (this->tmp - in), in); // smoothing amount * difference from last, smooths falloffs so the signal doesn't jump back to 0 constantly.
			// so this.tmp is the smoothed input signal. a signal that would correspond to instant attack and release based on smoothing value.
			float x = (this->e < this->tmp) ? this->a : this->r;
			this->e = M7::math::lerp(this->tmp, this->e, x);
			return this->e;
		}
	};


	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct MonoCompressor {

		static constexpr M7::PowCurvedParamCfg gAttackCfg{ 0.0f, 500.0f, 9 };
		static constexpr M7::PowCurvedParamCfg gReleaseCfg{ 0.0f, 1000.0f, 9 };

		static constexpr M7::DivCurvedParamCfg gRatioCfg{ 1, 50, 1.05f };

		// params
		//float mInputGainLin;
		//float mOutputGainLin;
		float mRatioCoef;
		float mKnee;
		float mThreshold;
		bool mEnableFilter;

		// outputs
		float mSidechain;
		float mDetector; // with channel mixing & RMS applied
		float mGainReduction; // multiplier that illustrates the gain reduction factor. (1 = no reduction)
		float mDiff;

		Maj7CompFollower mFollower;
		M7::BiquadFilter mHighpassFilter;
		M7::BiquadFilter mLowpassFilter;

		void SetParams(
			//float inputGainLin,
			//float outputGainLin,
			float ratio, // user-facing ratio (2.5 for example)
			float kneeDB,
			float thresholdDB,
			float attackMS,
			float releaseMS,
			bool enableFilter,
			float highPassFreq, // hz
			M7::Decibels highPassQ, // biquad q ~0.2 - ~12.0f
			float lowPassFreq, // hz
			M7::Decibels lowPassQ // biquad q ~0.2 - ~12.0f
		)
		{
			//mInputGainLin = inputGainLin;
			//mOutputGainLin = outputGainLin;
			mRatioCoef = (1.0f - (1.0f / ratio));
			mKnee = kneeDB;
			mThreshold = thresholdDB;
			mEnableFilter = enableFilter;

			mFollower.SetParams(attackMS, releaseMS);

			mHighpassFilter.SetBiquadParams(M7::FilterResponse::Highpass, highPassFreq, highPassQ, 0);
			mLowpassFilter.SetBiquadParams(M7::FilterResponse::Lowpass, lowPassFreq, lowPassQ, 0);
		}

		// this function defines the transfer curve.
		// given a decibel value (-inf to 0), 
		// returns the decibel value to reduce by, considering thresh, ratio, knee.
		// when no compression is applied, it returns a constant 0 (0dB reduction)
		// otherwise it returns a positive dB value.
		float TransferDecibels(float dB) const {
			float e = dB - std::min(dB, mThreshold - mKnee) + 0.0000001f; // avoid div0 with small value.
			e = (e * e) / (e + mKnee);
			e *= mRatioCoef;
			return e;
		}

		// responsible for calculating mInput, mDry, mSidechain, mPreDetector
		float ProcessSample(float inputAudio, float detectorInput) {
			//inputAudio *= mInputGainLin;
			//mDry = inputAudio;

			if (!mEnableFilter) {
				mSidechain = detectorInput;
			}
			else {
				// apply highpass and lowpass
				mSidechain = mHighpassFilter.ProcessSample(detectorInput);
				mSidechain = mLowpassFilter.ProcessSample(mSidechain);
			}
			mDetector = std::abs(mSidechain);
			//float attFactor = CompressorPeakSlow(mDetector);

			float detectorDB = M7::math::LinearToDecibels(mDetector);
			float e = TransferDecibels(detectorDB);
			e = M7::math::DecibelsToLinear(e);
			float attFactor = this->mFollower.ProcessSample(e);

			float wet = inputAudio / attFactor;
			mGainReduction = 1.0f / attFactor;
			mDiff = wet - inputAudio;
			return wet;// *mOutputGainLin;
		}

	}; // struct MonoCompressor






	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct Maj7Comp: public Device
	{
		enum class ParamIndices
		{
			InputGain, // db -inf to +24
			Threshold, // db -60 to 0, default -20
			Attack, // 0-500ms, default 60
			Release, // 0-1000ms, default 80
			Ratio, // 1-40, default 4
			Knee, // 0-30, default 8
			//MidSideEnable, // stereo or mid/side, default stereo
			ChannelLink, // 0-1 (0-100%, default 80%)
			//Pan,
			CompensationGain, // -inf to +24db, default 0
			DryWet,// mix 0-100%, default 100%
			//PeakRMSMix, // 0-100%, default 0 (peak)
			EnableSidechainFilter,
			HighPassFrequency, // biquad freq; default lo
			HighPassQ, // default 0.2
			LowPassFrequency, // biquad freq; default hi
			LowPassQ, // default 0.2
			OutputSignal, //
			OutputGain, // -inf to +24
			NumParams,
		};

		// NB: max 8 chars per string.
#define MAJ7COMP_PARAM_VST_NAMES(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::Maj7Comp::ParamIndices::NumParams]{ \
	{"InpGain"},\
	{"Thresh"},\
	{"Attack"},\
	{"Release"},\
	{"Ratio"},\
	{"Knee"},\
	{"ChanLink"},\
	{"CompGain"},\
	{"DryWet"},\
	{"SCFEn"},\
	{"HPF"},\
	{"HPQ"},\
	{"LPF"},\
	{"LPQ"},\
	{"OutSig"},\
	{"OutGain"},\
}

		static_assert((int)ParamIndices::NumParams == 16, "param count probably changed and this needs to be regenerated.");
		static constexpr int16_t gParamDefaults[(int)ParamIndices::NumParams] = {
		  8230, // InpGain = 0.25115966796875
		  21845, // Thresh = 0.666656494140625
		  15269, // Attack = 0.465972900390625
		  15909, // Release = 0.485504150390625
		  18939, // Ratio = 0.577972412109375
		  4369, // Knee = 0.133331298828125
		  26214, // ChanLink = 0.79998779296875
		  8230, // CompGain = 0.25115966796875
		  32767, // DryWet = 0.999969482421875
		  0, // SCFEn = 0
		  0, // HPF = 0
		  6553, // HPQ = 0.199981689453125
		  32767, // LPF = 1
		  6553, // LPQ = 0.199981689453125
		  16384, // OutSig
		  8230, // OutGain = 0.25115966796875
		};

		float mParamCache[(int)ParamIndices::NumParams];

		M7::ParamAccessor mParams;

		MonoCompressor mComp[2];

		enum class OutputSignal : byte {
			Normal = 0,
			Diff,
			Sidechain,
			Count__,
		};

		float mInputGainLin;
		float mOutputGainLin;
		float mCompensationGainLin;
		float mDryWetMix;

		OutputSignal mOutputSignal = OutputSignal::Normal;

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
		AnalysisStream mInputAnalysis[2];
		AnalysisStream mOutputAnalysis[2];
		AnalysisStream mDetectorAnalysis[2];
		AnalysisStream mAttenuationAnalysis[2];

		AnalysisStream mInputAnalysisSlow[2];
		AnalysisStream mOutputAnalysisSlow[2];

		static constexpr float gFastAnalysisPeakFalloffMS = 50;
		static constexpr float gSlowAnalysisPeakFalloffMS = 1000;
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

		Maj7Comp() :
			Device((int)ParamIndices::NumParams, mParamCache, gParamDefaults),
			mParams(mParamCache, 0)
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
			,
			// quite fast moving here because the user's looking for transients.
			mInputAnalysis{ AnalysisStream{gFastAnalysisPeakFalloffMS}, AnalysisStream{gFastAnalysisPeakFalloffMS} },
			mOutputAnalysis{ AnalysisStream{gFastAnalysisPeakFalloffMS}, AnalysisStream{gFastAnalysisPeakFalloffMS} },
			mInputAnalysisSlow{ AnalysisStream{gSlowAnalysisPeakFalloffMS}, AnalysisStream{gSlowAnalysisPeakFalloffMS} },
			mOutputAnalysisSlow{ AnalysisStream{gSlowAnalysisPeakFalloffMS}, AnalysisStream{gSlowAnalysisPeakFalloffMS} },
			mDetectorAnalysis{ AnalysisStream{gFastAnalysisPeakFalloffMS}, AnalysisStream{gFastAnalysisPeakFalloffMS} },
			mAttenuationAnalysis{ AnalysisStream{gFastAnalysisPeakFalloffMS}, AnalysisStream{gFastAnalysisPeakFalloffMS} }
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT
		{
			LoadDefaults();
		}

		virtual void SetParam(int index, float value) override
		{
			mParamCache[index] = value;
			mOutputSignal = mParams.GetEnumValue<OutputSignal>(ParamIndices::OutputSignal);
			mInputGainLin = mParams.GetLinearVolume(ParamIndices::InputGain, M7::gVolumeCfg24db);
			mOutputGainLin = mParams.GetLinearVolume(ParamIndices::OutputGain, M7::gVolumeCfg24db);
			mCompensationGainLin = mParams.GetLinearVolume(ParamIndices::CompensationGain, M7::gVolumeCfg24db);
			mDryWetMix = mParams.Get01Value(ParamIndices::DryWet);

			for (auto& c : mComp) {
				//c.OnParamChange();
				c.SetParams(
					mParams.GetDivCurvedValue(ParamIndices::Ratio, MonoCompressor::gRatioCfg, 0),
					mParams.GetScaledRealValue(ParamIndices::Knee, 0, 30, 0),
					mParams.GetScaledRealValue(ParamIndices::Threshold, -60, 0, 0),
					mParams.GetPowCurvedValue(ParamIndices::Attack, MonoCompressor::gAttackCfg, 0),
					mParams.GetPowCurvedValue(ParamIndices::Release, MonoCompressor::gReleaseCfg, 0),
					mParams.GetBoolValue(ParamIndices::EnableSidechainFilter),
					mParams.GetFrequency(ParamIndices::HighPassFrequency, M7::gFilterFreqConfig),
                    M7::Decibels{mParams.GetDivCurvedValue(ParamIndices::HighPassQ, M7::gBiquadFilterQCfg)},
					mParams.GetFrequency(ParamIndices::LowPassFrequency, M7::gFilterFreqConfig),
                    M7::Decibels{mParams.GetDivCurvedValue(ParamIndices::LowPassQ, M7::gBiquadFilterQCfg)}
				);
			}
		}

		virtual void Run(float** inputs, float** outputs, int numSamples) override
		{
			float channelLink01 = mParams.Get01Value(ParamIndices::ChannelLink);
			//bool midside = mParams.GetBoolValue(ParamIndices::MidSideEnable);
			for (size_t iSample = 0; iSample < (size_t)numSamples; ++iSample)
			{
				// apply stereo linking
				float s0 = inputs[0][iSample] * mInputGainLin;
				float s1 = inputs[1][iSample] * mInputGainLin;
				float monoDetector = (s0 + s1) * 0.5f;

				for (size_t ich = 0; ich < 2; ++ich) {
					float inpAudio = inputs[ich][iSample] * mInputGainLin;
					float detector = M7::math::lerp(inpAudio, monoDetector, channelLink01);
					float wetSignal = mComp[ich].ProcessSample(inpAudio, detector);
					
					// Apply compensation gain to wet signal
					wetSignal *= mCompensationGainLin;
					
					// Apply dry/wet mix
					float finalSignal = M7::math::lerp(inpAudio, wetSignal, mDryWetMix);
					
					// Apply output gain
					outputs[ich][iSample] = finalSignal * mOutputGainLin;

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
					if (IsGuiVisible())
					{
						mInputAnalysis[ich].WriteSample(inpAudio);
						mOutputAnalysis[ich].WriteSample(outputs[ich][iSample]);

						mInputAnalysisSlow[ich].WriteSample(inpAudio);
						mOutputAnalysisSlow[ich].WriteSample(outputs[ich][iSample]);

						mDetectorAnalysis[ich].WriteSample(mComp[ich].mDetector);
						mAttenuationAnalysis[ich].WriteSample(mComp[ich].mGainReduction);
					}
					switch (mOutputSignal) {
					case OutputSignal::Normal:
						break;
					case OutputSignal::Diff:
						outputs[ich][iSample] = mComp[ich].mDiff;
						break;
					case OutputSignal::Sidechain:
						outputs[ich][iSample] = mComp[ich].mSidechain;
						break;
					}
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

				}
			}

		}
	};
}

