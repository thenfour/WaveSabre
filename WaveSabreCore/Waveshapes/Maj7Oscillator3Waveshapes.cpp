#pragma once

#include "./Maj7Oscillator3Waveshapes.hpp"

namespace WaveSabreCore
{
namespace M7
{


  size_t EvolvingGrainNoiseCore::ComputeGrainSizeSamples() const
  {
    // Map 0..1 → [minSize, maxSize] exponentially
    const float minSize = float(kMinGrainSizeSamples);
    const float maxSize = float(kMaxGrainSizeSamples);
    const float sizeF = minSize * math::pow(maxSize / minSize, mGrainSize01);
    const int sizeI = math::round<int>(sizeF);
    return (size_t)math::ClampI(sizeI, (int)kMinGrainSizeSamples, (int)kMaxGrainSizeSamples);
  }

  void EvolvingGrainNoiseCore::EnsureGrainAllocated()
  {
    const size_t targetSize = ComputeGrainSizeSamples();

    // If size unchanged and we already have valid content, nothing to do
    if (targetSize == mGrainSizeSamples && mGrainValid)
      return;

    mGrainSizeSamples = targetSize;

    // Fill the active portion of the fixed buffer with noise
    for (size_t i = 0; i < mGrainSizeSamples; ++i)
    {
      mGrain[i] = math::randN11();  // [-1,1]
    }

    mGrainValid = true;
  }

  void EvolvingGrainNoiseCore::MutateGrainCycle()
  {
    if (!mGrainValid || mGrainSizeSamples == 0)
      return;

    const float targetCountF = mMutationRate01 * float(mGrainSizeSamples);
    const int targetCountI = math::round<int>(targetCountF);

    const size_t mutateCount = (size_t)math::ClampI(targetCountI, 0, (int)mGrainSizeSamples);

    for (size_t i = 0; i < mutateCount; ++i)
    {
      // Random index in [0, mGrainSizeSamples)
      const size_t index = (size_t)(math::rand01() * double(mGrainSizeSamples)) % mGrainSizeSamples;

      mGrain[index] = math::randN11();
    }
  }

  void EvolvingGrainNoiseCore::HandleParamsChanged()
  {
    mGrainSize01 = mWaveshapeA;
    mMutationRate01 = mWaveshapeB;

    EnsureGrainAllocated();
  }

  CoreSample EvolvingGrainNoiseCore::renderSampleAndAdvance(float /*audioRatePhaseOffset*/)
  {
    const auto step = mPhaseAcc.advanceOneSample();

    if (!mInitialized)
    {
      mInitialized = true;
      mGrainValid = false;  // force re-init
      EnsureGrainAllocated();
      MutateGrainCycle();
    }

    // assert grain valid & has samples.

    const size_t index = (size_t)(step.phaseBegin01 * mGrainSizeSamples) % mGrainSizeSamples;

    const float y = mGrain[index];

    const bool completedCycle = step.hasReset || (step.phaseBegin01 + step.dt >= 1.0f);
    if (completedCycle)
    {
      MutateGrainCycle();
    }

    return CoreSample{
        .amplitude = y,
        //.naive = y,
        //.correction = 0.0f,
        //.phaseAdvance = step,
    };
  }

  void EvolvingGrainNoiseCore::RestartDueToNoteOn()
  {
    OscillatorCore::RestartDueToNoteOn();
    mInitialized = false;
    // for fresh random grain per note
    // mGrainValid = false;
  }

}  // namespace M7

}  // namespace WaveSabreCore
