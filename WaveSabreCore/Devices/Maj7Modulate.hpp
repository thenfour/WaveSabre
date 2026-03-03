#pragma once
#include "../WSCore/Device.h"

/*
- N voices which are either
   - time-warped (delay mod)
   - phase-warped (all-pass)
- pre gain / hpf/dc blocker
- x[n] -> voice -> v[n] -> mixed -> y[n]
   - feedback (-1,+1) mixes y[n] into x[n+1]
- dry mix
- wet mix
- voices
   - enabled
   - gain
   - pan
   - polarity
   - feedback
   - modulation:
      - lfo shape
      - rate hz
      - depth
      - phase offset (Stereo spread?)
- delay warp voice
   - base delay 0-30ms-ish
   - mix mode ?
   - interpolation type
   - feedback send
- phase warp voice
   - stages 2,4,6,8. these are all-passed in series (8 stages = 8 all-pass filters in series)
      - why always in pairs?
   - center hz
   - spread hz or octaves
   - jitter - lerping each tap frequency towards something off-grid
      - and stereo spread

*/

namespace WaveSabreCore
{
	struct Maj7Modulate : public Device
	{
		enum class ParamIndices
		{
            OutputVolume,
			NumParams,
		};

#define MAJ7MODULATE_PARAM_VST_NAMES(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::Maj7Modulate::ParamIndices::NumParams]{ \
	{"OutputVolume"},\
}

		static_assert((int)Maj7Modulate::ParamIndices::NumParams == 1, "param count probably changed and this needs to be regenerated.");
		static constexpr int16_t gDefaults16[(int)Maj7Modulate::ParamIndices::NumParams] = {
		  5684, // OutputVolume = 0.17346939444541931152
		};

		float mParamCache[(int)ParamIndices::NumParams];
		M7::ParamAccessor mParams { mParamCache, 0 };

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
		AnalysisStream mInputAnalysis[2];
		AnalysisStream mOutputAnalysis[2];
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

		Maj7Modulate()
			: Device((int)ParamIndices::NumParams, mParamCache, gDefaults16)
		{
			LoadDefaults();
		}

		virtual void Run(float** inputs, float** outputs, int numSamples) override
		{
			for (int i = 0; i < numSamples; i++)
			{
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
				if (IsGuiVisible())
				{
					mInputAnalysis[0].WriteSample(inputs[0][i]);
					mInputAnalysis[1].WriteSample(inputs[1][i]);
				}
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

                float outputVolume = mParams.GetLinearVolume((int)ParamIndices::OutputVolume, M7::gVolumeCfg24db, 0);
                outputs[0][i] = inputs[0][i] * outputVolume;
                outputs[1][i] = inputs[1][i] * outputVolume;

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
				if (IsGuiVisible())
				{
					mOutputAnalysis[0].WriteSample(outputs[0][i]);
					mOutputAnalysis[1].WriteSample(outputs[1][i]);
				}
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT
			}
		}

		virtual void OnParamsChanged() override
		{
		}

	};
}
