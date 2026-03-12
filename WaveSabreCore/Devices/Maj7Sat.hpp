
#pragma once

#include "../WSCore/Device.h"
#include "../GigaSynth/Maj7Basic.hpp"
#include "../DSP/Maj7SaturationBase.hpp"

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
#include <vector>
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

namespace WaveSabreCore
{
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	struct Maj7Sat : public Device
	{
		struct BandConfig {
			// let's just assume bool is atomic.
			bool mMute = false;
			bool mSolo = false;
		};

		//static constexpr float CalcDivK(float alpha) {
		//	return 2.0f * alpha / (1.0f - alpha);
		//}
		using SatBase = M7::Maj7SaturationBase;

		static constexpr float gAnalogMaxLin = 2;

		enum class PanMode //: uint8_t 
		{
			Stereo,
			MidSide,
			Count__,
		};

		using Model = SatBase::Model;

#define MAJ7SAT_MODEL_CAPTIONS(symbolName) static constexpr auto& symbolName = ::WaveSabreCore::M7::Maj7SaturationBase::ModelCaptions

		enum class ParamIndices
		{
			InputGain,
			CrossoverAFrequency,
			CrossoverBFrequency,
			OverallDryWet,
			OutputGain,

			// low band
			//APanMode,
			//APan,
			AModel,
			ADrive,
			ACompensationGain,
			AOutputGain,
			AThreshold,
			AEvenHarmonics,
			ADryWet,
			AEnable,

			// mid band
			BModel,
			BDrive,
			BCompensationGain,
			BOutputGain,
			BThreshold,
			BEvenHarmonics,
			BDryWet,
			BEnable,

			// hi band
			CModel,
			CDrive,
			CCompensationGain,
			COutputGain,
			CThreshold,
			CEvenHarmonics,
			CDryWet,
			CEnable,

			NumParams,
		};

		// NB: max 8 chars per string.
#define MAJ7SAT_PARAM_VST_NAMES(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::Maj7Sat::ParamIndices::NumParams]{ \
			{"InpGain"}, /* InputGain */ \
			{"xAFreq"}, /* CrossoverAFrequency */ \
			{"xBFreq"}, /* CrossoverBFrequency */ \
			{"GDryWet"}, /* drywet global */ \
			{"OutpGain"}, /* OutputGain */ \
		{"0Model"}, /* Model */ \
		{"0Drive"}, /* Drive */ \
		{"0CmpGain"}, /* CompensationGain */ \
		{"0OutGain"}, /* CompensationGain */ \
		{"0Thresh"}, /* Threshold */ \
		{"0Analog"}, /* analog */ \
		{"0DryWet"}, /* DryWet */ \
		{"0Enable"}, /* */\
		{"1Model"}, /* Model */ \
		{"1Drive"}, /* Drive */ \
		{"1CmpGain"}, /* CompensationGain */ \
		{"1OutGain"}, /* CompensationGain */ \
		{"1Thresh"}, /* Threshold */ \
		{"1Analog"}, /* analog */ \
		{"1DryWet"}, /* DryWet */ \
		{"1Enable"}, /* */\
		{"2Model"}, /* Model */ \
		{"2Drive"}, /* Drive */ \
		{"2CmpGain"}, /* CompensationGain */ \
		{"2OutGain"}, /* CompensationGain */ \
		{"2Thresh"}, /* Threshold */ \
		{"2Analog"}, /* analog */ \
		{"2DryWet"}, /* DryWet */ \
		{"2Enable"}, /* */\
}

static_assert((int)ParamIndices::NumParams == 29, "param count probably changed and this needs to be regenerated.");
static constexpr int16_t gParamDefaults[(int)ParamIndices::NumParams] = {
  8230, // InpGain = 0.25118863582611083984
  13557, // xAFreq = 0.4137503504753112793
  21576, // xBFreq = 0.65849626064300537109
  32767, // GDryWet = 1
  8230, // OutpGain = 0.25118863582611083984
  3, // 0Model = 0.0001220703125
  4125, // 0Drive = 0.12589254975318908691
  16422, // 0CmpGain = 0.50118720531463623047
  16422, // 0OutGain = 0.50118720531463623047
  20674, // 0Thresh = 0.63095736503601074219
  1966, // 0Analog = 0.059999998658895492554
  32767, // 0DryWet = 1
  0, // 0Enable = 0
  3, // 1Model = 0.0001220703125
  4125, // 1Drive = 0.12589254975318908691
  16422, // 1CmpGain = 0.50118720531463623047
  16422, // 1OutGain = 0.50118720531463623047
  20674, // 1Thresh = 0.63095736503601074219
  1966, // 1Analog = 0.059999998658895492554
  32767, // 1DryWet = 1
  0, // 1Enable = 0
  3, // 2Model = 0.0001220703125
  4125, // 2Drive = 0.12589254975318908691
  16422, // 2CmpGain = 0.50118720531463623047
  16422, // 2OutGain = 0.50118720531463623047
  20674, // 2Thresh = 0.63095736503601074219
  1966, // 2Analog = 0.059999998658895492554
  32767, // 2DryWet = 1
  0, // 2Enable = 0
};



		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		struct FreqBand {
			enum class BandParam {
				//Mute,
				//Solo,
				//PanMode,
				//Pan,
				Model,
				Drive,
				CompensationGain,
				OutputGain,
				Threshold,
				EvenHarmonics,
				DryWet,
				EnableEffect,
			};

			float mDriveLin ;
			float mCompensationGain ;
			float mOutputGain ;
			float mAutoDriveCompensation ;
			float mThresholdLin ;
			float mDryWet ;
#ifdef MAJ7SAT_ENABLE_ANALOG
			float mEvenHarmonicsGainLin ;
#endif // MAJ7SAT_ENABLE_ANALOG
			float mCorrSlope;

#ifdef MAJ7SAT_ENABLE_MIDSIDE//#define MAJ7SAT_ENABLE_MIDSIDE
			float mPanN11;
			PanMode mPanMode;
#endif//#define MAJ7SAT_ENABLE_MIDSIDE

			Model mModel;
			bool mEnableEffect;
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
			bool mMute;
			bool mSolo;
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

			M7::ParamAccessor mParams;
#ifdef MAJ7SAT_ENABLE_ANALOG
			M7::DCFilter mDC[2];
#endif // MAJ7SAT_ENABLE_ANALOG

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
			AnalysisStream mInputAnalysis0;
			AnalysisStream mInputAnalysis1;
			AnalysisStream mOutputAnalysis0;
			AnalysisStream mOutputAnalysis1;

			bool mMuteSoloEnabled;
			BandConfig mVSTConfig;
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

			FreqBand(float* paramCache, ParamIndices baseParamID) : //
				mParams(paramCache, baseParamID)
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
				,
				mInputAnalysis0(700),
				mInputAnalysis1(700),
				mOutputAnalysis0(700),
				mOutputAnalysis1(700)
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT
			{}

			void Slider() {

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
				//mMute = mParams.GetBoolValue(BandParam::Mute);
				//mSolo = mParams.GetBoolValue(BandParam::Solo);
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT
				mEnableEffect = mParams.GetBoolValue(BandParam::EnableEffect);

				mDriveLin = mParams.GetLinearVolume(BandParam::Drive, M7::gVolumeCfg36db);
				mCompensationGain = mParams.GetLinearVolume(BandParam::CompensationGain, M7::gVolumeCfg12db);
				mOutputGain = mParams.GetLinearVolume(BandParam::OutputGain, M7::gVolumeCfg12db);
				mThresholdLin = mParams.GetLinearVolume(BandParam::Threshold, M7::gUnityVolumeCfg);
				// threshold must be corrected, otherwise it will cause a div by 0 when we scale the waveform up.
				mThresholdLin = M7::math::clamp(mThresholdLin, 0, 0.99f);

				mModel = mParams.GetEnumValue<Model>(BandParam::Model);

				mDryWet = mParams.Get01Value(BandParam::DryWet);
#ifdef MAJ7SAT_ENABLE_ANALOG
				mEvenHarmonicsGainLin = mParams.GetScaledRealValue(BandParam::EvenHarmonics, 0, gAnalogMaxLin, 0); //mParams.GetLinearVolume(BandParam::EvenHarmonics, M7::gVolumeCfg36db, 0);
#endif // MAJ7SAT_ENABLE_ANALOG

#ifdef MAJ7SAT_ENABLE_MIDSIDE//#define MAJ7SAT_ENABLE_MIDSIDE
				mPanMode = mParams.GetEnumValue<PanMode>(BandParam::PanMode);
				mPanN11 = mParams.GetN11Value(BandParam::Pan, 0);
#endif//#define MAJ7SAT_ENABLE_MIDSIDE

				// this controls how extreme the compensation is; extracted to shared saturation base.
				mAutoDriveCompensation = M7::Maj7SaturationBase::CalcAutoDriveCompensation(mDriveLin);

				mCorrSlope = M7::math::lerp(SatBase::ModelNaturalSlopes[(int)mModel], 1, mThresholdLin);
			}

			float distort(float s, size_t chanIndex)
			{
				s = SatBase::DistortSample(
					s,
					chanIndex,
					mModel,
					mThresholdLin,
					mCorrSlope,
					mAutoDriveCompensation
#ifdef MAJ7SAT_ENABLE_ANALOG
					,
					mDC,
					mEvenHarmonicsGainLin
#endif
				);
				s *= mCompensationGain;
				return s;
			}

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
			// transfer function for VST display purposes.
			float transfer(float s)
			{
				return SatBase::DistortSample(
					s,
					0, // chan index doesn't matter for transfer function since it's just for display.
					mModel,
					mThresholdLin,
					mCorrSlope,
					mAutoDriveCompensation
#ifdef MAJ7SAT_ENABLE_ANALOG
					,
					nullptr,
					0
#endif
				);
			}
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT


			M7::FloatPair ProcessSample(float s0, float s1, float masterDryWet
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
				, bool isGuiVisible
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT
			)
			{
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
				if (isGuiVisible) {
					if (mEnableEffect && mMuteSoloEnabled) {
						mInputAnalysis0.WriteSample(s0);
						mInputAnalysis1.WriteSample(s1);
					}
					else {
						// analysis halts when not processing. it's better visually.
						mInputAnalysis0.WriteSample(0);
						mInputAnalysis1.WriteSample(0);
					}
				}
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

				float sa[2] = { s0, s1 };
				float dry[2] = { s0, s1 };

				if (mEnableEffect) {
#ifdef MAJ7SAT_ENABLE_MIDSIDE
					if (mPanMode == PanMode::MidSide) {
						M7::MSEncode(sa[0], sa[1], &sa[0], &sa[1]);
						dry[0] = sa[0];
						dry[1] = sa[1];
					}

					// DO PANNING
					// do NOT use a pan law here; when it's centered it means both channels should get 100% wetness.
					float dryWet[2] = { 1,1 };
					if (mPanN11 < 0) dryWet[1] = mPanN11 + 1.0f; // left chan (right attenuated)
					if (mPanN11 > 0) dryWet[0] = 1.0f - mPanN11; // right chan (left attenuated)
#endif // MAJ7SAT_ENABLE_MIDSIDE

					for (int i = 0; i < 2; ++i) {
						float& s = *(sa + i);

						//s *= SatBase::ModelPregain[(int)mModel];
						s *= mDriveLin;
						s = distort(s, i);

						// note: when (e.g.) saturating only side channel, you'll get a signal that's too wide because of the natural
						// gain that saturation results in.
						// in these cases, dry/wet is very important param for balancing the stereo image again, in MidSide mode.
#ifdef MAJ7SAT_ENABLE_MIDSIDE
						s = M7::math::lerp(dry[i], s, dryWet[i] * mDryWet * masterDryWet);
#else
						s = M7::math::lerp(dry[i], s, mDryWet * masterDryWet);
#endif // MAJ7SAT_ENABLE_MIDSIDE
					}

#ifdef MAJ7SAT_ENABLE_MIDSIDE
					if (mPanMode == PanMode::MidSide) {
						M7::MSDecode(sa[0], sa[1], &sa[0], &sa[1]);
					}
#endif // MAJ7SAT_ENABLE_MIDSIDE
				}

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
				float output0, output1;
				if (mMuteSoloEnabled) {
					if (mEnableEffect) {
						output0 = sa[0];
						output1 = sa[1];
					}
					else {
						output0 = dry[0];
						output1 = dry[1];
					}
					output0 *= mOutputGain;
					output1 *= mOutputGain;
				}
				else {
					output0 = 0;
					output1 = 0;
				}

				if (isGuiVisible) {
					if (mEnableEffect && mMuteSoloEnabled) {
						mOutputAnalysis0.WriteSample(output0);
						mOutputAnalysis1.WriteSample(output1);
					}
					else {
						// analysis halts when not processing. it's better visually.
						mOutputAnalysis0.WriteSample(0);
						mOutputAnalysis1.WriteSample(0);
					}
				}
				return {output0, output1};
#else
				return {sa[0], sa[1]};
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

			}
		}; // struct FreqBand

