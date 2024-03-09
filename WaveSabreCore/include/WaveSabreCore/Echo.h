#ifndef __WAVESABRECORE_ECHO_H__
#define __WAVESABRECORE_ECHO_H__

#include "Device.h"
#include "DelayBuffer.h"
#include "Maj7Basic.hpp"
#include "Maj7Filter.hpp"
#include "BiquadFilter.h"
#include "RMS.hpp"

namespace WaveSabreCore
{
	struct Echo : public Device
	{
		static constexpr M7::IntParamConfig gDelayCoarseCfg{ 0, 48 };
		//static constexpr M7::IntParamConfig gDelayFineCfg{ 0, 200 };

		enum class ParamIndices
		{
			LeftDelayCoarse, // in 8ths range of 4 beats, 0-4*8
			LeftDelayFine, // -1 to 1 8th
			LeftDelayMS, // -200,200 ms

			RightDelayCoarse,// 0-1 => 0-16 inclusive integral
			RightDelayFine,
			RightDelayMS,

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
	{"LdlyMS"},\
	{"RdlyC"},\
	{"RdlyF"},\
	{"RdlyMS"},\
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

		static_assert((int)Echo::ParamIndices::NumParams == 15, "param count probably changed and this needs to be regenerated.");
		static constexpr int16_t gDefaults16[(int)Echo::ParamIndices::NumParams] = {
		  5684, // LdlyC = 0.17346939444541931152
		  16384, // LdlyF = 0.5
		  16384, // LdlyMS = 0.5
		  4346, // RdlyC = 0.13265305757522583008
		  16384, // RdlyF = 0.5
		  16384, // RdlyMS = 0.5
		  2221, // LCFreq = 0.06780719757080078125
		  6553, // LCQ = 0.20000000298023223877
		  26500, // HCFreq = 0.80874627828598022461
		  6553, // HCQ = 0.20000000298023223877
		  9782, // FbLvl = 0.29853826761245727539
		  19518, // FbDrive = 0.59566217660903930664
		  8192, // Cross = 0.25
		  16422, // DryOut = 0.50118720531463623047
		  8230, // WetOut = 0.25118863582611083984
		};

		float mParamCache[(int)ParamIndices::NumParams];
		M7::ParamAccessor mParams { mParamCache, 0 };

		DelayBuffer mBuffers[2];
		BiquadFilter mLowCutFilter[2];
		BiquadFilter mHighCutFilter[2];

		float mFeedbackLin;
		float mFeedbackDriveLin;

		float mDryLin;
		float mWetLin;
		float mCrossMix;


#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
		AnalysisStream mInputAnalysis[2];
		AnalysisStream mOutputAnalysis[2];
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

		Echo()
			: Device((int)ParamIndices::NumParams, mParamCache, gDefaults16)
		{
			LoadDefaults();
		}

		virtual void Run(double songPosition, float** inputs, float** outputs, int numSamples) override
		{
			for (int i = 0; i < numSamples; i++)
			{
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
				mInputAnalysis[0].WriteSample(inputs[0][i]);
				mInputAnalysis[1].WriteSample(inputs[1][i]);
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

				// read buffer, apply processing (filtering, cross mix, drive)
				M7::FloatPair delayBufferSignal = { mBuffers[0].ReadSample() ,mBuffers[1].ReadSample() };

				delayBufferSignal[0] = mHighCutFilter[0].ProcessSample(delayBufferSignal[0]);
				delayBufferSignal[1] = mHighCutFilter[1].ProcessSample(delayBufferSignal[1]);

				delayBufferSignal[0] = mLowCutFilter[0].ProcessSample(delayBufferSignal[0]);
				delayBufferSignal[1] = mLowCutFilter[1].ProcessSample(delayBufferSignal[1]);

				// apply drive
				delayBufferSignal[0] = M7::math::tanh(delayBufferSignal[0] * mFeedbackDriveLin);
				delayBufferSignal[1] = M7::math::tanh(delayBufferSignal[1] * mFeedbackDriveLin);

				// cross mix
				delayBufferSignal = {
					M7::math::lerp(delayBufferSignal[0], delayBufferSignal[1], mCrossMix),
					M7::math::lerp(delayBufferSignal[1], delayBufferSignal[0], mCrossMix)
				};

				M7::FloatPair dry{ inputs[0][i], inputs[1][i] };

				mBuffers[0].WriteSample(dry[0] + delayBufferSignal[0] * mFeedbackLin);
				mBuffers[1].WriteSample(dry[1] + delayBufferSignal[1] * mFeedbackLin);

				outputs[0][i] = dry[0] * mDryLin + delayBufferSignal[0] * mWetLin;
				outputs[1][i] = dry[1] * mDryLin + delayBufferSignal[1] * mWetLin;

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
				mOutputAnalysis[0].WriteSample(outputs[0][i]);
				mOutputAnalysis[1].WriteSample(outputs[1][i]);
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT
			}
		}

		float CalcDelayMS(int coarse, float fine, float msParam) {
			// 60000/bpm = milliseconds per beat. but we are going to be in 8 divisions per beat.
			// 60000/8 = 7500
			float eighths = fine + float(coarse);
			float ms = 7500.0f / Helpers::CurrentTempo * eighths;

			ms += msParam;
			return std::max(0.0f, ms);
		}

		virtual void SetParam(int index, float value) override
		{
			mParamCache[index] = value;
			mCrossMix = mParams.Get01Value(ParamIndices::Cross);
			mFeedbackLin = mParams.GetLinearVolume(ParamIndices::FeedbackLevel, M7::gVolumeCfg6db, 0);
			mFeedbackDriveLin = mParams.GetLinearVolume(ParamIndices::FeedbackDriveDB, M7::gVolumeCfg12db, 0);
			mDryLin = mParams.GetLinearVolume(ParamIndices::DryOutput, M7::gVolumeCfg12db);
			mWetLin = mParams.GetLinearVolume(ParamIndices::WetOutput, M7::gVolumeCfg12db);

			float leftBufferLengthMs = CalcDelayMS(
				mParams.GetIntValue(ParamIndices::LeftDelayCoarse, gDelayCoarseCfg),
				mParams.GetN11Value(ParamIndices::LeftDelayFine, 0),
				mParams.GetScaledRealValue(ParamIndices::LeftDelayMS, -200, 200, 0)
			);
			mBuffers[0].SetLength(leftBufferLengthMs);

			float rightBufferLengthMs = CalcDelayMS(
				mParams.GetIntValue(ParamIndices::RightDelayCoarse, gDelayCoarseCfg),
				mParams.GetN11Value(ParamIndices::RightDelayFine, 0),
				mParams.GetScaledRealValue(ParamIndices::RightDelayMS, -200, 200, 0)
			);
			mBuffers[1].SetLength(rightBufferLengthMs);

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
