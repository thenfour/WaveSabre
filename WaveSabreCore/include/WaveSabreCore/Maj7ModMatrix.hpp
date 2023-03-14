#pragma once

#include <WaveSabreCore/Maj7Basic.hpp>

namespace WaveSabreCore
{
	namespace M7
	{
		enum class ModulationRate : uint8_t
		{
			Disabled,
			KRate, // evaluated each buffer.
			ARate,
			Count,
		};

		enum class ModulationPolarity : uint8_t
		{
			Positive01,
			N11,
			Count,
		};

		enum class ModSource : uint8_t
		{
			None,
			Osc1AmpEnv, // arate, 01
			Osc2AmpEnv, // arate, 01
			Osc3AmpEnv, // arate, 01
			Osc4AmpEnv,
			Sampler1AmpEnv,
			Sampler2AmpEnv,
			Sampler3AmpEnv,
			Sampler4AmpEnv,
			ModEnv1, // arate, 01
			ModEnv2, // arate, 01
			LFO1, // arate, N11
			LFO2, // arate, N11
			PitchBend, // krate, N11
			Velocity, // krate, 01
			NoteValue, // krate, 01
			RandomTrigger, // krate, 01
			UnisonoVoice, // krate, 01
			SustainPedal, // krate, 01
			Macro1, // krate, 01
			Macro2, // krate, 01
			Macro3, // krate, 01
			Macro4, // krate, 01
			Macro5, // krate, 01
			Macro6, // krate, 01
			Macro7, // krate, 01

			Count,
			Invalid,
		};

		// size-optimize using macro
#define MOD_SOURCE_CAPTIONS { \
			"None", \
			"Osc1AmpEnv", \
			"Osc2AmpEnv", \
			"Osc3AmpEnv", \
			"Osc4AmpEnv", \
			"Sampler1AmpEnv", \
			"Sampler2AmpEnv", \
			"Sampler3AmpEnv", \
			"Sampler4AmpEnv", \
			"ModEnv1", \
			"ModEnv2", \
			"LFO1", \
			"LFO2", \
			"PitchBend", \
			"Velocity", \
			"NoteValue", \
			"RandomTrigger", \
			"UnisonoVoice", \
			"SustainPedal", \
			"Macro1", \
			"Macro2", \
			"Macro3", \
			"Macro4", \
			"Macro5", \
			"Macro6", \
			"Macro7", \
		}

		enum class ModDestination : uint8_t
		{
			None,
			FMBrightness, // krate, 01
			PortamentoTime, // krate, 01

			// NB!! the order of these must 1) be the same for all envelopes, and 2) stay in sync with the order expected by EnvelopeModulationValues::Fill
			Osc1Volume, // arate, 01 // KEEP IN SYNC WITH OscModParamIndexOffsets
			Osc1PreFMVolume, // KEEP IN SYNC WITH OscModParamIndexOffsets
			Osc1Waveshape, // arate, 01 // KEEP IN SYNC WITH OscModParamIndexOffsets
			Osc1SyncFrequency, // arate, 01 // KEEP IN SYNC WITH OscModParamIndexOffsets
			Osc1FrequencyParam, // arate, 01 // KEEP IN SYNC WITH OscModParamIndexOffsets
			Osc1PitchFine, // arate, 01 // KEEP IN SYNC WITH OscModParamIndexOffsets
			Osc1FMFeedback, // arate, 01 // KEEP IN SYNC WITH OscModParamIndexOffsets
			Osc1Phase, // krate, N11 // KEEP IN SYNC WITH OscModParamIndexOffsets
			Osc1AuxMix, // KEEP IN SYNC WITH OscModParamIndexOffsets

			// NB!! the order of these must 1) be the same for all envelopes, and 2) stay in sync with the order expected by 
			Osc1AmpEnvDelayTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Osc1AmpEnvAttackTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Osc1AmpEnvAttackCurve, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Osc1AmpEnvHoldTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Osc1AmpEnvDecayTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Osc1AmpEnvDecayCurve, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Osc1AmpEnvSustainLevel, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Osc1AmpEnvReleaseTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Osc1AmpEnvReleaseCurve, // KEEP IN SYNC WITH EnvModParamIndexOffsets

