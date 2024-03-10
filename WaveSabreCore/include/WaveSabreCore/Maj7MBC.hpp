
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

		static_assert((int)ParamIndices::NumParams == 41, "param count probably changed and this needs to be regenerated.");
		static constexpr int16_t gParamDefaults[(int)ParamIndices::NumParams] = {
		  8230, // InGain = 0.25118863582611083984
		  0,
		  13557, // xAFreq = 0.4137503504753112793
		  21577, // xBFreq = 0.65849626064300537109
		  8230, // OutGain = 0.25118863582611083984
		  8230, // AInVol = 0.25118863582611083984
		  8230, // AOutVol = 0.25118863582611083984
		  21845, // AThresh = 0.6666666865348815918
		  15269, // AAttack = 0.46597486734390258789
		  15909, // ARelease = 0.48552104830741882324
		  18939, // ARatio = 0.57798200845718383789
		  4369, // AKnee = 0.13333334028720855713
		  26214, // AChanLnk = 0.80000001192092895508
		  0, // AEnable = 0
		  0, // AHPF = 0
		  6553, // AHPQ = 0.20000000298023223877
  4125, // ADrive = 0.12589254975318908691
		  8230, // BInVol = 0.25118863582611083984
		  8230, // BOutVol = 0.25118863582611083984
		  21845, // BThresh = 0.6666666865348815918
		  15269, // BAttack = 0.46597486734390258789
		  15909, // BRelease = 0.48552104830741882324
		  18939, // BRatio = 0.57798200845718383789
		  4369, // BKnee = 0.133331298828125
		  26214, // BChanLnk = 0.80000001192092895508
		  0, // BEnable = 0
		  0, // BHPF = 0
		  6553, // BHPQ = 0.20000000298023223877
  4125, // ADrive = 0.12589254975318908691
		  8230, // CInVol = 0.25118863582611083984
		  8230, // COutVol = 0.25118863582611083984
		  21845, // CThresh = 0.6666666865348815918
		  15269, // CAttack = 0.46597486734390258789
		  15909, // CRelease = 0.48552104830741882324
		  18939, // CRatio = 0.57798200845718383789
		  4369, // CKnee = 0.13333334028720855713
		  26214, // CChanLnk = 0.80000001192092895508
		  0, // CEnable = 0
		  0, // CHPF = 0
		  6553, // CHPQ = 0.20000000298023223877
  4125, // ADrive = 0.12589254975318908691
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
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

		Maj7MBC() :
			Device((int)ParamIndices::NumParams, mParamCache, gParamDefaults),
			mParams(mParamCache, 0)
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
			,
			mInputAnalysis{ AnalysisStream{gAnalysisFalloffMS}, AnalysisStream{gAnalysisFalloffMS} },
			mOutputAnalysis{ AnalysisStream{gAnalysisFalloffMS}, AnalysisStream{gAnalysisFalloffMS} }
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT
		{
			LoadDefaults();
		}

		virtual void SetParam(int index, float value) override
		{
			mParamCache[index] = value;
			for (auto& b : mBands) {
				b.Slider();
			}
		}

		virtual void Run(double songPosition, float** inputs, float** outputs, int numSamples) override
		{
			float inputGainLin = mParams.GetLinearVolume(ParamIndices::InputGain, M7::gVolumeCfg24db);
			float outputGainLin = mParams.GetLinearVolume(ParamIndices::OutputGain, M7::gVolumeCfg24db);
			float crossoverFreqA = mParams.GetFrequency(ParamIndices::CrossoverAFrequency, M7::gFilterFreqConfig);
			float crossoverFreqB = mParams.GetFrequency(ParamIndices::CrossoverBFrequency, M7::gFilterFreqConfig);
			bool mbEnable = mParams.GetBoolValue(ParamIndices::MultibandEnable);

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