		float mParamCache[(int)ParamIndices::NumParams];

		M7::ParamAccessor mParams;

		Maj7Sat() :
			Device((int)ParamIndices::NumParams, mParamCache, gParamDefaults),
			mParams(mParamCache, 0)
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
			,
			mInputAnalysis0(1000),
			mInputAnalysis1(1000),
			mOutputAnalysis0(1000),
			mOutputAnalysis1(1000)
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT
		{
			LoadDefaults();
		}

		static constexpr size_t gBandCount = 3;
		FreqBand mBands[gBandCount] = {
			{mParamCache, ParamIndices::AModel },
			{mParamCache, ParamIndices::BModel},
			{mParamCache, ParamIndices::CModel},
		};

		float mInputGainLin = 0;
		float mOutputGainLin = 0;

		///float mCrossoverFreqA = 0;
		//float mCrossoverFreqB = 0;

		//M7::LinkwitzRileyFilter::Slope mCrossoverSlopeA = M7::LinkwitzRileyFilter::Slope::Slope_12dB;
		//M7::LinkwitzRileyFilter::Slope mCrossoverSlopeB = M7::LinkwitzRileyFilter::Slope::Slope_12dB;

		virtual void SetParam(int index, float value) override
		{
			mParamCache[index] = value;

			for (auto& b : mBands) {
				b.Slider();
			}

			mInputGainLin = mParams.GetLinearVolume(ParamIndices::InputGain, M7::gVolumeCfg24db);
			mOutputGainLin = mParams.GetLinearVolume(ParamIndices::OutputGain, M7::gVolumeCfg24db);

			const auto crossoverFreqA = mParams.GetFrequency(ParamIndices::CrossoverAFrequency, M7::gFilterFreqConfig);
			const auto crossoverFreqB = mParams.GetFrequency(ParamIndices::CrossoverBFrequency, M7::gFilterFreqConfig);
      splitter0.SetParams( crossoverFreqA, M7::CrossoverSlope::Slope_24dB, crossoverFreqB, M7::CrossoverSlope::Slope_24dB );
      splitter1.SetParams( crossoverFreqA, M7::CrossoverSlope::Slope_24dB, crossoverFreqB, M7::CrossoverSlope::Slope_24dB );

			//mCrossoverSlopeA = mParams.GetEnumValue<M7::LinkwitzRileyFilter::Slope>(ParamIndices::CrossoverASlope);
		}