			Osc2Volume, // KEEP IN SYNC WITH OscModParamIndexOffsets
			Osc2PreFMVolume, // KEEP IN SYNC WITH OscModParamIndexOffsets
			Osc2Waveshape, // KEEP IN SYNC WITH OscModParamIndexOffsets
			Osc2SyncFrequency, // KEEP IN SYNC WITH OscModParamIndexOffsets
			Osc2FrequencyParam, // KEEP IN SYNC WITH OscModParamIndexOffsets
			Osc2PitchFine, // arate, 01 // KEEP IN SYNC WITH OscModParamIndexOffsets
			Osc2FMFeedback, // KEEP IN SYNC WITH OscModParamIndexOffsets
			Osc2Phase, // KEEP IN SYNC WITH OscModParamIndexOffsets
			Osc2AuxMix, // KEEP IN SYNC WITH OscModParamIndexOffsets

			Osc2AmpEnvDelayTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Osc2AmpEnvAttackTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Osc2AmpEnvAttackCurve, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Osc2AmpEnvHoldTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Osc2AmpEnvDecayTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Osc2AmpEnvDecayCurve, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Osc2AmpEnvSustainLevel, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Osc2AmpEnvReleaseTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Osc2AmpEnvReleaseCurve, // KEEP IN SYNC WITH EnvModParamIndexOffsets

			Osc3Volume, // KEEP IN SYNC WITH OscModParamIndexOffsets
			Osc3PreFMVolume, // KEEP IN SYNC WITH OscModParamIndexOffsets
			Osc3Waveshape, // KEEP IN SYNC WITH OscModParamIndexOffsets
			Osc3SyncFrequency, // KEEP IN SYNC WITH OscModParamIndexOffsets
			Osc3FrequencyParam, // KEEP IN SYNC WITH OscModParamIndexOffsets
			Osc3PitchFine, // arate, 01 // KEEP IN SYNC WITH OscModParamIndexOffsets
			Osc3FMFeedback, // KEEP IN SYNC WITH OscModParamIndexOffsets
			Osc3Phase, // KEEP IN SYNC WITH OscModParamIndexOffsets
			Osc3AuxMix, // KEEP IN SYNC WITH OscModParamIndexOffsets

			Osc3AmpEnvDelayTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Osc3AmpEnvAttackTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Osc3AmpEnvAttackCurve, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Osc3AmpEnvHoldTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Osc3AmpEnvDecayTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Osc3AmpEnvDecayCurve, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Osc3AmpEnvSustainLevel, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Osc3AmpEnvReleaseTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Osc3AmpEnvReleaseCurve, // KEEP IN SYNC WITH EnvModParamIndexOffsets

			Osc4Volume, // KEEP IN SYNC WITH OscModParamIndexOffsets
			Osc4PreFMVolume, // KEEP IN SYNC WITH OscModParamIndexOffsets
			Osc4Waveshape, // KEEP IN SYNC WITH OscModParamIndexOffsets
			Osc4SyncFrequency, // KEEP IN SYNC WITH OscModParamIndexOffsets
			Osc4FrequencyParam, // KEEP IN SYNC WITH OscModParamIndexOffsets
			Osc4PitchFine, // arate, 01 // KEEP IN SYNC WITH OscModParamIndexOffsets
			Osc4FMFeedback, // KEEP IN SYNC WITH OscModParamIndexOffsets
			Osc4Phase, // KEEP IN SYNC WITH OscModParamIndexOffsets
			Osc4AuxMix, // KEEP IN SYNC WITH OscModParamIndexOffsets

