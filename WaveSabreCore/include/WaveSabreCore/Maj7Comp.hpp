// note that envelope smoothing is NOT serving the same function as RMS detection.
// envelope smoothing still allows instant attack, and serves to mitigate weird gain properties drifting because of imbalances of attack/release.
// RMS detection is for the incoming signal and slows response time of the compressor, making a sluggish attack and release.

// hm, midside is not well implemented IMO.
// basically it's just hard to find it useful to use, because mid/side channels tend to not be equal loudness like stereo is.. so a compensation panning control is implied?
// plus you'd likely be applying the effect to balance mid/side, so ANOTHER panning control is implied (effect dry-wet panning)
// this is like 4 params dedicated to an obscure feature (msenable, compensationpan, effectpan, chanlink). best to remove.

// or we could look at how C 2 does it, by not getting into that mess but still providing some MS options like mid-only, side-only, etc. but it's too obscure.
// kotelnikov doesn't offer MS at all.
// ozone vintage compressor makes MS processing very accessible, but makes you choose different settings for mid or side. so yea.

#pragma once

#include "Device.h"
#include "Maj7Basic.hpp"
#include "RMS.hpp"
#include "BiquadFilter.h"

// use to enable/disable
// - rms detection
// - parallel processing (dry-wet)
// - lowpass filter (it's rarely needed)
// - compensation gain (just use output gain if no parallel processing there's no point)
#define MAJ7COMP_FULL

namespace WaveSabreCore
{
	struct Maj7CompFollower {

		float e = 1.0f; // ongoing follower output
		float a = 0; // attack coef
		float r = 0; // release coef
		const float s = 0; // smoothing coef
		float tmp = 0; // a continuous value

		// the smoothing time ensures that attack/release changes don't cause weird gain compensation values.
		static constexpr float smoothingMS = 10.0f;

		Maj7CompFollower() : s(CalcFollowerCoef(smoothingMS)) //
		{
		}

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
			RMSWindow, // 0-50ms, default 30ms
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
	{"RMSMS"},\
	{"HPF"},\
	{"HPQ"},\
	{"LPF"},\
	{"LPQ"},\
	{"OutSig"},\
	{"OutGain"},\
}

		static_assert((int)ParamIndices::NumParams == 16, "param count probably changed and this needs to be regenerated.");
		static constexpr int16_t gParamDefaults[16] = {
		  8230, // InpGain = 0.25118863582611083984
		  21845, // Thresh = 0.6666666865348815918
		  15269, // Attack = 0.46597486734390258789
		  15909, // Release = 0.48552104830741882324
		  18939, // Ratio = 0.57798200845718383789
		  4369, // Knee = 0.13333334028720855713
		  26214, // ChanLink = 0.80000001192092895508
		  8230, // CompGain = 0.25118863582611083984
		  32767, // DryWet = 1
		  0, // RMSMS = 0
		  0, // HPF = 0
		  6553, // HPQ = 0.20000000298023223877
		  32767, // LPF = 1
		  6553, // LPQ = 0.20000000298023223877
		  8, // OutSig = 0.0002470355830155313015
		  8230, // OutGain = 0.25118863582611083984
		};

		float mParamCache[(int)ParamIndices::NumParams];

		static constexpr float gMinRMSWindowMS = 0.2f;

		static constexpr M7::PowCurvedParamCfg gAttackCfg{ 0.0f, 500.0f, 9 };
		static constexpr M7::PowCurvedParamCfg gReleaseCfg{ 0.0f, 1000.0f, 9 };
		static constexpr M7::PowCurvedParamCfg gRMSWindowSizeCfg{ 0.0f, 100.0f, 9 };

		static constexpr M7::DivCurvedParamCfg gRatioCfg{ 1, 50, 1.05f };

		M7::ParamAccessor mParams;

