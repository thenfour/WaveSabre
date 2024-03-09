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
		static constexpr int16_t gDefaults16[(int)Echo::ParamIndices::NumParams] = {
		  6746, // LdlyC = 0.20587199926376342773
		  81, // LdlyF = 0.0024719999637454748154
		  8673, // RdlyC = 0.26470589637756347656
		  81, // RdlyF = 0.0024875621311366558075
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

		virtual void SetParam(int index, float value) override
		{
			mParamCache[index] = value;
			mCrossMix = mParams.Get01Value(ParamIndices::Cross);

			float delayScalar = 120.0f / Helpers::CurrentTempo / 8.0f * 1000.0f;
			float leftBufferLengthMs = mParams.GetIntValue(ParamIndices::LeftDelayCoarse, gDelayCoarseCfg) * delayScalar + mParams.GetIntValue(ParamIndices::LeftDelayFine, gDelayFineCfg);
			mBuffers[0].SetLength(leftBufferLengthMs);
			float rightBufferLengthMs = mParams.GetIntValue(ParamIndices::RightDelayCoarse, gDelayCoarseCfg) * delayScalar + mParams.GetIntValue(ParamIndices::RightDelayFine, gDelayFineCfg);
			mBuffers[1].SetLength(leftBufferLengthMs);

			mFeedbackLin = mParams.GetLinearVolume(ParamIndices::FeedbackLevel, M7::gVolumeCfg6db, 0);
			mFeedbackDriveLin = mParams.GetLinearVolume(ParamIndices::FeedbackDriveDB, M7::gVolumeCfg12db, 0);
			mDryLin = mParams.GetLinearVolume(ParamIndices::DryOutput, M7::gVolumeCfg12db);
			mWetLin = mParams.GetLinearVolume(ParamIndices::WetOutput, M7::gVolumeCfg12db);

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
