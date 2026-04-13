#pragma once

#include <stdint.h>

namespace WaveSabreCore
{
namespace M7
{

// you need to sync up all these enums below to correspond with ordering et al.
enum class GigaSynthParamIndices : uint16_t
{
  MasterVolume,
  Pan,
  VoicingMode,
  Unisono,
  UnisonoDetune,
  UnisonoStereoSpread,
  FMBrightness,

  PortamentoTime,
  PitchBendRange,

  MaxVoices,

  Macro1,
  Macro2,
  Macro3,
  Macro4,

  FMAmt2to1,
  FMAmt3to1,
  FMAmt4to1,
  FMAmt1to2,
  FMAmt3to2,
  FMAmt4to2,
  FMAmt1to3,
  FMAmt2to3,
  FMAmt4to3,
  FMAmt1to4,
  FMAmt2to4,
  FMAmt3to4,

  Osc1Enabled,  // KEEP IN SYNC WITH OscParamIndexOffsets
  Osc1Volume,
  Osc1Pan,
  Osc1KeyrangeMin,
  Osc1KeyrangeMax,

  Osc1Waveform,
  Osc1WaveshapeA,
  Osc1WaveshapeB,
  Osc1PhaseRestart,
  Osc1PhaseOffset,
  Osc1SyncEnable,
  Osc1SyncFrequency,
  Osc1SyncFrequencyKT,
  Osc1FrequencyParam,
  Osc1FrequencyParamKT,
  Osc1PitchSemis,
  Osc1PitchFine,
  Osc1FreqMul,
  Osc1FMFeedback,

  Osc1AmpEnvDelayTime,  // KEEP IN SYNC WITH EnvParamIndexOffsets
  Osc1AmpEnvAttackTime,
  Osc1AmpEnvAttackCurve,
  Osc1AmpEnvHoldTime,
  Osc1AmpEnvDecayTime,
  Osc1AmpEnvDecayCurve,
  Osc1AmpEnvSustainLevel,
  Osc1AmpEnvReleaseTime,
  Osc1AmpEnvReleaseCurve,
  Osc1AmpEnvLegatoRestart,
  Osc1AmpEnvMode,

  Osc2Enabled,  // KEEP IN SYNC WITH OscParamIndexOffsets
  Osc2Volume,
  Osc2Pan,
  Osc2KeyrangeMin,
  Osc2KeyrangeMax,
  Osc2Waveform,
  Osc2WaveshapeA,
  Osc2WaveshapeB,
  Osc2PhaseRestart,
  Osc2PhaseOffset,
  Osc2SyncEnable,
  Osc2SyncFrequency,
  Osc2SyncFrequencyKT,
  Osc2FrequencyParam,
  Osc2FrequencyParamKT,
  Osc2PitchSemis,
  Osc2PitchFine,
  Osc2FreqMul,
  Osc2FMFeedback,

  Osc2AmpEnvDelayTime,  // KEEP IN SYNC WITH EnvParamIndexOffsets
  Osc2AmpEnvAttackTime,
  Osc2AmpEnvAttackCurve,
  Osc2AmpEnvHoldTime,
  Osc2AmpEnvDecayTime,
  Osc2AmpEnvDecayCurve,
  Osc2AmpEnvSustainLevel,
  Osc2AmpEnvReleaseTime,
  Osc2AmpEnvReleaseCurve,
  Osc2AmpEnvLegatoRestart,
  Osc2AmpEnvMode,

  Osc3Enabled,  // KEEP IN SYNC WITH OscParamIndexOffsets
  Osc3Volume,
  Osc3Pan,
  Osc3KeyrangeMin,
  Osc3KeyrangeMax,
  Osc3Waveform,
  Osc3WaveshapeA,
  Osc3WaveshapeB,
  Osc3PhaseRestart,
  Osc3PhaseOffset,
  Osc3SyncEnable,
  Osc3SyncFrequency,
  Osc3SyncFrequencyKT,
  Osc3FrequencyParam,
  Osc3FrequencyParamKT,
  Osc3PitchSemis,
  Osc3PitchFine,
  Osc3FreqMul,
  Osc3FMFeedback,

  Osc3AmpEnvDelayTime,  // KEEP IN SYNC WITH EnvParamIndexOffsets
  Osc3AmpEnvAttackTime,
  Osc3AmpEnvAttackCurve,
  Osc3AmpEnvHoldTime,
  Osc3AmpEnvDecayTime,
  Osc3AmpEnvDecayCurve,
  Osc3AmpEnvSustainLevel,
  Osc3AmpEnvReleaseTime,
  Osc3AmpEnvReleaseCurve,
  Osc3AmpEnvLegatoRestart,
  Osc3AmpEnvMode,

  Osc4Enabled,  // KEEP IN SYNC WITH OscParamIndexOffsets
  Osc4Volume,
  Osc4Pan,
  Osc4KeyrangeMin,
  Osc4KeyrangeMax,
  Osc4Waveform,
  Osc4WaveshapeA,
  Osc4WaveshapeB,
  Osc4PhaseRestart,
  Osc4PhaseOffset,
  Osc4SyncEnable,
  Osc4SyncFrequency,
  Osc4SyncFrequencyKT,
  Osc4FrequencyParam,
  Osc4FrequencyParamKT,
  Osc4PitchSemis,
  Osc4PitchFine,
  Osc4FreqMul,
  Osc4FMFeedback,

  Osc4AmpEnvDelayTime,  // KEEP IN SYNC WITH EnvParamIndexOffsets
  Osc4AmpEnvAttackTime,
  Osc4AmpEnvAttackCurve,
  Osc4AmpEnvHoldTime,
  Osc4AmpEnvDecayTime,
  Osc4AmpEnvDecayCurve,
  Osc4AmpEnvSustainLevel,
  Osc4AmpEnvReleaseTime,
  Osc4AmpEnvReleaseCurve,
  Osc4AmpEnvLegatoRestart,
  Osc4AmpEnvMode,

  Env1DelayTime,  // KEEP IN SYNC WITH EnvParamIndexOffsets
  Env1AttackTime,
  Env1AttackCurve,
  Env1HoldTime,
  Env1DecayTime,
  Env1DecayCurve,
  Env1SustainLevel,
  Env1ReleaseTime,
  Env1ReleaseCurve,
  Env1LegatoRestart,
  Env1AmpEnvMode,

