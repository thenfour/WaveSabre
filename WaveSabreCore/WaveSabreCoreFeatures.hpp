#pragma once

// SELECTABLE_OUTPUT_STREAM_SUPPORT is set by the cmake config;
// originally for enabling output stream support for Maj7 (giga)synth. Quickly
// became a general flag for anything that is not size-optimized.

// note you can also use MIN_SIZE_REL,
// defined for both MinSizeRel and MinSizeRelWithDebugInfo

// compensation gain is interesting - it works pretty well, but there's actually no great way to do this at the oscillator
// level. So while there may be some interesting use for it, it's not useful right now.
#undef ENABLE_BIQUAD_COMPENSATION_GAIN

#undef ENABLE_DIODE_FILTER // is about 3kb of binary.
#undef ENABLE_K35_FILTER // is about 3kb of binary.

// for sat / MBC saturation...
#undef MAJ7SAT_ENABLE_RARE_MODELS
#define MAJ7SAT_ENABLE_ANALOG
#undef MAJ7SAT_ENABLE_MIDSIDE

#undef ENABLE_TRIANGLE_FOLD_WAVEFORM

#define NOINLINE __declspec(noinline)

#define INLINE inline
#ifdef MIN_SIZE_REL
  #define FORCE_INLINE inline
#else
// for non-size-optimized builds (bloaty), force inline some things when profiling suggests its performant.
  #define FORCE_INLINE __forceinline
#endif

