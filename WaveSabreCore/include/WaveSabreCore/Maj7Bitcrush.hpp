// https://www.musicdsp.org/en/latest/Effects/114-waveshaper-simple-description.html
// https://www.musicdsp.org/en/latest/Effects/46-waveshaper.html

#pragma once

#include <WaveSabreCore/Maj7Basic.hpp>
#include "Maj7ModMatrix.hpp"

namespace WaveSabreCore
{
    namespace M7
    {
        static constexpr float gBitcrushFreqCenterFreq = 1000;
        static constexpr float gBitcrushFreqRange = 10;

        // very naive sample crushing. there are many ways to make a cleaner signal (blep, oversampling, filtering, interpolation),
        // but gritty broken-sounding is kinda what crushing is all about, plus very tiny code size.
        struct BitcrushAuxNode : IAuxEffect
        {
            FrequencyParam mFreqParam;
            int mModDestParam2ID;

            float prevValue = 0;
            float currentValue = 0;
            double t = 0;
            double dt = 0;
            //double ddt = 0;

            BitcrushAuxNode(float* auxParams, int modDestParam2ID) :
                // !! do not SET initial values; these get instantiated dynamically.
                mFreqParam(auxParams[(int)BitcrushAuxParamIndexOffsets::Freq], auxParams[(int)BitcrushAuxParamIndexOffsets::FreqKT], gBitcrushFreqCenterFreq, gBitcrushFreqRange),
                mModDestParam2ID(modDestParam2ID)
            {
            }

            virtual void AuxBeginBlock(float noteHz, ModMatrixNode& modMatrix) override
            {
                double freq = mFreqParam.GetFrequency(noteHz, modMatrix.GetDestinationValue(mModDestParam2ID + (int)BitcrushAuxModIndexOffsets::Freq));
                dt = freq / Helpers::CurrentSampleRate;
                //ddt = (newdt - dt) / nSamples;
            }

            virtual float AuxProcessSample(float s) override
            {
                //dt += ddt;
                auto newphase = t + dt;
                if (newphase > 1) {
                    t = newphase - 1;
                    currentValue = s;
                }
                else {
                    t = newphase;
                }
                return currentValue;
            }
        };

    } // namespace M7


} // namespace WaveSabreCore




