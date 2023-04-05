#ifndef __WAVESABRECORE_LEVELLER_H__
#define __WAVESABRECORE_LEVELLER_H__

#include <algorithm>

#include <WaveSabreCore/Maj7Basic.hpp>
#include <WaveSabreCore/Maj7ParamAccessor.hpp>
#include "Device.h"
#include "BiquadFilter.h"

namespace WaveSabreCore
{
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
#define LEVELLER_PARAM_VST_NAMES(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::LevellerParamIndices::NumParams]{ \
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

	static_assert((int)LevellerParamIndices::NumParams == 16, "param count probably changed and this needs to be regenerated.");
	const int16_t gLevellerDefaults16[16] = {
	  16422, // MstVol = 0.50118720531463623047
	  0, // HPFreq = 0
	  0, // _none1 = 0
	  0, // HPQ = 0
	  14347, // P1Freq = 0.43785116076469421387
	  16422, // P1Gain = 0.50118720531463623047
	  0, // P1Q = 0
	  19660, // P2Freq = 0.60000002384185791016
	  16422, // P2Gain = 0.50118720531463623047
	  0, // P2Q = 0
	  25583, // P3Freq = 0.78073549270629882812
	  16422, // P3Gain = 0.50118720531463623047
	  0, // P3Q = 0
	  32767, // LPFreq = 1
	  0, // _none2 = 0
	  0, // LPQ = 0
	};


	enum class LevellerBandParamOffsets : uint8_t
	{
		Freq,
		Gain,
		Q,
		Count,
	};

	struct LevellerBand
	{
		LevellerBand(BiquadFilterType type, float* paramCache, LevellerParamIndices baseParamID) :
			mParams(paramCache, baseParamID),
			mFilterType(type)
		{
			//mParams.SetDecibels(LevellerBandParamOffsets::Gain, gLevellerBandVolumeCfg, 0);
			//mParams.Set01Val(LevellerBandParamOffsets::Q, 0);
			//mParams.SetFrequencyAssumingNoKeytracking(LevellerBandParamOffsets::Freq, M7::gFilterFreqConfig, initialCutoffHz);
		}

		void RecalcFilters()
		{
			for (size_t i = 0; i < 2; ++i) {
				mFilters[i].SetParams(
					mFilterType,
					mParams.GetFrequency(LevellerBandParamOffsets::Freq, -1, M7::gFilterFreqConfig, 0, 0),
					mParams.GetWSQValue(LevellerBandParamOffsets::Q),
					mParams.GetDecibels(LevellerBandParamOffsets::Gain, gLevellerBandVolumeCfg)
				);
			}
		}

		M7::ParamAccessor mParams;
		BiquadFilter mFilters[2];

		BiquadFilterType mFilterType;
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
			auto recalcMask = M7::GetModulationRecalcSampleMask();
			float masterGain = mParams.GetLinearVolume(LevellerParamIndices::MasterVolume, gLevellerVolumeCfg);
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

		// minified
		virtual void SetChunk(void* data, int size) override
		{
			M7::Deserializer ds{ (const uint8_t*)data };
			SetMaj7StyleChunk(ds);
		}

		float mParamCache[(size_t)LevellerParamIndices::NumParams];
		M7::ParamAccessor mParams{ mParamCache, 0 };

		LevellerBand mBands[gBandCount] = {
			{BiquadFilterType::Highpass, mParamCache, LevellerParamIndices::LowCutFreq/*, 0*/},
			{BiquadFilterType::Peak, mParamCache, LevellerParamIndices::Peak1Freq /*, 650 */},
			{BiquadFilterType::Peak, mParamCache, LevellerParamIndices::Peak2Freq /*, 2000 */},
			{BiquadFilterType::Peak, mParamCache, LevellerParamIndices::Peak3Freq /*, 7000 */},
			{BiquadFilterType::Lowpass, mParamCache, LevellerParamIndices::HighCutFreq /*, 22050 */},
		};
	};
}

#endif