  Env2DelayTime,  // KEEP IN SYNC WITH EnvParamIndexOffsets
  Env2AttackTime,
  Env2AttackCurve,
  Env2HoldTime,
  Env2DecayTime,
  Env2DecayCurve,
  Env2SustainLevel,
  Env2ReleaseTime,
  Env2ReleaseCurve,
  Env2LegatoRestart,
  Env2AmpEnvMode,

  Env3DelayTime,  // KEEP IN SYNC WITH EnvParamIndexOffsets
  Env3AttackTime,
  Env3AttackCurve,
  Env3HoldTime,
  Env3DecayTime,
  Env3DecayCurve,
  Env3SustainLevel,
  Env3ReleaseTime,
  Env3ReleaseCurve,
  Env3LegatoRestart,
  Env3AmpEnvMode,

  Env4DelayTime,  // KEEP IN SYNC WITH EnvParamIndexOffsets
  Env4AttackTime,
  Env4AttackCurve,
  Env4HoldTime,
  Env4DecayTime,
  Env4DecayCurve,
  Env4SustainLevel,
  Env4ReleaseTime,
  Env4ReleaseCurve,
  Env4LegatoRestart,
  Env4AmpEnvMode,

  LFO1Waveform,  // KEEP IN SYNC WITH LFOParamIndexOffsets
  LFO1WaveshapeA,
  LFO1WaveshapeB,
  LFO1Restart,  // if restart, then LFO is per voice. if no restart, then it's per synth.
  LFO1PhaseOffset,
  LFO1FrequencyParam,
  LFO1Sharpness,

  LFO2Waveform,  // KEEP IN SYNC WITH LFOParamIndexOffsets
  LFO2WaveshapeA,
  LFO2WaveshapeB,
  LFO2Restart,  // if restart, then LFO is per voice. if no restart, then it's per synth.
  LFO2PhaseOffset,
  LFO2FrequencyParam,
  LFO2Sharpness,

  LFO3Waveform,  // KEEP IN SYNC WITH LFOParamIndexOffsets
  LFO3WaveshapeA,
  LFO3WaveshapeB,
  LFO3Restart,  // if restart, then LFO is per voice. if no restart, then it's per synth.
  LFO3PhaseOffset,
  LFO3FrequencyParam,
  LFO3Sharpness,

  LFO4Waveform,  // KEEP IN SYNC WITH LFOParamIndexOffsets
  LFO4WaveshapeA,
  LFO4WaveshapeB,
  LFO4Restart,  // if restart, then LFO is per voice. if no restart, then it's per synth.
  LFO4PhaseOffset,
  LFO4FrequencyParam,
  LFO4Sharpness,

  Filter1Enabled,
  Filter1Response,
  Filter1Q,
  Filter1Freq,
  Filter1FreqKT,

  Mod1Enabled,  // KEEP IN SYNC WITH ModParamIndexOffsets
  Mod1Source,
  Mod1Destination1,
  Mod1Destination2,
  Mod1Destination3,
  Mod1Destination4,
  Mod1Scale1,
  Mod1Scale2,
  Mod1Scale3,
  Mod1Scale4,
  Mod1AuxEnabled,
  Mod1AuxSource,
  Mod1AuxAttenuation,
  Mod1AuxCurve,
  Mod1SrcRangeMin,
  Mod1SrcRangeMax,
  Mod1AuxRangeMin,
  Mod1AuxRangeMax,

  Mod2Enabled,  // KEEP IN SYNC WITH ModParamIndexOffsets
  Mod2Source,
  Mod2Destination1,
  Mod2Destination2,
  Mod2Destination3,
  Mod2Destination4,
  Mod2Scale1,
  Mod2Scale2,
  Mod2Scale3,
  Mod2Scale4,
  Mod2AuxEnabled,
  Mod2AuxSource,
  Mod2AuxAttenuation,
  Mod2AuxCurve,
  Mod2SrcRangeMin,
  Mod2SrcRangeMax,
  Mod2AuxRangeMin,
  Mod2AuxRangeMax,

  Mod3Enabled,  // KEEP IN SYNC WITH ModParamIndexOffsets
  Mod3Source,
  Mod3Destination1,
  Mod3Destination2,
  Mod3Destination3,
  Mod3Destination4,
  Mod3Scale1,
  Mod3Scale2,
  Mod3Scale3,
  Mod3Scale4,
  Mod3AuxEnabled,
  Mod3AuxSource,
  Mod3AuxAttenuation,
  Mod3AuxCurve,
  Mod3SrcRangeMin,
  Mod3SrcRangeMax,
  Mod3AuxRangeMin,
  Mod3AuxRangeMax,

  Mod4Enabled,  // KEEP IN SYNC WITH ModParamIndexOffsets
  Mod4Source,
  Mod4Destination1,
  Mod4Destination2,
  Mod4Destination3,
  Mod4Destination4,
  Mod4Scale1,
  Mod4Scale2,
  Mod4Scale3,
  Mod4Scale4,
  Mod4AuxEnabled,
  Mod4AuxSource,
  Mod4AuxAttenuation,
  Mod4AuxCurve,
  Mod4SrcRangeMin,
  Mod4SrcRangeMax,
  Mod4AuxRangeMin,
  Mod4AuxRangeMax,

  Mod5Enabled,  // KEEP IN SYNC WITH ModParamIndexOffsets
  Mod5Source,
  Mod5Destination1,
  Mod5Destination2,
  Mod5Destination3,
  Mod5Destination4,
  Mod5Scale1,
  Mod5Scale2,
  Mod5Scale3,
  Mod5Scale4,
  Mod5AuxEnabled,
  Mod5AuxSource,
  Mod5AuxAttenuation,
  Mod5AuxCurve,
  Mod5SrcRangeMin,
  Mod5SrcRangeMax,
  Mod5AuxRangeMin,
  Mod5AuxRangeMax,

  Mod6Enabled,  // KEEP IN SYNC WITH ModParamIndexOffsets
  Mod6Source,
  Mod6Destination1,
  Mod6Destination2,
  Mod6Destination3,
  Mod6Destination4,
  Mod6Scale1,
  Mod6Scale2,
  Mod6Scale3,
  Mod6Scale4,
  Mod6AuxEnabled,
  Mod6AuxSource,
  Mod6AuxAttenuation,
  Mod6AuxCurve,
  Mod6SrcRangeMin,
  Mod6SrcRangeMax,
  Mod6AuxRangeMin,
  Mod6AuxRangeMax,