			Osc4AmpEnvDelayTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Osc4AmpEnvAttackTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Osc4AmpEnvAttackCurve, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Osc4AmpEnvHoldTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Osc4AmpEnvDecayTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Osc4AmpEnvDecayCurve, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Osc4AmpEnvSustainLevel, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Osc4AmpEnvReleaseTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Osc4AmpEnvReleaseCurve, // KEEP IN SYNC WITH EnvModParamIndexOffsets

			Env1DelayTime, // krate, 01
			Env1AttackTime, // krate, 01
			Env1AttackCurve, // krate, N11
			Env1HoldTime, // krate, 01
			Env1DecayTime, // krate, 01
			Env1DecayCurve, // krate, N11
			Env1SustainLevel, // krate, 01
			Env1ReleaseTime, // krate, 01
			Env1ReleaseCurve, // krate, N11

			Env2DelayTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Env2AttackTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Env2AttackCurve, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Env2HoldTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Env2DecayTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Env2DecayCurve, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Env2SustainLevel, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Env2ReleaseTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Env2ReleaseCurve, // KEEP IN SYNC WITH EnvModParamIndexOffsets

			LFO1Waveshape,
			LFO1FrequencyParam,

			LFO2Waveshape, // krate, 01
			LFO2FrequencyParam, // krate, 01

			FMAmt2to1, // arate, 01
			FMAmt3to1, // arate, 01
			FMAmt4to1, // arate, 01
			FMAmt1to2, // arate, 01
			FMAmt3to2, // arate, 01
			FMAmt4to2, // arate, 01
			FMAmt1to3, // arate, 01
			FMAmt2to3, // arate, 01
			FMAmt4to3, // arate, 01
			FMAmt1to4, // arate, 01
			FMAmt2to4, // arate, 01
			FMAmt3to4, // arate, 01

			// aux param 1 is not a mod destination. just keeps things slightly more compact and there's ALWAYS 1 param like this anyway so it's not necessary.
			// typically param 1 is an enum like FilterType or DistortionType.
			Aux1Param2,
			Aux1Param3,
			Aux1Param4,
			Aux1Param5,
			Aux2Param2,
			Aux2Param3,
			Aux2Param4,
			Aux2Param5,
			Aux3Param2,
			Aux3Param3,
			Aux3Param4,
			Aux3Param5,
			Aux4Param2,
			Aux4Param3,
			Aux4Param4,
			Aux4Param5,


			Sampler1Volume, //// KEEP IN SYNC WITH SamplerModParamIndexOffsets
			Sampler1HiddenVolume, //// KEEP IN SYNC WITH SamplerModParamIndexOffsets
			Sampler1PitchFine, //// KEEP IN SYNC WITH SamplerModParamIndexOffsets
			Sampler1FrequencyParam, // KEEP IN SYNC WITH SamplerModParamIndexOffsets
			Sampler1AuxMix, // KEEP IN SYNC WITH SamplerModParamIndexOffsets

			Sampler1AmpEnvDelayTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Sampler1AmpEnvAttackTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Sampler1AmpEnvAttackCurve, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Sampler1AmpEnvHoldTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Sampler1AmpEnvDecayTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Sampler1AmpEnvDecayCurve, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Sampler1AmpEnvSustainLevel, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Sampler1AmpEnvReleaseTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Sampler1AmpEnvReleaseCurve, // KEEP IN SYNC WITH EnvModParamIndexOffsets


			Sampler2Volume, //// KEEP IN SYNC WITH SamplerModParamIndexOffsets
			Sampler2HiddenVolume, //// KEEP IN SYNC WITH SamplerModParamIndexOffsets
			Sampler2PitchFine, //// KEEP IN SYNC WITH SamplerModParamIndexOffsets
			Sampler2FrequencyParam, // KEEP IN SYNC WITH SamplerModParamIndexOffsets
			Sampler2AuxMix, // KEEP IN SYNC WITH SamplerModParamIndexOffsets

