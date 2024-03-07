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

        Maj7::Maj7Voice::LFOVoice::LFOVoice(Maj7::LFODevice& device, ModMatrixNode& modMatrix) :
            mDevice(device),
            mNode(&modMatrix, &device.mDevice, nullptr)
        {}


		// why not make these static constexpr??
		LFOInfo gLFOInfo[gModLFOCount] = {
			{ParamIndices::LFO1Waveform, ModDestination::LFO1Waveshape, ModSource::LFO1},
			{ParamIndices::LFO2Waveform, ModDestination::LFO2Waveshape, ModSource::LFO2},
			{ParamIndices::LFO3Waveform, ModDestination::LFO3Waveshape, ModSource::LFO3},
			{ParamIndices::LFO4Waveform, ModDestination::LFO4Waveshape, ModSource::LFO4},
		};

		SourceInfo gSourceInfo[gSourceCount] = {
			{gModulationCount - 8, ParamIndices::Osc1Enabled, ParamIndices::Osc1AmpEnvDelayTime, ModSource::Osc1AmpEnv, ModDestination::Osc1Volume },
			{gModulationCount - 7, ParamIndices::Osc2Enabled, ParamIndices::Osc2AmpEnvDelayTime, ModSource::Osc2AmpEnv, ModDestination::Osc2Volume },
			{gModulationCount - 6, ParamIndices::Osc3Enabled, ParamIndices::Osc3AmpEnvDelayTime, ModSource::Osc3AmpEnv, ModDestination::Osc3Volume },
			{gModulationCount - 5,  ParamIndices::Osc4Enabled, ParamIndices::Osc4AmpEnvDelayTime, ModSource::Osc4AmpEnv, ModDestination::Osc4Volume },
			{ gModulationCount - 4, ParamIndices::Sampler1Enabled, ParamIndices::Sampler1AmpEnvDelayTime, ModSource::Sampler1AmpEnv, ModDestination::Sampler1Volume },
			{ gModulationCount - 3, ParamIndices::Sampler2Enabled, ParamIndices::Sampler2AmpEnvDelayTime, ModSource::Sampler2AmpEnv, ModDestination::Sampler2Volume },
			{ gModulationCount - 2, ParamIndices::Sampler3Enabled, ParamIndices::Sampler3AmpEnvDelayTime, ModSource::Sampler3AmpEnv, ModDestination::Sampler3Volume },
			{ gModulationCount - 1, ParamIndices::Sampler4Enabled, ParamIndices::Sampler4AmpEnvDelayTime, ModSource::Sampler4AmpEnv, ModDestination::Sampler4Volume },
		};

		EnvelopeInfo gEnvelopeInfo[gSourceCount + gModEnvCount] = {
			{ModDestination::Osc1AmpEnvDelayTime, ParamIndices::Osc1AmpEnvDelayTime, ModSource::Osc1AmpEnv },
			{ModDestination::Osc2AmpEnvDelayTime, ParamIndices::Osc2AmpEnvDelayTime, ModSource::Osc2AmpEnv },
			{ModDestination::Osc3AmpEnvDelayTime, ParamIndices::Osc3AmpEnvDelayTime, ModSource::Osc3AmpEnv },
			{ModDestination::Osc4AmpEnvDelayTime, ParamIndices::Osc4AmpEnvDelayTime, ModSource::Osc4AmpEnv },
			{ModDestination::Sampler1AmpEnvDelayTime, ParamIndices::Sampler1AmpEnvDelayTime, ModSource::Sampler1AmpEnv },
			{ModDestination::Sampler2AmpEnvDelayTime, ParamIndices::Sampler2AmpEnvDelayTime, ModSource::Sampler2AmpEnv },
			{ModDestination::Sampler3AmpEnvDelayTime, ParamIndices::Sampler3AmpEnvDelayTime, ModSource::Sampler3AmpEnv },
			{ModDestination::Sampler4AmpEnvDelayTime, ParamIndices::Sampler4AmpEnvDelayTime, ModSource::Sampler4AmpEnv },
			{ModDestination::Env1DelayTime, ParamIndices::Env1DelayTime, ModSource::ModEnv1 },
			{ModDestination::Env2DelayTime, ParamIndices::Env2DelayTime, ModSource::ModEnv2 },
		};

    } // namespace M7
} // namespace WaveSabreCore
