#pragma once

#include <WaveSabreCore/Maj7Basic.hpp>

namespace WaveSabreCore
{
	namespace M7
	{

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
			LFO3, // arate, N11
			LFO4, // arate, N11
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
			Const_1,
			Const_0_5,
			Const_0,

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
			"LFO3", \
			"LFO4", \
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
			"1 (const)", \
			"0.5 (const)", \
			"0 (const)", \
		}
#define MODSOURCE_SHORT_CAPTIONS(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::M7::ModSource::Count]{ \
			"-", \
			"O1Env", \
			"O2Env", \
			"O3Env", \
			"O4Env", \
			"S1Env", \
			"S2Env", \
			"S3Env", \
			"S4Env", \
			"MEnv1", \
			"MEnv2", \
			"LFO1", \
			"LFO2", \
			"LFO3", \
			"LFO4", \
			"PB", \
			"Vel", \
			"Note", \
			"Rng", \
			"UVox", \
			"Sus", \
			"Knob1", \
			"Knob2", \
			"Knob3", \
			"Knob4", \
			"Knob5", \
			"Knob6", \
			"Knob7", \
			"1", \
			"0.5", \
			"0", \
        };

		enum class ModDestination : uint8_t
		{
			None, // important that this is 0, for initialization of mModSpecLastDestinations
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

				LFO2Waveshape,
				LFO2FrequencyParam,

				LFO3Waveshape,
				LFO3FrequencyParam,

				LFO4Waveshape,
				LFO4FrequencyParam,

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
			"LFO3Waveshape", \
			"LFO3FrequencyParam", \
			"LFO4Waveshape", \
			"LFO4FrequencyParam", \
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

#define MODDEST_SHORT_CAPTIONS(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::M7::ModDestination::Count]{ \
"-", \
"FMAmt", \
"Port", \
"O1Vol", \
"O1PreVol", \
"O1Shape", \
"O1Sync", \
"O1Freq", \
"O1Fine", \
"O1FMFB", \
"O1Phase", \
"O1Pan", \
"O1EnvDly", \
"O1EnvAtt", \
"O1EnvAttCrv", \
"O1EnvHold", \
"O1EnvDcy", \
"O1EnvDcyCrv", \
"O1EnvSus", \
"O1EnvRel", \
"O1EnvRelCrv", \
"O2Vol", \
"O2PreVol", \
"O2Shape", \
"O2Sync", \
"O2Freq", \
"O2Fine", \
"O2FMFB", \
"O2Phase", \
"O2Pan", \
"O2EnvDly", \
"O2EnvAtt", \
"O2EnvAttCrv", \
"O2EnvHold", \
"O2EnvDcy", \
"O2EnvDcyCrv", \
"O2EnvSus", \
"O2EnvRel", \
"O2EnvRelCrv", \
"O3Vol", \
"O3PreVol", \
"O3Shape", \
"O3Sync", \
"O3Freq", \
"O3Fine", \
"O3FMFB", \
"O3Phase", \
"O3Pan", \
"O3EnvDly", \
"O3EnvAtt", \
"O3EnvAttCrv", \
"O3EnvHold", \
"O3EnvDcy", \
"O3EnvDcyCrv", \
"O3EnvSus", \
"O3EnvRel", \
"O3EnvRelCrv", \
"O4Vol", \
"O4PreVol", \
"O4Shape", \
"O4Sync", \
"O4Freq", \
"O4Fine", \
"O4FMFB", \
"O4Phase", \
"O4Pan", \
"O4EnvDly", \
"O4EnvAtt", \
"O4EnvAttCrv", \
"O4EnvHold", \
"O4EnvDcy", \
"O4EnvDcyCrv", \
"O4EnvSus", \
"O4EnvRel", \
"O4EnvRelCrv", \
"Env1Dly", \
"Env1Att", \
"Env1AttCrv", \
"Env1Hold", \
"Env1Dcy", \
"Env1DcyCrv", \
"Env1Sus", \
"Env1Rel", \
"Env1RelCrv", \
"Env2Dly", \
"Env2Att", \
"Env2AttCrv", \
"Env2Hold", \
"Env2Dcy", \
"Env2DcyCrv", \
"Env2Sus", \
"Env2Rel", \
"Env2RelCrv", \
"LFO1Shp", \
"LFO1Freq", \
"LFO2Shp", \
"LFO2Freq", \
"LFO3Shp", \
"LFO3Freq", \
"LFO4Shp", \
"LFO4Freq", \
"FM2to1", \
"FM3to1", \
"FM4to1", \
"FM1to2", \
"FM3to2", \
"FM4to2", \
"FM1to3", \
"FM2to3", \
"FM4to3", \
"FM1to4", \
"FM2to4", \
"FM3to4", \
"Aux1P2", \
"Aux1P3", \
"Aux1P4", \
"Aux1P5", \
"Aux2P2", \
"Aux2P3", \
"Aux2P4", \
"Aux2P5", \
"Aux3P2", \
"Aux3P3", \
"Aux3P4", \
"Aux3P5", \
"Aux4P2", \
"Aux4P3", \
"Aux4P4", \
"Aux4P5", \
"S1Vol", \
"S1PreVol", \
"S1Fine", \
"S1Freq", \
"S1Pan", \
"S1EnvDly", \
"S1EnvAtt", \
"S1EnvAttCrv", \
"S1EnvHold", \
"S1EnvDcy", \
"S1EnvDcyCrv", \
"S1EnvSus", \
"S1EnvRel", \
"S1EnvRelCrv", \
"S2Vol", \
"S2PreVol", \
"S2Fine", \
"S2Freq", \
"S2Pan", \
"S2EnvDly", \
"S2EnvAtt", \
"S2EnvAttCrv", \
"S2EnvHold", \
"S2EnvDcy", \
"S2EnvDcyCrv", \
"S2EnvSus", \
"S2EnvRel", \
"S2EnvRelCrv", \
"S3Vol", \
"S3PreVol", \
"S3Fine", \
"S3Freq", \
"S3Pan", \
"S3EnvDly", \
"S3EnvAtt", \
"S3EnvAttCrv", \
"S3EnvHold", \
"S3EnvDcy", \
"S3EnvDcyCrv", \
"S3EnvSus", \
"S3EnvRel", \
"S3EnvRelCrv", \
"S4Vol", \
"S4PreVol", \
"S4Fine", \
"S4Freq", \
"S4Pan", \
"S4EnvDly", \
"S4EnvAtt", \
"S4EnvAttCrv", \
"S4EnvHold", \
"S4EnvDcy", \
"S4EnvDcyCrv", \
"S4EnvSus", \
"S4EnvRel", \
"S4EnvRelCrv", \
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

		enum ModValueMapping
		{
			NoMapping,
			N11_1N1, // -1,1 => 1,-1 (bipolar invert)
			N11_01, // -1,1 => 0,1 (bipolar to positive)
			N11_10, // -1,1 => 1,0 (bipolar to invert positive)
			P01_10, // 0,1 => 1,0 (positive invert)
			P01_N11, // 0,1 => -1,1 (positive to bipolar)
			P01_1N1, // 0,1 => 1,-1 (positive to negative bipolar)
			Count,
		};
#define MOD_VALUE_MAPPING_CAPTIONS(symbolName) static constexpr char const* const symbolName[(int)::WaveSabreCore::M7::ModValueMapping::Count]{ \
			"(thru)", \
			"-1,1 => 1,-1 (N11 inv)", \
			"-1,1 => 0,1 (N11 to POS)", \
			"-1,1 => 1,0 (N11 to POS_inv)", \
			"0,1 => 1,0 (POS inv)", \
			"0,1 => -1,1 (POS to N11)", \
			"0,1 => 1,-1 (POS to N11_inv", \
        };

		struct ModulationSpec
		{
			float* const mParamCache;
			const int mBaseParamID;

			BoolParam mEnabled;
			EnumParam<ModSource> mSource;
			EnumParam<ModDestination> mDestinations[gModulationSpecDestinationCount] = {
				EnumParam<ModDestination>{mParamCache[mBaseParamID + (int)ModParamIndexOffsets::Destination1], ModDestination::Count},
				EnumParam<ModDestination>{mParamCache[mBaseParamID + (int)ModParamIndexOffsets::Destination2], ModDestination::Count},
				EnumParam<ModDestination>{mParamCache[mBaseParamID + (int)ModParamIndexOffsets::Destination3], ModDestination::Count},
				EnumParam<ModDestination>{mParamCache[mBaseParamID + (int)ModParamIndexOffsets::Destination4], ModDestination::Count},
			};
			FloatN11Param mScales[gModulationSpecDestinationCount] = {
				FloatN11Param{mParamCache[mBaseParamID + (int)ModParamIndexOffsets::Scale1]},
				FloatN11Param{mParamCache[mBaseParamID + (int)ModParamIndexOffsets::Scale2]},
				FloatN11Param{mParamCache[mBaseParamID + (int)ModParamIndexOffsets::Scale3]},
				FloatN11Param{mParamCache[mBaseParamID + (int)ModParamIndexOffsets::Scale4]},
			};
			ModulationSpecType mType = ModulationSpecType::General;
			BoolParam* mpDestSourceEnabledParam = nullptr;

			// you may ask why aux (aka sidechain) is necessary.
			// it's because we don't allow modulation of the modulation scale params, so it's a way of modulating the modulation itself.
			// now, because all mods are just added together you can absolutely just create a 2nd modulation mapped to the same param.
			// it's just a huge pain in the butt, plus you have to then make sure you get the scales lined up. modulation values are added
			// while aux values are used to scale the mod val, which doesn't have a true analog via normal modulations.
			BoolParam mAuxEnabled;
			EnumParam<ModSource> mAuxSource;

			EnumParam<ModValueMapping> mValueMapping;
			EnumParam<ModValueMapping> mAuxValueMapping;

			CurveParam mCurve;
			CurveParam mAuxCurve;
			Float01Param mAuxAttenuation;


			ModulationSpec(real_t* paramCache, int baseParamID) :
				mParamCache(paramCache),
				mBaseParamID(baseParamID),
				mEnabled(paramCache[baseParamID + (int)ModParamIndexOffsets::Enabled]),
				mSource(paramCache[baseParamID + (int)ModParamIndexOffsets::Source], ModSource::Count),
				//mDestination(paramCache[baseParamID + (int)ModParamIndexOffsets::Destination], ModDestination::Count),
				mCurve(paramCache[baseParamID + (int)ModParamIndexOffsets::Curve]),
				//mScale(paramCache[baseParamID + (int)ModParamIndexOffsets::Scale]),
				mAuxEnabled(paramCache[baseParamID + (int)ModParamIndexOffsets::AuxEnabled]),
				mAuxSource(paramCache[baseParamID + (int)ModParamIndexOffsets::AuxSource], ModSource::Count),
				mAuxCurve(paramCache[baseParamID + (int)ModParamIndexOffsets::AuxCurve]),
				mAuxAttenuation(paramCache[baseParamID + (int)ModParamIndexOffsets::AuxAttenuation]),
				mValueMapping(paramCache[baseParamID + (int)ModParamIndexOffsets::ValueMapping], ModValueMapping::Count),
				mAuxValueMapping(paramCache[baseParamID + (int)ModParamIndexOffsets::AuxValueMapping], ModValueMapping::Count)
			{
			}
			void BeginBlock()
			{
				mEnabled.CacheValue();
				mSource.CacheValue();
				for (auto& d : mDestinations) d.CacheValue();
				for (auto& d : mScales) d.CacheValue();
				mAuxEnabled.CacheValue();
				mValueMapping.CacheValue();
				mAuxValueMapping.CacheValue();
				mAuxSource.CacheValue();
			}

			void SetSourceAmp(ModSource mAmpEnvModSourceID, ModDestination mHiddenVolumeModDestID, BoolParam* pDestSourceEnabledParam)
			{
				mEnabled.SetBoolValue(true);
				mSource.SetEnumValue(mAmpEnvModSourceID);
				mDestinations[0].SetEnumValue(mHiddenVolumeModDestID);
				mScales[0].SetN11Value(1);
				mType = ModulationSpecType::SourceAmp;
				mpDestSourceEnabledParam = pDestSourceEnabledParam;
			}

		};

		struct ModMatrixNode
		{
			real_t mSourceValues[(size_t)ModSource::Count] = { 0 };
			real_t mDestValues[(size_t)ModDestination::Count] = { 0 };

			struct ModDestAlgo
			{
				ModDestination mDest = ModDestination::None;
				float mDeltaPerSample = 0;
			};

			//real_t mDestValueDeltas[(size_t)gModulationCount][gModulationSpecDestinationCount] = { 0 };
			ModDestAlgo mModulatedDestValueDeltas[(size_t)ModDestination::Count];
			size_t mModulatedDestValueCount = 0;

			// this is required to know when to reset a destination back to 0.
			// after the user disables a modulationspec or changes its destination,
			// we need to detect that scenario to remove its influence on the dest value.
			ModDestination mModSpecLastDestinations[(size_t)gModulationCount][gModulationSpecDestinationCount] = {ModDestination::None};
			int mnSampleCount = 0;

			template<typename Tmodid>
			inline void SetSourceValue(Tmodid id, real_t val)
			{
				mSourceValues[(size_t)id] = val;
			}

			//// when something like a note on / note off happens, do a full recalc immediately to pull new sample-accurate values.
			//void OnRecalcEvent()
			//{
			//	mnSampleCount = 0;
			//}

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

			//static constexpr ModulationPolarity GetPolarity(ModSource m)
			//{
			//	switch (m)
			//	{
			//	case ModSource::LFO1:
			//	case ModSource::LFO2:
			//	case ModSource::PitchBend:
			//		return ModulationPolarity::N11;
			//	}
			//	return ModulationPolarity::Positive01;
			//}
			//
			//static float InvertValue(float val, bool invertParam, const ModSource modSource);

			void BeginBlock()
			{
				// force a full recalc every block. this is required, otherwise our mod deltas will get messed up.
				// because voices which aren't running don't process any samples, our deltas will be inaccurate,
				// not to mention our samplecount will be inaccurate and take potentially too many samples between recalcs.
				// that would send dest values into an undefined state. Actually this will potentially mean our recalc
				// period is too short, which also leaves things in an inaccurate state, although a much safer state because
				// at least it's between the desired value and original. too long and it would go out of bounds. it will 
				// correct itself on the next recalc.
				mnSampleCount = 0; 
			}

			// caller passes in:
			// sourceValues_KRateOnly: a buffer indexed by (size_t)M7::ModSource. only krate values are used though.
			// sourceARateBuffers: a contiguous array of block-sized buffers. sequentially arranged indexed by (size_t)M7::ModSource.
			// the result will be placed 
			void ProcessSample(ModulationSpec(&modSpecs)[gModulationCount]);
		};
	} // namespace M7


} // namespace WaveSabreCore



