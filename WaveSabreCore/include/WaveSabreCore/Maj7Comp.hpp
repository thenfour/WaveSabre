// note that envelope smoothing is NOT serving the same function as RMS detection.
// envelope smoothing still allows instant attack, and serves to mitigate weird gain properties drifting because of imbalances of attack/release.
// RMS detection is for the incoming signal and slows response time of the compressor, making a sluggish attack and release.

#pragma once

#include "Device.h"
#include "Maj7Basic.hpp"
#include "RMS.hpp"
#include "BiquadFilter.h"

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

		Maj7CompFollower() : s(calcCoef(smoothingMS)) //
		{
		}

		float calcCoef(float ms) {
			return M7::math::expf(-1.0f / (Helpers::CurrentSampleRateF * ms / 1000.0f));
		}

		void SetParams(float attackMS, float releaseMS) {
			this->a = calcCoef(attackMS);
			this->r = calcCoef(releaseMS);
		}

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
			Knee, // 1-30, default 8
			MidSideEnable, // stereo or mid/side, default stereo
			ChannelLink, // 0-1 (0-100%, default 80%)
			CompensationGain, // -inf to +24db, default 0
			DryWet,// mix 0-100%, default 100%
			PeakRMSMix, // 0-100%, default 0 (peak)
			RMSWindow, // 3-50ms, default 30ms
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
	{"MSEnable"},\
	{"ChanLink"},\
	{"CompGain"},\
	{"DryWet"},\
	{"PeakRMS"},\
	{"RMSMS"},\
	{"HPF"},\
	{"HPQ"},\
	{"LPF"},\
	{"LPQ"},\
	{"OutSig"},\
	{"OutGain"},\
}

		static_assert((int)ParamIndices::NumParams == 18, "param count probably changed and this needs to be regenerated.");
		static constexpr int16_t gParamDefaults[18] = {
		  8230, // InpGain = 0.25118863582611083984
		  21845, // Thresh = 0.666656494140625
		  9472, // Attack = 0.2890625
		  10284, // Release = 0.3138427734375
		  2520, // Ratio = 0.076904296875
		  7909, // Knee = 0.241363525390625
		  0, // MSEnable = 0
		  26214, // ChanLink = 0.80000001192092895508
		  8230, // CompGain = 0.25118863582611083984
		  32767, // DryWet = 1
		  0, // PeakRMS = 0
		  21586, // RMSMS = 0.65875244140625
		  0, // HPF = 0
		  6553, // HPQ = 0.20000000298023223877
		  32767, // LPF = 0.999969482421875
		  6553, // LPQ = 0.20000000298023223877
		  8, // OutSig = 0.000244140625
		  8230, // OutGain = 0.25118863582611083984
		};

		float mParamCache[(int)ParamIndices::NumParams];

		static constexpr M7::TimeParamCfg gAttackCfg{ 0.0f, 500.0f };
		static constexpr M7::TimeParamCfg gReleaseCfg{ 0.0f, 1000.0f };
		static constexpr M7::TimeParamCfg gRMSWindowSizeCfg{ 2.0f, 50.0f };

		M7::ParamAccessor mParams;

		struct MonoCompressor {
			M7::ParamAccessor mParams;
			float mRatioCoef = 0;
			float mThreshCoef = 0;
			float mKnee = 0;
			float mInputGainLin = 0;
			float mCompensationGainLin = 0;
			float mDryWetMix = 0;
			float mPeakRMSMix = 0;
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
			RMSDetector mRMSDetector;
			BiquadFilter mLowpassFilter;
			BiquadFilter mHighpassFilter;

			MonoCompressor(float* paramCache) : mParams(paramCache, 0)
			{}

			void OnParamChange()
			{
				mInputGainLin = mParams.GetLinearVolume(ParamIndices::InputGain, M7::gVolumeCfg24db);
				mOutputGainLin = mParams.GetLinearVolume(ParamIndices::OutputGain, M7::gVolumeCfg24db);
				mCompensationGainLin = mParams.GetLinearVolume(ParamIndices::CompensationGain, M7::gVolumeCfg24db);
				mDryWetMix = mParams.Get01Value(ParamIndices::DryWet, 0);
				mPeakRMSMix = mParams.Get01Value(ParamIndices::PeakRMSMix, 0);

				float ratioParam = mParams.GetScaledRealValue(ParamIndices::Ratio, 1, 60, 0);
				mRatioCoef = (1.0f - (1.0f / ratioParam));
				mKnee = mParams.GetScaledRealValue(ParamIndices::Knee, 0, 30, 0);
				mThreshCoef = mParams.GetScaledRealValue(ParamIndices::Threshold, -60, 0, 0) - mKnee;

				mFollower.SetParams(
					mParams.GetTimeMilliseconds(ParamIndices::Attack, gAttackCfg, 0),
					mParams.GetTimeMilliseconds(ParamIndices::Release, gReleaseCfg, 0)
				);

				mRMSDetector.SetWindowSize(mParams.GetTimeMilliseconds(ParamIndices::RMSWindow, gRMSWindowSizeCfg, 0));

				float hpq = mParams.GetWSQValue(ParamIndices::HighPassQ);
				float hpf = mParams.GetFrequency(ParamIndices::HighPassFrequency, -1, M7::gFilterFreqConfig, 0, 0);
				float lpq = mParams.GetWSQValue(ParamIndices::LowPassQ);
				float lpf = mParams.GetFrequency(ParamIndices::LowPassFrequency, -1, M7::gFilterFreqConfig, 0, 0);

				mLowpassFilter.SetParams(::WaveSabreCore::BiquadFilterType::Lowpass, lpf, lpq, 0);
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
				//float e = dB - std::min(dB, mThreshCoef) + 0.0000001f; // why the small value?
				//e = (e * e) / (e + mKnee);
				//e *= mRatioCoef;
				e = M7::math::DecibelsToLinear(e);
				return this->mFollower.ProcessSample(e);
			}

			// responsible for calculating mInput, mDry, mSidechain, mPreDetector
			void ProcessSampleBeforeChannelMixing(float input) {
				mInput = input;
				mSidechain = mDry = input * mInputGainLin;

				// filter the sidechain before we destroy its listenability via rectification
				mSidechain = mHighpassFilter.Next(mSidechain);
				mSidechain = mLowpassFilter.Next(mSidechain);

				mPreDetector = std::abs(mSidechain);
			}

			void ProcessSampleAfterChannelMixing(float mixedDetector) {
				float trmsLevelL = mRMSDetector.ProcessSample(mixedDetector);

				mPostDetector = M7::math::lerp(mixedDetector, trmsLevelL, mPeakRMSMix);
				float attFactor = CompressorPeakSlow(mPostDetector);
				float wet = mDry / attFactor;
				mGainReduction = 1.0f / attFactor;
				mDiff = wet - mDry;
				wet *= mCompensationGainLin;
				mOutput = M7::math::lerp(mDry, wet, mDryWetMix);
				mOutput *= mOutputGainLin;
			}
		};

		MonoCompressor mComp[2] = { {mParamCache},{mParamCache} }; // 2 channels

		enum class OutputSignal : byte {
		//slider30:sOutputSignal = 0 < 0, 5, 1{Normal, Diff, Sidechain, GainReduction, RMSDetector, Detector} > Output
			Normal = 0,
			Diff,
			Sidechain,
			GainReduction,
			Detector,
			
			Count__,
		};

		float mChannelLink01 = 0;

		bool mMidSideEnable = false;
		OutputSignal mOutputSignal = OutputSignal::Normal;

		float mDrySignal0 = 0;
		float mDrySignal1 = 0;
		float mSidechainSignal0 = 0;
		float mSidechainSignal1 = 0;

		float mDetectorSignal0 = 0;
		float mDetectorSignal1 = 0;

		Maj7Comp() :
			Device((int)ParamIndices::NumParams),
			mParams(mParamCache, 0)
		{
			LoadDefaults();
		}

		virtual void LoadDefaults() override
		{
			M7::ImportDefaultsArray(std::size(gParamDefaults), gParamDefaults, mParamCache);
		}

		virtual void SetParam(int index, float value) override
		{
			mParamCache[index] = value;
			mChannelLink01 = mParams.Get01Value(ParamIndices::ChannelLink, 0);
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

			mComp[0].ProcessSampleBeforeChannelMixing(in0);
			mComp[1].ProcessSampleBeforeChannelMixing(in1);
			float sum = std::max(mComp[0].mPreDetector, mComp[1].mPreDetector); // NB: requires rectification before this.
			float mixedDetector0 = M7::math::lerp(mComp[0].mPreDetector, sum, mChannelLink01);
			mComp[0].ProcessSampleAfterChannelMixing(mixedDetector0);
			float mixedDetector1 = M7::math::lerp(mComp[1].mPreDetector, sum, mChannelLink01);
			mComp[1].ProcessSampleAfterChannelMixing(mixedDetector1);
		}

		virtual void Run(double songPosition, float** inputs, float** outputs, int numSamples) override
		{
			for (size_t iSample = 0; iSample < (size_t)numSamples; ++iSample)
			{
				float in0 = inputs[0][iSample];
				float in1 = inputs[1][iSample];

				float out0, out1;

				if (mMidSideEnable) {
					float m, s;
					M7::MSEncode(in0, in1, &m, &s);
					CompressorComb(m, s); // the output of this is mComp[...].mOutput
					M7::MSDecode(mComp[0].mOutput, mComp[1].mOutput, &out0, &out1);
				}
				else {
					CompressorComb(in0, in1); // the output of this is mComp[...].mOutput
					out0 = mComp[0].mOutput;
					out1 = mComp[1].mOutput;
				}

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
				case OutputSignal::GainReduction:
					out0 = mComp[0].mGainReduction;
					out1 = mComp[1].mGainReduction;
					break;
				case OutputSignal::Detector:
					out0 = mComp[0].mPostDetector;
					out1 = mComp[1].mPostDetector;
					break;
				}

				outputs[0][iSample] = out0;
				outputs[1][iSample] = out1;
			}

		}
	};
}