  Mod7Enabled,  // KEEP IN SYNC WITH ModParamIndexOffsets
  Mod7Source,
  Mod7Destination1,
  Mod7Destination2,
  Mod7Destination3,
  Mod7Destination4,
  Mod7Scale1,
  Mod7Scale2,
  Mod7Scale3,
  Mod7Scale4,
  Mod7AuxEnabled,
  Mod7AuxSource,
  Mod7AuxAttenuation,
  Mod7AuxCurve,
  Mod7SrcRangeMin,
  Mod7SrcRangeMax,
  Mod7AuxRangeMin,
  Mod7AuxRangeMax,

  Mod8Enabled,  // KEEP IN SYNC WITH ModParamIndexOffsets
  Mod8Source,
  Mod8Destination1,
  Mod8Destination2,
  Mod8Destination3,
  Mod8Destination4,
  Mod8Scale1,
  Mod8Scale2,
  Mod8Scale3,
  Mod8Scale4,
  Mod8AuxEnabled,
  Mod8AuxSource,
  Mod8AuxAttenuation,
  Mod8AuxCurve,
  Mod8SrcRangeMin,
  Mod8SrcRangeMax,
  Mod8AuxRangeMin,
  Mod8AuxRangeMax,

  Mod9Enabled,  // KEEP IN SYNC WITH ModParamIndexOffsets
  Mod9Source,
  Mod9Destination1,
  Mod9Destination2,
  Mod9Destination3,
  Mod9Destination4,
  Mod9Scale1,
  Mod9Scale2,
  Mod9Scale3,
  Mod9Scale4,
  Mod9AuxEnabled,
  Mod9AuxSource,
  Mod9AuxAttenuation,
  Mod9AuxCurve,
  Mod9SrcRangeMin,
  Mod9SrcRangeMax,
  Mod9AuxRangeMin,
  Mod9AuxRangeMax,

  Mod10Enabled,  // KEEP IN SYNC WITH ModParamIndexOffsets
  Mod10Source,
  Mod10Destination1,
  Mod10Destination2,
  Mod10Destination3,
  Mod10Destination4,
  Mod10Scale1,
  Mod10Scale2,
  Mod10Scale3,
  Mod10Scale4,
  Mod10AuxEnabled,
  Mod10AuxSource,
  Mod10AuxAttenuation,
  Mod10AuxCurve,
  Mod10SrcRangeMin,
  Mod10SrcRangeMax,
  Mod10AuxRangeMin,
  Mod10AuxRangeMax,

  Mod11Enabled,  // KEEP IN SYNC WITH ModParamIndexOffsets
  Mod11Source,
  Mod11Destination1,
  Mod11Destination2,
  Mod11Destination3,
  Mod11Destination4,
  Mod11Scale1,
  Mod11Scale2,
  Mod11Scale3,
  Mod11Scale4,
  Mod11AuxEnabled,
  Mod11AuxSource,
  Mod11AuxAttenuation,
  Mod11AuxCurve,
  Mod11SrcRangeMin,
  Mod11SrcRangeMax,
  Mod11AuxRangeMin,
  Mod11AuxRangeMax,

  Mod12Enabled,  // KEEP IN SYNC WITH ModParamIndexOffsets
  Mod12Source,
  Mod12Destination1,
  Mod12Destination2,
  Mod12Destination3,
  Mod12Destination4,
  Mod12Scale1,
  Mod12Scale2,
  Mod12Scale3,
  Mod12Scale4,
  Mod12AuxEnabled,
  Mod12AuxSource,
  Mod12AuxAttenuation,
  Mod12AuxCurve,
  Mod12SrcRangeMin,
  Mod12SrcRangeMax,
  Mod12AuxRangeMin,
  Mod12AuxRangeMax,

  Mod13Enabled,  // KEEP IN SYNC WITH ModParamIndexOffsets
  Mod13Source,
  Mod13Destination1,
  Mod13Destination2,
  Mod13Destination3,
  Mod13Destination4,
  Mod13Scale1,
  Mod13Scale2,
  Mod13Scale3,
  Mod13Scale4,
  Mod13AuxEnabled,
  Mod13AuxSource,
  Mod13AuxAttenuation,
  Mod13AuxCurve,
  Mod13SrcRangeMin,
  Mod13SrcRangeMax,
  Mod13AuxRangeMin,
  Mod13AuxRangeMax,

  Mod14Enabled,  // KEEP IN SYNC WITH ModParamIndexOffsets
  Mod14Source,
  Mod14Destination1,
  Mod14Destination2,
  Mod14Destination3,
  Mod14Destination4,
  Mod14Scale1,
  Mod14Scale2,
  Mod14Scale3,
  Mod14Scale4,
  Mod14AuxEnabled,
  Mod14AuxSource,
  Mod14AuxAttenuation,
  Mod14AuxCurve,
  Mod14SrcRangeMin,
  Mod14SrcRangeMax,
  Mod14AuxRangeMin,
  Mod14AuxRangeMax,

  Mod15Enabled,  // KEEP IN SYNC WITH ModParamIndexOffsets
  Mod15Source,
  Mod15Destination1,
  Mod15Destination2,
  Mod15Destination3,
  Mod15Destination4,
  Mod15Scale1,
  Mod15Scale2,
  Mod15Scale3,
  Mod15Scale4,
  Mod15AuxEnabled,
  Mod15AuxSource,
  Mod15AuxAttenuation,
  Mod15AuxCurve,
  Mod15SrcRangeMin,
  Mod15SrcRangeMax,
  Mod15AuxRangeMin,
  Mod15AuxRangeMax,

  Mod16Enabled,  // KEEP IN SYNC WITH ModParamIndexOffsets
  Mod16Source,
  Mod16Destination1,
  Mod16Destination2,
  Mod16Destination3,
  Mod16Destination4,
  Mod16Scale1,
  Mod16Scale2,
  Mod16Scale3,
  Mod16Scale4,
  Mod16AuxEnabled,
  Mod16AuxSource,
  Mod16AuxAttenuation,
  Mod16AuxCurve,
  Mod16SrcRangeMin,
  Mod16SrcRangeMax,
  Mod16AuxRangeMin,
  Mod16AuxRangeMax,

