// note that these are NOT multiband + gains + join. these are 5 cascaded biquad filters; each perform independent peaking.

#ifndef __WAVESABRECORE_LEVELLER_H__
#define __WAVESABRECORE_LEVELLER_H__

#include <algorithm>

#include <WaveSabreCore/Maj7Basic.hpp>
#include <WaveSabreCore/Maj7ParamAccessor.hpp>
#include "Device.h"
#include "BiquadFilter.h"

namespace WaveSabreCore
{
	//static constexpr M7::VolumeParamConfig gLevellerVolumeCfg{ 3.9810717055349722f, 12.0f };
	//static constexpr M7::VolumeParamConfig gLevellerBandVolumeCfg{ gLevellerVolumeCfg };
	
	enum class LevellerParamIndices : uint8_t
	{
		MasterVolume,

		LowShelfFreq,
		LowShelfGain,
		LowShelfQ,
		LowShelfEnable,

		Peak1Freq,
		Peak1Gain,
		Peak1Q,
		Peak1Enable,

		Peak2Freq,
		Peak2Gain,
		Peak2Q,
		Peak2Enable,

		Peak3Freq,
		Peak3Gain,
		Peak3Q,
		Peak3Enable,

		HighShelfFreq,
		HighShelfGain,
		HighShelfQ,
		HighShelfEnable,

