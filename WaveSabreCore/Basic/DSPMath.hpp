#pragma once

#include "math.hpp"
#include "Pair.hpp"
#include "StrongScalar.hpp"
// #include <algorithm>
// #include <memory>
// #include <optional>
// #include "./StrongScalar.hpp"

namespace WaveSabreCore
{
namespace M7
{

struct Param01Tag
{
};
using Param01 = StrongScalar<real_t, Param01Tag>;


// template <typename Tret, typename Tb>
// Tret AddEnum(Tret a, Tb b)
// {
//   return Tret((int)a + (int)b);
// }


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
};

struct IntParamConfig
{
  const int mMinValInclusive;
  const int mMaxValInclusive;
  constexpr int GetDiscreteValueCount() const
  {
    return mMaxValInclusive - mMinValInclusive + 1;
  }
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
extern __declspec(selectany) const FreqParamConfig gLFOLPFreqConfig{20, 7, 15.486820576352429f};
extern __declspec(selectany) const FreqParamConfig gBitcrushFreqConfig{gFilterFreqConfig};
extern __declspec(selectany) const FreqParamConfig gSourceFreqConfig{gFilterFreqConfig};
extern __declspec(selectany) const FreqParamConfig
    gLFOFreqConfig{1.5f, 8, -0.37631656229592636f};  // well midi note here is meaningless and i hope you never use it.
extern __declspec(selectany) const FreqParamConfig gSyncFreqConfig{gFilterFreqConfig};

extern __declspec(selectany) const IntParamConfig gSourcePitchSemisRange{-36, 36};
extern __declspec(selectany) const IntParamConfig gKeyRangeCfg{0, 127};
extern __declspec(selectany) const IntParamConfig gPitchBendCfg{-gPitchBendMaxRange, gPitchBendMaxRange};
extern __declspec(selectany) const IntParamConfig gUnisonoVoiceCfg{1, gUnisonoVoiceMax};
extern __declspec(selectany) const IntParamConfig gMaxVoicesCfg{1, gMaxMaxVoices};
extern __declspec(selectany) const IntParamConfig gGmDlsIndexParamCfg{-1, gGmDlsSampleCount};

extern __declspec(selectany) const VolumeParamConfig gVolumeCfg6db{1.9952623149688795f, 6.0f};
extern __declspec(selectany) const VolumeParamConfig gVolumeCfg12db{3.9810717055349722f, 12.0f};
extern __declspec(selectany) const VolumeParamConfig gVolumeCfg24db{15.848931924611133f, 24.0f};
extern __declspec(selectany) const VolumeParamConfig gVolumeCfg36db{63.09573444801933f, 36.0f};
extern __declspec(selectany) const VolumeParamConfig gMasterVolumeCfg = gVolumeCfg6db;
extern __declspec(selectany) const VolumeParamConfig gUnityVolumeCfg{1, 0};


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
bool DoesEncounter(double t1, double t2, float x);

/**
             * Converts a linear value to decibels.  Returns <= aMinDecibels if the linear
             * value is 0.
             */


// On a modular line with value [0,1),
// and start, length define a segment along the line; [start, start+length)
// return true if x falls on the segment.
inline bool DoesEncounter2(double start, double length, float xf)
{
  // Handle degenerate lengths quickly
  if (!(length > 0.0))
    return false;  // also makes NaN -> false
  if (length >= 1.0)
    return true;

  // x relative to start, wrapped into [0,1)
  double d = static_cast<double>(xf) - start;
  d += (d < 0.0);  // branchless: adds 1.0 if negative

  // Half-open membership test
  return d < length;
}


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
