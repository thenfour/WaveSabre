// note that these are NOT multiband + gains + join. these are 5 cascaded biquad filters; each perform independent peaking.

#ifndef __WAVESABRECORE_LEVELLER_H__
#define __WAVESABRECORE_LEVELLER_H__

#include <algorithm>

#include <WaveSabreCore/Maj7Basic.hpp>
#include <WaveSabreCore/Maj7ParamAccessor.hpp>
#include <WaveSabreCore/FFTAnalysis.hpp>
#include <WaveSabreCore/AnalysisStream.hpp>
#include "Device.h"
#include "BiquadFilter.h"
#include "RMS.hpp"
#include "Maj7Filter.hpp"
#include "DCFilter.hpp"


namespace WaveSabreCore
{
	struct Leveller : public Device
	{
		static constexpr size_t gBandCount = 5;

		enum class ParamIndices// : uint8_t
		{
			OutputVolume,
			EnableDCFilter,

			Band1Circuit,
			Band1Slope,
			Band1Response,
			Band1Freq,
			Band1Gain,
			Band1Q,
			Band1Enable,

			Band2Circuit,
			Band2Slope,
			Band2Response,
			Band2Freq,
			Band2Gain,
			Band2Q,
			Band2Enable,

			Band3Circuit,
			Band3Slope,
			Band3Response,
			Band3Freq,
			Band3Gain,
			Band3Q,
			Band3Enable,

			Band4Circuit,
			Band4Slope,
			Band4Response,
			Band4Freq,
			Band4Gain,
			Band4Q,
			Band4Enable,

			Band5Circuit,
			Band5Slope,
			Band5Response,
			Band5Freq,
			Band5Gain,
			Band5Q,
			Band5Enable,

			NumParams,
		};
#define LEVELLER_PARAM_VST_NAMES(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::Leveller::ParamIndices::NumParams]{ \
	{"OutpVol"},\
	{"DCEn"},\
		{"ACircuit"}, \
		{"ASlope"}, \
		{"AResp"}, \
		{"AFreq"}, \
		{"AGain"}, \
		{"AQ"}, \
		{"AEn"}, \
		{"BCircuit"}, \
		{"BSlope"}, \
		{"BResp"}, \
		{"BFreq"}, \
		{"BGain"}, \
		{"BQ"}, \
		{"BEn"}, \
		{"CCircuit"}, \
		{"CSlope"}, \
		{"CResp"}, \
		{"CFreq"}, \
		{"CGain"}, \
		{"CQ"}, \
		{"CEn"}, \
		{"DCircuit"}, \
		{"DSlope"}, \
		{"DResp"}, \
		{"DFreq"}, \
		{"DGain"}, \
		{"DQ"}, \
		{"DEn"}, \
		{"ECircuit"}, \
		{"ESlope"}, \
		{"EResp"}, \
		{"EFreq"}, \
		{"EGain"}, \
		{"EQ"}, \
		{"EEn"}, \
}

		static_assert((int)ParamIndices::NumParams == 37, "param count probably changed and this needs to be regenerated.");
		static constexpr int16_t gLevellerDefaults16[(int)ParamIndices::NumParams] = {
		  16422, // OutpVol = 0.50116002559661865234
		  0, // DCEn = 0
		  71, // ACircuit = 0.0021969999652355909348
		  40, // ASlope = 0.0012209999840706586838
		  40, // AResp = 0.0012209999840706586838
		  5000, // AFreq = 0.1526069939136505127
		  16422, // AGain = 0.5011870265007019043
		  14563, // AQ = 0.44444444775581359863
		  0, // AEn = 0
		  40, // BCircuit = 0.0012209999840706586838
		  40, // ASlope = 0.0012209999840706586838
		  40, // AResp = 0.0012209999840706586838
		  9830, // BFreq = 0.30000001192092895508
		  16422, // BGain = 0.5011870265007019043
		  14563, // BQ = 0.44444444775581359863
		  0, // BEn = 0
		  40, // CCircuit = 0.0012209999840706586838
		  40, // CSlope = 0.0012209999840706586838
		  40, // CResp = 0.0012209999840706586838
		  16834, // CFreq = 0.51375001668930053711
		  16422, // CGain = 0.5011870265007019043
		  14563, // CQ = 0.44444444775581359863
		  0, // CEn = 0
		  40, // DCircuit = 0.0012209999840706586838
		  40, // DSlope = 0.0012209999840706586838
		  40, // DResp = 0.0012209999840706586838
		  21577, // DFreq = 0.65849602222442626953
		  16422, // DGain = 0.5011870265007019043
		  14563, // DQ = 0.44444444775581359863
		  0, // DEn = 0
		  56, // ECircuit = 0.0017089999746531248093
		  56, // ESlope = 0.0017089999746531248093
		  56, // EResp = 0.0017089999746531248093
		  26500, // EFreq = 0.80874598026275634766
		  16422, // EGain = 0.5011870265007019043
		  14563, // EQ = 0.44444444775581359863
		  0, // EEn = 0
		};

		enum class BandParamOffsets : uint8_t
		{
			Circuit,
			Slope,
			Response,
			Freq,
			Gain,
			Q,
			Enable,
			Count__,
		};

		struct Band
		{
			Band(float* paramCache, ParamIndices baseParamID) :
				mParams(paramCache, baseParamID)
			{
			}