		M7::FrequencySplitter splitter0;
		M7::FrequencySplitter splitter1;

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
		AnalysisStream mInputAnalysis0;
		AnalysisStream mInputAnalysis1;
		AnalysisStream mOutputAnalysis0;
		AnalysisStream mOutputAnalysis1;
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

		virtual void Run(float** inputs, float** outputs, int numSamples) override
		{
			float masterDryWet = mParams.GetRawVal(ParamIndices::OverallDryWet);

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
			bool isGuiVisible = IsGuiVisible();
			bool muteSoloEnabled[Maj7MBC::gBandCount] = { false, false, false };
			bool mutes[Maj7MBC::gBandCount] = { mBands[0].mVSTConfig.mMute, mBands[1].mVSTConfig.mMute , mBands[2].mVSTConfig.mMute };
			bool solos[Maj7MBC::gBandCount] = { mBands[0].mVSTConfig.mSolo, mBands[1].mVSTConfig.mSolo , mBands[2].mVSTConfig.mSolo };
			M7::CalculateMuteSolo(mutes, solos, muteSoloEnabled);
			mBands[0].mMuteSoloEnabled = muteSoloEnabled[0];
			mBands[1].mMuteSoloEnabled = muteSoloEnabled[1];
			mBands[2].mMuteSoloEnabled = muteSoloEnabled[2];
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

			for (size_t i = 0; i < (size_t)numSamples; ++i)
			{
				float s0 = inputs[0][i] * mInputGainLin;
				float s1 = inputs[1][i] * mInputGainLin;

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
				if (isGuiVisible)
				{
					mInputAnalysis0.WriteSample(s0);
					mInputAnalysis1.WriteSample(s1);
				}
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

				// split into 3 bands
				const auto splitL = splitter0.frequency_splitter(s0);
				const auto splitR = splitter1.frequency_splitter(s1);

				s0 = 0;
				s1 = 0;
				for (int iBand = 0; iBand < gBandCount; ++iBand) {
					auto& band = mBands[iBand];
					auto r = band.ProcessSample(splitL.s[iBand], splitR.s[iBand], masterDryWet
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
						, isGuiVisible
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT
					);
					s0 += r.x[0] * mOutputGainLin;
					s1 += r.x[1] * mOutputGainLin;
				}

				//s0 = mBands[0].output0 + mBands[1].output0 + mBands[2].output0;
				//s1 = mBands[0].output1 + mBands[1].output1 + mBands[2].output1;

				//s0 *= mOutputGainLin;
				//s1 *= mOutputGainLin;

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
				if (isGuiVisible)
				{
					mOutputAnalysis0.WriteSample(s0);
					mOutputAnalysis1.WriteSample(s1);
				}
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

				// for sanity; avoid user error or filter madness causing crazy spikes.
				// so clamp at about +12db clip. todo: param smoothing to avoid these spikes.
				outputs[0][i] = M7::math::clamp(s0, -4, 4);
				outputs[1][i] = M7::math::clamp(s1, -4, 4);
			} // for i < numSamples
		} // Run()

	};
}