		struct MonoCompressor {
			M7::ParamAccessor mParams;
			float mRatioCoef = 0;
			float mThreshCoef = 0;
			float mThreshold = 0;
			float mKnee = 0;
			float mInputGainLin = 0;
			float mRMSWindowMS = 0;
#ifdef MAJ7COMP_FULL
			float mCompensationGainLin = 0;
			//float mDryWetMix = 0;
			//float mPeakRMSMix = 0;
#endif // MAJ7COMP_FULL
			float mOutputGainLin = 0;

			// pre-channel-mixing
			float mInput;
			float mDry;
			float mSidechain;
			float mPreDetector; // peaking-only, and pre-mixing. kind of a temp variable.

			// post-channel-mixing
			float mPostDetector; // with channel mixing & RMS applied
			float mGainReduction; // multiplier that illustrates the gain reduction factor. (1 = no reduction)
			float mDiff;
			float mOutput;

			Maj7CompFollower mFollower;
#ifdef MAJ7COMP_FULL
			RMSDetector mRMSDetector;
			BiquadFilter mLowpassFilter;
#endif // MAJ7COMP_FULL
			BiquadFilter mHighpassFilter;

			MonoCompressor(float* paramCache) : mParams(paramCache, 0)
			{}

			void OnParamChange()
			{
				mInputGainLin = mParams.GetLinearVolume(ParamIndices::InputGain, M7::gVolumeCfg24db);
				mOutputGainLin = mParams.GetLinearVolume(ParamIndices::OutputGain, M7::gVolumeCfg24db);
#ifdef MAJ7COMP_FULL
				mCompensationGainLin = mParams.GetLinearVolume(ParamIndices::CompensationGain, M7::gVolumeCfg24db);
				//mDryWetMix = ;
				//mPeakRMSMix = mParams.Get01Value(ParamIndices::PeakRMSMix, 0);
#endif // MAJ7COMP_FULL

				float ratioParam = mParams.GetDivCurvedValue(ParamIndices::Ratio, gRatioCfg, 0);
				mRatioCoef = (1.0f - (1.0f / ratioParam));
				mKnee = mParams.GetScaledRealValue(ParamIndices::Knee, 0, 30, 0);
				mThreshold = mParams.GetScaledRealValue(ParamIndices::Threshold, -60, 0, 0);
				mThreshCoef = mThreshold - mKnee;

				mFollower.SetParams(
					mParams.GetPowCurvedValue(ParamIndices::Attack, gAttackCfg, 0),
					mParams.GetPowCurvedValue(ParamIndices::Release, gReleaseCfg, 0)
				);

#ifdef MAJ7COMP_FULL
				mRMSWindowMS = mParams.GetPowCurvedValue(ParamIndices::RMSWindow, gRMSWindowSizeCfg, 0);
				if (mRMSWindowMS >= gMinRMSWindowMS) {
					mRMSDetector.SetWindowSize(mRMSWindowMS);
				}

				float lpf = mParams.GetFrequency(ParamIndices::LowPassFrequency, M7::gFilterFreqConfig);
				float lpq = mParams.GetWSQValue(ParamIndices::LowPassQ);
				mLowpassFilter.SetParams(::WaveSabreCore::BiquadFilterType::Lowpass, lpf, lpq, 0);
#endif // MAJ7COMP_FULL

				float hpq = mParams.GetWSQValue(ParamIndices::HighPassQ);
				float hpf = mParams.GetFrequency(ParamIndices::HighPassFrequency, M7::gFilterFreqConfig);
				mHighpassFilter.SetParams(::WaveSabreCore::BiquadFilterType::Highpass, hpf, hpq, 0);
			}

			// this function defines the transfer curve.
			// given a decibel value (-inf to 0), 
			// returns the decibel value to reduce by, considering thresh, ratio, knee.
			// when no compression is applied, it returns a constant 0 (0dB reduction)
			// otherwise it returns a positive dB value.
			float TransferDecibels(float dB) const {
				float e = dB - std::min(dB, mThreshCoef) + 0.0000001f; // avoid div0 with small value.
				e = (e * e) / (e + mKnee);
				e *= mRatioCoef;
				return e;
			}

