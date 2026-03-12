#pragma once

//#include <WaveSabreCore/Maj7Basic.hpp>
//#include <WaveSabreCore/BiquadFilter.h>
#include "../GigaSynth/Maj7ModMatrix.hpp"
#include "../Filters/FilterMoog.hpp"
#include "../Filters/FilterOnePole.hpp"
#include <array>
#include "SVFilter.hpp"

#include "LinkwitzRileyFilter.hpp"

#define CROSSOVER_SLOPE_CAPTIONS(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::M7::LinkwitzRileyFilter::Slope::Count__] { \
	"6dB",\
	"12dB",\
	"24dB",\
	"36dB",\
	"48dB",\
	}

namespace WaveSabreCore
{
	namespace M7
	{
  enum class CrossoverSlope : uint8_t {
  	Slope_6dB,
  	Slope_12dB,
  	Slope_24dB,
  	Slope_36dB,
  	Slope_48dB,
  	Count,
  };


		static constexpr int gMBBands = 3;

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		// mono signal processing.
		struct FrequencySplitterOutput
		{
			float s[gMBBands];
		};
		struct FrequencySplitterBand
		{
		    // 2 per band for LR4; 3 per band for LR6; 4 per band for LR8
			LinkwitzRileyFilter mLR[4];
#ifdef ENABLE_6db_oct_crossover
            // for 6 dB/oct crossover, 1 per-band.
			M7::MoogOnePoleFilter mOP;
#endif // ENABLE_6db_oct_crossover

			float mCrossoverFreq;
			CrossoverSlope mCrossoverSlope;
		};
		struct FrequencySplitter
		{
		private:
			// for 3 bands, we need 3 filter sets, but only 2 crossover freqs & slopes.
			// the last band's crossover freq and slope are ignored.
			FrequencySplitterBand mBands[gMBBands];

		public:

			void SetParams(float crossoverFreqA, CrossoverSlope crossoverSlopeA, float crossoverFreqB, CrossoverSlope crossoverSlopeB)
			{
				mBands[0].mCrossoverFreq = crossoverFreqA;
				mBands[0].mCrossoverSlope = crossoverSlopeA;
				mBands[1].mCrossoverFreq = crossoverFreqB;
				mBands[1].mCrossoverSlope = crossoverSlopeB;
				// last band's crossover freq are ignored.
			}

			FrequencySplitterOutput frequency_splitter(float x)
			{
#ifdef ENABLE_6db_oct_crossover
				// if (crossoverSlope == Slope::Slope_6dB) {
				// 	s[0] = mOP[0].ProcessSample(x, M7::FilterType::LP2, crossoverFreqA);
				// 	s[2] = mOP[1].ProcessSample(x, M7::FilterType::HP2, crossoverFreqB);
				// 	s[1] = x - s[0] - s[2];
				// }
				// else
#endif // ENABLE_6db_oct_crossover
				{
				//	FrequencySplitterOutput out{
				//	 s[0] = mLR[0].LR_LPF(x, mBands[0].mCrossoverFreq);
				//	 s[0] = mLR[1].LR_LPF(s[0], mBands[1].mCrossoverFreq);

				//	 s[1] = mLR[2].LR_HPF(x, mBands[0].mCrossoverFreq);
				//	 s[1] = mLR[3].LR_LPF(s[1], mBands[1].mCrossoverFreq);

				//	 s[2] = mLR[4].APF(x, mBands[0].mCrossoverFreq);
				//	 s[2] = mLR[5].LR_HPF(s[2], mBands[1].mCrossoverFreq);
		  //};
				}
        return {};
			}

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
// todo: comment specifically which this is for, and implement for other slopes.
			std::array<float, gMBBands> GetMagnitudesAtFrequency(float freqHz, float crossoverFreqA, float crossoverFreqB) const
			{
				// Return low/mid/high magnitudes at the given frequency using LR4 responses.
				// Keep the same clamping and ordering as the processing path.
				float a = math::clamp(crossoverFreqA, 30.0f, 18000.0f);
				float b = math::clamp(crossoverFreqB, 30.0f, 18000.0f);

				float low  = LinkwitzRileyFilter::MagnitudeLPF(freqHz, a) * LinkwitzRileyFilter::MagnitudeLPF(freqHz, b);
				float mid  = LinkwitzRileyFilter::MagnitudeHPF(freqHz, a) * LinkwitzRileyFilter::MagnitudeLPF(freqHz, b);
				// High band includes an APF at A which has unity magnitude; only HPF at B shapes magnitude.
				float high = LinkwitzRileyFilter::MagnitudeHPF(freqHz, b);

				return { low, mid, high };
				//std::array<float, gMBBands> ret;
				//return ret;
			}
#endif
		};


	} // namespace M7


} // namespace WaveSabreCore



































































