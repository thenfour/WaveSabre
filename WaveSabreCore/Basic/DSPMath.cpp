
#include "DSPMath.hpp"
#include "Helpers.h"
#include "Math.hpp"


namespace WaveSabreCore
{
namespace M7
{

bool gAlwaysTrue = true;


#ifdef ENABLE_OSC_LOG
M7::OscLog gOscLog;
#endif  // ENABLE_OSC_LOG

namespace math
{

real_t CalculateInc01PerSampleForMS(real_t ms)
{
  return clamp01(1000.0f / (std::max(0.01f, ms) * (real_t)Helpers::CurrentSampleRate));
}

float MillisecondsToSamples(float ms)
{
  static constexpr float oneOver1000 = 1.0f / 1000.0f;  // obsessive optimization?
  return (ms * ::WaveSabreCore::Helpers::CurrentSampleRateF) * oneOver1000;
}

bool DoesEncounter(double t1, double t2, float x)
{
  if (t1 < t2)
  {
    return (t1 < x && x <= t2);
  }
  return (x <= t2 || x > t1);
}

/*
Converts a linear value to decibels.  Returns <= aMinDecibels if the linear
value is 0.
*/
float LinearToDecibels(float aLinearValue, float aMinDecibels)
{
  return (aLinearValue > FloatEpsilon) ? 20.0f * math::log10(aLinearValue) : aMinDecibels;
}

float DecibelsToLinear(float aDecibels, float aNegInfDecibels)
{
  float lin = math::expf(.11512925464970228420089957273422f *
                         aDecibels);  //ln(10)/20 = .11512925464970228420089957273422;
  //float lin = math::pow(10.0f, 0.05f * aDecibels);
  if (lin <= aNegInfDecibels)
    return 0.0f;
  return lin;
}

// don't use a LUT because we want to support pitch bend and glides and stuff. using a LUT + interpolation would be
// asinine.
float MIDINoteToFreq(float x)
{
  // float a = 440;
  // return (a / 32.0f) * fast::pow(2.0f, (((float)x - 9.0f) / 12.0f));
  //return 440 * math::pow2((x - 69) / 12);
  static constexpr float oneTwelfth = 1.0f / 12;
  return 440 * math::pow2_N16_16((x - 69) * oneTwelfth);
}

float SemisToFrequencyMul(float x)
{
  static constexpr float OneTwelfth = 1.0f / 12.0f;
  return math::pow2_N16_16(x * OneTwelfth);
  //return math::pow2(x / 12.0f);
}

float CalcTanhGainCompensation(float driveFactor)
{
  if (driveFactor <= 1)
    return 1;  // no gain compensation if drive is actually attenuating

  // this constant just feels about right. there's no "perfect" way to gain compensate a non-linearity like tanh because it depends on
  // the incoming signal to get it perfect. what we DONT want is to feel too much attenuation as drive increases. this seems to balance well.
  //return M7::math::expf(-0.14f * driveFactor); // this has a good curve until past like 25db
  return 1.0f / M7::math::sqrt(driveFactor);
}

//float FrequencyToMIDINote(float hz)
//{
//	float ret = 12.0f * math::log2(max(8.0f, hz) / 440) + 69;
//	return ret;
//}

// center = 0.5 for both.
FloatPair PanToLRVolumeParams(float panN11)
{
  // -1..+1  -> 1..0
  panN11 = clampN11(panN11);
  float rightVol = (panN11 + 1) * 0.5f;
  return {1 - rightVol, rightVol};
}

FloatPair PanToFactor(float panN11)
{
  // SQRT pan law
  auto volumes = PanToLRVolumeParams(panN11);
  float leftChannel = math::sqrt01(volumes.x[0]);
  float rightChannel = math::sqrt01(volumes.x[1]);
  return {leftChannel, rightChannel};
}

}  // namespace math

void BipolarDistribute(size_t count, const bool* enabled, float* outp)
{
  size_t enabledCount = 0;
  // get enabled count first
  for (size_t i = 0; i < count; ++i)
  {
    if (enabled[i])
      enabledCount++;
  }
  size_t pairCount = enabledCount / 2;  // chops off the odd one.
  size_t iPair = 0;
  size_t iEnabled = 0;
  for (size_t iElement = 0; iElement < count; ++iElement)
  {
    outp[iElement] = 0;  // produce valid values even for disabled elements; they get used sometimes for optimization
    if (!enabled[iElement])
    {
      continue;
    }
    if (iPair >= pairCount)
    {
      // past the expected # of pairs; this means it's centered. there should only ever be 1 of these elements (the odd one)
      ++iEnabled;
      continue;
    }
    float val = float(iPair + 1) /
                pairCount;  // +1 so 1. we don't use 0; that's reserved for the odd element, and 2. so we hit +1.0f
    if (iEnabled & 1)
    {  // is this the 1st or 2nd in the pair
      val = -val;
      ++iPair;
    }
    ++iEnabled;
    outp[iElement] = val;
  }
}

void ImportDefaultsArray(size_t count, const int16_t* src, float* paramCacheOffset)
{
  for (size_t i = 0; i < count; ++i)
  {
    paramCacheOffset[i] = math::Sample16To32Bit(src[i]);
  }
}

FloatPair FloatPair::Mix(const FloatPair& a, const FloatPair& b, float aLin, float bLin)
{
  return {a.x[0] * aLin + b.x[0] * bLin, a.x[1] * aLin + b.x[1] * bLin};
}

float CalcDelayMS(float eighths, float msOffset, float frequencyHz)
{
  // 60000/bpm = milliseconds per beat. but we are going to be in 8 divisions per beat.
  // 60000/8 = 7500
  const float tempo = float(Helpers::CurrentTempo);
  const float tempoMs = (tempo > 0.0f) ? (7500.0f / tempo) * eighths : 0.0f;

  const float freqMs = (frequencyHz > 0) ? 1000.0f / frequencyHz : 0;
  const float totalMs = tempoMs + msOffset + freqMs;
  return std::max(0.0f, totalMs);
}

}  // namespace M7


}  // namespace WaveSabreCore
