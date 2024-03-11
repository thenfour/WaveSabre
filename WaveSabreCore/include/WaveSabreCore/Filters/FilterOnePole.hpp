
// "Designing Software Synthesizer Plug-Ins in C++" // https://willpirkle.com

#pragma once

#include "FilterBase.hpp"
#include <complex>

namespace WaveSabreCore
{
    namespace M7
    {
        struct OnePoleFilter : public IFilter
        {
            virtual void SetParams(FilterType type, float cutoffHz, float reso) override
            {
                if ((m_FilterType == type) && (m_cutoffHz == cutoffHz))
                {
                    return;
                }
                switch (type)
                {
                default:
                //se FilterType::LP:
                case FilterType::LP2:
                case FilterType::LP4:
                    m_FilterType = FilterType::LP2;
                    //Recalc();
                    break;
                //se FilterType::HP:
                case FilterType::HP2:
                case FilterType::HP4:
                    m_FilterType = FilterType::HP2;
                    //Recalc();
                    break;
                }

                m_cutoffHz = cutoffHz;
                Recalc();
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
                if (m_FilterType == FilterType::LP2)
                {
                    return float(lpf);
                }
                auto hpf = xn - lpf;
                return float(hpf);
            }

            float ProcessSample(float xn, FilterType type, float cutoffHz) {
                SetParams(type, cutoffHz, 0);
                return ProcessSample(xn);
            }

            // all get written to directly by other filters; so yea.
            real2 m_alpha = 1; // Feed Forward coeff
            real2 m_beta = 0;
            real2 m_gamma = 1;     // Pre-Gain
            real2 m_delta = 0;     // FB_IN Coeff
            real2 m_epsilon = 0;   // FB_OUT scalar
            real2 m_a_0 = 1;       // input gain
            real2 m_feedbackL = 0; // our own feedback coeff from S ..... this is written to by DiodeFilter

        private:

            // NOTE: it's important that this is set to LP by default; it's what the Moog filter expects.
            FilterType m_FilterType = FilterType::LP2;
            float m_cutoffHz = 10000;

            real2 m_z_1L = 0; // z-1 storage location, left/mono

            void Recalc()
            {
                // NB: LFOs use this filter so the cutoff should support VERY low frequencies with precision. fortunately single poles are fine with that.
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



