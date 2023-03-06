#pragma once

#include <WaveSabreCore/Maj7Basic.hpp>
#include "Maj7ModMatrix.hpp"
// TODO: find a way to only include filters which are actually used in the song.
#include "Filters/FilterDiode.hpp"
#include "Filters/FilterK35.hpp"
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
			LP_Diode,
			LP_K35,
			LP_Moog2,
			LP_Moog4,
			BP_Moog2,
			BP_Moog4,
			HP_OnePole,
			HP_K35,
			HP_Moog2,
			HP_Moog4,
            Count
		};

        // size-optimize using macro
        #define FILTER_MODEL_CAPTIONS { \
            "Disabled", \
            "LP_OnePole", \
            "LP_Diode", \
            "LP_K35", \
            "LP_Moog2", \
            "LP_Moog4", \
            "BP_Moog2", \
            "BP_Moog4", \
            "HP_OnePole", \
            "HP_K35", \
            "HP_Moog2", \
            "HP_Moog4", \
        }


		struct FilterNode
		{
			NullFilter mNullFilter;
			DiodeFilter mDiode;
			K35Filter mK35;
			MoogLadderFilter mMoog;
			OnePoleFilter mOnePole;
			//SEM12Filter mSem12;

			IFilter* mSelectedFilter = &mMoog;
            FilterModel mSelectedModel = FilterModel::LP_Moog4;

            void SetParams(FilterModel ctype, float cutoffHz, float reso, float saturation)
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
                case FilterModel::LP_Diode:
                    ft = FilterType::LP;
                    mSelectedFilter = &mDiode;
                    break;
                default:
                case FilterModel::LP_K35:
                    ft = FilterType::LP;
                    mSelectedFilter = &mK35;
                    break;
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
                case FilterModel::HP_K35:
                    ft = FilterType::HP;
                    mSelectedFilter = &mK35;
                    break;
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
                mSelectedFilter->SetParams(ft, cutoffHz, reso, saturation);
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

            EnumParam<FilterModel> mFilterTypeParam; // FilterType,
            Float01Param mFilterQParam;// FilterQ,
            Float01Param mFilterSaturationParam;// FilterSaturation,
            FrequencyParam mFilterFreqParam;// FilterFrequency,// FilterFrequencyKT,
            int mModDestParam2ID;

            FilterAuxNode(float* auxParams, int modDestParam2ID) :
                // !! do not SET initial values; these get instantiated dynamically.
                mFilterTypeParam(auxParams[(int)FilterAuxParamIndexOffsets::FilterType], FilterModel::Count),
                mFilterQParam(auxParams[(int)FilterAuxParamIndexOffsets::Q]),
                mFilterSaturationParam(auxParams[(int)FilterAuxParamIndexOffsets::Saturation]),
                mFilterFreqParam(auxParams[(int)FilterAuxParamIndexOffsets::Freq], auxParams[(int)FilterAuxParamIndexOffsets::FreqKT], gFilterCenterFrequency, gFilterFrequencyScale),
                mModDestParam2ID(modDestParam2ID)
            {}

            virtual void AuxBeginBlock(float noteHz, int nSamples, ModMatrixNode& modMatrix) override
            {
                mFilter.SetParams(
                    // TODO: apply mods
                    mFilterTypeParam.GetEnumValue(),
                    mFilterFreqParam.GetFrequency(noteHz, modMatrix.GetDestinationValue(mModDestParam2ID + (int)FilterAuxModIndexOffsets::Freq, 0)),
                    mFilterQParam.Get01Value(modMatrix.GetDestinationValue(mModDestParam2ID + (int)FilterAuxModIndexOffsets::Q, 0)),
                    mFilterSaturationParam.Get01Value(modMatrix.GetDestinationValue(mModDestParam2ID + (int)FilterAuxModIndexOffsets::Saturation, 0))
                );
            }

            virtual float AuxProcessSample(float inputSample) override
            {
                return mFilter.ProcessSample(inputSample);
            }

        };


        struct DCFilter// : public IFilter
        {
            DCFilter()
            {
                SetMinus3DBFreq(20);
            }
            void SetMinus3DBFreq(real hz)
            {
                // 'How to calculate "R" for a given (-3dB) low frequency point?'
                // R = 1 - (pi*2 * frequency /samplerate)
                // "R" between 0.9 .. 1
                // "R" depends on sampling rate and the low frequency point. Do not set "R" to a fixed value
                // (e.g. 0.99) if you don't know the sample rate. Instead set R to:
                // (-3dB @ 40Hz): R = 1-(250/samplerate)
                // (-3dB @ 20Hz): R = 1-(126/samplerate)
                R = Real1 - (PITimes2 * hz / Helpers::CurrentSampleRateF);
            }
            // IFilter
            //virtual void SetType(FilterType type) override
            //{
            //}
            //virtual FilterCapabilities GetCapabilities() override
            //{
            //    return FilterCapabilities::None;
            //}
            //virtual void SetCutoffFrequency(real hz) override
            //{
            //}
            //virtual void SetParams(FilterType type, real cutoffHz, real reso, real saturation) override
            //{
            //}

            //virtual void Reset() override
            //{
            //    xnminus1L = 0;
            //    ynminus1L = 0;
            //    xnminus1R = 0;
            //    ynminus1R = 0;
            //}

            //// mono processing
            //virtual void ProcessInPlace(real* samples, size_t sampleCount) override
            //{
            //    for (size_t i = 0; i < sampleCount; ++i)
            //    {
            //        samples[i] = InlineProcessSample(samples[i]);
            //    }
            //}
            //virtual real ProcessSample(real x) override
            //{
            //    return InlineProcessSample(x);
            //}

            real ProcessSample(real xn)
            {
                real yn = xn - xnminus1L + R * ynminus1L;
                xnminus1L = xn;
                ynminus1L = yn;
                return yn;
            }

            //// stereo processing
            //virtual void ProcessInPlace(real* samplesL, real* samplesR, size_t sampleCount) override
            //{
            //    for (size_t i = 0; i < sampleCount; ++i)
            //    {
            //        InlineProcessSample(samplesL[i], samplesR[i]);
            //    }
            //}
            //virtual void ProcessSample(real& xL, real& xR) override
            //{
            //    InlineProcessSample(xL, xR);
            //}

            //inline void InlineProcessSample(real& xnL, real& xnR)
            //{
            //    // left
            //    real ynL = xnL - xnminus1L + R * ynminus1L;
            //    xnminus1L = xnL;
            //    ynminus1L = ynL;
            //    // right
            //    real ynR = xnR - xnminus1R + R * ynminus1R;
            //    xnminus1R = xnR;
            //    ynminus1R = ynR;

            //    xnL = ynL;
            //    xnR = ynR;
            //}

        private:
            // state L
            real xnminus1L = 0;
            real ynminus1L = 0;

            //// state R
            //real xnminus1R = 0;
            //real ynminus1R = 0;

            real R = 0;
        };


	} // namespace M7


} // namespace WaveSabreCore







