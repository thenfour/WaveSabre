#pragma once

#include <WaveSabreCore/Maj7Basic.hpp>
#include <WaveSabreCore/BiquadFilter.h>
#include "Maj7ModMatrix.hpp"
// TODO: find a way to only include filters which are actually used in the song.
//#include "Filters/FilterDiode.hpp"
//#include "Filters/FilterK35.hpp"
#include "Filters/FilterMoog.hpp"
#include "Filters/FilterOnePole.hpp"
#include <array>

#define FILTER_MODEL_CAPTIONS { \
    "Disabled", \
    "LP_OnePole", \
    "LP_Moog2 (unsupported)", \
    "LP_Moog4 (unsupported)", \
    "BP_Moog2 (unsupported)", \
    "BP_Moog4 (unsupported)", \
    "HP_OnePole", \
    "HP_Moog2 (unsupported)", \
    "HP_Moog4 (unsupported)", \
    "-"/*"LP_Biquad"*/, \
    "-"/*"HP_Biquad"*/, \
}

namespace WaveSabreCore
{
	namespace M7
	{
		// todo: Diode, K35 are all really nice.
        enum class FilterModel : uint8_t
		{
			Disabled = 0,
			LP_OnePole,
			LP_Moog2,
			LP_Moog4,
			BP_Moog2,
			BP_Moog4,
			HP_OnePole,
			HP_Moog2,
			HP_Moog4,
			LP_Biquad,
			HP_Biquad,
			Count
		};

		struct FilterNode
		{
			NullFilter mNullFilter;
			//DiodeFilter mDiode;
			//K35Filter mK35;
#ifndef DISABLE_MOOG_FILTER
				MoogLadderFilter mMoog;
#endif // DISABLE_MOOG_FILTER
#ifndef DISABLE_onepole_maj7_filter
			OnePoleFilter mOnePole;
#endif // DISABLE_onepole_maj7_filter
			//SEM12Filter mSem12;
			//SVFilterNode mSVF;
#ifndef DISABLE_BIQUAD_MAJ7_FILTER
			BiquadFilter mBiquad;
#endif // DISABLE_BIQUAD_MAJ7_FILTER
			//LinkwitzRileyNode mLR;

			IFilter* mSelectedFilter = &mNullFilter;
            FilterModel mSelectedModel = FilterModel::LP_Moog4;

            void SetParams(FilterModel ctype, float cutoffHz, float reso)
            {
                // select filter & set type
                FilterType ft = FilterType::LP;
                switch (ctype)
                {
				default:
                case FilterModel::Disabled:
                    mSelectedFilter = &mNullFilter;
                    break;
#ifndef DISABLE_onepole_maj7_filter
				case FilterModel::LP_OnePole:
                    ft = FilterType::LP;
                    mSelectedFilter = &mOnePole;
                    break;
#endif // DISABLE_onepole_maj7_filter
#ifndef DISABLE_MOOG_FILTER
				case FilterModel::LP_Moog2:
                    ft = FilterType::LP2;
                    mSelectedFilter = &mMoog;
                    break;
                case FilterModel::LP_Moog4:
                    ft = FilterType::LP4;
                    mSelectedFilter = &mMoog;
                    break;
#endif // DISABLE_MOOG_FILTER
#ifndef DISABLE_onepole_maj7_filter
				case FilterModel::HP_OnePole:
                    ft = FilterType::HP;
                    mSelectedFilter = &mOnePole;
                    break;
#endif // DISABLE_onepole_maj7_filter
#ifndef DISABLE_MOOG_FILTER
				case FilterModel::HP_Moog2:
                    ft = FilterType::HP2;
                    mSelectedFilter = &mMoog;
                    break;
                case FilterModel::HP_Moog4:
                    ft = FilterType::HP4;
                    mSelectedFilter = &mMoog;
                    break;
                case FilterModel::BP_Moog2:
                    ft = FilterType::BP2;
                    mSelectedFilter = &mMoog;
                    break;
                case FilterModel::BP_Moog4:
                    ft = FilterType::BP4;
                    mSelectedFilter = &mMoog;
                    break;
#endif // DISABLE_MOOG_FILTER

#ifndef DISABLE_BIQUAD_MAJ7_FILTER
					// biquad
				case FilterModel::LP_Biquad:
					ft = FilterType::LP;
					mSelectedFilter = &mBiquad;
					break;
				case FilterModel::HP_Biquad:
					ft = FilterType::HP;
					mSelectedFilter = &mBiquad;
					break;
#endif // DISABLE_BIQUAD_MAJ7_FILTER

				}
                mSelectedFilter->SetParams(ft, cutoffHz, reso);
                if (mSelectedModel != ctype) {
                    mSelectedFilter->Reset();
                }
                mSelectedModel = ctype;
            }

            float ProcessSample(float inputSample)
			{
				return mSelectedFilter->ProcessSample(inputSample);
			}

		}; // FilterNode


        struct FilterAuxNode// : IAuxEffect
        {
            FilterNode mFilter; // stereo. cpu optimization: combine into a stereo filter to eliminate double recalc. but it's more code size.

            ParamAccessor mParams;

            FilterModel mFilterType;
            ModDestination mModDestBase;
            float mNoteHz = 0;
            size_t mnSampleCount = 0;
            ModMatrixNode* mModMatrix = nullptr;
            bool mEnabledCached = false;

            FilterAuxNode(float* paramCache, ParamIndices baseParamID, ModDestination modDestBase) :
                mParams(paramCache, baseParamID),
                mModDestBase(modDestBase)
            {}

            void AuxBeginBlock(float noteHz, ModMatrixNode& modMatrix)
            {
                mModMatrix = &modMatrix;
                mnSampleCount = 0; // ensure reprocessing after setting these params to avoid corrupt state.
                mNoteHz = noteHz;
                mFilterType = mParams.GetEnumValue<FilterModel>(FilterParamIndexOffsets::FilterType);
                mEnabledCached = mParams.GetBoolValue(FilterParamIndexOffsets::Enabled);
            }

            float AuxProcessSample(float inputSample)
            {
                if (!mEnabledCached) return inputSample;
                auto recalcMask = GetModulationRecalcSampleMask();
                bool calc = (mnSampleCount == 0);
                mnSampleCount = (mnSampleCount + 1) & recalcMask;
                if (calc) {
                    mFilter.SetParams(
                        mFilterType,
                        mParams.GetFrequency(FilterParamIndexOffsets::Freq, FilterParamIndexOffsets::FreqKT, gFilterFreqConfig, mNoteHz, mModMatrix->GetDestinationValue((int)mModDestBase + (int)FilterAuxModDestOffsets::Freq)),
                        mParams.Get01Value(FilterParamIndexOffsets::Q, mModMatrix->GetDestinationValue((int)mModDestBase + (int)FilterAuxModDestOffsets::Q))
                    );
                }

                return mFilter.ProcessSample(inputSample);
            }

        };

	} // namespace M7


} // namespace WaveSabreCore



































































