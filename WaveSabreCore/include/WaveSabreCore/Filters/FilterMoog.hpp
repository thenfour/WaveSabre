
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
            //MoogLadderFilter()
            //{
            //    for (auto& lpf : m_LPF) {
            //        lpf.SetParams(FilterType::LP2, 0, 0);
            //    }
            //}

            //virtual void SetResonance(real p_res)
            //{
            //    if (math::FloatEquals(p_res, m_resonance))
            //        return;
            //    m_resonance = p_res;
            //    // this maps dQControl = 0->1 to 0-4 * 0.97 to avoid clippy self oscillation
            //    static const auto t88 = Real(3.88);
            //    m_k = t88 * p_res;
            //    // this is not smaller.
            //    //if (m_k < 0) m_k = 0;
            //    //if (m_k > t88) m_k = t88;
            //    m_k = math::clamp(m_k, Real0, Real(3.88));
            //    Recalc();
            //}

            virtual void SetParams(FilterType type, real cutoffHz, real reso) override
            {
                bool recalc = false;
                if (m_FilterType != type) {
                    m_FilterType = type;
                    recalc = true;
                }
                if (!math::FloatEquals(cutoffHz, m_cutoffHz)) {
                    m_cutoffHz = cutoffHz;
                    recalc = true;
                }
                if (!math::FloatEquals(reso, m_resonance)) {
                    m_resonance = reso;
                    // this maps dQControl = 0->1 to 0-4 * 0.97 to avoid clippy self oscillation
                    static const auto t88 = Real(3.88);
                    m_k = t88 * reso;
                    // this is not smaller.
                    //if (m_k < 0) m_k = 0;
                    //if (m_k > t88) m_k = t88;
                    m_k = math::clamp(m_k, Real0, Real(3.88));
                    recalc = true;
                }
                if (recalc) Recalc();


                //if (!math::FloatEquals(m_cutoffHz, cutoffHz))
                //{
                //    return;
                //}

                ////m_overdrive = saturation;
                //m_resonance = reso;
                //m_k = Real(3.88) * reso;
                //m_k = math::clamp(m_k, Real0, Real(3.88));
                //Recalc();
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

                // --- Oberheim variations
                //real output = m_a * dU + m_b * dLP1 + m_c * dLP2 + m_d * dLP3 + m_e * dLP4;
                //real output = m_a * dLP[0] + m_b * dLP[1] + m_c * dLP[2] + m_d * dLP[3] + m_e * dLP[4];

                //applyOverdrive(output, m_overdrive, Real(3.5));

                return output;
            }

            //inline void InlineProcessSample(real& xnL, real& xnR)
            //{
            //    real dSigmaL = m_LPF1.getFeedbackOutputL() + m_LPF2.getFeedbackOutputL() + m_LPF3.getFeedbackOutputL() +
            //        m_LPF4.getFeedbackOutputL();
            //    real dSigmaR = m_LPF1.getFeedbackOutputR() + m_LPF2.getFeedbackOutputR() + m_LPF3.getFeedbackOutputR() +
            //        m_LPF4.getFeedbackOutputR();

            //    // calculate input to first filter
            //    real dUL = (xnL - m_k * dSigmaL) * m_alpha_0;
            //    real dUR = (xnR - m_k * dSigmaR) * m_alpha_0;

            //    // --- cascade of 4 filters
            //    real dLP1L = dUL, dLP1R = dUR;
            //    m_LPF1.InlineProcessSample(dLP1L, dLP1R);
            //    real dLP2L = dLP1L, dLP2R = dLP1R;
            //    m_LPF2.InlineProcessSample(dLP2L, dLP2R);
            //    real dLP3L = dLP2L, dLP3R = dLP2R;
            //    m_LPF3.InlineProcessSample(dLP3L, dLP3R);
            //    real dLP4L = dLP3L, dLP4R = dLP3R;
            //    m_LPF4.InlineProcessSample(dLP4L, dLP4R);

            //    // --- Oberheim variations
            //    real outputL = m_a * dUL + m_b * dLP1L + m_c * dLP2L + m_d * dLP3L + m_e * dLP4L;
            //    real outputR = m_a * dUR + m_b * dLP1R + m_c * dLP2R + m_d * dLP3R + m_e * dLP4R;

            //    applyOverdrive(outputL, outputR, m_overdrive, Real(3.5));
            //    xnL = outputL;
            //    xnR = outputR;
            //}

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
                //real g = wa * Helpers::CurrentSampleRateRecipF * Real(0.5);

                // wa = 2 * sr * tan
                // g = wa / sr * 

                // G - the feedforward coeff in the VA One Pole
                //     same for LPF, HPF
                const real oneOver1plusg = 1.0f / (g + 1);
                real G = g * oneOver1plusg;

                // set alphas
                //for (auto& lpf : m_LPF) {
                //    lpf.m_alpha = G;
                //}
                //m_LPF1.m_alpha = G;
                //m_LPF2.m_alpha = G;
                //m_LPF3.m_alpha = G;
                //m_LPF4.m_alpha = G;

                m_gamma = 1;
                for (int i = 3; i >= 0; --i) {
                    m_LPF[i].m_alpha = G;
                    m_LPF[i].m_beta = m_gamma * oneOver1plusg;
                    m_gamma *= G;
                }

                // set betas
                //const real GG = G * G;
                //m_LPF1.m_beta = G * G * G * oneOver1plusg;
                //m_LPF2.m_beta = G * G * oneOver1plusg;
                //m_LPF3.m_beta = G * oneOver1plusg;
                //m_LPF4.m_beta = oneOver1plusg;

                //m_gamma = G * G * G * G;

                m_alpha_0 = Real1 / (m_k * m_gamma + 1);

            //    // Oberheim variation
            //    switch (m_FilterType)
            //    {
            //    case FilterType::LP4:
            //        mLetters[0] = Real(0.0);
            //        mLetters[1] = Real(0.0);
            //        mLetters[2] = Real(0.0);
            //        mLetters[3] = Real(0.0);
            //        mLetters[4] = Real(1.0);
            //        break;
            //    case FilterType::LP2:
            //        mLetters[0] = Real(0.0);
            //        mLetters[1] = Real(0.0);
            //        mLetters[2] = Real(1.0);
            //        mLetters[3] = Real(0.0);
            //        mLetters[4] = Real(0.0);
            //        break;
            //    case FilterType::BP4:
            //        mLetters[0] = Real(0.0);
            //        mLetters[1] = Real(0.0);
            //        mLetters[2] = Real(4.0);
            //        mLetters[3] = Real(-8.0);
            //        mLetters[4] = Real(4.0);
            //        break;
            //    case FilterType::BP2:
            //        mLetters[0] = Real(0.0);
            //        mLetters[1] = Real(2.0);
            //        mLetters[2] = Real(-2.0);
            //        mLetters[3] = Real(0.0);
            //        mLetters[4] = Real(0.0);
            //        break;
            //    case FilterType::HP4:
            //        mLetters[0] = Real(1.0);
            //        mLetters[1] = Real(-4.0);
            //        mLetters[2] = Real(6.0);
            //        mLetters[3] = Real(-4.0);
            //        mLetters[4] = Real(1.0);
            //        break;
            //    case FilterType::HP2:
            //        mLetters[0] = Real(1.0);
            //        mLetters[1] = Real(-2.0);
            //        mLetters[2] = Real(1.0);
            //        mLetters[3] = Real(0.0);
            //        mLetters[4] = Real(0.0);
            //        break;
            //    }
            }

            //real m_overdrive = 0;


            OnePoleFilter m_LPF[4];

            FilterType m_FilterType = FilterType::LP2;
            real m_alpha_0 = 1; // see block diagram
            real m_cutoffHz = 0;
            real m_resonance = 0;// Real(-1); // cached resonance for knowing when recalc is not needed.
            real m_k = 0;       // K, set with Q
            real m_gamma = 0;       // see block diagram

            // Oberheim Xpander variations
            //real m_a = 0;
            //real m_b = 0;
            //real m_c = 0;
            //real m_d = 0;
            //real m_e = 0;
            //uint8_t mLetters[5];

        };
    } // namespace M7
} // namespace WaveSabreCore



