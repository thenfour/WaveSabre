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

        ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
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

            real xnminus1L = 0;
            real ynminus1L = 0;

            static constexpr real R = 0.998f;
        };


		struct SVFilter
		{
			// one pole apf
			float SVFOPapf_temp(float x, float cutoff) {
				if (d1 != d0) {
					d0 = cutoff;
					c = M7::math::tan(M7::math::gPI * (cutoff * Helpers::CurrentSampleRateRecipF - 0.25f)) * 0.5f + 0.5f;
				}
				float r = (1 - c) * i + c * x;
				i = 2 * r - i;
				return x - 2 * r;
			}

			// Update filter coefficients if cutoff or Q has changed
			M7::FloatPair updateCoefficients(float v0, float cutoff, float Q) {
				float new_d0 = cutoff + Q;
				if (d1 != new_d0) {
					d1 = d0;
					d0 = new_d0;
					g = M7::math::tan(M7::math::gPI * cutoff * Helpers::CurrentSampleRateRecipF);
					k = 1.0f / Q;
					a1 = 1.0f / (1.0f + g * (g + k));
					a2 = g * a1;
				}
				float v1 = a1 * ic1eq + a2 * (v0 - ic2eq);
				float v2 = ic2eq + g * v1;
				ic1eq = 2 * v1 - ic1eq;
				ic2eq = 2 * v2 - ic2eq;
				return { v1, v2 };
			}

			float SVFlow(float v0, float cutoff, float Q) {
				auto x =updateCoefficients(v0, cutoff, Q);
				return x.second;
			}

			float SVFhigh(float v0, float cutoff, float Q) {
				auto x = updateCoefficients(v0, cutoff, Q);
				return v0 - k * x.first - x.second;
			}

			float SVFall(float v0, float cutoff, float Q) {
				auto x = updateCoefficients(v0, cutoff, Q);
				return v0 - 2 * k * x.first;
			}

		private:
			float g = 0, k = 0, a1 = 0, a2 = 0;
			float ic1eq = 0, ic2eq = 0;
			float d0 = 0, d1 = 0; // Used for tracking changes in cutoff and Q
			float c = 0, i = 0; // Additional state variables for SVFOPapf_temp
		}; // SVFilter

		struct LinkwitzRileyFilter {
			using real = float;

			static constexpr real q24 = 0.707106781187f;// sqrt(0.5);
			static constexpr real q48_1 = 0.541196100146f;// 0.5 / cos($pi / 8 * 1);
			static constexpr real q48_2 = 1.30656296488f;// 0.5 / cos($pi / 8 * 3);

			enum class Slope : uint8_t {
				Slope_6dB,
				Slope_12dB,
				Slope_24dB,
				Slope_36dB,
				Slope_48dB,
				Count__,
			};

			SVFilter svf[4];

			real LR_LPF(real x, real freq, Slope slope) {
				switch (slope) {
				case Slope::Slope_12dB:
					return svf[0].SVFlow(x, freq, 0.5f);
				case Slope::Slope_24dB:
					x = svf[0].SVFlow(x, freq, q24);
					return svf[1].SVFlow(x, freq, q24);
				case Slope::Slope_36dB:
					x = svf[0].SVFlow(x, freq, 1);
					x = svf[1].SVFlow(x, freq, 1);
					return svf[2].SVFlow(x, freq, 0.5f);
				case Slope::Slope_48dB:
					x = svf[0].SVFlow(x, freq, q48_1);
					x = svf[1].SVFlow(x, freq, q48_2);
					x = svf[2].SVFlow(x, freq, q48_1);
					return svf[3].SVFlow(x, freq, q48_2);
				default:
					return x; // Invalid slope, return unprocessed signal
				}
			}

			real LR_HPF(real x, real freq, Slope slope) {
				switch (slope) {
				case Slope::Slope_12dB:
					return svf[0].SVFhigh(-x, freq, 0.5f);
				case Slope::Slope_24dB:
					x = svf[0].SVFhigh(x, freq, q24);
					return svf[1].SVFhigh(x, freq, q24);
				case Slope::Slope_36dB:
					x = svf[0].SVFhigh(-x, freq, 1);
					x = svf[1].SVFhigh(x, freq, 1);
					return svf[2].SVFhigh(x, freq, 0.5f);
				case Slope::Slope_48dB:
					x = svf[0].SVFhigh(x, freq, q48_1);
					x = svf[1].SVFhigh(x, freq, q48_2);
					x = svf[2].SVFhigh(x, freq, q48_1);
					return svf[3].SVFhigh(x, freq, q48_2);
				default:
					return x; // Invalid slope, return unprocessed signal
				}
			}

			real APF(real x, real freq, Slope slope) {
				switch (slope) {
				case Slope::Slope_12dB:
					return svf[0].SVFOPapf_temp(-x, freq);
				case Slope::Slope_24dB:
					return svf[0].SVFall(x, freq, q24);
				case Slope::Slope_36dB:
					x = svf[0].SVFall(-x, freq, 1);
					return svf[1].SVFOPapf_temp(x, freq);
				case Slope::Slope_48dB:
					x = svf[0].SVFall(x, freq, q48_1);
					return svf[1].SVFall(x, freq, q48_2);
				default:
					return x; // Invalid slope, return unprocessed signal
				}
			}
		};

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		struct FrequencySplitter
		{
			LinkwitzRileyFilter mLR[6];
			M7::OnePoleFilter mOP[2];

			// low, med, high bands.
			float s[3];

			void frequency_splitter(float x, float crossoverFreqA, LinkwitzRileyFilter::Slope crossoverSlope, float crossoverFreqB)
			{
				if (crossoverSlope == LinkwitzRileyFilter::Slope::Slope_6dB) {
					s[0] = mOP[0].ProcessSample(x, M7::FilterType::LP2, crossoverFreqA);
					s[2] = mOP[1].ProcessSample(x, M7::FilterType::HP2, crossoverFreqB);
					s[1] = x - s[0] - s[2];
				}
				else
				{
					s[0] = mLR[0].LR_LPF(x, crossoverFreqA, crossoverSlope);
					s[0] = mLR[1].LR_LPF(s[0], crossoverFreqB, crossoverSlope);

					s[1] = mLR[2].LR_HPF(x, crossoverFreqA, crossoverSlope);
					s[1] = mLR[3].LR_LPF(s[1], crossoverFreqB, crossoverSlope);

					s[2] = mLR[4].APF(x, crossoverFreqA, crossoverSlope);
					s[2] = mLR[5].LR_HPF(s[2], crossoverFreqB, crossoverSlope);
				}
			}
		};


	} // namespace M7


} // namespace WaveSabreCore







