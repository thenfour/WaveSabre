// note that these are NOT multiband + gains + join. these are 5 cascaded biquad filters; each perform independent peaking.

#ifndef __WAVESABRECORE_LEVELLER_H__
#define __WAVESABRECORE_LEVELLER_H__

#include <algorithm>

#include <WaveSabreCore/Maj7Basic.hpp>
#include <WaveSabreCore/Maj7ParamAccessor.hpp>
#include <WaveSabreCore/FFTAnalysis.hpp>
#include "Device.h"
#include "BiquadFilter.h"
#include "RMS.hpp"
#include "Maj7Filter.hpp"

namespace WaveSabreCore
{
	struct Leveller : public Device
	{
		static constexpr size_t gBandCount = 5;

		enum class ParamIndices : uint8_t
		{
			OutputVolume,
			EnableDCFilter,

			Band1Type,
			Band1Freq,
			Band1Gain,
			Band1Q,
			Band1Enable,

			Band2Type,
			Band2Freq,
			Band2Gain,
			Band2Q,
			Band2Enable,

			Band3Type,
			Band3Freq,
			Band3Gain,
			Band3Q,
			Band3Enable,

			Band4Type,
			Band4Freq,
			Band4Gain,
			Band4Q,
			Band4Enable,

			Band5Type,
			Band5Freq,
			Band5Gain,
			Band5Q,
			Band5Enable,

			NumParams,
		};
#define LEVELLER_PARAM_VST_NAMES(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::Leveller::ParamIndices::NumParams]{ \
	{"OutpVol"},\
	{"DCEn"},\
		{"AType"}, \
		{"AFreq"}, \
		{"AGain"}, \
		{"AQ"}, \
		{"AEn"}, \
		{"BType"}, \
		{"BFreq"}, \
		{"BGain"}, \
		{"BQ"}, \
		{"BEn"}, \
		{"CType"}, \
		{"CFreq"}, \
		{"CGain"}, \
		{"CQ"}, \
		{"CEn"}, \
		{"DType"}, \
		{"DFreq"}, \
		{"DGain"}, \
		{"DQ"}, \
		{"DEn"}, \
		{"EType"}, \
		{"EFreq"}, \
		{"EGain"}, \
		{"EQ"}, \
		{"EEn"}, \
}

		static_assert((int)ParamIndices::NumParams == 27, "param count probably changed and this needs to be regenerated.");
		static constexpr int16_t gLevellerDefaults16[(int)ParamIndices::NumParams] = {
		  16422, // OutpVol = 0.50116002559661865234
		  0, // DCEn = 0
		  71, // AType = 0.0021969999652355909348
		  5000, // AFreq = 0.1526069939136505127
		  16422, // AGain = 0.5011870265007019043
		  14563, // AQ = 0.44444444775581359863
		  0, // AEn = 0
		  40, // BType = 0.0012209999840706586838
		  9830, // BFreq = 0.30000001192092895508
		  16422, // BGain = 0.5011870265007019043
		  14563, // BQ = 0.44444444775581359863
		  0, // BEn = 0
		  40, // CType = 0.0012209999840706586838
		  16834, // CFreq = 0.51375001668930053711
		  16422, // CGain = 0.5011870265007019043
		  14563, // CQ = 0.44444444775581359863
		  0, // CEn = 0
		  40, // DType = 0.0012209999840706586838
		  21577, // DFreq = 0.65849602222442626953
		  16422, // DGain = 0.5011870265007019043
		  14563, // DQ = 0.44444444775581359863
		  0, // DEn = 0
		  56, // EType = 0.0017089999746531248093
		  26500, // EFreq = 0.80874598026275634766
		  16422, // EGain = 0.5011870265007019043
		  14563, // EQ = 0.44444444775581359863
		  0, // EEn = 0
		};

		enum class BandParamOffsets : uint8_t
		{
			Type,
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
				for (size_t i = 0; i < 2; ++i) {
					mFilters[i].SetParams(
						mParams.GetEnumValue<BiquadFilterType>(BandParamOffsets::Type),
						mParams.GetFrequency(BandParamOffsets::Freq, M7::gFilterFreqConfig),
						mParams.GetDivCurvedValue(BandParamOffsets::Q, M7::gBiquadFilterQCfg),
						mParams.GetDecibels(BandParamOffsets::Gain, M7::gVolumeCfg12db)
					);
				}
			}

			M7::ParamAccessor mParams;

