
// "Designing Software Synthesizer Plug-Ins in C++" // https://willpirkle.com

#pragma once

#include "FilterOnePole.hpp"

namespace WaveSabreCore
{
    namespace M7
    {
        struct MoogLadderFilter : IFilter
        {
            // assumes that lpfs are initialized with LP

            virtual void SetParams(FilterType type, real cutoffHz, real reso) override
            {
                bool recalc = false;
                if (m_FilterType != type) {
                    m_FilterType = type;
                    recalc = true;
                }
                if (cutoffHz != m_cutoffHz) {
                    m_cutoffHz = cutoffHz;
                    recalc = true;
                }
                if (reso != m_resonance) {
                    m_resonance = reso;
                    // this maps dQControl = 0->1 to 0-4 * 0.97 to avoid clippy self oscillation
                    static const auto t88 = Real(3.88);
                    m_k = t88 * reso;
                    // this is not smaller code; use clamp.
                    //if (m_k < 0) m_k = 0;
                    //if (m_k > t88) m_k = t88;
                    m_k = math::clamp(m_k, Real0, Real(3.88));
                    recalc = true;
                }
                if (recalc) Recalc();
            }

            virtual real ProcessSample(real x) override
            {
                return InlineProcessSample(x);
            }

            virtual void Reset() override
            {
                for (auto& lpf : m_LPF) {
                    lpf.Reset();
                }
            }

            inline real InlineProcessSample(real xn)
            {
                real dSigma = 0;
                for (auto& lpf : m_LPF) {
                    dSigma += lpf.getFeedbackOutputL();
                }

                // calculate input to first filter
                float dLP[5];
                dLP[0] = (xn - m_k * dSigma) * m_alpha_0;

                static const int8_t letterVals[6][5] = {
                    {0,0,1,0,0}, // lp2
                    {0,0,0,0,1}, // lp4
                    {0,2,-2,0,0}, // bp2
                    {0,0,4,-8,4}, // bp4
                    {1,-2,1,0,0}, // hp2
                    {1,-4,6,-4,1}, // hp4
                };

                // --- cascade of 4 filters
                float output = 0;
                for (size_t i = 0; i < 4; ++i) {
                    dLP[i + 1] = m_LPF[i].ProcessSample(dLP[i]);
                    output += dLP[i] * letterVals[(size_t)m_FilterType][i];
                }
                output += dLP[4] * letterVals[(size_t)m_FilterType][4];

                return output;
            }

        private:
            void Recalc()
            {
                // prewarp for BZT
                real wd = PITimes2 * m_cutoffHz;

                // note: measured input to tan function, it seemed limited to (0.005699, 1.282283).
                // input for fasttan shall be limited to (-pi/2, pi/2) according to documentation
                //real wa = (2 * Helpers::CurrentSampleRateF) * math::tan(wd * Helpers::CurrentSampleRateRecipF * Real(0.5));
                //real g = wa * Helpers::CurrentSampleRateRecipF * Real(0.5);

                real g = math::tan(wd * Helpers::CurrentSampleRateRecipF * Real(0.5));

                // G - the feedforward coeff in the VA One Pole
                //     same for LPF, HPF
                const real oneOver1plusg = 1.0f / (g + 1);
                real G = g * oneOver1plusg;

                m_gamma = 1;
                for (int i = 3; i >= 0; --i) {
                    m_LPF[i].m_alpha = G;
                    m_LPF[i].m_beta = m_gamma * oneOver1plusg;
                    m_gamma *= G;
                }

                m_alpha_0 = Real1 / (m_k * m_gamma + 1);
            }

            OnePoleFilter m_LPF[4];

            FilterType m_FilterType = FilterType::LP2;
            real m_alpha_0 = 1; // see block diagram
            real m_cutoffHz = 0;
            real m_resonance = 0;// Real(-1); // cached resonance for knowing when recalc is not needed.
            real m_k = 0;       // K, set with Q
            real m_gamma = 0;       // see block diagram
        };
    } // namespace M7
} // namespace WaveSabreCore



