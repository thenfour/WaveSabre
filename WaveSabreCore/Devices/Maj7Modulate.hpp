#pragma once
#include "../WSCore/Device.h"

/*

underlying we should make an audio buffer that can also replace our existing
- comb
- allpass
- delay line
and aim to support...

------------
scenarios to support:
- tremolo (lfo modulating gain)
- vibrato (lfo modulating delay time)
- chorus (multiple delay mods with feedback)
- flanger (delay mod with feedback)
- phaser (lfo modulating all-pass frequencies) -- this may not be necessary for v1 because it's a lot of infra for a rare effect.

------------
design:

- N voices (8) which supports
	- gain
	- delay
	- feedback
	- allpass
	- each of which supports LFO modulation
- probably some amount of macro control like
	- per-voice param, a variation amt.
- x[n] -> voice -> v[n] -> mixed -> y[n]
   - feedback (-1,+1) mixes y[n] into x[n+1]

------------
params:
- pre gain / hpf/dc blocker
- dry mix
- wet mix
- interpolation mode (nearest, linear, cubic)

- each of N voices:
   - enabled
   - polarity
   - base gain / pan
   - base delay
   - feedback
      - gain
	  - flip polarity
	  - ping-pong
   - lfo:
      - waveform
	  - shapeA
	  - shapeB
      - rate hz
	  - -> delay time
	  - -> gain
	- phase warp
		- stages 0-8 ap filters in series.
		- slope
		- response could be experimental
		- cutoff center hz
		- cutoff spread hz
		- jitter - lerping each tap frequency towards something off-grid

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