			BiquadFilter mFilters[2];
		};

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
		AnalysisStream mInputAnalysis[2];
		AnalysisStream mOutputAnalysis[2];
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

		// FFT analysis for spectrum display
		FFTAnalysis mFFTAnalysis;
		SpectrumDisplaySmoother mSpectrumSmoother;

		Leveller() :
			Device((int)ParamIndices::NumParams, mParamCache, gLevellerDefaults16),
			mFFTAnalysis(FFTAnalysis::FFTSize::FFT1024, FFTAnalysis::WindowType::Hanning, 44100.0f)
		{
			// Configure FFT for clean technical analysis (no artificial boost needed)
			mFFTAnalysis.SetSmoothingFactor(0.0f);  // Light technical smoothing only
			mFFTAnalysis.SetOverlapFactor(2);       // Good balance of smoothness vs CPU
			
			// Configure display smoother for Pro-Q3 style behavior
			mSpectrumSmoother.SetPeakHoldTime(0, 44100.0f);  // 80ms peak hold (reduced from 300ms)
			mSpectrumSmoother.SetFalloffRate(200.0f, 44100.0f);  // 200ms for -60dB falloff (faster than 1.2s)
			
			LoadDefaults();
		}

		virtual void Run(double songPosition, float** inputs, float** outputs, int numSamples) override
		{
			// you may be tempted to "size optimize" this by looping over 0,1 channel to eliminate all the double calls.
			// it doesn't help i assure you. this code is more compressible.
			//auto recalcMask = M7::GetModulationRecalcSampleMask();
			float masterGain = mParams.GetLinearVolume(ParamIndices::OutputVolume, M7::gVolumeCfg12db);
			bool enableDC = mParams.GetBoolValue(ParamIndices::EnableDCFilter);
			
			// Update FFT analysis sample rate if needed
			if (mFFTAnalysis.GetNyquistFrequency() * 2.0f != static_cast<float>(Helpers::CurrentSampleRate))
			{
				mFFTAnalysis.SetSampleRate(static_cast<float>(Helpers::CurrentSampleRate));
				// Update smoother sample rate dependent settings
				mSpectrumSmoother.SetPeakHoldTime(0, static_cast<float>(Helpers::CurrentSampleRate));
				mSpectrumSmoother.SetFalloffRate(200, static_cast<float>(Helpers::CurrentSampleRate));
			}
			
			for (int iSample = 0; iSample < numSamples; iSample++)
			{
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
				mInputAnalysis[0].WriteSample(inputs[0][iSample]);
				mInputAnalysis[1].WriteSample(inputs[1][iSample]);
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

				float s1 = inputs[0][iSample];
				float s2 = inputs[1][iSample];
				
				// Process input samples for FFT analysis (before filtering)
				mFFTAnalysis.ProcessSamples(s1, s2);
				
				// Process FFT data through display smoother when new data is available
				if (mFFTAnalysis.HasNewSpectrum())
				{
					mSpectrumSmoother.ProcessSpectrum(mFFTAnalysis.GetSpectrumLeft(), false);
					mSpectrumSmoother.ProcessSpectrum(mFFTAnalysis.GetSpectrumRight(), true);
					mFFTAnalysis.ConsumeSpectrum(); // Mark as processed
				}

				if (enableDC) {
					s1 = mDCFilters[0].ProcessSample(s1);
					s2 = mDCFilters[1].ProcessSample(s2);
				}

				for (int iBand = 0; iBand < gBandCount; ++iBand)
				{
					auto& b = mBands[iBand];
					//if ((iSample & recalcMask) == 0) {
					//	b.RecalcFilters();
					//}
					if (b.mParams.GetBoolValue(BandParamOffsets::Enable)) {
						s1 = b.mFilters[0].ProcessSample(s1);
						s2 = b.mFilters[1].ProcessSample(s2);
					}
				}

				outputs[0][iSample] = masterGain * s1;
				outputs[1][iSample] = masterGain * s2;

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
				mOutputAnalysis[0].WriteSample(outputs[0][iSample]);
				mOutputAnalysis[1].WriteSample(outputs[1][iSample]);
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
			{mParamCache, ParamIndices::Band1Type /*, 0*/},
			{mParamCache, ParamIndices::Band2Type/*, 650 */},
			{mParamCache, ParamIndices::Band3Type/*, 2000 */},
			{mParamCache, ParamIndices::Band4Type/*, 7000 */},
			{mParamCache, ParamIndices::Band5Type/*, 22050 */},
		};

		M7::DCFilter mDCFilters[2];
	};
}

#endif