		NumParams,
	};
#define LEVELLER_PARAM_VST_NAMES(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::LevellerParamIndices::NumParams]{ \
	{"MstVol"},\
	{"LSFreq"},\
	{"LSGain"},\
	{"LSQ"},\
	{"LSEn"},\
	{"P1Freq"},\
	{"P1Gain"},\
	{"P1Q"},\
	{"P1En"},\
	{"P2Freq"},\
	{"P2Gain"},\
	{"P2Q"},\
	{"P2En"},\
	{"P3Freq"},\
	{"P3Gain"},\
	{"P3Q"},\
	{"P3En"},\
	{"HSFreq"},\
	{"HSGain"},\
	{"HSQ"},\
	{"HSEn"},\
}

	static_assert((int)LevellerParamIndices::NumParams == 21, "param count probably changed and this needs to be regenerated.");
	static constexpr int16_t gLevellerDefaults16[21] = {
	  16422, // MstVol = 0.50115966796875
	  8144, // LSFreq = 0.24854269623756408691
	  16422, // LSGain = 0.50115966796875
	  3932, // LSQ = 0.1199951171875
	  0, // LSEn = 0
	  14347, // P1Freq = 0.437835693359375
	  16422, // P1Gain = 0.50115966796875
	  3932, // P1Q = 0.1199951171875
	  0, // P1En = 0
	  19660, // P2Freq = 0.5999755859375
	  16422, // P2Gain = 0.50115966796875
	  3932, // P2Q = 0.1199951171875
	  0, // P2En = 0
	  25583, // P3Freq = 0.780731201171875
	  16422, // P3Gain = 0.50115966796875
	  3932, // P3Q = 0.1199951171875
	  0, // P3En = 0
	  24854, // HSFreq = 0.75849628448486328125
	  16422, // HSGain = 0.50115966796875
	  3932, // HSQ = 0.1199951171875
	  0, // HSEn = 0
	};

	enum class LevellerBandParamOffsets : uint8_t
	{
		Freq,
		Gain,
		Q,
		Enable,
		Count,
	};

	struct LevellerBand
	{
		LevellerBand(BiquadFilterType typeWhenZero, BiquadFilterType typeWhenNonZero, float* paramCache, LevellerParamIndices baseParamID) :
			mParams(paramCache, baseParamID),
			mFilterTypeWhen0(typeWhenZero),
			mFilterType(typeWhenNonZero),
			mIsEnabled(false)
		{
		}

		void RecalcFilters()
		{
			mIsEnabled = mParams.GetBoolValue(LevellerBandParamOffsets::Enable);
			for (size_t i = 0; i < 2; ++i) {
				//float gainParam = ;
				//float db = 
				mFilters[i].SetParams(
					(mParams.GetRawVal(LevellerBandParamOffsets::Gain) <= 0) ? mFilterTypeWhen0 : mFilterType,
					mParams.GetFrequency(LevellerBandParamOffsets::Freq, -1, M7::gFilterFreqConfig, 0, 0),
					mParams.GetWSQValue(LevellerBandParamOffsets::Q),
					mParams.GetDecibels(LevellerBandParamOffsets::Gain, M7::gVolumeCfg12db)
				);
			}
		}

		M7::ParamAccessor mParams;

		BiquadFilterType mFilterTypeWhen0;
		BiquadFilterType mFilterType;
		bool mIsEnabled;

		BiquadFilter mFilters[2];
	};

	struct Leveller : public Device
	{
		static constexpr size_t gBandCount = 5;

		Leveller() :
			Device((int)LevellerParamIndices::NumParams)
		{
			LoadDefaults();
		}

		virtual void LoadDefaults() override
		{
			M7::ImportDefaultsArray(std::size(gLevellerDefaults16), gLevellerDefaults16, mParamCache);
		}

		virtual void Run(double songPosition, float** inputs, float** outputs, int numSamples) override
		{
			//mParams.SetFrequencyAssumingNoKeytracking(LevellerParamIndices::LowShelfFreq, M7::gFilterFreqConfig, 175);
			//mParams.SetFrequencyAssumingNoKeytracking(LevellerParamIndices::HighShelfFreq, M7::gFilterFreqConfig, 6000);

			auto recalcMask = M7::GetModulationRecalcSampleMask();
			float masterGain = mParams.GetLinearVolume(LevellerParamIndices::MasterVolume, M7::gVolumeCfg12db);
			for (int iSample = 0; iSample < numSamples; iSample++)
			{
				float s1 = inputs[0][iSample];
				float s2 = inputs[1][iSample];

				for (int iBand = 0; iBand < gBandCount; ++iBand)
				{
					auto& b = mBands[iBand];
					if ((iSample & recalcMask) == 0) {
						b.RecalcFilters();
					}
					if (b.mIsEnabled) {
						s1 = b.mFilters[0].Next(s1);
						s2 = b.mFilters[1].Next(s2);
					}
				}

				outputs[0][iSample] = masterGain * s1;
				outputs[1][iSample] = masterGain * s2;
			}
		}

		virtual void SetParam(int index, float value) override
		{
			mParamCache[index] = value;
		}

		virtual float GetParam(int index) const override
		{
			return mParamCache[index];
		}

		// minified
		virtual void SetChunk(void* data, int size) override
		{
			M7::Deserializer ds{ (const uint8_t*)data };
			SetMaj7StyleChunk(ds);
		}

		float mParamCache[(size_t)LevellerParamIndices::NumParams];
		M7::ParamAccessor mParams{ mParamCache, 0 };

		LevellerBand mBands[gBandCount] = {
			{BiquadFilterType::Highpass, BiquadFilterType::LowShelf, mParamCache, LevellerParamIndices::LowShelfFreq/*, 0*/},
			{BiquadFilterType::Peak, BiquadFilterType::Peak, mParamCache, LevellerParamIndices::Peak1Freq /*, 650 */},
			{BiquadFilterType::Peak, BiquadFilterType::Peak, mParamCache, LevellerParamIndices::Peak2Freq /*, 2000 */},
			{BiquadFilterType::Peak, BiquadFilterType::Peak, mParamCache, LevellerParamIndices::Peak3Freq /*, 7000 */},
			{BiquadFilterType::Lowpass, BiquadFilterType::HighShelf, mParamCache, LevellerParamIndices::HighShelfFreq /*, 22050 */},
		};
	};
}

#endif
