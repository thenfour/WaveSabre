#pragma once

#include <WaveSabreCore/Maj7Basic.hpp>

namespace WaveSabreCore
{
	namespace M7
	{
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


	} // namespace M7


} // namespace WaveSabreCore

