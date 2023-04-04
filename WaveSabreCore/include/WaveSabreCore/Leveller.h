#ifndef __WAVESABRECORE_LEVELLER_H__
#define __WAVESABRECORE_LEVELLER_H__

#include <algorithm>

#include <WaveSabreCore/Maj7Basic.hpp>
#include <WaveSabreCore/Maj7ParamAccessor.hpp>
#include "Device.h"
#include "BiquadFilter.h"

namespace WaveSabreCore
{
	//static constexpr float gLevellerVolumeMaxDb = 12;
	static constexpr M7::VolumeParamConfig gLevellerVolumeCfg{ 3.9810717055349722f, 12.0f };
	static constexpr M7::VolumeParamConfig gLevellerBandVolumeCfg{ gLevellerVolumeCfg };
	
	enum class LevellerParamIndices : uint8_t
	{
		MasterVolume,

		LowCutFreq,
		unused1__,
		LowCutQ,

		Peak1Freq,
		Peak1Gain,
		Peak1Q,

		Peak2Freq,
		Peak2Gain,
		Peak2Q,

		Peak3Freq,
		Peak3Gain,
		Peak3Q,

		HighCutFreq,
		unused2__,
		HighCutQ,

		NumParams,
	};
#define LEVELLER_PARAM_VST_NAMES { \
	{"MstVol"},\
	{"HPFreq"},\
	{"_none1"},\
	{"HPQ"},\
	{"P1Freq"},\
	{"P1Gain"},\
	{"P1Q"},\
	{"P2Freq"},\
	{"P2Gain"},\
	{"P2Q"},\
	{"P3Freq"},\
	{"P3Gain"},\
	{"P3Q"},\
	{"LPFreq"},\
	{"_none2"},\
	{"LPQ"},\
}

	enum class LevellerBandParamOffsets : uint8_t
	{
		Freq,
		Gain,
		Q,
		Count,
	};

	struct LevellerBand
	{
		LevellerBand(BiquadFilterType type, float* paramCache, LevellerParamIndices baseParamID, float initialCutoffHz) :
			mParams(paramCache, baseParamID),
			mFilterType(type)//,
			//mFrequency(paramCache[(int)baseParamID + (int)LevellerBandParamOffsets::Freq], mKTBacking, M7::gFilterFreqConfig),
			//mVolume(paramCache[(int)baseParamID + (int)LevellerBandParamOffsets::Gain], gLevellerBandVolumeCfg),
			//mQ(paramCache[(int)baseParamID + (int)LevellerBandParamOffsets::Q])
		{
			mParams.SetDecibels(LevellerBandParamOffsets::Gain, gLevellerBandVolumeCfg, 0);
			mParams.Set01Val(LevellerBandParamOffsets::Q, 0);
			mParams.SetFrequencyAssumingNoKeytracking(LevellerBandParamOffsets::Freq, M7::gFilterFreqConfig, initialCutoffHz);
		}

		void RecalcFilters()
		{
			for (size_t i = 0; i < 2; ++i) {
				mFilters[i].SetParams(
					mFilterType,
					mParams.GetFrequency(LevellerBandParamOffsets::Freq, -1, M7::gFilterFreqConfig, 0, 0),
					mParams.GetWSQValue(LevellerBandParamOffsets::Q),
					mParams.GetDecibels(LevellerBandParamOffsets::Gain, gLevellerBandVolumeCfg)
					//mQ.GetQValue()
					//mVolume.GetDecibels()
				);
			}
		}

		M7::ParamAccessor mParams;
		BiquadFilter mFilters[2];
		//float mKTBacking = 0;

		BiquadFilterType mFilterType;
		//M7::FrequencyParam mFrequency;
		//M7::VolumeParam mVolume;
		//M7::WSQParam mQ;
	};

	struct Leveller : public Device
	{
		static constexpr size_t gBandCount = 5;

		Leveller() :
			Device((int)LevellerParamIndices::NumParams)
		{
			mParams.SetDecibels(LevellerParamIndices::MasterVolume, gLevellerVolumeCfg, 0);
			//mMasterVolume.SetDecibels(0);
		}

		virtual void Run(double songPosition, float** inputs, float** outputs, int numSamples) override
		{
			auto recalcMask = M7::GetModulationRecalcSampleMask();
			float masterGain = mParams.GetLinearVolume(LevellerParamIndices::MasterVolume, gLevellerVolumeCfg);
			//float masterGain = mMasterVolume.GetLinearGain();
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
					s1 = b.mFilters[0].Next(s1);
					s2 = b.mFilters[1].Next(s2);
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

	private:
		float mParamCache[(size_t)LevellerParamIndices::NumParams] = { 0 };
		M7::ParamAccessor mParams{ mParamCache, 0 };

		//M7::VolumeParam mMasterVolume{ mParamCache[(int)LevellerParamIndices::MasterVolume], gLevellerVolumeCfg};

		LevellerBand mBands[gBandCount] = {
			{BiquadFilterType::Highpass, mParamCache, LevellerParamIndices::LowCutFreq, 0 },
			{BiquadFilterType::Peak, mParamCache, LevellerParamIndices::Peak1Freq, 650 },
			{BiquadFilterType::Peak, mParamCache, LevellerParamIndices::Peak2Freq, 2000 },
			{BiquadFilterType::Peak, mParamCache, LevellerParamIndices::Peak3Freq, 7000 },
			{BiquadFilterType::Lowpass, mParamCache, LevellerParamIndices::HighCutFreq, 22050 },
		};
	};
}

#endif