  Mod17Enabled,  // KEEP IN SYNC WITH ModParamIndexOffsets
  Mod17Source,
  Mod17Destination1,
  Mod17Destination2,
  Mod17Destination3,
  Mod17Destination4,
  Mod17Scale1,
  Mod17Scale2,
  Mod17Scale3,
  Mod17Scale4,
  Mod17AuxEnabled,
  Mod17AuxSource,
  Mod17AuxAttenuation,
  Mod17AuxCurve,
  Mod17SrcRangeMin,
  Mod17SrcRangeMax,
  Mod17AuxRangeMin,
  Mod17AuxRangeMax,

  Mod18Enabled,  // KEEP IN SYNC WITH ModParamIndexOffsets
  Mod18Source,
  Mod18Destination1,
  Mod18Destination2,
  Mod18Destination3,
  Mod18Destination4,
  Mod18Scale1,
  Mod18Scale2,
  Mod18Scale3,
  Mod18Scale4,
  Mod18AuxEnabled,
  Mod18AuxSource,
  Mod18AuxAttenuation,
  Mod18AuxCurve,
  Mod18SrcRangeMin,
  Mod18SrcRangeMax,
  Mod18AuxRangeMin,
  Mod18AuxRangeMax,

  Sampler1Enabled,  // KEEP IN SYNC WITH SamplerParamIndexOffsets
  Sampler1Volume,
  Sampler1Pan,
  Sampler1KeyrangeMin,
  Sampler1KeyrangeMax,
  Sampler1BaseNote,
  Sampler1LegatoTrig,
  Sampler1Reverse,
  Sampler1GmDlsIndex,
  Sampler1SampleStart,
  Sampler1LoopMode,
  Sampler1LoopSource,
  Sampler1LoopStart,
  Sampler1LoopLength,
  Sampler1TuneSemis,
  Sampler1TuneFine,
  Sampler1FreqParam,
  Sampler1FreqKT,
  Sampler1ReleaseExitsLoop,
  Sampler1Delay,

  Sampler1AmpEnvDelayTime,  // KEEP IN SYNC WITH EnvParamIndexOffsets
  Sampler1AmpEnvAttackTime,
  Sampler1AmpEnvAttackCurve,
  Sampler1AmpEnvHoldTime,
  Sampler1AmpEnvDecayTime,
  Sampler1AmpEnvDecayCurve,
  Sampler1AmpEnvSustainLevel,
  Sampler1AmpEnvReleaseTime,
  Sampler1AmpEnvReleaseCurve,
  Sampler1AmpEnvLegatoRestart,
  Sampler1AmpEnvMode,

  Sampler2Enabled,  // KEEP IN SYNC WITH SamplerParamIndexOffsets
  Sampler2Volume,
  Sampler2Pan,
  Sampler2KeyrangeMin,
  Sampler2KeyrangeMax,
  Sampler2BaseNote,
  Sampler2LegatoTrig,
  Sampler2Reverse,
  Sampler2GmDlsIndex,
  Sampler2SampleStart,
  Sampler2LoopMode,
  Sampler2LoopSource,
  Sampler2LoopStart,
  Sampler2LoopLength,
  Sampler2TuneSemis,
  Sampler2TuneFine,
  Sampler2FreqParam,
  Sampler2FreqKT,
  Sampler2ReleaseExitsLoop,
  Sampler2Delay,

  Sampler2AmpEnvDelayTime,  // KEEP IN SYNC WITH EnvParamIndexOffsets
  Sampler2AmpEnvAttackTime,
  Sampler2AmpEnvAttackCurve,
  Sampler2AmpEnvHoldTime,
  Sampler2AmpEnvDecayTime,
  Sampler2AmpEnvDecayCurve,
  Sampler2AmpEnvSustainLevel,
  Sampler2AmpEnvReleaseTime,
  Sampler2AmpEnvReleaseCurve,
  Sampler2AmpEnvLegatoRestart,
  Sampler2AmpEnvMode,

  Sampler3Enabled,  // KEEP IN SYNC WITH SamplerParamIndexOffsets
  Sampler3Volume,
  Sampler3Pan,
  Sampler3KeyrangeMin,
  Sampler3KeyrangeMax,
  Sampler3BaseNote,
  Sampler3LegatoTrig,
  Sampler3Reverse,
  Sampler3GmDlsIndex,
  Sampler3SampleStart,
  Sampler3LoopMode,
  Sampler3LoopSource,
  Sampler3LoopStart,
  Sampler3LoopLength,
  Sampler3TuneSemis,
  Sampler3TuneFine,
  Sampler3FreqParam,
  Sampler3FreqKT,
  Sampler3ReleaseExitsLoop,
  Sampler3Delay,

  Sampler3AmpEnvDelayTime,  // KEEP IN SYNC WITH EnvParamIndexOffsets
  Sampler3AmpEnvAttackTime,
  Sampler3AmpEnvAttackCurve,
  Sampler3AmpEnvHoldTime,
  Sampler3AmpEnvDecayTime,
  Sampler3AmpEnvDecayCurve,
  Sampler3AmpEnvSustainLevel,
  Sampler3AmpEnvReleaseTime,
  Sampler3AmpEnvReleaseCurve,
  Sampler3AmpEnvLegatoRestart,
  Sampler3AmpEnvMode,

  Sampler4Enabled,  // KEEP IN SYNC WITH SamplerParamIndexOffsets
  Sampler4Volume,
  Sampler4Pan,
  Sampler4KeyrangeMin,
  Sampler4KeyrangeMax,
  Sampler4BaseNote,
  Sampler4LegatoTrig,
  Sampler4Reverse,
  Sampler4GmDlsIndex,
  Sampler4SampleStart,
  Sampler4LoopMode,
  Sampler4LoopSource,
  Sampler4LoopStart,
  Sampler4LoopLength,
  Sampler4TuneSemis,
  Sampler4TuneFine,
  Sampler4FreqParam,
  Sampler4FreqKT,
  Sampler4ReleaseExitsLoop,
  Sampler4Delay,

  Sampler4AmpEnvDelayTime,  // KEEP IN SYNC WITH EnvParamIndexOffsets
  Sampler4AmpEnvAttackTime,
  Sampler4AmpEnvAttackCurve,
  Sampler4AmpEnvHoldTime,
  Sampler4AmpEnvDecayTime,
  Sampler4AmpEnvDecayCurve,
  Sampler4AmpEnvSustainLevel,
  Sampler4AmpEnvReleaseTime,
  Sampler4AmpEnvReleaseCurve,
  Sampler4AmpEnvLegatoRestart,
  Sampler4AmpEnvMode,

