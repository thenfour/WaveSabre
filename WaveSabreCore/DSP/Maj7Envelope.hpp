#pragma once

#include "../GigaSynth/Maj7ModMatrix.hpp"
#include "../GigaSynth/GigaParams.hpp"
#include "../Params/Maj7ParamAccessor.hpp"
#include "../Devices/VoiceAllocator.hpp"

namespace WaveSabreCore
{
namespace M7
{
struct EnvelopeNode
{
  ModMatrixNode& mModMatrix;
  ParamAccessor mParams;
  EnvelopeMode mMode;
  ModSource mMyModSource;  // not used by this object, but useful for mappings.
  const int mModDestBase;  // ModDestination of delay time

  explicit EnvelopeNode(
      ModMatrixNode& modMatrix,
      real_t* paramCache,
      const EnvelopeInfo& envInfo);  // ModDestination modDestIDDelayTime, int paramBaseID, ModSource myModSource);

  enum class EnvelopeStage : uint8_t
  {
    Idle,  // output = 0
    FixedDelay,  // output = 0(*); a 3-sample delay which allows the mod matrix to catch up and properly calculate the subsequent stages.
    Delay,           // output = 0(*)
    Attack,          // output = curve from 0(*) to 1
    Hold,            // output = 1
    Decay,           // output = curve from 1 to 0
    Sustain,         // output = SustainLevel
    Release,         // output = curve from Y to 0, based on when release occurred
    ReleaseSilence,  // output = 0. one sample of silence after the release phase is required to bring modulation destinations to 0 before "turning off" the envelope.

    // (*) -- level may depend on circumstances. legato / voice stealing may cause start from non-zero.
  };

  // advances to a specified stage. The point of this is to advance through 0-length stages so we don't end up with
  // >=1sample per stage.
  void AdvanceToStage(EnvelopeStage stage);

  void EnvelopeNoteOn(VoiceNoteOnFlags flags);

  void EnvelopeNoteOff();

  void KillEnvelope();

  bool IsPlaying() const
  {
    return (this->mStage != EnvelopeStage::Idle);
  }

  // helps calculate releaseability
  float GetLastOutputLevel() const
  {
    return mLastOutputLevel;
  }

  void BeginBlock();

  void ProcessSampleFull();
  float ProcessSample();

private:
  void RecalcState();

  int mFixedDelaySamplesRemaining = 0;
  size_t mnSampleCount = 0;
  EnvelopeStage mStage = EnvelopeStage::Idle;
  real_t mStagePos01 = 0;       // where in the current stage are we?
  real_t mLastOutputLevel = 0;  // most recent output; used when latching the next stage start level.
  float mOutputDeltaPerSample = 0;
  real_t mStagePosIncPerSample =
      0;  // how much mStagePos01 changes per recalc samples block (recalculated when mStage changes, or when spec changes)

  float mReleaseStagePos01 =
      0;  // where in the release stage are we? delay acts like 2 stages at the same time so this is needed.
  float mReleaseStagePosIncPerSample = 0;

  // Latched output level captured when entering a stage that needs a fixed start value
  // (attack, release, or the delay pre-ramp used before attack).
  real_t mStageStartLevel01 = 0;
};

}  // namespace M7


}  // namespace WaveSabreCore
