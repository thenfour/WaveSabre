
#pragma once

#include "Device.h"
#include "Maj7Basic.hpp"
#include "Maj7Comp.hpp"

namespace WaveSabreCore
{
	struct Maj7MBC : public Device
	{
		enum class OutputStream : uint8_t {
			Normal,
			Delta,
			Sidechain,
			Count__
		};

		struct BandConfig {
			// let's just assume bool is atomic.
			bool mMute = false;
			bool mSolo = false;
			OutputStream mOutputStream = OutputStream::Normal;
		};

		enum class ParamIndices
		{
			InputGain,
			MultibandEnable,
			CrossoverAFrequency,
			CrossoverBFrequency,
			OutputGain,
			SoftClipEnable,
			SoftClipThresh,
			SoftClipOutput,

			AInputGain,
			AOutputGain,
			AThreshold,
			AAttack,
			ARelease,
			ARatio,
			AKnee,
			AChannelLink,
			AEnable, // tempting to remove this because you can set ratio=1. however, this enables parameter optimization much easier.
			AHighPassFrequency, // biquad freq; default lo
			AHighPassQ, // default 0.2
			ADrive,

			BInputGain,
			BOutputGain,
			BThreshold,
			BAttack,
			BRelease,
			BRatio,
			BKnee,
			BChannelLink,
			BEnable, // tempting to remove this because you can set ratio=1. however, this enables parameter optimization much easier.
			BHighPassFrequency, // biquad freq; default lo
			BHighPassQ, // default 0.2
			BDrive,

			CInputGain,
			COutputGain,
			CThreshold,
			CAttack,
			CRelease,
			CRatio,
			CKnee,
			CChannelLink,
			CEnable, // tempting to remove this because you can set ratio=1. however, this enables parameter optimization much easier.
			CHighPassFrequency, // biquad freq; default lo
			CHighPassQ, // default 0.2
			CDrive,

			NumParams,
		};

		// NB: max 8 chars per string.
#define MAJ7MBC_PARAM_VST_NAMES(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::Maj7MBC::ParamIndices::NumParams]{ \
			{"InGain"},\
			{"MBEnable"},\
			{"xAFreq"},\
			{"xBFreq"},\
			{"OutGain"},\
			{"SCEn"},\
			{"SCThr"},\
			{"SCOutp"},\
			{"AInVol"},\
			{"AOutVol"},\
			{"AThresh"},\
			{"AAttack"},\
			{"ARelease"},\
			{"ARatio"},\
			{"AKnee"},\
			{"AChanLnk"},\
			{"AEnable"},\
			{"AHPF"},\
			{"AHPQ"},\
			{"ADrive"},\
			{"BInVol"},\
			{"BOutVol"},\
			{"BThresh"},\
			{"BAttack"},\
			{"BRelease"},\
			{"BRatio"},\
			{"BKnee"},\
			{"BChanLnk"},\
			{"BEnable"},\
			{"BHPF"},\
			{"BHPQ"},\
			{"BDrive"},\
			{"CInVol"},\
			{"COutVol"},\
			{"CThresh"},\
			{"CAttack"},\
			{"CRelease"},\
			{"CRatio"},\
			{"CKnee"},\
			{"CChanLnk"},\
			{"CEnable"},\
			{"CHPF"},\
			{"CHPQ"},\
			{"CDrive"},\
}