  NumParams,
  Invalid,
};

// clang-format off
#define MAJ7_PARAM_VST_NAMES                                                                                           \
  {                                                                                                                    \
{"Master"}, \
{"Pan"},    \
{"PolyMon"},\
{"Unisono"},\
{"UniDet"}, \
{"UniSpr"}, \
{"FMBrigh"},\
{"PortTm"},           \
{"PBRng"},  \
{"MaxVox"}, \
{"Macro1"}, \
{"Macro2"}, \
{"Macro3"}, \
{"Macro4"}, \
{"FM2to1"}, \
{"FM3to1"}, \
{"FM4to1"}, \
{"FM1to2"}, \
{"FM3to2"}, \
{"FM4to2"}, \
{"FM1to3"},           \
{"FM2to3"}, \
{"FM4to3"}, \
{"FM1to4"}, \
{"FM2to4"}, \
{"FM3to4"}, \
{"O1En"},   \
{"O1Vol"},  \
{"O1Pan"},            \
{"O1KRmin"},\
{"O1KRmax"},\
{"O1Wave"}, \
{"O1ShpA"}, \
{"O1ShpB"}, \
{"O1PRst"}, \
{"O1Poff"}, \
{"O1Scen"},           \
{"O1ScFq"}, \
{"O1ScKt"}, \
{"O1Fq"},   \
{"O1FqKt"}, \
{"O1Semi"}, \
{"O1Fine"}, \
{"O1Mul"},  \
{"O1FMFb"},           \
{"AE1dlt"}, \
{"AE1att"}, \
{"AE1atc"}, \
{"AE1ht"},  \
{"AE1dt"},  \
{"AE1dc"},  \
{"AE1sl"},  \
{"AE1rt"},            \
{"AE1tc"},  \
{"AE1rst"}, \
{"AE1mode"},\
{"O2En"},   \
{"O2Vol"},  \
{"O2Pan"},  \
{"O2KRmin"},\
{"O2KRmax"},          \
{"O2Wave"}, \
{"O2ShpA"}, \
{"O2ShpB"}, \
{"O2PRst"}, \
{"O2Poff"}, \
{"O2Scen"}, \
{"O2ScFq"}, \
{"O2ScKt"},           \
{"O2Fq"},   \
{"O2FqKt"}, \
{"O2Semi"}, \
{"O2Fine"}, \
{"O2Mul"},  \
{"O2FMFb"}, \
{"AE2dlt"}, \
{"AE2att"},           \
{"AE2atc"}, \
{"AE2ht"},  \
{"AE2dt"},  \
{"AE2dc"},  \
{"AE2sl"},  \
{"AE2rt"},  \
{"AE2tc"},  \
{"AE2rst"},           \
{"AE2mode"},\
{"O3En"},   \
{"O3Vol"},  \
{"O3Pan"},  \
{"O3KRmin"},\
{"O3KRmax"},\
{"O3Wave"}, \
{"O3ShpA"},           \
{"O3ShpB"}, \
{"O3PRst"}, \
{"O3Poff"}, \
{"O3Scen"}, \
{"O3ScFq"}, \
{"O3ScKt"}, \
{"O3Fq"},   \
{"O3FqKt"},           \
{"O3Semi"}, \
{"O3Fine"}, \
{"O3Mul"},  \
{"O3FMFb"}, \
{"AE3dlt"}, \
{"AE3att"}, \
{"AE3atc"}, \
{"AE3ht"},            \
{"AE3dt"},  \
{"AE3dc"},  \
{"AE3sl"},  \
{"AE3rt"},  \
{"AE3tc"},  \
{"AE3rst"}, \
{"AE3mode"},\
{"O4En"},             \
{"O4Vol"},  \
{"O4Pan"},  \
{"O4KRmin"},\
{"O4KRmax"},\
{"O4Wave"}, \
{"O4ShpA"}, \
{"O4ShpB"}, \
{"O4PRst"},           \
{"O4Poff"}, \
{"O4Scen"}, \
{"O4ScFq"}, \
{"O4ScKt"}, \
{"O4Fq"},   \
{"O4FqKt"}, \
{"O4Semi"}, \
{"O4Fine"},           \
{"O4Mul"},  \
{"O4FMFb"}, \
{"AE4dlt"}, \
{"AE4att"}, \
{"AE4atc"}, \
{"AE4ht"},  \
{"AE4dt"},  \
{"AE4dc"},            \
{"AE4sl"},  \
{"AE4rt"},  \
{"AE4tc"},  \
{"AE4rst"}, \
{"AE4mode"},\
{"E1dlt"},  \
{"E1att"},  \
{"E1atc"},            \
{"E1ht"},   \
{"E1dt"},   \
{"E1dc"},   \
{"E1sl"},   \
{"E1rt"},   \
{"E1tc"},   \
{"E1rst"},  \
{"E1mode"},           \
{"E2dlt"},  \
{"E2att"},  \
{"E2atc"},  \
{"E2ht"},   \
{"E2dt"},   \
{"E2dc"},   \
{"E2sl"},   \
{"E2rt"},             \
{"E2tc"},   \
{"E2rst"},  \
{"E2mode"}, \
{"E3dlt"},  \
{"E3att"},  \
{"E3atc"},            \
{"E3ht"},   \
{"E3dt"},   \
{"E3dc"},   \
{"E3sl"},   \
{"E3rt"},   \
{"E3tc"},   \
{"E3rst"},  \
{"E3mode"},           \
{"E4dlt"},  \
{"E4att"},  \
{"E4atc"},  \
{"E4ht"},   \
{"E4dt"},   \
{"E4dc"},   \
{"E4sl"},   \
{"E4rt"},             \
{"E4tc"},   \
{"E4rst"},  \
{"E4mode"}, \
{"LFO1wav"},\
{"LFO1shA"},\
{"LFO1shB"},\
{"LFO1rst"},\
{"LFO1ph"},           \
{"LFO1fr"}, \
{"LFO1lp"}, \
{"LFO2wav"},\
{"LFO2shA"},\
{"LFO2shB"},\
{"LFO2rst"},\
{"LFO2ph"},           \
{"LFO2fr"}, \
{"LFO2lp"}, \
{"LFO3wav"},\
{"LFO3shA"},\
{"LFO3shB"},\
{"LFO3rst"},\
{"LFO3ph"},           \
{"LFO3fr"}, \
{"LFO3lp"}, \
{"LFO4wav"},\
{"LFO4shA"},\
{"LFO4shB"},\
{"LFO4rst"},\
{"LFO4ph"},           \
{"LFO4fr"}, \
{"LFO4lp"}, \
{"F1En"},   \
{"F1Resp"}, \
{"F1Q"},    \
{"F1Freq"}, \
{"F1FKT"},            \
{"M1en"},   \
{"M1src"},  \
{"M1dest1"},          \
{"M1dest2"},\
{"M1dest3"},\
{"M1dest4"},\
{"M1scl1"}, \
{"M1scl2"}, \
{"M1scl3"}, \
{"M1scl4"},           \
{"M1Aen"},  \
{"M1Asrc"}, \
{"M1Aatt"}, \
{"M1Acrv"}, \
{"M1rngA"}, \
{"M1rngB"}, \
{"M1rngXA"},\
{"M1rngXB"},          \
{"M2en"},   \
{"M2src"},  \
{"M2dest1"},\
{"M2dest2"},\
{"M2dest3"},\
{"M2dest4"},\
{"M2scl1"},           \
{"M2scl2"}, \
{"M2scl3"}, \
{"M2scl4"}, \
{"M2Aen"},  \
{"M2Asrc"}, \
{"M2Aatt"}, \
{"M2Acrv"}, \
{"M2rngA"},           \
{"M2rngB"}, \
{"M2rngXA"},\
{"M2rngXB"},\
{"M3en"},   \
{"M3src"},  \
{"M3dest1"},\
{"M3dest2"},\
{"M3dest3"},          \
{"M3dest4"},\
{"M3scl1"}, \
{"M3scl2"}, \
{"M3scl3"}, \
{"M3scl4"}, \
{"M3Aen"},  \
{"M3Asrc"},           \
{"M3Aatt"}, \
{"M3Acrv"}, \
{"M3rngA"}, \
{"M3rngB"}, \
{"M3rngXA"},\
{"M3rngXB"},\
{"M4en"},   \
{"M4src"},            \
{"M4dest1"},\
{"M4dest2"},\
{"M4dest3"},\
{"M4dest4"},\
{"M4scl1"}, \
{"M4scl2"}, \
{"M4scl3"},           \
{"M4scl4"}, \
{"M4Aen"},  \
{"M4Asrc"}, \
{"M4Aatt"}, \
{"M4Acrv"}, \
{"M4rngA"}, \
{"M4rngB"}, \
{"M4rngXA"},          \
{"M4rngXB"},\
{"M5en"},   \
{"M5src"},  \
{"M5dest1"},\
{"M5dest2"},\
{"M5dest3"},\
{"M5dest4"},\
{"M5scl1"}, \
{"M5scl2"}, \
{"M5scl3"}, \
{"M5scl4"}, \
{"M5Aen"},  \
{"M5Asrc"}, \
{"M5Aatt"}, \
{"M5Acrv"},           \
{"M5rngA"}, \
{"M5rngB"}, \
{"M5rngXA"},\
{"M5rngXB"},\
{"M6en"},   \
{"M6src"},  \
{"M6dest1"},\
{"M6dest2"},          \
{"M6dest3"},\
{"M6dest4"},\
{"M6scl1"}, \
{"M6scl2"}, \
{"M6scl3"}, \
{"M6scl4"}, \
{"M6Aen"},            \
{"M6Asrc"}, \
{"M6Aatt"}, \
{"M6Acrv"}, \
{"M6rngA"}, \
{"M6rngB"}, \
{"M6rngXA"},\
{"M6rngXB"},\
{"M7en"},             \
{"M7src"},  \
{"M7dest1"},\
{"M7dest2"},\
{"M7dest3"},\
{"M7dest4"},\
{"M7scl1"}, \
{"M7scl2"},           \
{"M7scl3"}, \
{"M7scl4"}, \
{"M7Aen"},  \
{"M7Asrc"}, \
{"M7Aatt"}, \
{"M7Acrv"}, \
{"M7rngA"}, \
{"M7rngB"},           \
{"M7rngXA"},\
{"M7rngXB"},\
{"M8en"},   \
{"M8src"},  \
{"M8dest1"},\
{"M8dest2"},\
{"M8dest3"},\
{"M8dest4"},          \
{"M8scl1"}, \
{"M8scl2"}, \
{"M8scl3"}, \
{"M8scl4"}, \
{"M8Aen"},  \
{"M8Asrc"}, \
{"M8Aatt"},           \
{"M8Acrv"}, \
{"M8rngA"}, \
{"M8rngB"}, \
{"M8rngXA"},\
{"M8rngXB"},\
{"M9en"},   \
{"M9src"},  \
{"M9dest1"},          \
{"M9dest2"},\
{"M9dest3"},\
{"M9dest4"},\
{"M9scl1"}, \
{"M9scl2"}, \
{"M9scl3"}, \
{"M9scl4"},           \
{"M9Aen"},  \
{"M9Asrc"}, \
{"M9Aatt"}, \
{"M9Acrv"}, \
{"M9rngA"}, \
{"M9rngB"}, \
{"M9rngXA"},\
{"M9rngXB"},          \
{"M10en"},  \
{"M10src"}, \
{"M10dst1"},\
{"M10dst2"},\
{"M10dst3"},\
{"M10dst4"},\
{"M10scl1"},          \
{"M10scl2"},\
{"M10scl3"},\
{"M10scl4"},\
{"M10Aen"}, \
{"M10Asrc"},\
{"M10Aatt"},\
{"M10Acrv"},\
{"M10rgA"},           \
{"M10rgB"}, \
{"M10rgXA"},\
{"M10rgXB"},\
{"M11en"},  \
{"M11src"}, \
{"M11dst1"},\
{"M11dst2"},\
{"M11dst3"},          \
{"M11dst4"},\
{"M11scl1"},\
{"M11scl2"},\
{"M11scl3"},\
{"M11scl4"},\
{"M11Aen"}, \
{"M11Asrc"},          \
{"M11Aatt"},\
{"M11Acrv"},\
{"M11rgA"}, \
{"M11rgB"}, \
{"M11rgXA"},\
{"M11rgXB"},\
{"M12en"},  \
{"M12src"},           \
{"M12dst1"},\
{"M12dst2"},\
{"M12dst3"},\
{"M12dst4"},\
{"M12scl1"},\
{"M12scl2"},\
{"M12scl3"},          \
{"M12scl4"},\
{"M12Aen"}, \
{"M12Asrc"},\
{"M12Aatt"},\
{"M12Acrv"},\
{"M12rgA"}, \
{"M12rgB"}, \
{"M12rgXA"},          \
{"M12rgXB"},\
{"M13en"},  \
{"M13src"}, \
{"M13dst1"},\
{"M13dst2"},\
{"M13dst3"},\
{"M13dst4"},\
{"M13scl1"},\
{"M13scl2"},\
{"M13scl3"},\
{"M13scl4"},\
{"M13Aen"}, \
{"M13Asrc"},\
{"M13Aatt"},\
{"M13Acrv"},          \
{"M13rgA"}, \
{"M13rgB"}, \
{"M13rgXA"},\
{"M13rgXB"},\
{"M14en"},  \
{"M14src"}, \
{"M14dst1"},\
{"M14dst2"},          \
{"M14dst3"},\
{"M14dst4"},\
{"M14scl1"},\
{"M14scl2"},\
{"M14scl3"},\
{"M14scl4"},\
{"M14Aen"},           \
{"M14Asrc"},\
{"M14Aatt"},\
{"M14Acrv"},\
{"M14rgA"}, \
{"M14rgB"}, \
{"M14rgXA"},\
{"M14rgXB"},\
{"M15en"},            \
{"M15src"}, \
{"M15dst1"},\
{"M15dst2"},\
{"M15dst3"},\
{"M15dst4"},\
{"M15scl1"},\
{"M15scl2"},          \
{"M15scl3"},\
{"M15scl4"},\
{"M15Aen"}, \
{"M15Asrc"},\
{"M15Aatt"},\
{"M15Acrv"},\
{"M15rgA"}, \
{"M15rgB"},           \
{"M15rgXA"},\
{"M15rgXB"},\
{"M16en"},  \
{"M16src"}, \
{"M16dst1"},\
{"M16dst2"},\
{"M16dst3"},\
{"M16dst4"},          \
{"M16scl1"},\
{"M16scl2"},\
{"M16scl3"},\
{"M16scl4"},\
{"M16Aen"}, \
{"M16Asrc"},\
{"M16Aatt"},          \
{"M16Acrv"},\
{"M16rgA"}, \
{"M16rgB"}, \
{"M16rgXA"},\
{"M16rgXB"},\
{"M17en"},  \
{"M17src"}, \
{"M17dst1"},          \
{"M17dst2"},\
{"M17dst3"},\
{"M17dst4"},\
{"M17scl1"},\
{"M17scl2"},\
{"M17scl3"},\
{"M17scl4"},          \
{"M17Aen"}, \
{"M17Asrc"},\
{"M17Aatt"},\
{"M17Acrv"},\
{"M17rgA"}, \
{"M17rgB"}, \
{"M17rgXA"},\
{"M17rgXB"},          \
{"M18en"},  \
{"M18src"}, \
{"M18dst1"},\
{"M18dst2"},\
{"M18dst3"},\
{"M18dst4"},\
{"M18scl1"},          \
{"M18scl2"},\
{"M18scl3"},\
{"M18scl4"},\
{"M18Aen"}, \
{"M18Asrc"},\
{"M18Aatt"},\
{"M18Acrv"},\
{"M18rgA"},           \
{"M18rgB"}, \
{"M18rgXA"},\
{"M18rgXB"},\
{"S1En"},   \
{"S1Vol"},  \
{"S1Pan"},  \
{"S1KRmin"},\
{"S1KRmax"},          \
{"S1base"}, \
{"S1LTrig"},\
{"S1Rev"},  \
{"S1gmidx"},\
{"S1strt"}, \
{"S1LMode"},\
{"S1LSrc"},           \
{"S1Lbeg"}, \
{"S1Llen"}, \
{"S1TunS"}, \
{"S1TunF"}, \
{"S1Frq"},  \
{"S1FrqKT"},\
{"S1RelX"},           \
{"S1Dly"},  \
{"S1Edlt"}, \
{"S1Eatt"}, \
{"S1Eatc"}, \
{"S1Eht"},  \
{"S1Edt"},  \
{"S1Edc"},  \
{"S1Esl"},            \
{"S1Ert"},  \
{"S1Etc"},  \
{"S1Erst"}, \
{"S1Emode"},\
{"S2En"},   \
{"S2Pan"},  \
{"S2Vol"},  \
{"S2KRmin"},          \
{"S2KRmax"},\
{"S2base"}, \
{"S2LTrig"},\
{"S2Rev"},  \
{"S2gmidx"},\
{"S2strt"}, \
{"S2LMode"},          \
{"S2LSrc"}, \
{"S2Lbeg"}, \
{"S2Llen"}, \
{"S2TunS"}, \
{"S2TunF"}, \
{"S2Frq"},  \
{"S2FrqKT"},\
{"S2RelX"}, \
{"S2Dly"},  \
{"S2Edlt"}, \
{"S2Eatt"}, \
{"S2Eatc"}, \
{"S2Eht"},  \
{"S2Edt"},  \
{"S2Edc"},            \
{"S2Esl"},  \
{"S2Ert"},  \
{"S2Etc"},  \
{"S2Erst"}, \
{"S2Emode"},\
{"S3En"},   \
{"S3Vol"},  \
{"S3Pan"},            \
{"S3KRmin"},\
{"S3KRmax"},\
{"S3base"}, \
{"S3LTrig"},\
{"S3Rev"},  \
{"S3gmidx"},\
{"S3strt"},           \
{"S3LMode"},\
{"S3LSrc"}, \
{"S3Lbeg"}, \
{"S3Llen"}, \
{"S3TunS"}, \
{"S3TunF"}, \
{"S3Frq"},  \
{"S3FrqKT"},          \
{"S3RelX"}, \
{"S3Dly"},  \
{"S3Edlt"}, \
{"S3Eatt"}, \
{"S3Eatc"}, \
{"S3Eht"},  \
{"S3Edt"},            \
{"S3Edc"},  \
{"S3Esl"},  \
{"S3Ert"},  \
{"S3Etc"},  \
{"S3Erst"}, \
{"S3Emode"},\
{"S4En"},   \
{"S4Vol"},            \
{"S4Pan"},  \
{"S4KRmin"},\
{"S4KRmax"},\
{"S4base"}, \
{"S4LTrig"},\
{"S4Rev"},  \
{"S4gmidx"},          \
{"S4strt"}, \
{"S4LMode"},\
{"S4LSrc"}, \
{"S4Lbeg"}, \
{"S4Llen"}, \
{"S4TunS"}, \
{"S4TunF"}, \
{"S4Frq"},            \
{"S4FrqKT"},\
{"S4RelX"}, \
{"S4Dly"},  \
{"S4Edlt"}, \
{"S4Eatt"}, \
{"S4Eatc"}, \
{"S4Eht"},            \
{"S4Edt"},  \
{"S4Edc"},  \
{"S4Esl"},  \
{"S4Ert"},  \
{"S4Etc"},  \
{"S4Erst"}, \
{"S4Emode"},                       \
  }
// clang-format on

enum class MainParamIndices //: uint8_t
{
  MasterVolume,
  Pan,
  VoicingMode,
  Unisono,
  UnisonoDetune,
  UnisonoStereoSpread,
  FMBrightness,

