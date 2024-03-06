#pragma once

#include "Device.h"
#include "Maj7Basic.hpp"
#include "Filters/FilterOnePole.hpp"
#include "RMS.hpp"

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
			0, // left source
			32767, // right source
			16383, // RotationAngle
			0, // Side HP
			16383, // mid side balance
			16383, // pan
			16383, // output gain
		};

		M7::OnePoleFilter mFilter;

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
		AnalysisStream mInputAnalysis[2];
		AnalysisStream mOutputAnalysis[2];
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

		Maj7Width() :
			Device((int)ParamIndices::NumParams),
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

		virtual void Run(double songPosition, float** inputs, float** outputs, int numSamples) override
		{
			//float width = mParams.Get01Value(ParamIndices::Width, 0);
			auto gains = M7::math::PanToFactor(mParams.GetN11Value(ParamIndices::Pan, 0));
			mFilter.SetParams(M7::FilterType::HP, mParams.GetFrequency(ParamIndices::SideHPFrequency, -1, M7::gFilterFreqConfig, 0, 0), 0);
			float masterLinearGain = mParams.GetLinearVolume(ParamIndices::OutputGain, gVolumeCfg);

			for (size_t i = 0; i < (size_t)numSamples; ++i)
			{
				float l = inputs[0][i];
				float r = inputs[1][i];

				float left = M7::math::lerp(l, r, mParamCache[(size_t)ParamIndices::LeftSource]);
				float right = M7::math::lerp(l, r, mParamCache[(size_t)ParamIndices::RightSource]);

				Rotate2(left, right, ParamIndices::RotationAngle);

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
				left *= gains.first * masterLinearGain;
				outputs[0][i] = left;
				right *= gains.second * masterLinearGain;
				outputs[1][i] = right;


#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
				mInputAnalysis[0].WriteSample(inputs[0][i]);
				mInputAnalysis[1].WriteSample(inputs[1][i]);
				mOutputAnalysis[0].WriteSample(outputs[0][i]);
				mOutputAnalysis[1].WriteSample(outputs[1][i]);
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

			}
		}
		virtual void LoadDefaults() override {
			cc::log("Width::LoadDefaults 1, importing %d", std::size(gParamDefaults));

			M7::ImportDefaultsArray(std::size(gParamDefaults), gParamDefaults, mParamCache);

			cc::log("Width::LoadDefaults 2");

			SetParam(0, mParamCache[0]); // force recalcing

			cc::log("Width::LoadDefaults 3");
		}

		virtual void SetParam(int index, float value) override
		{
			mParamCache[index] = value;
		}

		virtual float GetParam(int index) const override
		{
			return mParamCache[index];
		}

	};
}

