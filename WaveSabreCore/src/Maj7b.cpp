#include <WaveSabreCore/Maj7.hpp>
namespace WaveSabreCore {
    namespace M7 {
        Maj7::LFODevice::LFODevice(float* paramCache, size_t ilfo) :
            mInfo(gLFOInfo[ilfo]),
            mDevice{ paramCache, ilfo }
        {}
        // when you load params or defaults, you need to seed voice initial states so that
        // when for example a NoteOn happens, the voice will have correct values in its mod matrix.
        void Maj7::SetVoiceInitialStates()
        {
            int numSamples = GetModulationRecalcSampleMask() + 1;
            float x[gModulationRecalcSampleMaskValues[0] * 2];
            memset(x, 0, sizeof(x));
            float* outputs[2] = { x, x };
            ProcessBlock(0, outputs, numSamples, true);
        }

    } // namespace M7
} // namespace WaveSabreCore
