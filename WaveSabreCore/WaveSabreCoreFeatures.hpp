#pragma once

// SELECTABLE_OUTPUT_STREAM_SUPPORT is set by the cmake config;
// originally for enabling output stream support for Maj7 (giga)synth. Quickly
// became a general flag for anything that is not size-optimized.

// note you can also use MIN_SIZE_REL,
// defined for both MinSizeRel and MinSizeRelWithDebugInfo

// compensation gain is interesting - it works pretty well, but there's actually no great way to do this at the oscillator
// level. So while there may be some interesting use for it, it's not useful right now.
#undef ENABLE_BIQUAD_COMPENSATION_GAIN

#undef ENABLE_DIODE_FILTER // is about ~350 bytes of compressed binary.
#undef ENABLE_K35_FILTER // is about ~380 bytes of binary.
#define ENABLE_BUTTERWORTH_FILTER // about 150 bytes of compressed binary.
#undef ENABLE_NOTCH_FILTER
#undef ENABLE_ALLPASS_FILTER

// for sat / MBC saturation...
#undef MAJ7SAT_ENABLE_RARE_MODELS
#define MAJ7SAT_ENABLE_ANALOG
#undef MAJ7SAT_ENABLE_MIDSIDE

#define ENABLE_PITCHBEND

#undef ENABLE_TRIANGLE_FOLD_WAVEFORM // quite a huge amount of binary due to complex shape building.
#undef ENABLE_PULSE4_WAVEFORM

// user sample support (different than gmdls)
#undef MAJ7_INCLUDE_GSM_SUPPORT // ~1kb of minified code.

// see width sources for more; mostly the rotation stuff.
#define MAJ7WIDTH_FULL_FEATURE


// 8 = 96db/oct
// 4 = 48db/oct
// this doesn't  really affect binary size. keep @ 8
static constexpr size_t kMaxBiquadStages = 8;