			// inputs: `in` = rectified input SAMPLE VALUE (not db)
			// outputs a value which the original can be divided by
			float CompressorPeakSlow(float in) {
				float dB = M7::math::LinearToDecibels(in);
				float e = TransferDecibels(dB);
				e = M7::math::DecibelsToLinear(e);
				return this->mFollower.ProcessSample(e);
			}

			// responsible for calculating mInput, mDry, mSidechain, mPreDetector
			void ProcessSampleBeforeChannelMixing(float input) {
				mSidechain = mDry = mInput = input * mInputGainLin;

				// filter the sidechain before we destroy its listenability via rectification
				mSidechain = mHighpassFilter.Next(mSidechain);
#ifdef MAJ7COMP_FULL
				mSidechain = mLowpassFilter.Next(mSidechain);
#endif // MAJ7COMP_FULL

				mPreDetector = std::abs(mSidechain);
			}

			void ProcessSampleAfterChannelMixing(float mixedDetector) {
#ifdef MAJ7COMP_FULL
				if (mRMSWindowMS >= gMinRMSWindowMS) {
					float trmsLevelL = mRMSDetector.ProcessSample(mixedDetector);
					mPostDetector = trmsLevelL;
				}
				else {
					mPostDetector = mixedDetector;
				}
				//mPostDetector = M7::math::lerp(mixedDetector, trmsLevelL, mPeakRMSMix);
#else
				mPostDetector = mixedDetector;
#endif // MAJ7COMP_FULL

				float attFactor = CompressorPeakSlow(mPostDetector);
				float wet = mDry / attFactor;
				mGainReduction = 1.0f / attFactor;
				mDiff = wet - mDry;
#ifdef MAJ7COMP_FULL
				wet *= mCompensationGainLin;
				mOutput = M7::math::lerp(mDry, wet, mParams.Get01Value(ParamIndices::DryWet, 0));
#endif // MAJ7COMP_FULL
				mOutput *= mOutputGainLin;
			}
		};

		MonoCompressor mComp[2] = { {mParamCache},{mParamCache} }; // 2 channels

		enum class OutputSignal : byte {
			Normal = 0,
			Diff,
			Sidechain,
			Count__,
		};

		float mChannelLink01 = 0;

#ifdef MAJ7COMP_FULL
		//bool mMidSideEnable = false;
#endif // MAJ7COMP_FULL
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
			Device((int)ParamIndices::NumParams),
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

		virtual void LoadDefaults() override
		{
			M7::ImportDefaultsArray(std::size(gParamDefaults), gParamDefaults, mParamCache);
			SetParam(0, mParamCache[0]); // force recalcing
		}

		virtual void SetParam(int index, float value) override
		{
			mParamCache[index] = value;
			mChannelLink01 = mParams.Get01Value(ParamIndices::ChannelLink);
			mOutputSignal = mParams.GetEnumValue<OutputSignal>(ParamIndices::OutputSignal);

			for (auto& c : mComp) {
				c.OnParamChange();
			}
		}

		virtual float GetParam(int index) const override
		{
			return mParamCache[index];
		}

