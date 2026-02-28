
#pragma once

#include "FilterOnePole.hpp"
#include "WaveSabreCore/BiquadFilter.h"

namespace WaveSabreCore
{
namespace M7
{
struct ButterworthFilter : IFilter
{
  static constexpr int kMaxStages = 8;

  BiquadFilter mStages[kMaxStages];
  int mNStages = 0;
  FilterResponse mResponse = FilterResponse::Lowpass;
  float mCutoffHz = 1000.0f;

  static inline int SlopeToStages(FilterSlope slope)
  {
    int n = (int)slope - 1;
    return math::ClampI(n, 0, kMaxStages);
  }

  static inline float ButterworthQForSection(int sectionIndex, int nStages)
  {
    const int N = nStages * 2;  // filter order
    const float k = (float)(sectionIndex + 1);
    const float angle = ((2.0f * k) - 1.0f) * math::gPI / (2.0f * (float)N);
    const float c = math::cos(angle);
    const float denom = (2.0f * c > 1e-6f) ? (2.0f * c) : 1e-6f;
    return 1.0f / denom;
  }

  void RecalcStages()
  {
    if (mNStages <= 0)
      return;

    for (int i = 0; i < mNStages; ++i)
    {
      const float q = ButterworthQForSection(i, mNStages);
      mStages[i].SetBiquadParams(mResponse, mCutoffHz, q, 0.0f);
    }
  }

  virtual void SetParams(FilterCircuit circuit,
                         FilterSlope slope,
                         FilterResponse response,
                         real cutoffHz,
                         real reso01) override
  {
    if (!DoesSupport(circuit, slope, response))
    {
      mNStages = 0;
      return;
    }

    const int nextStages = SlopeToStages(slope);
    const float nextCutoff = math::clamp((float)cutoffHz, 20.0f, 20000.0f);

    if (nextStages != mNStages)
    {
      mNStages = nextStages;
      Reset();
    }

    if (response != mResponse || nextCutoff != mCutoffHz)
    {
      mResponse = response;
      mCutoffHz = nextCutoff;
      RecalcStages();
    }
  }

  virtual bool DoesSupport(FilterCircuit circuit, FilterSlope slope, FilterResponse response) override
  {
    if (circuit != FilterCircuit::Butterworth)
      return false;
    if (slope < FilterSlope::Slope12dbOct || slope > FilterSlope::Slope96dbOct)
      return false;
    return response == FilterResponse::Lowpass || response == FilterResponse::Highpass;
  }

  virtual real ProcessSample(real x) override
  {
    float y = (float)x;
    for (int i = 0; i < mNStages; ++i)
    {
      y = mStages[i].ProcessSample(y);
    }
    return y;
  }

  virtual real GetMagnitudeAtFrequency(real freqHz) const override
  {
    if (mNStages <= 0)
      return 1.0f;

    double mag = 1.0;
    for (int i = 0; i < mNStages; ++i)
    {
      mag *= (double)mStages[i].GetMagnitudeAtFrequency((float)freqHz);
    }
    return (real)((mag > 0.0) ? mag : 0.0);
  }

  virtual void Reset() override
  {
    for (int i = 0; i < mNStages; ++i)
    {
      mStages[i].Reset();
    }
  }
}; // class ButterworthFilter
}  // namespace M7
}  // namespace WaveSabreCore
