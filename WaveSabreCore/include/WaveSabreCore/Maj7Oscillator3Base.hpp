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
enum class PhaseEventKind
{
  Wrap,
  Reset,
  Unknown,
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

  // there are a limited number of scenarios that can happen.
  // 1. nothing
  // 2. wrap
  // 3. reset
  // 4. reset + wrap
  // 5. wrap + reset
  // 6. wrap + reset + wrap; this can only happen if your arate phase offset is near the end of the cycle.
  InSamplePhaseEvent events[3];
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

  // only for debugging:
  double mFrequencyHz = 0;
  std::string mDebugName;

public:
  PhaseAccumulator(const std::string& debugName)
      : mDebugName(debugName)
  {
  }

  void setPhase01(double phase01)
  {
    mPhase01 = math::wrap01(phase01);
  }

  // used only by vst editor.
  double getPhase01() const
  {
    return mPhase01;
  }

  double getPhaseDeltaPerSample01() const
  {
    return mPhaseDeltaPerSample01;
  }

  void SynchronizeWith(const PhaseAccumulator& src)
  {
    mPhase01 = src.mPhase01;
    mPhaseDeltaPerSample01 = src.mPhaseDeltaPerSample01;
  }

  void setFrequencyHz(float hz, float offset = 0)
  {
    mPhaseDeltaPerSample01 = std::max(hz * Helpers::CurrentSampleRateRecipF + offset, 0.0f);
    mFrequencyHz = hz;
  }

  PhaseAdvance advanceArbitrary(double phaseDelta01, double audioRatePhaseOffset)
  {
    PhaseAdvance out{};
    const double phaseBeginEval = math::wrap01(mPhase01 + audioRatePhaseOffset);
    const double endPhaseEval = phaseBeginEval + phaseDelta01;

    out.phaseBegin01 = phaseBeginEval;
    out.lengthInPhase01 = phaseDelta01;

    if (endPhaseEval >= 1.0)
    {
      double t = (1.0 - phaseBeginEval) / mPhaseDeltaPerSample01;  // fraction of full sample
      // this can occur at the very end of the sample, meaning t can reach 1.0.
      // TODO: decide how to handle. for now let the next sample handle it @ u=0.
      if (t < 1)
      {
        out.events[out.eventCount++] = InSamplePhaseEvent{t, PhaseEventKind::Wrap};
      }
    }

    out.phaseEnd01 = math::wrap01(endPhaseEval);

    // advance internal state WITHOUT offset
    mPhase01 = math::wrap01(mPhase01 + phaseDelta01);
    return out;
  }

  // Advance one sample, wrapping phase, emitting wrap events (no reset events are relevant at this level).
  // audioRatePhaseOffset = phase offset to apply.
  PhaseAdvance advanceOneSample(double audioRatePhaseOffset)
  {
    return advanceArbitrary(mPhaseDeltaPerSample01, audioRatePhaseOffset);
  }
};

// connects 2 phase accumulators to track master / slave for hard sync.
struct HardSyncPhaseAccumulator
{
  // better name alternatives:
  // source / target
  // main / sync
  // pitch / shape
  PhaseAccumulator mMaster{"MasterPhase"};
  PhaseAccumulator mSlave{"SlavePhase"};
  bool mHardSyncEnabled = false;

  void setParams(float mainHz, bool hardSyncEnable, float syncHz)
  {
    // if hardsync is enabled, slave freq is sync freq, else slave freq = main freq.
    mHardSyncEnabled = hardSyncEnable;
    mMaster.setFrequencyHz(mainHz);
    mSlave.setFrequencyHz(hardSyncEnable ? syncHz : mainHz);
  }

  double GetAudiblePhase01() const
  {
    return mSlave.getPhase01();
  }

  void setPhase01(double phase01)
  {
    mMaster.setPhase01(phase01);
    mSlave.setPhase01(phase01);
  }

  void SynchronizeWith(const HardSyncPhaseAccumulator& src)
  {
    mMaster.SynchronizeWith(src.mMaster);
    mSlave.SynchronizeWith(src.mSlave);
  }