  PortamentoTime,
  PitchBendRange,

  MaxVoices,

  Macro1,
  Macro2,
  Macro3,
  Macro4,

  FMAmt2to1,
  FMAmt3to1,
  FMAmt4to1,
  FMAmt1to2,
  FMAmt3to2,
  FMAmt4to2,
  FMAmt1to3,
  FMAmt2to3,
  FMAmt4to3,
  FMAmt1to4,
  FMAmt2to4,
  FMAmt3to4,

  Count,
};

// offsets which are shared between sampler & oscillator, usable then by the caller on all sources equally.
enum class SourceParamIndexOffsets : uint8_t  // MUST BE IN SYNC WITH ABOVE
{
  Enabled,      // keep in sync: SamplerParamIndexOffsets, OscParamIndexOffsets, SourceParamIndexOffsets
  Volume,       // keep in sync: SamplerParamIndexOffsets, OscParamIndexOffsets, SourceParamIndexOffsets
  Pan,          // keep in sync: SamplerParamIndexOffsets, OscParamIndexOffsets, SourceParamIndexOffsets
  KeyRangeMin,  // keep in sync: SamplerParamIndexOffsets, OscParamIndexOffsets, SourceParamIndexOffsets
  KeyRangeMax,  // keep in sync: SamplerParamIndexOffsets, OscParamIndexOffsets, SourceParamIndexOffsets
};

enum class SamplerParamIndexOffsets : uint8_t  // MUST BE IN SYNC WITH ABOVE
{
  Enabled,  // keep in sync: SamplerParamIndexOffsets, OscParamIndexOffsets, SourceParamIndexOffsets
  Volume,// keep in sync: SamplerParamIndexOffsets, OscParamIndexOffsets, SourceParamIndexOffsets
  Pan,// keep in sync: SamplerParamIndexOffsets, OscParamIndexOffsets, SourceParamIndexOffsets
  KeyRangeMin,// keep in sync: SamplerParamIndexOffsets, OscParamIndexOffsets, SourceParamIndexOffsets
  KeyRangeMax,// keep in sync: SamplerParamIndexOffsets, OscParamIndexOffsets, SourceParamIndexOffsets

