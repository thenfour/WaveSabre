#pragma once

#include "Pair.hpp"
#include "StrongScalar.hpp"
#include "math.hpp"
#include "Helpers.h"
#include "Enum.hpp"

namespace WaveSabreCore
{
namespace M7
{

struct Param01Tag
{
};
using Param01 = StrongScalar<real_t, Param01Tag>;


struct FreqParamConfig
{
  const float mCenterFrequency;
  const float mScale;
  const float mCenterMidiNote;

  // calculate center midi note with this js in browser console:
  // let hztomidi = function(hz) { return 12.0 * Math.log2(Math.max(8.0, hz) / 440) + 69; }
  // hztomidi(1000)
  // = 83.21309485364912

  // i would prefer a compile-time version but whatev
  constexpr FreqParamConfig(float centerFrequency, float scale, float centerMidiNote)
      : mCenterFrequency(centerFrequency)
      , mScale(scale)
      , mCenterMidiNote(centerMidiNote)
  {
  }
  constexpr FreqParamConfig(const FreqParamConfig& rhs) = default;

  float GetFrequency(float freqParam01, float ktParam01, float noteHz) const;

  float GetFrequencyWithoutKT(float freqParam01) const
  {
    return GetFrequency(freqParam01, 0, 0);
  }

  // // param modulation is normal krate param mod
  // // noteModulation includes osc.mPitchFine + osc.mPitchSemis + detune;
  // float GetMidiNote(float freqParam01, float ktParam01, float playingMidiNote) const
  // {
  //   float ktNote = playingMidiNote + 24;  // center represents playing note + 2 octaves.

  //   float centerNote = math::lerp(mCenterMidiNote, ktNote, ktParam01);

  //   freqParam01 = math::clamp01(freqParam01);
  //   freqParam01 -= 0.5f;
  //   freqParam01 *= mScale;  // 10; // rescale from 0-1 to -5 to +5 (octaves)
  //   float paramSemis = centerNote +
  //                      freqParam01 * 12;  // each 1 param = 1 octave. because we're in semis land, it's just a mul.
  //   return paramSemis;
  // }

  float GetParam01ValueForFrequencyAssumingNoKeytracking(float hz) const;
};

struct FloatN11ParamCore
{
  [[nodiscard]]
  static constexpr float serializeToVstParam(float floatN11)
  {
    return math::clampN11(floatN11);
  }

  [[nodiscard]]
  static constexpr float deserializeFromVstParam(float param)
  {
    return math::clampN11(param);
  }

#ifndef MIN_SIZE_REL

  // this is for knobs / UI -- careful never to use this as part of the synth's internal param handling
  [[nodiscard]]
  static constexpr float serializeToFloat01ForKnob(float floatN11)
  {
    return math::clamp01((floatN11 + 1) * 0.5f);
  }

  [[nodiscard]]
  static constexpr float deserializeFromFloat01ForKnob(float float01)
  {
    return math::clampN11(float01 * 2 - 1);
  }
#endif  // !MIN_SIZE_REL
};

struct IntParamConfig
{
  const int mMinValInclusive;
  const int mMaxValInclusive;
  const int mUsableCount;

  // #128: this should be a FIXED value for all enums/ints,
  // so when the synth design changes it doesn't clobber existing presets.
  // #130; this should also not be too large, otherwise discrete values are extremely
  // close together in float space and dragging knobs can feel jumpy / awkward / unstable.
  //
  // NB: keep also in mind that we must serialize to 0-1 floats, not bipolars.
  // -4096 .. +4096 shall map to 0 .. 8192.
  //static constexpr int kSerializableOffset = 4096;
  //static constexpr int kSerializableScale = 2 * kSerializableOffset;
  static constexpr int kSerializableScale = 8192;

  constexpr IntParamConfig(int minValInclusive, int maxValInclusive)
      : mMinValInclusive(minValInclusive)
      , mMaxValInclusive(maxValInclusive)
      , mUsableCount(1 + maxValInclusive - minValInclusive)
  {
  }

  [[nodiscard]]
  static constexpr float serializeToFloatN11(int code)
  {
    return float(code) / float(kSerializableScale);
    //const int clamped = math::ClampI(code, -kSerializableOffset, kSerializableOffset);
    //const int index = clamped + kSerializableOffset;
    //return (float(index) + 0.5f) / float(kSerializableScale);
  }

  [[nodiscard]]
  static constexpr int deserializeFromFloatN11(float f01)
  {
    return math::round<int>(f01 * kSerializableScale);
    //const float clamped = math::clamp01(f01);
    //const int index = int(clamped * float(kSerializableScale));
    //return index - kSerializableOffset;
  }

#ifndef MIN_SIZE_REL

  // this is for knobs / UI -- careful never to use this as part of the synth's internal param handling
  [[nodiscard]]
  float serializeToFloat01WithUsableRange(int code) const
  {
    const int clamped = math::ClampI(code, mMinValInclusive, mMaxValInclusive);
    const int index = clamped - mMinValInclusive;
    return (float(index) + 0.5f) / float(mUsableCount);
  }