  // Advances one sample, collecting phase events along the way.
  PhaseAdvance advanceOneSample(double audoRatePhaseOffset)
  {
    if (!mHardSyncEnabled)
    {
      PhaseAdvance s = mSlave.advanceOneSample(audoRatePhaseOffset);
      return s;
    }

    // possible scenarios:
    // 1. nothing
    // 2. wrap
    // 3. reset
    // 4. reset + wrap
    // 5. wrap + reset
    // 6. wrap + reset + wrap; this can only happen if your arate phase offset is near the end of the cycle.

    PhaseAdvance m = mMaster.advanceOneSample(0);
    if (m.eventCount == 0)
    {
      // No reset: return the slave step (possibly with its own Wrap)...
      // scenarios #1 and #2.
      PhaseAdvance s = mSlave.advanceOneSample(audoRatePhaseOffset);
      return s;
    }

    // hard sync event occurred.

    // --------------- advance slave up to the reset point
    const double preResetInSample01 = m.events[0].whenInSample01;  // [0,1)
    const double postResetInSample01 = 1.0 - preResetInSample01;
    const double preResetPhaseDelta = preResetInSample01 * mSlave.getPhaseDeltaPerSample01();
    const double postResetPhaseDelta = postResetInSample01 * mSlave.getPhaseDeltaPerSample01();
    const PhaseAdvance preResetAdv = mSlave.advanceArbitrary(preResetPhaseDelta, audoRatePhaseOffset);
    PhaseAdvance out = preResetAdv;  // start from slave's kinematics

    // for #5 and #6, the first wrap event is already there.
    // add the reset event
    out.events[out.eventCount++] = InSamplePhaseEvent{m.events[0].whenInSample01, PhaseEventKind::Reset};

    // --------------- evaluate the remaining post-reset window
    mSlave.setPhase01(0);
    const auto postResetAdv = mSlave.advanceArbitrary(postResetPhaseDelta, audoRatePhaseOffset);
    if (postResetAdv.eventCount > 0)
    {
      // scenario #4: reset + wrap
      // scenario #6: wrap + reset + wrap
      // convert the wrap event position from post-reset-window to full sample window
      const double wrapInSample01 = preResetInSample01 + postResetAdv.events[0].whenInSample01;
      if (wrapInSample01 < 1.0)  // expect always true
      {
        out.events[out.eventCount++] = InSamplePhaseEvent{.whenInSample01 = wrapInSample01,
                                                          .kind = PhaseEventKind::Wrap};
      }
    }

    out.lengthInPhase01 = mSlave.getPhaseDeltaPerSample01();  // but full sample length
    //out.phaseEnd01 = math::wrap01(out.phaseBegin01 + out.lengthInPhase01);
    out.phaseEnd01 = math::wrap01(math::wrap01(audoRatePhaseOffset) + postResetPhaseDelta);

    return out;
  }
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct EdgeEvent
{
  PhaseEventKind reason = PhaseEventKind::Unknown;
  float atPhase01 = 0.0f;       // phase location of the edge [0,1)
  float whenInSample01 = 0.0f;  // when in the current sample [0,1)

  float preEdgeAmp = 0.0f;   // amplitude just before the edge
  float postEdgeAmp = 0.0f;  // amplitude just after the edge
  float dAmp = 0.0f;         // post - pre

  float preEdgeSlope = 0.0f;   // slope just before the edge (per phase)
  float postEdgeSlope = 0.0f;  // slope just after the edge (per phase)
  float dSlope = 0.0f;         // post - pre
};
////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct CoreSample
{
  float amplitude = 0.0f;  // the final sample value (with bandlimiting applied if applicable)

  float naive = 0.0f;                 // the naive sample value (without bandlimiting)
  float correction = 0.0f;            // the bandlimiting correction to add to the naive value
  PhaseAdvance phaseAdvance;          // phase kinematics for this sample
  std::vector<EdgeEvent> edgeEvents;  // discontinuity events that happened during this sample
  std::string log;
};

////////////////////////////////////////////////////////////////////////////////////////////////////////////
struct OscillatorCore
{
public:
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
