#pragma once

#include "Device.h"
#include "Maj7Basic.hpp"
#include "Filters/FilterOnePole.hpp"
#include "RMS.hpp"

//#define MAJ7WIDTH_FULL_FEATURE

namespace WaveSabreCore
{
	struct Maj7Width : public Device
	{
		// roughly in order of processing,
		enum class ParamIndices
		{
			LeftSource, // 0 = left, 1 = right
			RightSource, // 0 = left, 1 = right
			RotationAngle,
			SideHPFrequency,
			MidSideBalance,
			Pan,
			OutputGain,

			NumParams,
		};

		// NB: max 8 chars per string.
#define MAJ7WIDTH_PARAM_VST_NAMES(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::Maj7Width::ParamIndices::NumParams]{ \
	{"LSrc"},\
	{"RSrc"},\
	{"Rot"},\
	{"SideHPF"},\
	{"MSBal"},\
	{"Pan"},\
	{"OutGain"},\
}

		static constexpr M7::VolumeParamConfig gVolumeCfg{ 3.9810717055349722f, 12.0f };

		float mParamCache[(int)ParamIndices::NumParams];
		M7::ParamAccessor mParams;

		static_assert((int)ParamIndices::NumParams == 7, "param count probably changed and this needs to be regenerated.");
		static constexpr int16_t gParamDefaults[7] = {
		  0, // LSrc = 0
		  32767, // RSrc = 1
		  16384, // Rot = 0.5
		  0, // SideHPF = 0
		  16384, // MSBal = 0.5
		  16384, // Pan = 0.5
		  16422, // OutGain = 0.50118720531463623047
		};

		M7::OnePoleFilter mFilter;

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
		AnalysisStream mInputAnalysis[2];
		AnalysisStream mOutputAnalysis[2];
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

		Maj7Width() :
			Device((int)ParamIndices::NumParams, mParamCache, gParamDefaults),
			mParams(mParamCache, 0)
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
			,
			// quite fast moving here because the user's looking for transients.
			mInputAnalysis{ AnalysisStream{1200}, AnalysisStream{1200} },
			mOutputAnalysis{ AnalysisStream{1200}, AnalysisStream{1200} }
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT
		{
			LoadDefaults();
		}

#ifdef MAJ7WIDTH_FULL_FEATURE
		// 2D rotation around origin
		void Rotate2(float& l, float& r, ParamIndices param) {
			// https://www.musicdsp.org/en/latest/Effects/255-stereo-field-rotation-via-transformation-matrix.html
			//r = rotation_angle
			float rot = mParams.GetScaledRealValue(param, -M7::math::gPIHalf, M7::math::gPIHalf, 0);
			float s = M7::math::sin(rot);
			float c = M7::math::cos(rot);
			float t = l * c - r * s;
			r = l * s + r * c;
			l = t;
		}
#endif // MAJ7WIDTH_FULL_FEATURE

		virtual void Run(double songPosition, float** inputs, float** outputs, int numSamples) override
		{
			auto gains = M7::math::PanToFactor(mParams.GetN11Value(ParamIndices::Pan, 0));
			mFilter.SetParams(M7::FilterType::HP, mParams.GetFrequency(ParamIndices::SideHPFrequency, M7::gFilterFreqConfig), 0);
			float masterLinearGain = mParams.GetLinearVolume(ParamIndices::OutputGain, gVolumeCfg) * M7::math::gPanCompensationGainLin;

			for (size_t i = 0; i < (size_t)numSamples; ++i)
			{
				float l = inputs[0][i];
				float r = inputs[1][i];

				float left = M7::math::lerp(l, r, mParamCache[(size_t)ParamIndices::LeftSource]);
				float right = M7::math::lerp(l, r, mParamCache[(size_t)ParamIndices::RightSource]);

#ifdef MAJ7WIDTH_FULL_FEATURE
				Rotate2(left, right, ParamIndices::RotationAngle);
#endif // MAJ7WIDTH_FULL_FEATURE

				float mid;
				float side;
				M7::MSEncode(left, right, &mid, &side);

				// hp on side channel
				side = mFilter.ProcessSample(side);
				float msbal = mParams.GetN11Value(ParamIndices::MidSideBalance, 0);
				if (msbal < 0) {
					// reduce side
					side *= msbal + 1.0f;
				}
				if (msbal > 0) {
					mid *= 1.0f - msbal;
				}

				M7::MSDecode(mid, side, &left, &right);
				left *= gains.x[0] * masterLinearGain;
				outputs[0][i] = left;
				right *= gains.x[1] * masterLinearGain;
				outputs[1][i] = right;


#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
				mInputAnalysis[0].WriteSample(inputs[0][i]);
				mInputAnalysis[1].WriteSample(inputs[1][i]);
				mOutputAnalysis[0].WriteSample(outputs[0][i]);
				mOutputAnalysis[1].WriteSample(outputs[1][i]);
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

			}
		}
	};
}

