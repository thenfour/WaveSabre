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
			LowCutQ, // Helpers::ParamToFrequency(value)
			HighCutFreq, // Helpers::ParamToFrequency(value)
			HighCutQ, // Helpers::ParamToFrequency(value)

			FeedbackLevel, // 0-1
			FeedbackDriveDB,
			Cross, // 0-1

			DryWet, // 0-1

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
	{"DryWet"},\
}

		static_assert((int)Echo::ParamIndices::NumParams == 12, "param count probably changed and this needs to be regenerated.");
		static constexpr int16_t gDefaults16[12] = {
		  6746, // LdlyC = 0.20587199926376342773
		  81, // LdlyF = 0.0024719999637454748154
		  8673, // RdlyC = 0.26467901468276977539
		  81, // RdlyF = 0.0024719999637454748154
		  0, // LCFreq = 0
		  6553, // LCQ = 0.20000000298023223877
		  32767, // HCFreq = 1
		  6553, // HCQ = 0.20000000298023223877
		  16416, // FbLvl = 0.50099998712539672852
		  16422, // FbDrive = 0.50118720531463623047
		  8192, // Cross = 0.25
		  9830, // DryWet = 0.30000001192092895508
		};

		float mParamCache[(int)ParamIndices::NumParams];
		M7::ParamAccessor mParams { mParamCache, 0 };

		DelayBuffer leftBuffer;
		DelayBuffer rightBuffer;

		BiquadFilter mLowCutFilter[2];
		BiquadFilter mHighCutFilter[2];

		Echo()
			: Device((int)ParamIndices::NumParams)
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
			float cross = mParams.Get01Value(ParamIndices::Cross, 0);
			float dryWet = mParams.Get01Value(ParamIndices::DryWet, 0);

			for (int i = 0; i < numSamples; i++)
			{
				float leftInput = inputs[0][i];
				float rightInput = inputs[1][i];

				float leftDelay = mLowCutFilter[0].Next(mHighCutFilter[0].Next(leftBuffer.ReadSample()));
				float rightDelay = mLowCutFilter[1].Next(mHighCutFilter[1].Next(rightBuffer.ReadSample()));

				float leftFeedback = M7::math::tanh((leftInput + M7::math::lerp(leftDelay, rightDelay, cross)) * feedback * feedbackDriveLin) / feedbackDriveLin;
				float rightFeedback = M7::math::tanh((rightInput + M7::math::lerp(rightDelay, leftDelay, cross)) * feedback * feedbackDriveLin) / feedbackDriveLin;

				leftBuffer.WriteSample(leftFeedback);
				rightBuffer.WriteSample(rightFeedback);

				outputs[0][i] = M7::math::lerp(leftInput, leftDelay, dryWet);
				outputs[1][i] = M7::math::lerp(rightInput, rightDelay, dryWet);
			}
		}

		virtual void LoadDefaults() override
		{
			M7::ImportDefaultsArray(std::size(gDefaults16), gDefaults16, mParamCache);
			SetParam(0, mParamCache[0]);
		}

		virtual void SetParam(int index, float value) override
		{
			mParamCache[index] = value;

			for (int i = 0; i < 2; i++)
			{
				mLowCutFilter[i].SetParams(
					BiquadFilterType::Highpass,
					mParams.GetFrequency(ParamIndices::LowCutFreq, -1, M7::gFilterFreqConfig, 0, 0),
					mParams.Get01Value(ParamIndices::LowCutQ, 0),
					0);
				mHighCutFilter[i].SetParams(
					BiquadFilterType::Lowpass,
					mParams.GetFrequency(ParamIndices::HighCutFreq, -1, M7::gFilterFreqConfig, 0, 0),
					mParams.Get01Value(ParamIndices::HighCutQ, 0),
					0);
			}
		}

		virtual float GetParam(int index) const override
		{
			return mParamCache[index];
		}

		// minified
		//virtual void SetChunk(void* data, int size) override
		//{
		//	M7::Deserializer ds{ (const uint8_t*)data };
		//	SetMaj7StyleChunk(ds);
		//}

	};
}

#endif
