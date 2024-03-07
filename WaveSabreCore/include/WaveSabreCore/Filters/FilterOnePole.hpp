
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
            virtual void SetParams(FilterType type, real cutoffHz, real reso) override
            {
                if ((m_FilterType == type) && (m_cutoffHz == cutoffHz) && (m_q == reso))
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
                m_q = reso;
                Recalc();
            }

            virtual void Reset() override
            {
                m_z_1L = 0;
                m_feedbackL = 0;
            }

            real getFeedbackOutputL()
            {
                return m_beta * (m_z_1L + m_feedbackL * m_delta);
            }

            virtual real ProcessSample(real xn) override
            {
                // for diode filter support
                xn = xn * m_gamma + m_feedbackL + m_epsilon * getFeedbackOutputL();
                // calculate v(n)
                real vn = (m_a_0 * xn - m_z_1L) * m_alpha;
                // form LP output
                real lpf = vn + m_z_1L;
                // update memory
                m_z_1L = vn + lpf;
                if (m_FilterType == FilterType::LP2)
                {
                    return lpf;
                }
                auto hpf = xn - lpf;
                return hpf;
            }

            real ProcessSample(real xn, FilterType type, real cutoffHz) {
                SetParams(type, cutoffHz, 0);
                return ProcessSample(xn);
            }

            // all get written to directly by other filters; so yea.
            real m_alpha = 1; // Feed Forward coeff
            real m_beta = 0;
            real m_gamma = 1;     // Pre-Gain
            real m_delta = 0;     // FB_IN Coeff
            real m_epsilon = 0;   // FB_OUT scalar
            real m_a_0 = 1;       // input gain
            real m_feedbackL = 0; // our own feedback coeff from S ..... this is written to by DiodeFilter

        private:

            // NOTE: it's important that this is set to LP by default; it's what the Moog filter expects.
            FilterType m_FilterType = FilterType::LP2;
            real m_cutoffHz = 10000;
            real m_q = 0;

            real m_z_1L = 0; // z-1 storage location, left/mono

            void Recalc()
            {
                real wd = PITimes2 * m_cutoffHz;
                real T = Helpers::CurrentSampleRateRecipF;
                //real wa = (2 / T) * math::tan(wd * T / 2);
                //real g = wa * T / 2;
                float g = math::tan(wd * T * Real(0.5));
                m_alpha = g / (g + 1);
            }
        };


    } // namespace M7
} // namespace WaveSabreCore