			Sampler2AmpEnvDelayTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Sampler2AmpEnvAttackTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Sampler2AmpEnvAttackCurve, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Sampler2AmpEnvHoldTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Sampler2AmpEnvDecayTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Sampler2AmpEnvDecayCurve, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Sampler2AmpEnvSustainLevel, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Sampler2AmpEnvReleaseTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Sampler2AmpEnvReleaseCurve, // KEEP IN SYNC WITH EnvModParamIndexOffsets


			Sampler3Volume, //// KEEP IN SYNC WITH SamplerModParamIndexOffsets
			Sampler3HiddenVolume, //// KEEP IN SYNC WITH SamplerModParamIndexOffsets
			Sampler3PitchFine, //// KEEP IN SYNC WITH SamplerModParamIndexOffsets
			Sampler3FrequencyParam, // KEEP IN SYNC WITH SamplerModParamIndexOffsets
			Sampler3AuxMix, // KEEP IN SYNC WITH SamplerModParamIndexOffsets

			Sampler3AmpEnvDelayTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Sampler3AmpEnvAttackTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Sampler3AmpEnvAttackCurve, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Sampler3AmpEnvHoldTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Sampler3AmpEnvDecayTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Sampler3AmpEnvDecayCurve, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Sampler3AmpEnvSustainLevel, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Sampler3AmpEnvReleaseTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Sampler3AmpEnvReleaseCurve, // KEEP IN SYNC WITH EnvModParamIndexOffsets


			Sampler4Volume, //// KEEP IN SYNC WITH SamplerModParamIndexOffsets
			Sampler4HiddenVolume, //// KEEP IN SYNC WITH SamplerModParamIndexOffsets
			Sampler4PitchFine, //// KEEP IN SYNC WITH SamplerModParamIndexOffsets
			Sampler4FrequencyParam, // KEEP IN SYNC WITH SamplerModParamIndexOffsets
			Sampler4AuxMix, // KEEP IN SYNC WITH SamplerModParamIndexOffsets

			Sampler4AmpEnvDelayTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Sampler4AmpEnvAttackTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Sampler4AmpEnvAttackCurve, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Sampler4AmpEnvHoldTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Sampler4AmpEnvDecayTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Sampler4AmpEnvDecayCurve, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Sampler4AmpEnvSustainLevel, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Sampler4AmpEnvReleaseTime, // KEEP IN SYNC WITH EnvModParamIndexOffsets
			Sampler4AmpEnvReleaseCurve, // KEEP IN SYNC WITH EnvModParamIndexOffsets

			Count,
			Invalid,
		};
		enum class EnvModParamIndexOffsets : uint8_t // MUST BE IN SYNC WITH ABOVE
		{
			DelayTime,
			AttackTime,
			AttackCurve,
			HoldTime,
			DecayTime,
			DecayCurve,
			SustainLevel,
			ReleaseTime,
			ReleaseCurve,
		};
		enum class OscModParamIndexOffsets : uint8_t // MUST BE IN SYNC WITH ABOVE
		{
			Volume,
			PreFMVolume,
			Waveshape,
			SyncFrequency,
			FrequencyParam,
			PitchFine, // arate, 01 // KEEP IN SYNC WITH OscModParamIndexOffsets
			FMFeedback,
			Phase,
			AuxMix,
		};

		enum class SamplerModParamIndexOffsets : uint8_t // MUST BE IN SYNC WITH ABOVE
		{
			Volume, //// KEEP IN SYNC WITH SamplerModParamIndexOffsets
			HiddenVolume, //// KEEP IN SYNC WITH SamplerModParamIndexOffsets
			PitchFine, //// KEEP IN SYNC WITH SamplerModParamIndexOffsets
			FrequencyParam, // KEEP IN SYNC WITH SamplerModParamIndexOffsets
			AuxMix, // KEEP IN SYNC WITH SamplerModParamIndexOffsets
		};

		enum class LFOModParamIndexOffsets : uint8_t // MUST BE IN SYNC WITH ABOVE
		{
			Waveshape,
			FrequencyParam,
		};