  BaseNote,
  LegatoTrig,
  Reverse,
  GmDlsIndex,
  SampleStart,
  LoopMode,
  LoopSource,
  LoopStart,
  LoopLength,
  TuneSemis,
  TuneFine,
  FreqParam,
  FreqKT,
  ReleaseExitsLoop,
  Delay,
  AmpEnvDelayTime,
  Count = AmpEnvDelayTime,
};

enum class OscParamIndexOffsets : uint8_t  // MUST BE IN SYNC WITH ABOVE
{
  Enabled,// keep in sync: SamplerParamIndexOffsets, OscParamIndexOffsets, SourceParamIndexOffsets
  Volume,// keep in sync: SamplerParamIndexOffsets, OscParamIndexOffsets, SourceParamIndexOffsets
  Pan,// keep in sync: SamplerParamIndexOffsets, OscParamIndexOffsets, SourceParamIndexOffsets
  KeyRangeMin,// keep in sync: SamplerParamIndexOffsets, OscParamIndexOffsets, SourceParamIndexOffsets
  KeyRangeMax,// keep in sync: SamplerParamIndexOffsets, OscParamIndexOffsets, SourceParamIndexOffsets

  Waveform,
  WaveshapeA,
  WaveshapeB,
  PhaseRestart,
  PhaseOffset,
  SyncEnable,
  SyncFrequency,
  SyncFrequencyKT,
  FrequencyParam,
  FrequencyParamKT,
  PitchSemis,
  PitchFine,
  FreqMul,
  FMFeedback,
  AmpEnvDelayTime,
  Count = AmpEnvDelayTime,
};

enum class ModParamIndexOffsets : uint8_t  // MUST BE IN SYNC WITH ABOVE
{
  Enabled,
  Source,
  Destination1,
  Destination2,
  Destination3,
  Destination4,
  Scale1,
  Scale2,
  Scale3,
  Scale4,
  AuxEnabled,
  AuxSource,
  AuxAttenuation,
  AuxCurve,

