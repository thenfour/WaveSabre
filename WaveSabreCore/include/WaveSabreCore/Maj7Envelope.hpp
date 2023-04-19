#pragma once

#include <WaveSabreCore/Maj7Basic.hpp>
#include <WaveSabreCore/Maj7ModMatrix.hpp>

namespace WaveSabreCore
{
    namespace M7
    {
        struct EnvelopeNode
        {
            ParamAccessor mParams;

            EnvelopeMode mMode; 
            
            ModSource mMyModSource; // not used by this object, but useful for mappings.
            const int mModDestBase;// ModDestination of delay time

            explicit EnvelopeNode(ModMatrixNode& modMatrix, ModDestination modDestIDDelayTime, real_t* paramCache, int paramBaseID, ModSource myModSource);

            enum class EnvelopeStage : uint8_t
            {
                Idle,    // output = 0
                FixedDelay, // output = 0; a 1-sample delay which allows the mod matrix to catch up and properly calculate the subsequent stages.
                Delay,   // output = 0
                Attack,  // output = curve from 0 to 1
                Hold,    // output = 1
                Decay,   // output = curve from 1 to 0
                Sustain, // output = SustainLevel
                Release, // output = curve from Y to 0, based on when release occurred
                ReleaseSilence, // output = 0. one sample of silence after the release phase is required to bring modulation destinations to 0 before "turning off" the envelope.
            };

            // advances to a specified stage. The point of this is to advance through 0-length stages so we don't end up with
            // >=1sample per stage.
            void AdvanceToStage(EnvelopeStage stage);

            void noteOn(bool isLegato);

            void noteOff();

            float kill();

            // used by debug displays
            EnvelopeStage GetStage() const
            {
                return this->mStage;
            }

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
            real_t mStagePos01 = 0;      // where in the current stage are we?
            real_t mLastOutputLevel = 0; // used to calculated release from value.
            float mOutputDeltaPerSample = 0;
            real_t mStagePosIncPerSample =
                0; // how much mStagePos01 changes per recalc samples block (recalculated when mStage changes, or when spec changes)

            float mReleaseStagePos01 = 0; // where in the release stage are we? delay acts like 2 stages at the same time so this is needed.
            float mReleaseStagePosIncPerSample = 0;

            // when release stage begins, what value is it releasing from?
            real_t mReleaseFromValue01 = 0;

            // in monophonic situations, attack stage may not start from 0.
            real_t mAttackFromValue01 = 0; 

            ModMatrixNode& mModMatrix;
        };

    } // namespace M7


} // namespace WaveSabreCore








