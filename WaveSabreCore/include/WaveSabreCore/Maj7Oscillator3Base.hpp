#pragma once

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

#include "Maj7Basic.hpp"

namespace WaveSabreCore
{
namespace M7
{
////////////////////////////////////////////////////////////////////////////////////////////////////////////
enum class PhaseEventKind
{
  Wrap,
  Reset
};

struct InSamplePhaseEvent
{
  // Fraction of the current sample when this event happens, [0,1).
  double whenInSample01 = 0.0f;
  PhaseEventKind kind = PhaseEventKind::Wrap;
};

struct PhaseAdvance
{
  // Phase at start/end of the sample, normalized [0,1).
  double phaseBegin01 = 0.0f;  // a sample is emitted AT this location in the wave cycle.
  // this is the "end" of the sample, i.e. where the next sample will be emitted (immediately after the end of this sample window).
  double phaseEnd01 = 0.0f;

  // sample window size, in phase units (this is phaseEnd01 - phaseBegin01, wrapped to [0,1) )
  double lengthInPhase01 = 0.0f;

  // In-sample events (at most: one wrap + one external reset).
  // more than 1 of either kind of event implies frequencies above Nyquist so it's not supported and expect artifacts.
  // Events are sorted by time.
  InSamplePhaseEvent events[2];
  int eventCount = 0;


  float ComputeFrequencyHz() const
  {
    return static_cast<float>(lengthInPhase01 * Helpers::CurrentSampleRateF);
  }
};


// handles advancing phase based on frequency.
// never outputs reset events.
struct PhaseAccumulator
{
private:
  double mPhase01 = 0.0f;                // current phase [0,1)
  double mPhaseDeltaPerSample01 = 0.0f;  // [0,1)

public:
  void setPhase01(double phase01)
  {
    mPhase01 = math::wrap01(phase01);
  }

  // used only by vst editor.
  double getPhase01() const
  {
    return mPhase01;
  }

  void SynchronizeWith(const PhaseAccumulator& src)
  {
    mPhase01 = src.mPhase01;
    mPhaseDeltaPerSample01 = src.mPhaseDeltaPerSample01;
  }

  void setFrequencyHz(float hz, float offset = 0)
  {
    mPhaseDeltaPerSample01 = std::max(hz * Helpers::CurrentSampleRateRecipF, 0.0f) + offset;
  }

  // Advance one sample, wrapping phase, emitting wrap events (no reset events are relevant at this level).
  PhaseAdvance advanceOneSample()
  {
    PhaseAdvance out{};
    out.phaseBegin01 = mPhase01;
    out.lengthInPhase01 = mPhaseDeltaPerSample01;

    double next = mPhase01 + mPhaseDeltaPerSample01;

    // At most one natural wrap per sample (your stated constraint)
    if (next >= 1.0)
    {
      // Where inside this sample the wrap happens
      double wrapAt01 = (1.0 - mPhase01) / std::max(mPhaseDeltaPerSample01, 1e-12);
      wrapAt01 = std::clamp(wrapAt01, 0.0, 1.0 - std::numeric_limits<double>::epsilon());

      out.events[out.eventCount++] = InSamplePhaseEvent{.whenInSample01 = wrapAt01, .kind = PhaseEventKind::Wrap};

      // Continue after wrap
      next -= 1.0;
    }

    out.phaseEnd01 = math::wrap01(next);
    mPhase01 = out.phaseEnd01;  // commit

    return out;
  }
};

// connects 2 phase accumulators to track master / slave for hard sync.
struct HardSyncPhaseAccumulator
{
  PhaseAccumulator mMaster;
  PhaseAccumulator mSlave;  // the audible oscillator phase

  void setParams(float mainHz, bool hardSyncEnable, float syncHz)
  {
    // if hardsync is enabled, slave freq is sync freq, else slave freq = main freq.
    mMaster.setFrequencyHz(hardSyncEnable ? syncHz : mainHz);
    mSlave.setFrequencyHz(mainHz);
  }

  void setPhase01(double phase01)
  {
    mMaster.setPhase01(phase01);
    mSlave.setPhase01(phase01);
  }

  // used only by vst editor.
  float GetAudiblePhase01() const
  {
    return (float)mSlave.getPhase01();
  }

  void SynchronizeWith(const HardSyncPhaseAccumulator& src)
  {
    mMaster.SynchronizeWith(src.mMaster);
    mSlave.SynchronizeWith(src.mSlave);
  }