		void CompressorComb(float in0, float in1) {
#ifdef MAJ7COMP_FULL
			mComp[0].ProcessSampleBeforeChannelMixing(in0);
			mComp[1].ProcessSampleBeforeChannelMixing(in1);
			float sum = std::max(mComp[0].mPreDetector, mComp[1].mPreDetector); // NB: requires rectification before this.
			//float panN11 = mParams.GetN11Value(ParamIndices::Pan, 0);
			float mixedDetector0 = M7::math::lerp(mComp[0].mPreDetector, sum, mChannelLink01);
			//float dryWet = mParams.Get01Value(ParamIndices::DryWet, 0);
			//float dryWet0 = 1.0f;
			//float dryWet1 = 1.0f;
			//if (panN11 < 0) dryWet1 = panN11 + 1;
			//if (panN11 > 0) dryWet0 = 1.0f - panN11;
			mComp[0].ProcessSampleAfterChannelMixing(mixedDetector0);
			float mixedDetector1 = M7::math::lerp(mComp[1].mPreDetector, sum, mChannelLink01);
			mComp[1].ProcessSampleAfterChannelMixing(mixedDetector1);
#else
			mComp[0].ProcessSampleBeforeChannelMixing(in0);
			mComp[0].ProcessSampleAfterChannelMixing(mComp[0].mPreDetector);
			mComp[1].ProcessSampleBeforeChannelMixing(in1);
			mComp[1].ProcessSampleAfterChannelMixing(mComp[1].mPreDetector);
#endif // MAJ7COMP_FULL
		}

		virtual void Run(double songPosition, float** inputs, float** outputs, int numSamples) override
		{
			//bool midside = mParams.GetBoolValue(ParamIndices::MidSideEnable);
			for (size_t iSample = 0; iSample < (size_t)numSamples; ++iSample)
			{
				float in0 = inputs[0][iSample];
				float in1 = inputs[1][iSample];

				float out0, out1;

#ifdef MAJ7COMP_FULL
				//if (mParams.GetBoolValue(ParamIndices::MidSideEnable)) {
				//	float m, s;
				//	M7::MSEncode(in0, in1, &m, &s);
				//	CompressorComb(m, s); // the output of this is mComp[...].mOutput
				//	M7::MSDecode(mComp[0].mOutput, mComp[1].mOutput, &out0, &out1);
				//	M7::MSDecode(mComp[0].mDiff, mComp[1].mDiff, &mComp[0].mDiff, &mComp[1].mDiff);
				//	M7::MSDecode(mComp[0].mSidechain, mComp[1].mSidechain, &mComp[0].mSidechain, &mComp[1].mSidechain);
				//}
				//else {
#endif // MAJ7COMP_FULL
				CompressorComb(in0, in1); // the output of this is mComp[...].mOutput
				out0 = mComp[0].mOutput;
				out1 = mComp[1].mOutput;
#ifdef MAJ7COMP_FULL
				//}
#endif // MAJ7COMP_FULL

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
				mInputAnalysis[0].WriteSample(mComp[0].mInput);
				mInputAnalysis[1].WriteSample(mComp[1].mInput);

				mOutputAnalysis[0].WriteSample(mComp[0].mOutput);
				mOutputAnalysis[1].WriteSample(mComp[1].mOutput);

				mInputAnalysisSlow[0].WriteSample(mComp[0].mInput);
				mInputAnalysisSlow[1].WriteSample(mComp[1].mInput);

				mOutputAnalysisSlow[0].WriteSample(mComp[0].mOutput);
				mOutputAnalysisSlow[1].WriteSample(mComp[1].mOutput);

				mDetectorAnalysis[0].WriteSample(mComp[0].mPostDetector);
				mDetectorAnalysis[1].WriteSample(mComp[1].mPostDetector);

				mAttenuationAnalysis[0].WriteSample(mComp[0].mGainReduction);
				mAttenuationAnalysis[1].WriteSample(mComp[1].mGainReduction);

				switch (mOutputSignal) {
				case OutputSignal::Normal:
					break;
				case OutputSignal::Diff:
					out0 = mComp[0].mDiff;
					out1 = mComp[1].mDiff;
					break;
				case OutputSignal::Sidechain:
					out0 = mComp[0].mSidechain;
					out1 = mComp[1].mSidechain;
					break;
				}
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

				outputs[0][iSample] = out0;
				outputs[1][iSample] = out1;
			}

		}
	};
}

