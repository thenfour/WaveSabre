#include "../GigaSynth/Maj7.hpp"


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
            ProcessBlock(outputs, numSamples, true);
        }

        Maj7::Maj7Voice::LFOVoice::LFOVoice(Maj7::LFODevice& device, ModMatrixNode& modMatrix) :
            mDevice(device),
            mNode(&device.mDevice, OscillatorIntention::LFO, &modMatrix, nullptr)
        {}


		// why not make these static constexpr??
		LFOInfo gLFOInfo[gModLFOCount] = {
			{GigaSynthParamIndices::LFO1Waveform, ModDestination::LFO1WaveshapeA, ModSource::LFO1},
			{GigaSynthParamIndices::LFO2Waveform, ModDestination::LFO2WaveshapeA, ModSource::LFO2},
			{GigaSynthParamIndices::LFO3Waveform, ModDestination::LFO3WaveshapeA, ModSource::LFO3},
			{GigaSynthParamIndices::LFO4Waveform, ModDestination::LFO4WaveshapeA, ModSource::LFO4},
		};

		SourceInfo gSourceInfo[gSourceCount] = {
			{gModulationCount - 8, GigaSynthParamIndices::Osc1Enabled, GigaSynthParamIndices::Osc1AmpEnvDelayTime, ModSource::Osc1AmpEnv, ModDestination::Osc1Volume },
			{gModulationCount - 7, GigaSynthParamIndices::Osc2Enabled, GigaSynthParamIndices::Osc2AmpEnvDelayTime, ModSource::Osc2AmpEnv, ModDestination::Osc2Volume },
			{gModulationCount - 6, GigaSynthParamIndices::Osc3Enabled, GigaSynthParamIndices::Osc3AmpEnvDelayTime, ModSource::Osc3AmpEnv, ModDestination::Osc3Volume },
			{gModulationCount - 5,  GigaSynthParamIndices::Osc4Enabled, GigaSynthParamIndices::Osc4AmpEnvDelayTime, ModSource::Osc4AmpEnv, ModDestination::Osc4Volume },
#ifdef ENABLE_SAMPLER_DEVICE
			{ gModulationCount - 4, GigaSynthParamIndices::Sampler1Enabled, GigaSynthParamIndices::Sampler1AmpEnvDelayTime, ModSource::Sampler1AmpEnv, ModDestination::Sampler1Volume },
			{ gModulationCount - 3, GigaSynthParamIndices::Sampler2Enabled, GigaSynthParamIndices::Sampler2AmpEnvDelayTime, ModSource::Sampler2AmpEnv, ModDestination::Sampler2Volume },
			{ gModulationCount - 2, GigaSynthParamIndices::Sampler3Enabled, GigaSynthParamIndices::Sampler3AmpEnvDelayTime, ModSource::Sampler3AmpEnv, ModDestination::Sampler3Volume },
			{ gModulationCount - 1, GigaSynthParamIndices::Sampler4Enabled, GigaSynthParamIndices::Sampler4AmpEnvDelayTime, ModSource::Sampler4AmpEnv, ModDestination::Sampler4Volume },
#endif  // ENABLE_SAMPLER_DEVICE
		};

		EnvelopeInfo gEnvelopeInfo[gSourceCount + gModEnvCount] = {
			{ModDestination::Osc1AmpEnvDelayTime, GigaSynthParamIndices::Osc1AmpEnvDelayTime, ModSource::Osc1AmpEnv },
			{ModDestination::Osc2AmpEnvDelayTime, GigaSynthParamIndices::Osc2AmpEnvDelayTime, ModSource::Osc2AmpEnv },
			{ModDestination::Osc3AmpEnvDelayTime, GigaSynthParamIndices::Osc3AmpEnvDelayTime, ModSource::Osc3AmpEnv },
			{ModDestination::Osc4AmpEnvDelayTime, GigaSynthParamIndices::Osc4AmpEnvDelayTime, ModSource::Osc4AmpEnv },
#ifdef ENABLE_SAMPLER_DEVICE
			{ModDestination::Sampler1AmpEnvDelayTime, GigaSynthParamIndices::Sampler1AmpEnvDelayTime, ModSource::Sampler1AmpEnv },
			{ModDestination::Sampler2AmpEnvDelayTime, GigaSynthParamIndices::Sampler2AmpEnvDelayTime, ModSource::Sampler2AmpEnv },
			{ModDestination::Sampler3AmpEnvDelayTime, GigaSynthParamIndices::Sampler3AmpEnvDelayTime, ModSource::Sampler3AmpEnv },
			{ModDestination::Sampler4AmpEnvDelayTime, GigaSynthParamIndices::Sampler4AmpEnvDelayTime, ModSource::Sampler4AmpEnv },
#endif  // ENABLE_SAMPLER_DEVICE
			{ModDestination::Env1DelayTime, GigaSynthParamIndices::Env1DelayTime, ModSource::ModEnv1 },
			{ModDestination::Env2DelayTime, GigaSynthParamIndices::Env2DelayTime, ModSource::ModEnv2 },
			{ModDestination::Env3DelayTime, GigaSynthParamIndices::Env3DelayTime, ModSource::ModEnv3 },
			{ModDestination::Env4DelayTime, GigaSynthParamIndices::Env4DelayTime, ModSource::ModEnv4 },
		};

    } // namespace M7
} // namespace WaveSabreCore
