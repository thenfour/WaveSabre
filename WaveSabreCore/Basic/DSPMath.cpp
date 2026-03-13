
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





  float FreqParamConfig::GetFrequency(float freqParam01, float ktParam01, float noteHz) const
  {
    // at 0.5, we use 1khz.
    // for each 0.1 param value, it's +/- one octave

    //float centerFreq = 1000; // the cutoff frequency at 0.5 param value.

    // with no KT,
    // so if param is 0.8, we want to multiply by 8 (2^3)
    // if param is 0.3, multiply by 1/4 (2^(1/4))

    // with full KT,
    // at 0.3, we use playFrequency.
    // for each 0.1 param value, it's +/- one octave.
    // to copy massive, 1:1 is at paramvalue 0.3. 0.5 is 2 octaves above playing freq.
    const float ktFreq = noteHz * 4;
    //float ktParamVal = (ktOffset < 0) ? 0 : Get01Value__(ktOffset, 0);
    const float centerFreq = math::lerp(mCenterFrequency, ktFreq, ktParam01);

    freqParam01 = math::clamp01(freqParam01);
    freqParam01 -= 0.5f;    // signed distance from 0.5 -.2 (0.3 = -.2, 0.8 = .3)   [-.5,+.5]
    freqParam01 *= mScale;  // 10.0f; // (.3 = -2, .8 = 3) [-15,+15]
    //float fact = math::pow(2, param);
    float fact = math::pow2_N16_16(freqParam01);
    return centerFreq * fact;
    //return math::clamp(centerFreq * fact, 0.0f, 22050.0f);
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

  float FreqParamConfig::GetParam01ValueForFrequencyAssumingNoKeytracking(float hz) const
  {
    // 2 ^ param
    float p = math::log2(hz / mCenterFrequency);
    p /= mScale;
    p += 0.5f;
    return p;
  }









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
    paramCacheOffset[i] = math::Default16ToFloatN11(src[i]);
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
