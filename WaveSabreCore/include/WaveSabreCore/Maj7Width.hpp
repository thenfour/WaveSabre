#pragma once

#include "Device.h"
#include "Maj7Basic.hpp"
#include "Filters/FilterOnePole.hpp"

// TODO: would be nice to have things like rotation, but it's more code than is worth it.

namespace WaveSabreCore
{
	struct Maj7Width : public Device
	{
		enum class ParamIndices
		{
			Rotation, // placing here simply for future potential use
			Width,
			SideHPFrequency,
			Pan,
			OutputGain,

			NumParams,
		};

		static constexpr float gMaxOutputVolumeDb = 12;

		float mParamCache[(int)ParamIndices::NumParams] = { 0 };

		M7::ScaledRealParam mRotationParam{ mParamCache[(int)ParamIndices::Rotation], -180, 180 };
		M7::FloatN11Param mWidthParam{ mParamCache[(int)ParamIndices::Width] };
		M7::FloatN11Param mPanParam { mParamCache[(int)ParamIndices::Pan] };
		float mKTBacking = 0;
		M7::FrequencyParam mSideHPFrequencyParam{ mParamCache[(int)ParamIndices::SideHPFrequency], mKTBacking, M7::gFilterCenterFrequency, M7::gFilterFrequencyScale };
		M7::VolumeParam mOutputVolume{ mParamCache[(int)ParamIndices::OutputGain], gMaxOutputVolumeDb };

		M7::OnePoleFilter mFilter;

		Maj7Width() :
			Device((int)ParamIndices::NumParams)
		{
			mRotationParam.SetRangedValue(0);
			mWidthParam.SetN11Value(1);
			mPanParam.SetN11Value(0);
			mOutputVolume.SetDecibels(0);
			mSideHPFrequencyParam.mValue.SetParamValue(0);
		}

		virtual void Run(double songPosition, float** inputs, float** outputs, int numSamples) override
		{
			mWidthParam.CacheValue();
			mPanParam.CacheValue();
			mFilter.SetParams(M7::FilterType::HP, mSideHPFrequencyParam.GetFrequency(0, 0), 0, 0);
			float masterLinearGain = mOutputVolume.GetLinearGain();

			for (size_t i = 0; i < (size_t)numSamples; ++i)
			{
				float left = inputs[0][i];
				float right = inputs[1][i];

				// for rotation, and for a stereo visualization, calculate radius & angle.
				// for rotation, add to angle and convert back to cartesian.

				float width = mWidthParam.mCachedVal;
				if (width < 0) {
					float tmp = left;
					left = right;
					right = tmp;
					width = -width;
				}

				float mid;
				float side;
				M7::MSEncode(inputs[0][i], inputs[1][i], &mid, &side);
				side *= width;

				// hp on side channel
				side = mFilter.ProcessSample(side);

				M7::MSDecode(mid, side, &left, &right);
				auto gains = M7::math::PanToFactor(mPanParam.mCachedVal);
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

