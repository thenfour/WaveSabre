#pragma once

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <format>
#include <memory>
#include <utility>
#include <vector>

#include "Maj7Basic.hpp"
#include <WaveSabreCore/Maj7Oscillator.hpp>

namespace WaveSabreCore
{
namespace M7
{
//#define ENABLE_OSC_LOG

#ifdef ENABLE_OSC_LOG
////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct OscLog
{
  std::string mBuffer;
  size_t mIndent = 0;
  bool mEnabled = true;

  void IncreaseIndent()
  {
    mIndent++;
  }
  void DecreaseIndent()
  {
    if (mIndent > 0)
      mIndent--;
  }
  void Log(const std::string& msg)
  {
    std::string indent(mIndent * 2, ' ');
    mBuffer += indent + msg + "\n";
  }
  void Clear()
  {
    mBuffer.clear();
    mIndent = 0;
  }

  void SetEnabled(bool enabled)
  {
    mEnabled = enabled;
  }

  struct EnabledScope
  {
    OscLog& mLog;
    bool mPrev;
    EnabledScope(OscLog& log, bool enabled)
        : mLog(log)
        , mPrev(log.mEnabled)
    {
      mLog.SetEnabled(enabled);
    }
    ~EnabledScope()
    {
      mLog.SetEnabled(mPrev);
    }
  };

  EnabledScope EnabledBlock(bool en)
  {
    return EnabledScope(*this, en);
  }

  struct IndentScope
  {
    OscLog& mLog;
    IndentScope(OscLog& log, const std::string& msg)
        : mLog(log)
    {
      mLog.Log("{ " + msg);
      mLog.IncreaseIndent();
    }
    ~IndentScope()
    {
      mLog.DecreaseIndent();
      mLog.Log("}");
    }
  };

  IndentScope IndentBlock(const std::string& msg)
  {
    return IndentScope(*this, msg);
  }
};

extern M7::OscLog gOscLog;

#endif  // ENABLE_OSC_LOG

////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct PhaseStep
{
  double phaseBegin01;  // slave phase at sample start (no offset), ∈ [0,1)
  double dt;            // per-sample phase advance in phase units, ∈ [0,1)
  bool hasReset;        // master wrapped inside this sample?
  double resetAlpha01;  // when in this sample the reset occurs, ∈ [0,1) (valid if hasReset)
  //double phaseEnd01;    // slave phase at sample end (no offset), ∈ [0,1)
};

// Simple accumulator: no offset, no wrap events
struct PhaseAccumulator
{
  double mPhase01 = 0.0;
  double mDelta = 0.0;

  void setPhase01(double p)
  {
    mPhase01 = math::wrap01(p);
  }
  void setFrequencyHz(double hz)
  {
    mDelta = std::max(hz * Helpers::CurrentSampleRateRecipF, 0.0);
  }
  double getPhase01() const
  {
    return mPhase01;
  }
  double getDelta() const
  {
    return mDelta;
  }

  void SynchronizeWith(const PhaseAccumulator& src)
  {
    mPhase01 = src.mPhase01;
    mDelta = src.mDelta;
  }

  // advance without resets (used for slave when no hard sync)
  PhaseStep advanceOneSampleNoReset()
  {
    const double begin = mPhase01;
    const double end = math::wrap01(begin + mDelta);
    mPhase01 = end;
    return {begin, mDelta, false, 0.0};//, end};
  }

  // advance with an externally provided reset alpha (0..1); updates internal phase
  PhaseStep advanceOneSampleWithReset(double alpha01)
  {
    const double begin = mPhase01;
    // pre segment: alpha01 * dt (ignored for state end)
    // post segment after reset: (1 - alpha01) * dt
    const double end = math::wrap01((1.0 - alpha01) * mDelta);
    mPhase01 = end;
    return
    {
      begin, mDelta, true, alpha01
    }  ;//, end};
  }
};

// Hard-sync pair: master only detects reset; slave just uses alpha (no offset here)
struct HardSyncPhase
{
  PhaseAccumulator master;
  PhaseAccumulator slave;
  bool enabled = false;

  void setPhase01(double p)
  {
    master.setPhase01(p);
    slave.setPhase01(p);
  }

  void setParams(float mainHz, bool hardSyncEnable, float syncHz)
  {
    enabled = hardSyncEnable;
    master.setFrequencyHz(mainHz);
    slave.setFrequencyHz(hardSyncEnable ? syncHz : mainHz);
  }

  void SynchronizeWith(const HardSyncPhase& src)
  {
    master.SynchronizeWith(src.master);
    slave.SynchronizeWith(src.slave);
  }

  // returns slave step; includes hasReset/resetAlpha01 if master wrapped
  PhaseStep advanceOneSample()
  {
    // detect master wrap (at most one, under dt<1)
    const double mBegin = master.getPhase01();
    const double mDt = master.getDelta();
    const double mEnd = mBegin + mDt;

    bool hasReset = false;
    double alpha = 0.0;
    if (enabled && mEnd >= 1.0)  // master wraps this sample
    {
      // when-in-sample = time to reach phase 1.0 divided by dt
      alpha = (1.0 - mBegin) / mDt;  // ∈ (0,1]
      if (alpha >= 1.0)
        alpha = 0.0;  // push to next sample per your policy
      hasReset = (alpha > 0.0);
    }

    // advance master state (always)
    master.setPhase01(math::wrap01(mEnd));

    // advance slave accordingly (no offset)
    if (!hasReset)
      return slave.advanceOneSampleNoReset();
    return slave.advanceOneSampleWithReset(alpha);
  }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct CoreSample
{
  float amplitude = 0.0f;  // the final sample value (with bandlimiting applied if applicable)

  float naive = 0.0f;                 // the naive sample value (without bandlimiting)
  float correction = 0.0f;            // the bandlimiting correction to add to the naive value
  PhaseStep phaseAdvance;          // phase kinematics for this sample
  std::string log;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct OscillatorCore
{
public:
  HardSyncPhase mPhaseAcc;
  float mWaveshapeA = 0.0f;
  float mWaveshapeB = 0.0f;
  OscillatorWaveform mWaveformType;

protected:
  OscillatorCore(OscillatorWaveform waveformType)
      : mWaveformType(waveformType)
  {
  }

public:
  virtual void SetKRateParams(float shapeA, float shapeB, float mainFreqHz, bool enableHardSync, float syncFreqHz)
  {
    mWaveshapeA = shapeA;
    mWaveshapeB = shapeB;
    mPhaseAcc.setParams(mainFreqHz, enableHardSync, syncFreqHz);
    HandleParamsChanged();
  };

  virtual void SetCorrectionFactor(float factor) {}

  // used by LFOs to just hard-set the phase. LFO phase, when "note restart" is disabled, is global, so
  // all individual voice LFOs should be in sync and act as if they're the same.
  // Everything after the 1st call will effectively be a NOP, so no special bandlimiting or processing necessary.
  virtual void ForcefullySynchronizePhase(const OscillatorCore& src)
  {
    mPhaseAcc.SynchronizeWith(src.mPhaseAcc);
  };

  virtual void RestartDueToNoteOn()
  {
    // set phase to 0 (because the oscillator's "phase offset" is performed via audioRatePhaseOffset in renderSampleAndAdvance.
    mPhaseAcc.setPhase01(0);
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
};

}  // namespace M7

}  // namespace WaveSabreCore
