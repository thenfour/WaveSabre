// "Designing Software Synthesizer Plug-Ins in C++" // https://willpirkle.com
// this is NOT a completely general-purpose one-pole filter; it's designed to be used as a building block for the Moog ladder filter

#pragma once

#include "FilterBase.hpp"
#include <complex>

namespace WaveSabreCore
{
    namespace M7
    {
        struct MoogOnePoleFilter : public IFilter
        {
            virtual void SetParams(FilterCircuit circuit, FilterSlope slope, FilterResponse response, float cutoffHz, float Qdb) override
            {
                if (!DoesSupport(circuit, slope, response))
                {
                    return;
                }

                mResponse = response;
                m_cutoffHz = cutoffHz;
                Recalc();
            }

            virtual bool DoesSupport(FilterCircuit circuit, FilterSlope slope, FilterResponse response) override
            {
                if (circuit != FilterCircuit::OnePole) return false;
                if (slope != FilterSlope::Slope6dbOct) return false;
                if (response != FilterResponse::Lowpass && response != FilterResponse::Highpass) return false;
                return true;
            }

            virtual void Reset() override
            {
                m_z_1L = 0;
                m_feedbackL = 0;
            }

            real2 getFeedbackOutputL()
            {
                return m_beta * (m_z_1L + m_feedbackL * m_delta);
            }

            virtual float ProcessSample(float xn__) override
            {
                // for diode filter support
                real2 xn = xn__ * m_gamma + m_feedbackL + m_epsilon * getFeedbackOutputL();
                // calculate v(n)
                real2 vn = (m_a_0 * xn - m_z_1L) * m_alpha;
                // form LP output
                real2 lpf = vn + m_z_1L;
                // update memory
                m_z_1L = vn + lpf;
                if (mResponse == FilterResponse::Lowpass)
                {
                    return float(lpf);
                }
                auto hpf = xn - lpf;
                return float(hpf);
            }

            virtual real GetMagnitudeAtFrequency(real freqHz) const override
            {
                const double clampedFreq = math::clamp(double(freqHz), 0.0, 0.5 * Helpers::CurrentSampleRate);
                const double w = math::gPITimes2d * clampedFreq * Helpers::CurrentSampleRateRecipF;
                const double cw = std::cos(w);
                const double sw = std::sin(w);

                const double alpha = double(m_alpha);
                const double p = 1.0 - alpha;
                const double den2 = (1.0 - p * cw) * (1.0 - p * cw) + (p * sw) * (p * sw);
                const double safeDen2 = (den2 > 1e-20) ? den2 : 1e-20;

                const double hlpRe = alpha * (1.0 - p * cw) / safeDen2;
                const double hlpIm = -alpha * p * sw / safeDen2;

                if (mResponse == FilterResponse::Lowpass)
                {
                    return real(std::sqrt(hlpRe * hlpRe + hlpIm * hlpIm));
                }

                const double hhpRe = 1.0 - hlpRe;
                const double hhpIm = -hlpIm;
                return real(std::sqrt(hhpRe * hhpRe + hhpIm * hhpIm));
            }

            // float ProcessSample(float xn, FilterType type, float cutoffHz) {
            //     SetParams(type, cutoffHz, 0);
            //     return ProcessSample(xn);
            // }

            // all get written to directly by other filters; so yea.
            real2 m_alpha = 1; // Feed Forward coeff
            real2 m_beta = 0;
            real2 m_gamma = 1;     // Pre-Gain
            real2 m_delta = 0;     // FB_IN Coeff
            real2 m_epsilon = 0;   // FB_OUT scalar
            real2 m_a_0 = 1;       // input gain
            real2 m_feedbackL = 0; // our own feedback coeff from S ..... this is written to by DiodeFilter

        private:

            FilterResponse mResponse = FilterResponse::Lowpass;
            float m_cutoffHz = 10000;

            real2 m_z_1L = 0; // z-1 storage location, left/mono

            void Recalc()
            {
                // NB: LFOs use this filter so the cutoff should support VERY low frequencies with precision. fortunately single poles are fine with that.
                // TPT (topology-preserving transform) 1-pole integrator formula
                real2 cutoff = math::clamp(m_cutoffHz, 0, 20000);
                real2 wd = math::gPI * cutoff;
                real2 T = Helpers::CurrentSampleRateRecipF;
                //real wa = (2 / T) * math::tan(wd * T / 2);
                //real g = wa * T / 2;
                real2 g = real2(math::tan(float(wd * T)));
                m_alpha = g / (g + 1);
            }
        };


    } // namespace M7
} // namespace WaveSabreCore



