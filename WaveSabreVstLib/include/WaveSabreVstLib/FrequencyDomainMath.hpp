#pragma once

#include <WaveSabreCore/Helpers.h>
#include <WaveSabreCore/Maj7Filter.hpp>

namespace WaveSabreVstLib {

// Linear magnitude of a Biquad at a frequency in Hz
inline float BiquadMagnitudeForFrequency(const WaveSabreCore::BiquadFilter& bq, double freqHz)
{
    static constexpr double tau = 6.283185307179586476925286766559;
    const double w = tau * freqHz / WaveSabreCore::Helpers::CurrentSampleRate;

    const double ma1 = double(bq.coeffs[1]) / bq.coeffs[0];
    const double ma2 = double(bq.coeffs[2]) / bq.coeffs[0];
    const double mb0 = double(bq.coeffs[3]) / bq.coeffs[0];
    const double mb1 = double(bq.coeffs[4]) / bq.coeffs[0];
    const double mb2 = double(bq.coeffs[5]) / bq.coeffs[0];

    const double numerator   = mb0 * mb0 + mb1 * mb1 + mb2 * mb2
                             + 2 * (mb0 * mb1 + mb1 * mb2) * ::cos(w)
                             + 2 * mb0 * mb2 * ::cos(2 * w);
    const double denominator = 1 + ma1 * ma1 + ma2 * ma2
                             + 2 * (ma1 + ma1 * ma2) * ::cos(w)
                             + 2 * ma2 * ::cos(2 * w);
    return (float)::sqrt(numerator / denominator);
}

} // namespace WaveSabreVstLib
