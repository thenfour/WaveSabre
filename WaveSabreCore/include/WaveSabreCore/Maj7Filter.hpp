#pragma once

#include <WaveSabreCore/Maj7Basic.hpp>
#include <WaveSabreCore/BiquadFilter.h>
#include "Maj7ModMatrix.hpp"
// TODO: find a way to only include filters which are actually used in the song.
//#include "Filters/FilterDiode.hpp"
//#include "Filters/FilterK35.hpp"
#include "Filters/FilterMoog.hpp"
#include "Filters/FilterOnePole.hpp"
//#include "Filters/FilterSEM12.hpp"

// disabling these filters saves ~60 bytes of final code
#define DISABLE_6db_oct_crossover
#define DISABLE_36db_oct_crossover
//#define DISABLE_48db_oct_crossover
//#define DISABLE_onepole_maj7_filter // actually DON'T disable this because it's required for the LFO "sharpness" control. it's pretty much free anyway.
//#define DISABLE_MOOG_FILTER
#define DISABLE_BIQUAD_MAJ7_FILTER
//
//#define LR_SLOPE_CAPTIONS(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::M7::LinkwitzRileyFilter::Slope::Count__] { \
//	"6dB (unsupported)",\
//	"12dB",\
//	"24dB",\
//	"36dB (unsupported)",\
//	"48dB",\
//	}

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
		struct SVFilter
		{
			SVFilter() {
				Reset();
			}
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
				auto x = updateCoefficients(v0, cutoff, Q);
				return x.x[1];
			}

			float SVFhigh(float v0, float cutoff, float Q) {
				auto x = updateCoefficients(v0, cutoff, Q);
				return v0 - k * x.x[0] - x.x[1];
			}

			float SVFall(float v0, float cutoff, float Q) {
				auto x = updateCoefficients(v0, cutoff, Q);
				return v0 - 2 * k * x.x[0];
			}

			void Reset() {
				g = 0, k = 0, a1 = 0, a2 = 0;
				ic1eq = 0, ic2eq = 0;
				d0 = 0, d1 = 0; // Used for tracking changes in cutoff and Q
				c = 0, i = 0; // Additional state variables for SVFOPapf_temp
			}

		private:
			float g, k, a1, a2;
			float ic1eq, ic2eq;
			float d0, d1; // Used for tracking changes in cutoff and Q
			float c, i; // Additional state variables for SVFOPapf_temp
		}; // SVFilter

		struct LinkwitzRileyFilter {
			using real = float;

			static constexpr real q24 = 0.707106781187f;// sqrt(0.5);
			static constexpr real q48_1 = 0.541196100146f;// 0.5 / cos($pi / 8 * 1);
			static constexpr real q48_2 = 1.30656296488f;// 0.5 / cos($pi / 8 * 3);

			//enum class Slope : uint8_t {
			//	Slope_6dB,
			//	Slope_12dB,
			//	Slope_24dB,
			//	Slope_36dB,
			//	Slope_48dB,
			//	Count__,
			//};

			//SVFilter svf[4];
			SVFilter svf[2];

			void Reset() {
				for (auto& f : svf) {
					f.Reset();
				}
			}

			real LR_LPF(real x, real freq/*, Slope slope*/) {
				//switch (slope) {
				//default:
				//case Slope::Slope_12dB:
//					return svf.SVFlow(x, freq, 0.5f);
//				case Slope::Slope_24dB:
					x = svf[0].SVFlow(x, freq, q24);
					return svf[1].SVFlow(x, freq, q24);
//#ifndef DISABLE_6db_oct_crossover
//				case Slope::Slope_36dB:
//					x = svf[0].SVFlow(x, freq, 1);
//					x = svf[1].SVFlow(x, freq, 1);
//					return svf[2].SVFlow(x, freq, 0.5f);
//#endif // DISABLE_6db_oct_crossover
//#ifndef DISABLE_48db_oct_crossover
//				case Slope::Slope_48dB:
//					x = svf[0].SVFlow(x, freq, q48_1);
//					x = svf[1].SVFlow(x, freq, q48_2);
//					x = svf[2].SVFlow(x, freq, q48_1);
//					return svf[3].SVFlow(x, freq, q48_2);
//#endif // DISABLE_48db_oct_crossover
//				}
			}

			real LR_HPF(real x, real freq/*, Slope slope*/) {
				//switch (slope) {
				//default:
				//case Slope::Slope_12dB:
					//return svf.SVFhigh(-x, freq, 0.5f);
//				case Slope::Slope_24dB:
					x = svf[0].SVFhigh(x, freq, q24);
					return svf[1].SVFhigh(x, freq, q24);
//#ifndef DISABLE_6db_oct_crossover
//				case Slope::Slope_36dB:
//					x = svf[0].SVFhigh(-x, freq, 1);
//					x = svf[1].SVFhigh(x, freq, 1);
//					return svf[2].SVFhigh(x, freq, 0.5f);
//#endif // DISABLE_6db_oct_crossover	
//#ifndef DISABLE_48db_oct_crossover
//				case Slope::Slope_48dB:
//					x = svf[0].SVFhigh(x, freq, q48_1);
//					x = svf[1].SVFhigh(x, freq, q48_2);
//					x = svf[2].SVFhigh(x, freq, q48_1);
//					return svf[3].SVFhigh(x, freq, q48_2);
//#endif // DISABLE_48db_oct_crossover
//				}
			}

			real APF(real x, real freq/*, Slope slope*/) {
				//switch (slope) {
				//default:
				//case Slope::Slope_12dB:
					//return svf.SVFOPapf_temp(-x, freq);
//				case Slope::Slope_24dB:
					return svf[0].SVFall(x, freq, q24);
//#ifndef DISABLE_6db_oct_crossover
//				case Slope::Slope_36dB:
//					x = svf[0].SVFall(-x, freq, 1);
//					return svf[1].SVFOPapf_temp(x, freq);
//#endif // DISABLE_6db_oct_crossover
//#ifndef DISABLE_48db_oct_crossover
//				case Slope::Slope_48dB:
//					x = svf[0].SVFall(x, freq, q48_1);
//					return svf[1].SVFall(x, freq, q48_2);
//#endif // DISABLE_48db_oct_crossover
//				}
			}
		};

		//struct SVFilterNode : IFilter {
		//	SVFilter mFilter;

		//	FilterType mType;
		//	float mCutoffHz;
		//	float mQ;
		//	
		//	virtual void SetParams(FilterType type, float cutoffHz, float reso) override {
		//		mQ = reso;
		//		mCutoffHz = cutoffHz;
		//		switch (type) {
		//		default:
		//		case FilterType::LP2:
		//		case FilterType::LP4:
		//			mType = FilterType::LP;
		//			break;
		//		case FilterType::HP2:
		//		case FilterType::HP4:
		//			mType = FilterType::HP;
		//			break;
		//		}
		//	}
		//	virtual float ProcessSample(float x) override {
		//		if (mType == FilterType::LP) {
		//			return mFilter.SVFlow(x, mCutoffHz, mQ);
		//		}
		//		return mFilter.SVFhigh(x, mCutoffHz, mQ);
		//	}
		//	virtual void Reset() override {
		//		mFilter.Reset();
		//	}
		//};


		//struct LinkwitzRileyNode : IFilter {
		//	LinkwitzRileyFilter mFilter;

		//	FilterType mType;
		//	LinkwitzRileyFilter::Slope mSlope;
		//	float mCutoffHz;

		//	virtual void SetParams(FilterType type, float cutoffHz, float reso) override {
		//		mCutoffHz = cutoffHz;
		//		switch (type) {
		//		default:
		//		case FilterType::LP2:
		//			mType = FilterType::LP;
		//			mSlope = LinkwitzRileyFilter::Slope::Slope_24dB;
		//			break;
		//		case FilterType::LP4:
		//			mType = FilterType::LP;
		//			mSlope = LinkwitzRileyFilter::Slope::Slope_48dB;
		//			break;
		//		case FilterType::HP2:
		//			mType = FilterType::HP;
		//			mSlope = LinkwitzRileyFilter::Slope::Slope_24dB;
		//			break;
		//		case FilterType::HP4:
		//			mType = FilterType::HP;
		//			mSlope = LinkwitzRileyFilter::Slope::Slope_48dB;
		//			break;
		//		}
		//	}
		//	virtual float ProcessSample(float x) override {
		//		if (mType == FilterType::LP) {
		//			return mFilter.LR_LPF(x, mCutoffHz, mSlope);
		//		}
		//		return mFilter.LR_HPF(x, mCutoffHz, mSlope);
		//	}
		//	virtual void Reset() override {
		//		mFilter.Reset();
		//	}
		//};

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
					//case FilterModel::HP_K35:
                //    ft = FilterType::HP;
                //    mSelectedFilter = &mK35;
                //    break;
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

				//	// SVF filter alone is not so compelling.
				//case FilterModel::LP_SVF:
				//	ft = FilterType::LP;
				//	mSelectedFilter = &mSVF;
				//	break;
				//case FilterModel::HP_SVF:
				//	ft = FilterType::HP;
				//	mSelectedFilter = &mSVF;
				//	break;

				//	// linkwitz riley is also not that interesting.
				//case FilterModel::LP_LinkwitzRiley24:
				//	ft = FilterType::LP2;
				//	mSelectedFilter = &mLR;
				//	break;
				//case FilterModel::LP_LinkwitzRiley48:
				//	ft = FilterType::LP4;
				//	mSelectedFilter = &mLR;
				//	break;
				//case FilterModel::HP_LinkwitzRiley24:
				//	ft = FilterType::HP2;
				//	mSelectedFilter = &mLR;
				//	break;
				//case FilterModel::HP_LinkwitzRiley48:
				//	ft = FilterType::HP4;
				//	mSelectedFilter = &mLR;
				//	break;

				//
				
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

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		struct FrequencySplitter
		{
			LinkwitzRileyFilter mLR[6];
#ifndef DISABLE_6db_oct_crossover
			M7::OnePoleFilter mOP[2];
#endif // DISABLE_6db_oct_crossover

			// low, med, high bands.
			float s[3];

			void frequency_splitter(float x, float crossoverFreqA, /*, LinkwitzRileyFilter::Slope crossoverSlope,*/ float crossoverFreqB)
			{
				// do some fixing of crossover frequencies.
				float a = math::clamp(crossoverFreqA, 30, 18000);
				float b = math::clamp(crossoverFreqB, 30, 18000);
				//if (a > b) {
				//	std::swap(a, b);
				//}

#ifndef DISABLE_6db_oct_crossover
				if (crossoverSlope == LinkwitzRileyFilter::Slope::Slope_6dB) {
					s[0] = mOP[0].ProcessSample(x, M7::FilterType::LP2, crossoverFreqA);
					s[2] = mOP[1].ProcessSample(x, M7::FilterType::HP2, crossoverFreqB);
					s[1] = x - s[0] - s[2];
				}
				else
#endif // DISABLE_6db_oct_crossover
				{
					s[0] = mLR[0].LR_LPF(x, crossoverFreqA/*crossoverSlope*/);
					s[0] = mLR[1].LR_LPF(s[0], crossoverFreqB/*crossoverSlope*/);

					s[1] = mLR[2].LR_HPF(x, crossoverFreqA /*crossoverSlope*/);
					s[1] = mLR[3].LR_LPF(s[1], crossoverFreqB /*crossoverSlope*/);

					s[2] = mLR[4].APF(x, crossoverFreqA/*, crossoverSlope*/);
					s[2] = mLR[5].LR_HPF(s[2], crossoverFreqB/*, crossoverSlope*/);
				}
			}
		};


	} // namespace M7


} // namespace WaveSabreCore