		static_assert((int)ParamIndices::NumParams == 44, "param count probably changed and this needs to be regenerated.");
		static constexpr int16_t gParamDefaults[(int)ParamIndices::NumParams] = {
		  20512, // InGain = 0.62599998712539672852
		  0, // MBEnable = 0
		  13557, // xAFreq = 0.41372698545455932617
		  21577, // xBFreq = 0.65847802162170410156
		  8230, // OutGain = 0.25118899345397949219
		  0, // SCEn = 0
		  23197, // SCThr = 0.70794582366943359375
		  32206, // SCOutp = 0.98287886381149291992
		  8230, // AInVol = 0.25115999579429626465
		  8230, // AOutVol = 0.25115999579429626465
		  21845, // AThresh = 0.66665697097778320312
		  15269, // AAttack = 0.46597298979759216309
		  15908, // ARelease = 0.48550400137901306152
		  18938, // ARatio = 0.57797199487686157227
		  4368, // AKnee = 0.13333100080490112305
		  26214, // AChanLnk = 0.79998797178268432617
		  0, // AEnable = 0
		  0, // AHPF = 0
		  6553, // AHPQ = 0.1999820023775100708
		  4124, // ADrive = 0.12588499486446380615
		  8230, // BInVol = 0.25115999579429626465
		  8230, // BOutVol = 0.25115999579429626465
		  21845, // BThresh = 0.66665697097778320312
		  15269, // BAttack = 0.46597298979759216309
		  15908, // BRelease = 0.48550400137901306152
		  18938, // BRatio = 0.57797199487686157227
		  4368, // BKnee = 0.13333100080490112305
		  26214, // BChanLnk = 0.79998797178268432617
		  0, // BEnable = 0
		  0, // BHPF = 0
		  6553, // BHPQ = 0.1999820023775100708
		  4124, // BDrive = 0.12588499486446380615
		  8230, // CInVol = 0.25115999579429626465
		  8230, // COutVol = 0.25115999579429626465
		  21845, // CThresh = 0.66665697097778320312
		  15269, // CAttack = 0.46597298979759216309
		  15908, // CRelease = 0.48550400137901306152
		  18938, // CRatio = 0.57797199487686157227
		  4368, // CKnee = 0.13333100080490112305
		  26214, // CChanLnk = 0.79998797178268432617
		  0, // CEnable = 0
		  0, // CHPF = 0
		  6553, // CHPQ = 0.1999820023775100708
		  4124, // CDrive = 0.12588499486446380615
		};


		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		struct FreqBand {
			enum class BandParam : uint8_t {
				InputGain,
				OutputGain,
				Threshold,
				Attack,
				Release,
				Ratio,
				Knee,
				ChannelLink,
				Enable,
				HighPassFrequency,
				HighPassQ,
				Drive,
				Count__,
			};

			static_assert((int)ParamIndices::BInputGain - (int)ParamIndices::AInputGain == (int)BandParam::Count__, "bandparams need to be in sync with main params");

			M7::ParamAccessor mParams;
			MonoCompressor mComp[2];
			bool mEnable;
			float mDriveLin;
			float mInputGainLin;
			float mOutputGainLin;

			FreqBand(float* paramCache, ParamIndices baseParamID) : //
				mParams(paramCache, baseParamID)
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
				,
				mInputAnalysis{ AnalysisStream{gAnalysisFalloffMS}, AnalysisStream{gAnalysisFalloffMS} },
				mOutputAnalysis{ AnalysisStream{gAnalysisFalloffMS}, AnalysisStream{gAnalysisFalloffMS} },
				mDetectorAnalysis{ AnalysisStream{gAnalysisFalloffMS}, AnalysisStream{gAnalysisFalloffMS} },
				mAttenuationAnalysis{ AnalysisStream{gAnalysisFalloffMS}, AnalysisStream{gAnalysisFalloffMS} }
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT
			{}


#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
			static constexpr float gAnalysisFalloffMS = 500;
			AnalysisStream mInputAnalysis[2];
			AnalysisStream mOutputAnalysis[2];
			AnalysisStream mDetectorAnalysis[2];
			AnalysisStream mAttenuationAnalysis[2];

			BandConfig mVSTConfig;
			bool mMuteSoloEnable;
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT


			void Slider() {

				mEnable = mParams.GetBoolValue(BandParam::Enable);
				mDriveLin = mParams.GetLinearVolume(BandParam::Drive, M7::gVolumeCfg36db);
				mInputGainLin = mParams.GetLinearVolume(BandParam::InputGain, M7::gVolumeCfg24db);
				mOutputGainLin = mParams.GetLinearVolume(BandParam::OutputGain, M7::gVolumeCfg24db);
					
					for (auto& c : mComp) {
					c.SetParams(
						mParams.GetDivCurvedValue(BandParam::Ratio, MonoCompressor::gRatioCfg, 0),
						mParams.GetScaledRealValue(BandParam::Knee, 0, 30, 0),
						mParams.GetScaledRealValue(BandParam::Threshold, -60, 0, 0),
						mParams.GetPowCurvedValue(BandParam::Attack, MonoCompressor::gAttackCfg, 0),
						mParams.GetPowCurvedValue(BandParam::Release, MonoCompressor::gReleaseCfg, 0),
						mParams.GetFrequency(BandParam::HighPassFrequency, M7::gFilterFreqConfig),
						mParams.GetDivCurvedValue(BandParam::HighPassQ, M7::gBiquadFilterQCfg)
					);
				}
			}

			M7::FloatPair ProcessSample(M7::FloatPair input)
			{
				// apply stereo linking
				float monoDetector = input.x[0] + input.x[1];
				float channelLink01 = mParams.Get01Value(BandParam::ChannelLink);
				M7::FloatPair output{ input };

				if (mEnable) {
					for (size_t ich = 0; ich < 2; ++ich) {
						float inpAudio = input.x[ich] * mInputGainLin;
						float detector = M7::math::lerp(inpAudio, monoDetector, channelLink01);
						float sout = mComp[ich].ProcessSample(inpAudio, detector);
						if (mDriveLin > 1) {
							// i'd love to do some kind of volume correction but it's not really practical.
							sout = M7::math::tanh(mDriveLin * sout);
						}
						sout *= mOutputGainLin;
						output.x[ich] = sout;

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
mInputAnalysis[ich].WriteSample(inpAudio);
mOutputAnalysis[ich].WriteSample(sout);
mDetectorAnalysis[ich].WriteSample(detector);
mAttenuationAnalysis[ich].WriteSample(mComp[ich].mGainReduction);
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT
					}
				}
				else {
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
				for (size_t ich = 0; ich < 2; ++ich) {
					mInputAnalysis[ich].WriteSample(0);
					mOutputAnalysis[ich].WriteSample(0);
					mDetectorAnalysis[ich].WriteSample(0);
					mAttenuationAnalysis[ich].WriteSample(0);
				}
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT
				}
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
				if (!mMuteSoloEnable) {
					return { 0,0 };
				}
				switch (mVSTConfig.mOutputStream) {
				case OutputStream::Normal:
					break;
				case OutputStream::Delta: {
					if (!mEnable) return { 0,0 };
					return { mComp[0].mDiff, mComp[1].mDiff };
				}
				case OutputStream::Sidechain: {
					if (!mEnable) return input;
					return { mComp[0].mSidechain, mComp[1].mSidechain };
				}
				}
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

				return output;
			}
		}; // struct FreqBand
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


