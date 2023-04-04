#pragma once

#include <WaveSabreCore/Maj7Basic.hpp>
#include "Maj7ModMatrix.hpp"
// TODO: find a way to only include filters which are actually used in the song.
//#include "Filters/FilterDiode.hpp"
//#include "Filters/FilterK35.hpp"
#include "Filters/FilterMoog.hpp"
#include "Filters/FilterOnePole.hpp"
//#include "Filters/FilterSEM12.hpp"
// TODO: comb, notch, butterworth, ??

namespace WaveSabreCore
{
	namespace M7
	{
		enum class FilterModel : uint8_t
		{
			Disabled = 0,
			LP_OnePole,
			//LP_SEM12,
			//LP_Diode,
			//LP_K35,
			LP_Moog2,
			LP_Moog4,
			BP_Moog2,
			BP_Moog4,
			HP_OnePole,
			//HP_K35,
			HP_Moog2,
			HP_Moog4,
            Count
		};

        // size-optimize using macro
        #define FILTER_MODEL_CAPTIONS { \
            "Disabled", \
            "LP_OnePole", \
            "LP_Moog2", \
            "LP_Moog4", \
            "BP_Moog2", \
            "BP_Moog4", \
            "HP_OnePole", \
            "HP_Moog2", \
            "HP_Moog4", \
        }


		struct FilterNode
		{
			NullFilter mNullFilter;
			//DiodeFilter mDiode;
			//K35Filter mK35;
			MoogLadderFilter mMoog;
			OnePoleFilter mOnePole;
			//SEM12Filter mSem12;

			IFilter* mSelectedFilter = &mMoog;
            FilterModel mSelectedModel = FilterModel::LP_Moog4;

            void SetParams(FilterModel ctype, float cutoffHz, float reso)
            {
                // select filter & set type
                FilterType ft = FilterType::LP;
                switch (ctype)
                {
                case FilterModel::Disabled:
                    mSelectedFilter = &mNullFilter;
                    break;
                case FilterModel::LP_OnePole:
                    ft = FilterType::LP;
                    mSelectedFilter = &mOnePole;
                    break;
                //case FilterModel::LP_SEM12:
                //    ft = FilterType::LP;
                //    mSelectedFilter = &mSem12;
                //    break;
                //case FilterModel::LP_Diode:
                //    ft = FilterType::LP;
                //    mSelectedFilter = &mDiode;
                //    break;
                //default:
                //case FilterModel::LP_K35:
                //    ft = FilterType::LP;
                //    mSelectedFilter = &mK35;
                //    break;
                case FilterModel::LP_Moog2:
                    ft = FilterType::LP2;
                    mSelectedFilter = &mMoog;
                    break;
                case FilterModel::LP_Moog4:
                    ft = FilterType::LP4;
                    mSelectedFilter = &mMoog;
                    break;
                case FilterModel::HP_OnePole:
                    ft = FilterType::HP;
                    mSelectedFilter = &mOnePole;
                    break;
                //case FilterModel::HP_K35:
                //    ft = FilterType::HP;
                //    mSelectedFilter = &mK35;
                //    break;
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


        struct FilterAuxNode : IAuxEffect
        {
            FilterNode mFilter;

            ParamAccessor mParams;

            FilterModel mFilterType;
            //EnumParam<FilterModel> mFilterTypeParam; // FilterType,
            //Float01Param mFilterQParam;// FilterQ,
            //Float01Param mFilterSaturationParam;// FilterSaturation,
            //FrequencyParam mFilterFreqParam;// FilterFrequency,// FilterFrequencyKT,

            int mModDestParam2ID;
            float mNoteHz = 0;
            size_t mnSampleCount = 0;
            ModMatrixNode* mModMatrix = nullptr;

            FilterAuxNode(ParamAccessor& params, int modDestParam2ID) :
                mParams(params),
                // !! do not SET initial values; these get instantiated dynamically.
                //mFilterTypeParam(auxParams[(int)FilterAuxParamIndexOffsets::FilterType], FilterModel::Count),
                //mFilterQParam(auxParams[(int)FilterAuxParamIndexOffsets::Q]),
                //mFilterSaturationParam(auxParams[(int)FilterAuxParamIndexOffsets::Saturation]),
                //mFilterFreqParam(auxParams[(int)FilterAuxParamIndexOffsets::Freq], auxParams[(int)FilterAuxParamIndexOffsets::FreqKT], gFilterCenterFrequency, gFilterFrequencyScale),
                mModDestParam2ID(modDestParam2ID)
            {}

            virtual void AuxBeginBlock(float noteHz, ModMatrixNode& modMatrix) override
            {
                mModMatrix = &modMatrix;
                mnSampleCount = 0; // ensure reprocessing after setting these params to avoid corrupt state.
                mNoteHz = noteHz;
                mFilterType = mParams.GetEnumValue<FilterModel>(FilterAuxParamIndexOffsets::FilterType);
                //mFilterTypeParam.CacheValue();
            }

            virtual float AuxProcessSample(float inputSample) override
            {
                auto recalcMask = GetModulationRecalcSampleMask();
                bool calc = (mnSampleCount == 0);
                mnSampleCount = (mnSampleCount + 1) & recalcMask;
                if (calc) {
                    mFilter.SetParams(
                        mFilterType,
                        mParams.GetFrequency(FilterAuxParamIndexOffsets::Freq, FilterAuxParamIndexOffsets::FreqKT, gFilterFreqConfig, mNoteHz, mModMatrix->GetDestinationValue(mModDestParam2ID + (int)FilterAuxModIndexOffsets::Freq)),
                        //mFilterFreqParam.GetFrequency(mNoteHz, ),
                        mParams.Get01Value(FilterAuxParamIndexOffsets::Q, mModMatrix->GetDestinationValue(mModDestParam2ID + (int)FilterAuxModIndexOffsets::Q))
                        //mFilterQParam.Get01Value(mModMatrix->GetDestinationValue(mModDestParam2ID + (int)FilterAuxModIndexOffsets::Q))
                    );
                }

                return mFilter.ProcessSample(inputSample);
            }

        };


        struct DCFilter
        {
            //DCFilter()
            //{
            //    //SetMinus3DBFreq(10);
            //}
            //void SetMinus3DBFreq(real hz)
            //{
            //    // 'How to calculate "R" for a given (-3dB) low frequency point?'
            //    // R = 1 - (pi*2 * frequency /samplerate)
            //    // "R" between 0.9 .. 1
            //    // "R" depends on sampling rate and the low frequency point. Do not set "R" to a fixed value
            //    // (e.g. 0.99) if you don't know the sample rate. Instead set R to:
            //    // (-3dB @ 40Hz): R = 1-(250/samplerate)
            //    // (-3dB @ 20Hz): R = 1-(126/samplerate)
            //    R = Real1 - (PITimes2 * hz / Helpers::CurrentSampleRateF);
            //}

            real ProcessSample(real xn)
            {
                real yn = xn - xnminus1L + R * ynminus1L;
                xnminus1L = xn;
                ynminus1L = yn;
                return yn;
            }

        //private:
            // state L
            real xnminus1L = 0;
            real ynminus1L = 0;

            static constexpr real R = 0.998f;
        };


	} // namespace M7


} // namespace WaveSabreCore