		// size-optimize using macro
#define MOD_DEST_CAPTIONS { \
			"None", \
			"FMBrightness", \
			"PortamentoTime", \
			"Osc1Volume", \
			"Osc1PreFMVolume", \
			"Osc1Waveshape", \
			"Osc1SyncFrequency", \
			"Osc1FrequencyParam", \
			"Osc1PitchFine", \
			"Osc1FMFeedback", \
			"Osc1Phase", \
			"Osc1AuxMix", \
			"Osc1AmpEnvDelayTime",  \
			"Osc1AmpEnvAttackTime", \
			"Osc1AmpEnvAttackCurve", \
			"Osc1AmpEnvHoldTime", \
			"Osc1AmpEnvDecayTime", \
			"Osc1AmpEnvDecayCurve", \
			"Osc1AmpEnvSustainLevel", \
			"Osc1AmpEnvReleaseTime", \
			"Osc1AmpEnvReleaseCurve", \
		"Osc2Volume", \
			"Osc2PreFMVolume", \
			"Osc2Waveshape", \
			"Osc2SyncFrequency", \
			"Osc2FrequencyParam", \
			"Osc2PitchFine", \
			"Osc2FMFeedback", \
			"Osc2Phase", \
			"Osc2AuxMix", \
			"Osc2AmpEnvDelayTime",  \
			"Osc2AmpEnvAttackTime", \
			"Osc2AmpEnvAttackCurve", \
			"Osc2AmpEnvHoldTime", \
			"Osc2AmpEnvDecayTime", \
			"Osc2AmpEnvDecayCurve", \
			"Osc2AmpEnvSustainLevel", \
			"Osc2AmpEnvReleaseTime", \
			"Osc2AmpEnvReleaseCurve", \
			"Osc3Volume", \
			"Osc3PreFMVolume", \
			"Osc3Waveshape", \
			"Osc3SyncFrequency", \
			"Osc3FrequencyParam", \
			"Osc3PitchFine", \
			"Osc3FMFeedback", \
			"Osc3Phase", \
			"Osc3AuxMix", \
			"Osc3AmpEnvDelayTime",  \
			"Osc3AmpEnvAttackTime", \
			"Osc3AmpEnvAttackCurve", \
			"Osc3AmpEnvHoldTime", \
			"Osc3AmpEnvDecayTime", \
			"Osc3AmpEnvDecayCurve", \
			"Osc3AmpEnvSustainLevel", \
			"Osc3AmpEnvReleaseTime", \
			"Osc3AmpEnvReleaseCurve", \
			"Osc4Volume", \
			"Osc4PreFMVolume", \
			"Osc4Waveshape", \
			"Osc4SyncFrequency", \
			"Osc4FrequencyParam", \
			"Osc4PitchFine", \
			"Osc4FMFeedback", \
			"Osc4Phase", \
			"Osc4AuxMix", \
			"Osc4AmpEnvDelayTime", \
			"Osc4AmpEnvAttackTime", \
			"Osc4AmpEnvAttackCurve", \
			"Osc4AmpEnvHoldTime", \
			"Osc4AmpEnvDecayTime", \
			"Osc4AmpEnvDecayCurve", \
			"Osc4AmpEnvSustainLevel", \
			"Osc4AmpEnvReleaseTime", \
			"Osc4AmpEnvReleaseCurve", \
		"Env1DelayTime", \
			"Env1AttackTime", \
			"Env1AttackCurve", \
			"Env1HoldTime", \
			"Env1DecayTime", \
			"Env1DecayCurve", \
			"Env1SustainLevel", \
			"Env1ReleaseTime", \
			"Env1ReleaseCurve", \
			"Env2DelayTime", \
			"Env2AttackTime", \
			"Env2AttackCurve", \
			"Env2HoldTime", \
			"Env2DecayTime", \
			"Env2DecayCurve", \
			"Env2SustainLevel", \
			"Env2ReleaseTime", \
			"Env2ReleaseCurve", \
			"LFO1Waveshape", \
			"LFO1FrequencyParam", \
			"LFO2Waveshape", \
			"LFO2FrequencyParam", \
			"FMAmt2to1", \
			"FMAmt3to1", \
			"FMAmt4to1", \
			"FMAmt1to2", \
			"FMAmt3to2", \
			"FMAmt4to2", \
			"FMAmt1to3", \
			"FMAmt2to3", \
			"FMAmt4to3", \
			"FMAmt1to4", \
			"FMAmt2to4", \
			"FMAmt3to4", \
			"Aux1Param2", \
			"Aux1Param3", \
			"Aux1Param4", \
			"Aux1Param5", \
			"Aux2Param2", \
			"Aux2Param3", \
			"Aux2Param4", \
			"Aux2Param5", \
			"Aux3Param2", \
			"Aux3Param3", \
			"Aux3Param4", \
			"Aux3Param5", \
			"Aux4Param2", \
			"Aux4Param3", \
			"Aux4Param4", \
			"Aux4Param5", \
			"Sampler1Volume", \
		"Sampler1HiddenVolume", \
			"Sampler1PitchFine", \
		"Sampler1FrequencyParam", \
		"Sampler1AuxMix", \
		"Sampler1AmpEnvDelayTime",  \
			"Sampler1AmpEnvAttackTime", \
			"Sampler1AmpEnvAttackCurve", \
			"Sampler1AmpEnvHoldTime", \
			"Sampler1AmpEnvDecayTime", \
			"Sampler1AmpEnvDecayCurve", \
			"Sampler1AmpEnvSustainLevel", \
			"Sampler1AmpEnvReleaseTime", \
			"Sampler1AmpEnvReleaseCurve", \
"Sampler2Volume", \
"Sampler2HiddenVolume", \
"Sampler2PitchFine", \
"Sampler2FrequencyParam", \
"Sampler2AuxMix", \
"Sampler2AmpEnvDelayTime", \
"Sampler2AmpEnvAttackTime", \
"Sampler2AmpEnvAttackCurve", \
"Sampler2AmpEnvHoldTime", \
"Sampler2AmpEnvDecayTime", \
"Sampler2AmpEnvDecayCurve", \
"Sampler2AmpEnvSustainLevel", \
"Sampler2AmpEnvReleaseTime", \
"Sampler2AmpEnvReleaseCurve", \
"Sampler3Volume", \
"Sampler3HiddenVolume", \
"Sampler3PitchFine", \
"Sampler3FrequencyParam", \
"Sampler3AuxMix", \
"Sampler3AmpEnvDelayTime", \
"Sampler3AmpEnvAttackTime", \
"Sampler3AmpEnvAttackCurve", \
"Sampler3AmpEnvHoldTime", \
"Sampler3AmpEnvDecayTime", \
"Sampler3AmpEnvDecayCurve", \
"Sampler3AmpEnvSustainLevel", \
"Sampler3AmpEnvReleaseTime", \
"Sampler3AmpEnvReleaseCurve", \
"Sampler4Volume", \
"Sampler4HiddenVolume", \
"Sampler4PitchFine", \
"Sampler4FrequencyParam", \
"Sampler4AuxMix", \
"Sampler4AmpEnvDelayTime", \
"Sampler4AmpEnvAttackTime", \
"Sampler4AmpEnvAttackCurve", \
"Sampler4AmpEnvHoldTime", \
"Sampler4AmpEnvDecayTime", \
"Sampler4AmpEnvDecayCurve", \
"Sampler4AmpEnvSustainLevel", \
"Sampler4AmpEnvReleaseTime", \
"Sampler4AmpEnvReleaseCurve", \
		}

		// some modulation specs are for internal purposes / locked into certain behavior.
		enum ModulationSpecType
		{
			General,

			// osc amplification is done pre-FM. so it's not like modulating the output volume, it's before the signal
			// is fed to other oscillators. so
			// 1. the destination here is locked to a hidden osc volume param
			SourceAmp,
		};

		struct ModulationSpec
		{
			BoolParam mEnabled;
			EnumParam<ModSource> mSource;
			EnumParam<ModDestination> mDestination;
			ModulationSpecType mType = ModulationSpecType::General;
			BoolParam* mpDestSourceEnabledParam = nullptr;

			// you may ask why aux (aka sidechain) is necessary.
			// it's because we don't allow modulation of the modulation scale params, so it's a way of modulating the modulation itself.
			// now, because all mods are just added together you can absolutely just create a 2nd modulation mapped to the same param.
			// it's just a huge pain in the butt, plus you have to then make sure you get the scales lined up. modulation values are added
			// while aux values are used to scale the mod val, which doesn't have a true analog via normal modulations.
			BoolParam mAuxEnabled;
			EnumParam<ModSource> mAuxSource;

			BoolParam mInvert;
			BoolParam mAuxInvert;

			const int mBaseParamID;

			CurveParam mCurve;
			CurveParam mAuxCurve;
			FloatN11Param mScale;
			Float01Param mAuxAttenuation;

			void BeginBlock()
			{
				mEnabled.CacheValue();
				mInvert.CacheValue();
				mSource.CacheValue();
				mDestination.CacheValue();
				mAuxEnabled.CacheValue();
				mAuxInvert.CacheValue();
				mScale.CacheValue();
				mAuxSource.CacheValue();
			}

			void SetSourceAmp(ModSource mAmpEnvModSourceID, ModDestination mHiddenVolumeModDestID, BoolParam* pDestSourceEnabledParam)
			{
				mEnabled.SetBoolValue(true);
				mSource.SetEnumValue(mAmpEnvModSourceID);
				mDestination.SetEnumValue(mHiddenVolumeModDestID);
				mScale.SetN11Value(1);
				mType = ModulationSpecType::SourceAmp;
				mpDestSourceEnabledParam = pDestSourceEnabledParam;
			}

			ModulationSpec(real_t* paramCache, int baseParamID);
		};


		struct ModMatrixNode
		{
		private:
			real_t mSourceValues[(size_t)ModSource::Count] = { 0 };
			real_t mDestValues[(size_t)ModDestination::Count] = { 0 };
			real_t mDestValueDeltas[(size_t)gModulationCount] = { 0 };
			int mnSampleCount = 0;

			void FullProcessSample(ModulationSpec(&modSpecs)[gModulationCount]);
			//void DeltaProcessSample(ModulationSpec(&modSpecs)[gModulationCount]);

			// in order to process modulations only every N samples, linearly interpolate during those N samples.
			// when the dest value is processed, then this is the value that's added every sample to reach the final
			// dest value. It does mean a N sample delay.
			//float mDestAddendPerSample = 0;

		public:
			static constexpr size_t gRecalcSampleMask = 7;

			template<typename Tmodid>
			inline void SetSourceValue(Tmodid id, real_t val)
			{
				mSourceValues[(size_t)id] = val;
			}

			template<typename Tmodid>
			inline real_t GetSourceValue(Tmodid id) const
			{
				return mSourceValues[(size_t)id];
			}

			template<typename Tmodid>
			inline real_t GetDestinationValue(Tmodid id) const
			{
				return mDestValues[(size_t)id];
			}

			static constexpr ModulationPolarity GetPolarity(ModSource m)
			{
				switch (m)
				{
				case ModSource::LFO1:
				case ModSource::LFO2:
				case ModSource::PitchBend:
					return ModulationPolarity::N11;
				}
				return ModulationPolarity::Positive01;
			}
			static float InvertValue(float val, bool invertParam, const ModSource modSource);
			// caller passes in:
			// sourceValues_KRateOnly: a buffer indexed by (size_t)M7::ModSource. only krate values are used though.
			// sourceARateBuffers: a contiguous array of block-sized buffers. sequentially arranged indexed by (size_t)M7::ModSource.
			// the result will be placed 
			void ProcessSample(ModulationSpec(&modSpecs)[gModulationCount]);
		};
	} // namespace M7


} // namespace WaveSabreCore