		float mParamCache[(int)ParamIndices::NumParams];

		M7::ParamAccessor mParams;

		static constexpr size_t gBandCount = 3;

		FreqBand mBands[gBandCount] = {
			{mParamCache, ParamIndices::AInputGain },
			{mParamCache, ParamIndices::BInputGain},
			{mParamCache, ParamIndices::CInputGain},
		};

		M7::FrequencySplitter splitter0;
		M7::FrequencySplitter splitter1;

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
		static constexpr float gAnalysisFalloffMS = 500;
		AnalysisStream mInputAnalysis[2];
		AnalysisStream mOutputAnalysis[2];
		AttenuationAnalysisStream mClippingAnalysis[2];
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

		Maj7MBC() :
			Device((int)ParamIndices::NumParams, mParamCache, gParamDefaults),
			mParams(mParamCache, 0)
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
			,
			mInputAnalysis{ AnalysisStream{gAnalysisFalloffMS}, AnalysisStream{gAnalysisFalloffMS} },
			mOutputAnalysis{ AnalysisStream{gAnalysisFalloffMS}, AnalysisStream{gAnalysisFalloffMS} },
			mClippingAnalysis {
				AttenuationAnalysisStream{ 6000 },
				AttenuationAnalysisStream{ 6000 }
		}
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT
		{
			LoadDefaults();
		}

		virtual void OnParamsChanged() override
		{
			for (auto& b : mBands) {
				b.Slider();
			}
		}

