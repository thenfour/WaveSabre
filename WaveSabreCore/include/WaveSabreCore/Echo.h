#ifndef __WAVESABRECORE_ECHO_H__
#define __WAVESABRECORE_ECHO_H__

#include "Device.h"
#include "DelayBuffer.h"
#include "Maj7Basic.hpp"
#include "Maj7Filter.hpp"
#include "BiquadFilter.h"

namespace WaveSabreCore
{
	struct Echo : public Device
	{
		static constexpr M7::IntParamConfig gDelayCoarseCfg{ 0, 16 };
		static constexpr M7::IntParamConfig gDelayFineCfg{ 0, 200 };

		enum class ParamIndices
		{
			LeftDelayCoarse, // 0-1 => 0-16 inclusive integral
			LeftDelayFine, // 0-1 => 0-200 inclusive integral

			RightDelayCoarse,// 0-1 => 0-16 inclusive integral
			RightDelayFine,// 0-1 => 0-200 inclusive integral

			LowCutFreq, // Helpers::ParamToFrequency(value)
			LowCutQ,
			HighCutFreq, // Helpers::ParamToFrequency(value)
			HighCutQ,

			FeedbackLevel, // 0-1
			FeedbackDriveDB,
			Cross, // 0-1

			DryOutput, // 0-1
			WetOutput, // 0-1

			NumParams,
		};

#define ECHO_PARAM_VST_NAMES(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::Echo::ParamIndices::NumParams]{ \
	{"LdlyC"},\
	{"LdlyF"},\
	{"RdlyC"},\
	{"RdlyF"},\
	{"LCFreq"},\
	{"LCQ"},\
	{"HCFreq"},\
	{"HCQ"},\
	{"FbLvl"},\
	{"FbDrive"},\
	{"Cross"},\
	{"DryOut"},\
	{"WetOut"},\
}

		static_assert((int)Echo::ParamIndices::NumParams == 13, "param count probably changed and this needs to be regenerated.");
		static constexpr int16_t gDefaults16[13] = {
		  6746, // LdlyC = 0.20587199926376342773
		  81, // LdlyF = 0.0024719999637454748154
		  8673, // RdlyC = 0.26470589637756347656
		  81, // RdlyF = 0.0024719999637454748154
		  2221, // LCFreq = 0.06780719757080078125
		  6553, // LCQ = 0.20000000298023223877
		  26500, // HCFreq = 0.80874627828598022461
		  6553, // HCQ = 0.1999820023775100708
		  17396, // FbLvl = 0.5308844447135925293
		  16422, // FbDrive = 0.50118720531463623047
		  8192, // Cross = 0.25
		  16422, // DryOut = 0.50118720531463623047
		  11626, // WetOut = 0.35481339693069458008
		};

		float mParamCache[(int)ParamIndices::NumParams];
		M7::ParamAccessor mParams { mParamCache, 0 };

		DelayBuffer leftBuffer;
		DelayBuffer rightBuffer;

		BiquadFilter mLowCutFilter[2];
		BiquadFilter mHighCutFilter[2];

		Echo()
			: Device((int)ParamIndices::NumParams, mParamCache, gDefaults16)
		{
			LoadDefaults();
		}

		virtual void Run(double songPosition, float** inputs, float** outputs, int numSamples) override
		{
			float delayScalar = 120.0f / Helpers::CurrentTempo / 8.0f * 1000.0f;
			float leftBufferLengthMs = mParams.GetIntValue(ParamIndices::LeftDelayCoarse, gDelayCoarseCfg) * delayScalar + mParams.GetIntValue(ParamIndices::LeftDelayFine, gDelayFineCfg);
			float rightBufferLengthMs = mParams.GetIntValue(ParamIndices::RightDelayCoarse, gDelayCoarseCfg) * delayScalar + mParams.GetIntValue(ParamIndices::RightDelayFine, gDelayFineCfg);
			leftBuffer.SetLength(leftBufferLengthMs);
			rightBuffer.SetLength(rightBufferLengthMs);

			float feedback = mParams.GetLinearVolume(ParamIndices::FeedbackLevel, M7::gVolumeCfg6db, 0);
			float feedbackDriveLin = std::max(0.01f, mParams.GetLinearVolume(ParamIndices::FeedbackDriveDB, M7::gVolumeCfg12db, 0));
			float cross = mParams.Get01Value(ParamIndices::Cross);
			//float dryWet = mParams.Get01Value(ParamIndices::DryWet);

			float dryMul = mParams.GetLinearVolume(ParamIndices::DryOutput, M7::gVolumeCfg12db);
			float wetMul = mParams.GetLinearVolume(ParamIndices::WetOutput, M7::gVolumeCfg12db);

			for (int i = 0; i < numSamples; i++)
			{
				float leftInput = inputs[0][i];
				float rightInput = inputs[1][i];

				float leftDelay = mLowCutFilter[0].ProcessSample(mHighCutFilter[0].ProcessSample(leftBuffer.ReadSample()));
				leftDelay = M7::math::tanh(leftDelay * feedbackDriveLin) / feedbackDriveLin;
				float rightDelay = mLowCutFilter[1].ProcessSample(mHighCutFilter[1].ProcessSample(rightBuffer.ReadSample()));
				rightDelay = M7::math::tanh(rightDelay * feedbackDriveLin) / feedbackDriveLin;

				float leftFeedback = (leftInput + M7::math::lerp(leftDelay, rightDelay, cross)) * feedback;
				float rightFeedback = (rightInput + M7::math::lerp(rightDelay, leftDelay, cross)) * feedback;

				leftBuffer.WriteSample(leftFeedback);
				rightBuffer.WriteSample(rightFeedback);

				outputs[0][i] = leftInput * dryMul + leftDelay * wetMul;// M7::math::lerp(leftInput, leftDelay, dryWet);
				outputs[1][i] = rightInput * dryMul + rightDelay * wetMul; //M7::math::lerp(rightInput, rightDelay, dryWet);
			}
		}

		virtual void SetParam(int index, float value) override
		{
			mParamCache[index] = value;

			for (int i = 0; i < 2; i++)
			{
				mLowCutFilter[i].SetParams(
					BiquadFilterType::Highpass,
					mParams.GetFrequency(ParamIndices::LowCutFreq, M7::gFilterFreqConfig),
					mParams.Get01Value(ParamIndices::LowCutQ, 0),
					0);
				mHighCutFilter[i].SetParams(
					BiquadFilterType::Lowpass,
					mParams.GetFrequency(ParamIndices::HighCutFreq, M7::gFilterFreqConfig),
					mParams.Get01Value(ParamIndices::HighCutQ, 0),
					0);
			}
		}

	};
}

#endif
