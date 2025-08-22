#pragma once

#include <WaveSabreCore/Maj7Basic.hpp>
#include <WaveSabreCore/BiquadFilter.h>
#include "Maj7ModMatrix.hpp"
#include "Filters/FilterMoog.hpp"
#include "Filters/FilterOnePole.hpp"
#include <array>
#include "SVFilter.hpp"

namespace WaveSabreCore
{
	namespace M7
	{
		struct LinkwitzRileyFilter {
			using real = float;

			// Q values to realize Linkwitz-Riley responses via cascaded 2nd-order SVFs
			// q24 = 1/sqrt(2) (?0.7071) gives a Butterworth 2nd-order section; two in cascade -> LR4 (24 dB/oct)
			static constexpr real q24 = 0.707106781187f;// sqrt(0.5);
			// The following are Q values for constructing an LR8 (48 dB/oct) via four 2nd-order sections.
			// They are kept for reference but not used in the current build.
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
//				return svf.SVFlow(x, freq, 0.5f);
//			case Slope::Slope_24dB:
				x = svf[0].SVFlow(x, freq, q24);
				return svf[1].SVFlow(x, freq, q24);
//#ifndef DISABLE_6db_oct_crossover
//			case Slope::Slope_36dB:
//				x = svf[0].SVFlow(x, freq, 1);
//				x = svf[1].SVFlow(x, freq, 1);
//				return svf[2].SVFlow(x, freq, 0.5f);
//#endif // DISABLE_6db_oct_crossover
//#ifndef DISABLE_48db_oct_crossover
//			case Slope::Slope_48dB:
//				x = svf[0].SVFlow(x, freq, q48_1);
//				x = svf[1].SVFlow(x, freq, q48_2);
//				x = svf[2].SVFlow(x, freq, q48_1);
//				return svf[3].SVFlow(x, freq, q48_2);
//#endif // DISABLE_48db_oct_crossover
//			}
			}

			real LR_HPF(real x, real freq/*, Slope slope*/) {
				//switch (slope) {
				//default:
				//case Slope::Slope_12dB:
					//return svf.SVFhigh(-x, freq, 0.5f);
//			case Slope::Slope_24dB:
				x = svf[0].SVFhigh(x, freq, q24);
				return svf[1].SVFhigh(x, freq, q24);
//#ifndef DISABLE_6db_oct_crossover
//			case Slope::Slope_36dB:
//				x = svf[0].SVFhigh(-x, freq, 1);
//				x = svf[1].SVFhigh(x, freq, 1);
//				return svf[2].SVFhigh(x, freq, 0.5f);
//#endif // DISABLE_6db_oct_crossover	
//#ifndef DISABLE_48db_oct_crossover
//			case Slope::Slope_48dB:
//				x = svf[0].SVFhigh(x, freq, q48_1);
//				x = svf[1].SVFhigh(x, freq, q48_2);
//				x = svf[2].SVFhigh(x, freq, q48_1);
//				return svf[3].SVFhigh(x, freq, q48_2);
//#endif // DISABLE_48db_oct_crossover
//			}
			}

			real APF(real x, real freq/*, Slope slope*/) {
				//switch (slope) {
				//default:
				//case Slope::Slope_12dB:
					//return svf.SVFOPapf_temp(-x, freq);
//			case Slope::Slope_24dB:
				return svf[0].SVFall(x, freq, q24);
//#ifndef DISABLE_6db_oct_crossover
//			case Slope::Slope_36dB:
//				x = svf[0].SVFall(-x, freq, 1);
//				return svf[1].SVFOPapf_temp(x, freq);
//#endif // DISABLE_6db_oct_crossover
//#ifndef DISABLE_48db_oct_crossover
//			case Slope::Slope_48dB:
//				x = svf[0].SVFall(x, freq, q48_1);
//				return svf[1].SVFall(x, freq, q48_2);
//#endif // DISABLE_48db_oct_crossover
//			}
			}

			static inline float MagnitudeLPF(float freqHz, float crossoverHz)
			{
				if (crossoverHz <= 0.0f) return 0.0f;

				const float r = freqHz / crossoverHz;
				const float r2 = r * r;
				const float r4 = r2 * r2;

				// LR4 low-pass amplitude: 1 / (1 + r^4)
				const float denom = 1.0f + r4;
				return 1.0f / denom;
			}

			static inline float MagnitudeHPF(float freqHz, float crossoverHz)
			{
				if (crossoverHz <= 0.0f) return 1.0f;

				const float r = freqHz / crossoverHz;
				const float r2 = r * r;
				const float r4 = r2 * r2;

				// LR4 high-pass amplitude: r^4 / (1 + r^4)
				const float denom = 1.0f + r4;
				return r4 / denom;
			}


		};

	} // namespace M7


} // namespace WaveSabreCore



































