		// returns {output sample, clip amount}
		// NOTE: clip amount is 0 for anything below clipping level, even though shaping actually does mess with amplitudes.
		// the visualizations just don't care about that area though; users only care about how much is being clipped.
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
		static M7::FloatPair Softclip(float s, float thresh, float outputGain) {
#else
		static float Softclip(float s, float thresh, float outputGain) {
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT
				// thresh of 1 (unity gain) means hard clipping. 0.99 is something like 0.001dB so this is an optimal way to approximate hard clipping
			// and a thresh of 0 would cause our attenuation calc to fail.
				thresh = M7::math::clamp(thresh, 0.01f, 0.99f);
				if (s < 0) {
					s = -s;
					outputGain = -outputGain;
				}
				static constexpr float naturalSlope = M7::math::gPIHalf;
				float corrSlope = M7::math::lerp(naturalSlope, 1, thresh);
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
				float atten = 1;
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT
				s *= corrSlope;
				if (s > thresh) {
					s = M7::math::lerp_rev(thresh, 1, s); // pull down above thresh range to 0,1
					s /= naturalSlope;
					if (s > 1) {
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
						atten = (1.0f / s);
						// we are clipping. for analysis purposes, make it CLEAR.
						atten = std::min(0.95f, atten); // this means attenuation is MINIMUM reported as .5dB
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT
						s = 1;
					}
					else {
						s = M7::math::sin(s * M7::math::gPIHalf);
					}
					s = M7::math::lerp(thresh, 1, s);
				}
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
				return { s * outputGain, atten }; // readd sign bit
#else
				return s * outputGain; // readd sign bit
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT
			}

		virtual void Run(double songPosition, float** inputs, float** outputs, int numSamples) override
		{
			float inputGainLin = mParams.GetLinearVolume(ParamIndices::InputGain, M7::gVolumeCfg24db);
			float outputGainLin = mParams.GetLinearVolume(ParamIndices::OutputGain, M7::gVolumeCfg24db);
			float crossoverFreqA = mParams.GetFrequency(ParamIndices::CrossoverAFrequency, M7::gFilterFreqConfig);
			float crossoverFreqB = mParams.GetFrequency(ParamIndices::CrossoverBFrequency, M7::gFilterFreqConfig);
			bool mbEnable = mParams.GetBoolValue(ParamIndices::MultibandEnable);

			bool softClipEnabled = mParams.GetBoolValue(ParamIndices::SoftClipEnable);
			float softClipThreshLin = mParams.GetLinearVolume(ParamIndices::SoftClipThresh, M7::gUnityVolumeCfg);
			float softClipOutputLin = mParams.GetLinearVolume(ParamIndices::SoftClipOutput, M7::gUnityVolumeCfg);

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
			bool muteSoloEnabled[Maj7MBC::gBandCount] = { false, false, false };
			bool mutes[Maj7MBC::gBandCount] = { mBands[0].mVSTConfig.mMute, mBands[1].mVSTConfig.mMute , mBands[2].mVSTConfig.mMute };
			bool solos[Maj7MBC::gBandCount] = { mBands[0].mVSTConfig.mSolo, mBands[1].mVSTConfig.mSolo , mBands[2].mVSTConfig.mSolo };
			M7::CalculateMuteSolo(mutes, solos, muteSoloEnabled);
			mBands[0].mMuteSoloEnable = muteSoloEnabled[0];
			mBands[1].mMuteSoloEnable = muteSoloEnabled[1];
			mBands[2].mMuteSoloEnable = muteSoloEnabled[2];
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

			for (size_t i = 0; i < (size_t)numSamples; ++i)
			{
				float s0 = inputs[0][i] * inputGainLin;
				float s1 = inputs[1][i] * inputGainLin;

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
				mInputAnalysis[0].WriteSample(s0);
				mInputAnalysis[1].WriteSample(s1);
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

				if (mbEnable) {
					// split into 3 bands
					splitter0.frequency_splitter(s0, crossoverFreqA, crossoverFreqB);
					splitter1.frequency_splitter(s1, crossoverFreqA, crossoverFreqB);

					s0 = 0;
					s1 = 0;
					for (int iBand = 0; iBand < gBandCount; ++iBand) {
						auto& band = mBands[iBand];
						auto r = band.ProcessSample({ splitter0.s[iBand], splitter1.s[iBand] });
						s0 += r.x[0] * outputGainLin;
						s1 += r.x[1] * outputGainLin;
					}
				}
				else {
					auto& band = mBands[1];
					auto r = band.ProcessSample({ s0, s1 });
					s0 = r[0] * outputGainLin;
					s1 = r[1] * outputGainLin;
				}

				if (softClipEnabled) {
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
					auto sc0 = Softclip(s0, softClipThreshLin, softClipOutputLin);
					auto sc1 = Softclip(s1, softClipThreshLin, softClipOutputLin);
					s0 = sc0[0];
					s1 = sc1[0];
					mClippingAnalysis[0].WriteSample(sc0[1]);
					mClippingAnalysis[1].WriteSample(sc1[1]);
#else
					s0 = Softclip(s0, softClipThreshLin, softClipOutputLin);
					s1 = Softclip(s1, softClipThreshLin, softClipOutputLin);
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT
				}

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
				mOutputAnalysis[0].WriteSample(s0);
				mOutputAnalysis[1].WriteSample(s1);
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

				outputs[0][i] = s0;
				outputs[1][i] = s1;

			} // for i < numSamples

		}
	};
}
