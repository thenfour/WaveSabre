#pragma once

#include "Device.h"
#include "Maj7Basic.hpp"
#include "Filters/FilterOnePole.hpp"

namespace WaveSabreCore
{
	struct Maj7Width : public Device
	{
		enum class ParamIndices
		{
			// in order of processing,
			LeftSource, // 0 = left, 1 = right
			RightSource, // 0 = left, 1 = right
			RotationAngle,
			SideHPFrequency,
			//MidAmt,
			//SideAmt,
			MidSideBalance,
			Pan,
			OutputGain,

			NumParams,
		};

		static constexpr M7::VolumeParamConfig gVolumeCfg{ 3.9810717055349722f, 12.0f };

		float mParamCache[(int)ParamIndices::NumParams] = {
			0, // left source
			1, // right source
			0.5f, // RotationAngle
			0, // Side HP
			0.5f, // mid side balance
			0.5f, // pan
			0.5f, // output gain
		};
		M7::ParamAccessor mParams;

		M7::OnePoleFilter mFilter;

		Maj7Width() :
			Device((int)ParamIndices::NumParams),
			mParams(mParamCache, 0)
		{
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
				//side *= mParamCache[(size_t)ParamIndices::SideAmt];
				//mid *= mParamCache[(size_t)ParamIndices::MidAmt];

				M7::MSDecode(mid, side, &left, &right);
				left *= gains.first * masterLinearGain;
				outputs[0][i] = left;
				right *= gains.second * masterLinearGain;
				outputs[1][i] = right;
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

	};
}

