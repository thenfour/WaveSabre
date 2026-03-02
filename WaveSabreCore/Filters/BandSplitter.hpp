#pragma once

//#include <WaveSabreCore/Maj7Basic.hpp>
//#include <WaveSabreCore/BiquadFilter.h>
#include "../GigaSynth/Maj7ModMatrix.hpp"
#include "../Filters/FilterMoog.hpp"
#include "../Filters/FilterOnePole.hpp"
#include <array>
#include "SVFilter.hpp"

#include "LinkwitzRileyFilter.hpp"

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

namespace WaveSabreCore
{
	namespace M7
	{
		static constexpr int gMBBands = 3;

		///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		struct FrequencySplitter
		{
			LinkwitzRileyFilter mLR[6];
#ifndef DISABLE_6db_oct_crossover
			M7::OnePoleFilter mOP[2];
#endif // DISABLE_6db_oct_crossover

			// low, med, high bands.
			float s[gMBBands];

			void frequency_splitter(float x, float crossoverFreqA, /*LinkwitzRileyFilter::Slope crossoverSlope, */ float crossoverFreqB)
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
		};


	} // namespace M7


} // namespace WaveSabreCore



































































