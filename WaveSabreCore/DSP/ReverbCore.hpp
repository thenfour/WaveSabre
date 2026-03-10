#pragma once

#include "../Filters/AllPass.h"
#include "../Filters/Comb.h"
#include "DelayBuffer.h"
//#include "Maj7Basic.hpp"
//#include "Maj7Filter.hpp"
#include "../Filters/SVFilter.hpp"

namespace WaveSabreCore
{
struct ReverbCore
{
  float gain = 0;
  float roomSize = 0;
  float damp = 0;
  float width = 0;
  float lowCutFreq = 0;
  float highCutFreq = 0;
  float preDelayMS = 0;

  ReverbCore()
  {
    static constexpr int16_t CombTuning[numCombs] = {1116, 1188, 1277, 1356, 1422, 1491, 1557, 1617};
    static constexpr int16_t AllPassTuning[numAllPasses] = {556, 441, 341, 225};
    static constexpr int16_t stereoSpread = 23;

    for (int i = 0; i < numCombs; i++)
    {
      combLeft[i].SetLengthSamples(CombTuning[i]);
      combRight[i].SetLengthSamples(CombTuning[i] + stereoSpread);
    }

    for (int i = 0; i < numAllPasses; i++)
    {
      allPassLeft[i].SetLengthSamples(AllPassTuning[i]);
      allPassRight[i].SetLengthSamples(AllPassTuning[i] + stereoSpread);

      // NOTE: this is never set again, so effectively it means a fixed feedback amount of 0.5.
      allPassLeft[i].SetCombParams(0, /*roomSize*/ 0.5f);
      allPassRight[i].SetCombParams(0, /*roomSize*/ 0.5f);
    }
  }

  M7::FloatPair ProcessSample(const M7::FloatPair& in)
  {
    static constexpr float SVQ = 1;

    // this is done in processSample (not in UpdateParams) to avoid GUI / Audio thread contention.
    preDelayBuffer.SetLengthMilliseconds(preDelayMS);

    float leftInput = in.Left();
    float rightInput = in.Right();

    // mono feed into the reverb network
    float input = (leftInput + rightInput) * 0.015f;

    // Pre-EQ: apply LP/HP to the signal feeding the reverb network (more typical usage)
    input = highCutFilter[0].SVFlow(input, highCutFreq, SVQ);  // low-pass
    input = lowCutFilter[0].SVFhigh(input, lowCutFreq, SVQ);   // high-pass

    // predelay is part of the reverb network; feed it the pre-filtered signal
    if (preDelayMS > 0)
    {
      preDelayBuffer.WriteAndAdvance(input);
      input = preDelayBuffer.PeekAtCursor();
    }

    float outL = 0;
    float outR = 0;

    // Accumulate comb filters in parallel
    for (int i = 0; i < numCombs; i++)
    {
      outL += combLeft[i].ProcessComb(input);
      outR += combRight[i].ProcessComb(input);
    }

    // Feed through allpasses in series
    for (int i = 0; i < numAllPasses; i++)
    {
      outL = allPassLeft[i].ProcessAllPass(outL);
      outR = allPassRight[i].ProcessAllPass(outR);
    }

    // Width cross-mix for the wet signal (0=mono center, 1=full width)
    float wetL = outL * wet1 + outR * wet2;
    float wetR = outR * wet1 + outL * wet2;

    return {wetL, wetR};
  }

  void Update()
  {
    // Width mapping: 0 -> mono center (wet1=0.5, wet2=0.5), 1 -> full width (wet1=1.0, wet2=0.0)
    wet1 = (width * 0.5f) + 0.5f;
    wet2 = (1.0f - width) * 0.5f;

    //// this does not exist in the original. it feels like it should belong but causes a lot of ringing and ugliness.
    //for (int i = 0; i < numAllPasses; i++)
    //{
    //	allPassLeft[i].SetFeedback(roomSize);
    //	allPassRight[i].SetFeedback(roomSize);
    //}

    for (int i = 0; i < numCombs; i++)
    {
      combLeft[i].SetCombParams(damp, roomSize);
      combRight[i].SetCombParams(damp, roomSize);
    }
  }

private:
  static constexpr int numCombs = 8;
  static constexpr int numAllPasses = 4;

  float wet1 = 0;
  float wet2 = 0;  // stereo width cross-mix factors

  M7::SVFilter lowCutFilter[2], highCutFilter[2];

  AudioBuffer combLeft[numCombs];
  AudioBuffer combRight[numCombs];

  AudioBuffer allPassLeft[numAllPasses];
  AudioBuffer allPassRight[numAllPasses];

  AudioBuffer preDelayBuffer;
};

}  // namespace WaveSabreCore