  SrcRangeMin,
  SrcRangeMax,
  AuxRangeMin,
  AuxRangeMax,

  Count,
};
enum class LFOParamIndexOffsets : uint8_t  // MUST BE IN SYNC WITH ABOVE
{
  Waveform,
  WaveshapeA,
  WaveshapeB,
  Restart,
  PhaseOffset,
  FrequencyParam,
  Sharpness,
  Count,
};
enum class EnvParamIndexOffsets : uint8_t  // MUST BE IN SYNC WITH ABOVE
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
  LegatoRestart,
  Mode,
  Count,
};


//// FILTER AUX INFO  ------------------------------------------------------------
enum class
    FilterParamIndexOffsets : uint8_t  // MUST SYNC WITH PARAMINDICES & AuxParamIndexOffsets & FilterAuxModIndexOffsets
{
  Enabled,
  FilterResponse,  // filter type
  Q,           // param2: filter Q
  Freq,        // filter freq
  FreqKT,      // filter KT
  Count,
};

//enum class FilterAuxModDestOffsets : uint8_t  // MUST SYNC WITH PARAMINDICES & AuxParamIndexOffsets
//{
//  Freq,  // filter freq
//  Q,     // filter Q
//  Count,
//};

// ------------------------------------------------------------
enum class EnvelopeMode : uint8_t
{
  Sustain,  // uses all features of the envelope.
  OneShot,  // ignores note offs, decays to 0.
  Count,
};


}  // namespace M7


}  // namespace WaveSabreCore