  PhaseAdvance advanceOneSample()
  {
    // Advance both; then synthesize the *slave*'s PhaseAdvance with an optional in-sample reset
    PhaseAdvance m = mMaster.advanceOneSample();
    PhaseAdvance s = mSlave.advanceOneSample();

    // If master wrapped inside this sample, slave gets a Reset at that time
    bool masterWrapped = (m.eventCount > 0 && m.events[0].kind == PhaseEventKind::Wrap);

    if (!masterWrapped)
    {
      // No reset: just return the slave’s step (possibly with its own Wrap)
      return s;
    }

    // Compute slave's end-phase if it is reset at t = tR within the sample
    const double tReset = m.events[0].whenInSample01;  // [0,1)

    PhaseAdvance out = s;  // start from slave’s kinematics

    // Insert Reset event (and possibly also keep a natural slave wrap if it occurred before the reset)
    InSamplePhaseEvent tmpEvents[2];

    // keep events in chronological order; there are only 2 cases: wrap happened before reset, or not.
    // assumes max 1 event
    // assumes it's a wrap if it exists
    if (s.eventCount > 0 && s.events[0].whenInSample01 < tReset)
    {
      out.events[0] = s.events[0];
      out.events[1] = InSamplePhaseEvent{.whenInSample01 = tReset, .kind = PhaseEventKind::Reset};
      out.eventCount = 2;
    }
    else
    {
      // if wrapped after reset, ignore it (because the phase gets reset to 0 and the wrap is no longer valid)
      out.events[0] = InSamplePhaseEvent{.whenInSample01 = tReset, .kind = PhaseEventKind::Reset};
      out.eventCount = 1;
    }

    // Recompute the end phase after reset:
    // - phase grows from s.phaseBegin01 up to reset (tR), then jumps to 0,
    // - then advances the remaining (1 - tR) fraction at the same delta.
    const double delta = s.lengthInPhase01;
    const double afterResetAdvance = (1.0 - tReset) * delta;
    out.phaseEnd01 = math::wrap01(afterResetAdvance);  // starts at 0 after reset

    // Commit slave’s internal phase to the recomputed end
    mSlave.setPhase01(out.phaseEnd01);

    return out;
  }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct CoreSample
{
  float amplitude = 0.0f;  // the final sample value (with bandlimiting applied if applicable)

  float naive = 0.0f;         // the naive sample value (without bandlimiting)
  float correction = 0.0f;    // the bandlimiting correction to add to the naive value
  PhaseAdvance phaseAdvance;  // phase kinematics for this sample
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct OscillatorCore
{
  HardSyncPhaseAccumulator mPhaseAcc;
  float mWaveshapeA = 0.0f;
  float mWaveshapeB = 0.0f;
  OscillatorWaveform mWaveformType;

protected:
  OscillatorCore(OscillatorWaveform waveformType)
      : mWaveformType(waveformType)
  {
  }

public:
  void SetKRateParams(float shapeA, float shapeB, float mainFreqHz, bool enableHardSync, float syncFreqHz)
  {
    mWaveshapeA = shapeA;
    mWaveshapeB = shapeB;
    mPhaseAcc.setParams(mainFreqHz, enableHardSync, syncFreqHz);
  };

  // used by LFOs to just hard-set the phase. LFO phase, when "note restart" is disabled, is global, so
  // all individual voice LFOs should be in sync and act as if they're the same.
  // Everything after the 1st call will effectively be a NOP, so no special bandlimiting or processing necessary.
  void ForcefullySynchronizePhase(const OscillatorCore& src)
  {
    mPhaseAcc.SynchronizeWith(src.mPhaseAcc);
  };

  virtual void RestartDueToNoteOn() {
    // set phase to 0 (because the oscillator's "phase offset" is performed via audioRatePhaseOffset in renderSampleAndAdvance.
  };

  // allows cores to react to param changes
  virtual void HandleParamsChanged() {}

  // Render the current sample, and advance phase by 1 sample.
  // step : describes the phase kinematics for this sample.
  // The reason we output the "current" sample and not the sample at the end of the step,
  // is so we render the first sample of the waveform at the initial phase. Otherwise we would always skip the 1st sample of the waveform.
  // - audioRatePhaseOffset: optional phase offset applied at audio rate (e.g. for PM)
  //   I think audio-rate PM may add too much complexity to emit proper band-limiting so we can just assume:
  //   - proper band-limiting if no PM.
  //   - a-rate PM just doesn't get band-limited.
  //
  // Because phase offset is constant over the duration of a single sample,
  // in-sample event times remain the same relative to the window—only their phase locations shift.
  // so we can just shift phaseBegin01 and phaseEnd01 by the offset when you evaluate the shape, keeping the logic simple.
  virtual CoreSample renderSampleAndAdvance(float audioRatePhaseOffset) = 0;

  // if necessary, provide utility methods to keep cores clean, tight, and minimal.
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////
// example of simplest core: a sine wave; no band limiting supported
struct SineCore : public OscillatorCore
{
  SineCore() : OscillatorCore(OscillatorWaveform::SineClip)
  {
  }
  CoreSample renderSampleAndAdvance(float audioRatePhaseOffset) override
  {
    const auto step = mPhaseAcc.advanceOneSample();
    const double renderPhase = math::wrap01(step.phaseBegin01 + audioRatePhaseOffset);
    float y = math::sin(math::gPITimes2 * (float)renderPhase);
    return {
        .amplitude = y,
        .naive = y,
        .correction = 0.0f,
        .phaseAdvance = step,
    };
  }
};

}  // namespace M7

}  // namespace WaveSabreCore