  [[nodiscard]]
  int deserializeFromFloat01WithUsableRange(float f01) const
  {
    const float clamped = math::clamp01(f01);
    int index = int(clamped * float(mUsableCount));
    if (index >= mUsableCount)
      index = mUsableCount - 1;
    return mMinValInclusive + index;
  }
#endif  // !MIN_SIZE_REL
};

struct VolumeParamConfig
{
  const float mMaxValLinear;
  const float mMaxValDb;
  // plug into a browser console to convert around
  /*
            let todb = function(aLinearValue, aMinDecibels) {
                const LOG10E = Math.LOG10E || Math.log(10); // Constant value of log base 10 of e
                const decibels = 20 * Math.log10(aLinearValue); // Calculate decibels using logarithmic function
                return (decibels !== -Infinity) ? decibels : minimum decibels; // Check for infinite value and return the result or minimum decibels
            }
            let tolinear = function(aDecibelValue) {
                return Math.pow(10, aDecibelValue / 20);
            }
            */
};

extern bool gAlwaysTrue;

// these help avoid -infinity and NaN and other problems with "0" linear representing -infinity decibels.
// it should represent the full dynamic range of audio, because these will be used in converting sample values in audio processing.
// note: 16-bit audio has around 96db of dynamic range
// 24-bit has ~144db
// 32-bit has ~186db.
static constexpr float gMinGainDecibels = -192.0f;
static constexpr float gMinGainLinear = 0.00000000025118864315095819916852f;  // DecibelsToLinear(gMinGainDecibels);

static constexpr real_t gFreqParamKTUnity = 0.3f;
static constexpr size_t gModulationSpecDestinationCount = 4;

static constexpr float gSourcePitchFineRangeSemis = 2;

static constexpr int gPitchBendMaxRange = 24;
static constexpr int gUnisonoVoiceMax = 12;


static constexpr int gMaxMaxVoices = 64;

static constexpr int gGmDlsSampleCount = 495;
static constexpr real_t gFrequencyMulMax = 64;

// these structs appear as duplicate data in the binary due to being static constexpr,
// however the constexpr aspect reduces code size, and the compressor handles this fine.
// so SizeBench will alert that there's redundant data, but by making these extern, you can only increase the resulting binary.
extern __declspec(selectany) const FreqParamConfig gFilterFreqConfig{1000, 10, 83.21309485364912f};
extern __declspec(selectany) const FreqParamConfig gBitcrushFreqConfig{gFilterFreqConfig};
extern __declspec(selectany) const FreqParamConfig gSourceFreqConfig{gFilterFreqConfig};
extern __declspec(selectany) const FreqParamConfig gLFOLPFreqConfig{60, 8, 0};
extern __declspec(selectany) const FreqParamConfig gLFOFreqConfig{1.5f, 8, 0};  // midi note here is meaningless
extern __declspec(selectany) const FreqParamConfig gSyncFreqConfig{gFilterFreqConfig};

// int ranges are NOT used in the optimized synth param accessors; they are used by VSTs for knob
// ranges and display conversions. The synth itself uses the raw int values and can
// refer to these if needed as constants
static constexpr IntParamConfig gSourcePitchSemisRange{-36, 36};
static constexpr IntParamConfig gKeyRangeCfg{0, 127};
static constexpr IntParamConfig gPitchBendCfg{-gPitchBendMaxRange, gPitchBendMaxRange};
static constexpr IntParamConfig gUnisonoVoiceCfg{1, gUnisonoVoiceMax};
static constexpr IntParamConfig gMaxVoicesCfg{1, gMaxMaxVoices};
static constexpr IntParamConfig gGmDlsIndexParamCfg{-1, gGmDlsSampleCount};

extern __declspec(selectany) const VolumeParamConfig gVolumeCfg6db{1.9952623149688795f, 6.0f};
extern __declspec(selectany) const VolumeParamConfig gVolumeCfg12db{3.9810717055349722f, 12.0f};
extern __declspec(selectany) const VolumeParamConfig gVolumeCfg24db{15.848931924611133f, 24.0f};
extern __declspec(selectany) const VolumeParamConfig gVolumeCfg36db{63.09573444801933f, 36.0f};
extern __declspec(selectany) const VolumeParamConfig gMasterVolumeCfg = gVolumeCfg6db;
extern __declspec(selectany) const VolumeParamConfig gUnityVolumeCfg{1, 0};

static constexpr IntParamConfig gLFOBeatNumeratorCfg{0, 16};
static constexpr IntParamConfig gLFOBeatDenominatorCfg{1, 24};

namespace math
{

real_t CalculateInc01PerSampleForMS(real_t ms);

inline float sign(float x)
{
  return x < 0 ? -1.0f : 1.0f;  // ::signbit(x);
}

inline bool IsSilentGain(float gain)
{
  return gain <= gMinGainLinear;
}

float MillisecondsToSamples(float ms);

// where t1, t2, and x are periodic values [0,1).
// and t1 is "before" t2,
// return true if x falls between t1 and t2.
// so if t2 < t1, it means a cycle wrap.
//bool DoesEncounter(double t1, double t2, float x);

/**
             * Converts a linear value to decibels.  Returns <= aMinDecibels if the linear
             * value is 0.
             */


// On a modular line with value [0,1),
// and start, length define a segment along the line; [start, start+length)
// return true if x falls on the segment.
//inline bool DoesEncounter2(double start, double length, float xf)
//{
//  // Handle degenerate lengths quickly
//  if (!(length > 0.0))
//    return false;  // also makes NaN -> false
//  if (length >= 1.0)
//    return true;
//
//  // x relative to start, wrapped into [0,1)
//  double d = static_cast<double>(xf) - start;
//  d += (d < 0.0);  // branchless: adds 1.0 if negative
//
//  // Half-open membership test
//  return d < length;
//}


float LinearToDecibels(float aLinearValue, float aMinDecibels = gMinGainDecibels);

/**
             * Converts a decibel value to a linear value.
             */
float DecibelsToLinear(float aDecibels, float aNegInfDecibels = gMinGainDecibels);

// don't use a LUT because we want to support pitch bend and glides and stuff. using a LUT + interpolation would be
// asinine.
float MIDINoteToFreq(float x);
float SemisToFrequencyMul(float x);

//float FrequencyToMIDINote(float hz);

float CalcTanhGainCompensation(float driveFactor);

// our pan functions have a sqrt pan law, which by default attenuates the center position ~3db.
// however take for example the width control, or any effect with a pan control. in default center position,
// users expect the output to be at unity. so instead of attenuating center, compensate for that which
// will have the side-effect of boosting when panned.
const float gPanCompensationGainLin = 1.41421356237f;  // sqrt(2)
FloatPair PanToLRVolumeParams(float panN11);

FloatPair PanToFactor(float panN11);


inline float Sample16To32Bit(int16_t s)
{
  return clampN11(float(s) / 32768);
}

inline int16_t Sample32To16(float f)
{
  return int16_t(ClampI(int32_t(f * 32768), -32768, 32767));
}

// converts the 16-bit serialized default value [-32767..32767] to a
// -1..1 float. Using 32767 as divisor lets us represent -1 and +1 fully using the same
// number of steps in pos / neg.
inline float Default16ToFloatN11(int16_t s)
{
  return clampN11(float(s) / 32767);
}
inline int16_t FloatN11ToDefault16(float f)
{
  return int16_t(ClampI(int32_t(f * 32767), -32767, 32767));
}

inline float ClampFrequencyHz(float hz)
{
  return math::ClampI(hz, 20.0f, Helpers::NyquistHz);
}

}  // namespace math

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
// takes an array of mute & solo switches, and outputs whether each band would be output eventually.
template <size_t NBands>
void CalculateMuteSolo(bool (&mutes)[NBands], bool (&solos)[NBands], bool (&outputs)[NBands])
{
  auto x = outputs[0];
  bool soloExists = false;
  for (size_t i = 0; i < NBands; ++i)
  {
    if (solos[i])
    {
      soloExists = true;
      break;
    }
  }

  for (size_t i = 0; i < NBands; ++i)
  {
    bool mute = mutes[i];
    bool solo = solos[i];
    if (soloExists)
    {  // if solo exists, it's enabled if it's part of the solo'd group.
      outputs[i] = solo;
    }
    else
    {
      outputs[i] = !mute;
    }
  }
}
#endif  // SELECTABLE_OUTPUT_STREAM SUPPORT


// for detune & unisono, based on enabled oscillators etc, distribute a [-1,1] value among many items.
// there could be various ways of doing this but to save space just unify.
// if an odd number of items, then 1 = centered @ 0.0f.
// then, the remaining even # of items gets + and - respectively, distributed between 0 - 1.
void BipolarDistribute(size_t count, const bool* enabled, float* outp);

void ImportDefaultsArray(size_t count, const int16_t* src, float* paramCacheOffset);

INLINE float MillisecondsToHertz(float ms)
{
  return (ms > 0) ? (1000.0f / ms) : 0;
}

float CalcDelayMS(float eighths, float ms, float frequencyHz);

// no binary savings by de-inlining this.
INLINE float CalcFrequencyHz(int beatNumerator, int beatDenominator, float eighthsOffset)
{
  if (beatDenominator < 1)
    return 0;
  float beats = beatNumerator / float(beatDenominator);
  float eighths = eighthsOffset + beats * 8;
  return MillisecondsToHertz(CalcDelayMS(eighths, 0, 0));
}


enum class TimeBasis  //: uint8_t
{
  Frequency,
  Time,
  Beats,
  Count,
};
#define TIME_BASIS_CAPTIONS(symbolName)                                                                                \
  static constexpr char const* const symbolName[(int)::WaveSabreCore::M7::TimeBasis::Count]{                           \
      "Frequency",                                                                                                     \
      "Time",                                                                                                          \
      "Beats",                                                                                                         \
  };


}  // namespace M7


}  // namespace WaveSabreCore
