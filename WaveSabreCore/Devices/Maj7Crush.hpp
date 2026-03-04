#pragma once

#include "../WSCore/Device.h"

/*
design goals:
- continuous control over most params (bits especially)

SAMPLE CRUSHING
- Sampleratecrush Frequency
- interpolation (off, linear, cubic)
- bandlimiting LP filter (off, on)
- jitter (randomize sample position a little bit to reduce aliasing)

BIT CRUSHING
- Gate (off -> thresh)
- bits (continuous 1-24)
- distribution. few bits = fewer discrete Y values. those values can be distributed in different ways. options:
	- uniform distribution (even distribution of discrete Y values, but 0 may not be available. for example 1-bit audio only has +1 and -1. 2-bit gives -1, -.5, +.5, +1)
	- log distribution. Y values are distributed more like decibel values.
- stereo mode (MS or LR).
- stereo width (0 - full)

- wet/dry/diff if possible.

*/

namespace WaveSabreCore
{
	struct Maj7Crush : public Device
	{
		enum class ParamIndices
		{
            OutputVolume,
			NumParams,
		};

#define MAJ7CRUSH_PARAM_VST_NAMES(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::Maj7Crush::ParamIndices::NumParams]{ \
	{"OutputVolume"},\
}

		static_assert((int)Maj7Crush::ParamIndices::NumParams == 1, "param count probably changed and this needs to be regenerated.");
		static constexpr int16_t gDefaults16[(int)Maj7Crush::ParamIndices::NumParams] = {
		  5684, // OutputVolume = 0.17346939444541931152
		};

		float mParamCache[(int)ParamIndices::NumParams];
		M7::ParamAccessor mParams { mParamCache, 0 };

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
		AnalysisStream mInputAnalysis[2];
		AnalysisStream mOutputAnalysis[2];
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

		Maj7Crush()
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