			void RecalcFilters()
			{
				const auto circuit = mParams.GetEnumValue<M7::FilterCircuit>(BandParamOffsets::Circuit);
				const auto slope = mParams.GetEnumValue<M7::FilterSlope>(BandParamOffsets::Slope);
				const auto response = mParams.GetEnumValue<M7::FilterResponse>(BandParamOffsets::Response);
				const auto cutoffHz = mParams.GetFrequency(BandParamOffsets::Freq, M7::gFilterFreqConfig);
				//const auto q = mParams.GetDivCurvedValue(BandParamOffsets::Q, M7::gBiquadFilterQCfg);
        const auto reso01 = M7::Param01{mParams.Get01Value(BandParamOffsets::Q)};
				const auto gain = mParams.GetScaledRealValue(BandParamOffsets::Gain, M7::gEqBandGainMin, M7::gEqBandGainMax, 0);

				for (size_t i = 0; i < 2; ++i) {
					// mFilters[i].SetBiquadParams(
					// 	mParams.GetEnumValue<M7::FilterResponse>(BandParamOffsets::Type),
					// 	mParams.GetFrequency(BandParamOffsets::Freq, M7::gFilterFreqConfig),
					// 	mParams.GetDivCurvedValue(BandParamOffsets::Q, M7::gBiquadFilterQCfg),
					// 	mParams.GetScaledRealValue(BandParamOffsets::Gain, M7::gEqBandGainMin, M7::gEqBandGainMax, 0)
					// );
					mFilters[i].SetParams(
						circuit,
						slope,
						response,
						cutoffHz,
						reso01,
						gain
					);
				}
			}

			M7::ParamAccessor mParams;

			//M7::BiquadFilter mFilters[2];
			M7::FilterNode mFilters[2];
		};

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
		AnalysisStream mInputAnalysis[2];
		AnalysisStream mOutputAnalysis[2];
		SmoothedStereoFFT mInputSpectrumSmoother;   // Input display smoother + analyzer
		SmoothedStereoFFT mOutputSpectrumSmoother;  // Output display smoother + analyzer
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT


		Leveller() :
			Device((int)ParamIndices::NumParams, mParamCache, gLevellerDefaults16)
		{
			// Configure input/output FFT/smoothers
			//mInputSpectrumSmoother.SetFFTSmoothing(0.7f);
			//mInputSpectrumSmoother.SetOverlapFactor(2);
			//mInputSpectrumSmoother.SetPeakHoldTime(60);
			//mInputSpectrumSmoother.SetFalloffRate(1200);
			//mInputSpectrumSmoother.SetFFTUpdateRate(1024, 2);

			//mOutputSpectrumSmoother.SetFFTSmoothing(0.7f);
			//mOutputSpectrumSmoother.SetOverlapFactor(2);
			//mOutputSpectrumSmoother.SetPeakHoldTime(60);
			//mOutputSpectrumSmoother.SetFalloffRate(1200);
			//mOutputSpectrumSmoother.SetFFTUpdateRate(1024, 2);
			
			LoadDefaults();
		}

		virtual void Run(float** inputs, float** outputs, int numSamples) override
		{
			float masterGain = mParams.GetLinearVolume(ParamIndices::OutputVolume, M7::gVolumeCfg12db);
			bool enableDC = mParams.GetBoolValue(ParamIndices::EnableDCFilter);
			
			for (int iSample = 0; iSample < numSamples; iSample++)
			{

				float s1 = inputs[0][iSample];
				float s2 = inputs[1][iSample];

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
				if (IsGuiVisible()) {
					mInputAnalysis[0].WriteSample(inputs[0][iSample]);
					mInputAnalysis[1].WriteSample(inputs[1][iSample]);
					mInputSpectrumSmoother.ProcessSamples(s1, s2);
				}
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

				if (enableDC) {
					s1 = mDCFilters[0].ProcessSample(s1);
					s2 = mDCFilters[1].ProcessSample(s2);
				}

				for (int iBand = 0; iBand < gBandCount; ++iBand)
				{
					auto& b = mBands[iBand];
					if (b.mParams.GetBoolValue(BandParamOffsets::Enable)) {
						s1 = b.mFilters[0].ProcessSample(s1);
						s2 = b.mFilters[1].ProcessSample(s2);
					}
				}

				outputs[0][iSample] = masterGain * s1;
				outputs[1][iSample] = masterGain * s2;
				

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
				if (IsGuiVisible()) {
					mOutputSpectrumSmoother.ProcessSamples(outputs[0][iSample], outputs[1][iSample]);
					mOutputAnalysis[0].WriteSample(outputs[0][iSample]);
					mOutputAnalysis[1].WriteSample(outputs[1][iSample]);
				}
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

			}

		}

		virtual void OnParamsChanged() override
		{
			for (int iBand = 0; iBand < gBandCount; ++iBand)
			{
				auto& b = mBands[iBand];
				b.RecalcFilters();
			}
		}

		float mParamCache[(size_t)ParamIndices::NumParams];
		M7::ParamAccessor mParams{ mParamCache, 0 };

		Band mBands[gBandCount] = {
			{mParamCache, ParamIndices::Band1Circuit /*, 0*/},
			{mParamCache, ParamIndices::Band2Circuit/*, 650 */},
			{mParamCache, ParamIndices::Band3Circuit/*, 2000 */},
			{mParamCache, ParamIndices::Band4Circuit/*, 7000 */},
			{mParamCache, ParamIndices::Band5Circuit/*, 22050 */},
		};

		M7::DCFilter mDCFilters[2];
	};
}

#endif
