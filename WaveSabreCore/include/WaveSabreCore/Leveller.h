// note that these are NOT multiband + gains + join. these are 5 cascaded biquad filters; each perform independent peaking.

#ifndef __WAVESABRECORE_LEVELLER_H__
#define __WAVESABRECORE_LEVELLER_H__

#include <algorithm>

#include <WaveSabreCore/Maj7Basic.hpp>
#include <WaveSabreCore/Maj7ParamAccessor.hpp>
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
		static constexpr int16_t gLevellerDefaults16[27] = {
		  16422, // OutpVol = 0.50118720531463623047
		  0,// dc enable
		  72, // AType = 0.0022233200725167989731
		  8144, // AFreq = 0.24853515625
		  16422, // AGain = 0.50115966796875
		  6553, // AQ = 0.20000000298023223877
		  0, // AEn = 0
		  40, // BType = 0.001235177856869995594
		  9830, // BFreq = 0.30000001192092895508
		  16422, // BGain = 0.50115966796875
		  6553, // BQ = 0.20000000298023223877
		  0, // BEn = 0
		  40, // CType = 0.001235177856869995594
		  16834, // CFreq = 0.51375037431716918945
		  16422, // CGain = 0.50115966796875
		  6553, // CQ = 0.20000000298023223877
		  0, // CEn = 0
		  40, // DType = 0.001235177856869995594
		  21577, // DFreq = 0.65849626064300537109
		  16422, // DGain = 0.50115966796875
		  6553, // DQ = 0.20000000298023223877
		  0, // DEn = 0
		  56, // EType = 0.001729249022901058197
		  26500, // EFreq = 0.80874627828598022461
		  16422, // EGain = 0.50115966796875
		  6553, // EQ = 0.20000000298023223877
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

		Leveller() :
			Device((int)ParamIndices::NumParams, mParamCache, gLevellerDefaults16)
		{
			LoadDefaults();
		}

		virtual void Run(double songPosition, float** inputs, float** outputs, int numSamples) override
		{
			// you may be tempted to "size optimize" this by looping over 0,1 channel to eliminate all the double calls.
			// it doesn't help i assure you. this code is more compressible.
			//auto recalcMask = M7::GetModulationRecalcSampleMask();
			float masterGain = mParams.GetLinearVolume(ParamIndices::OutputVolume, M7::gVolumeCfg12db);
			bool enableDC = mParams.GetBoolValue(ParamIndices::EnableDCFilter);
			for (int iSample = 0; iSample < numSamples; iSample++)
			{
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
				mInputAnalysis[0].WriteSample(inputs[0][iSample]);
				mInputAnalysis[1].WriteSample(inputs[1][iSample]);
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

				float s1 = inputs[0][iSample];
				float s2 = inputs[1][iSample];

				if (enableDC) {
					s1 = mDCFilters->ProcessSample(s1);
					s2 = mDCFilters->ProcessSample(s2);
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
