#pragma once

#include <WaveSabreCore/Maj7Basic.hpp>
#include <WaveSabreCore/Maj7ModMatrix.hpp>

namespace WaveSabreCore
{
    namespace M7
    {
        struct EnvelopeNode
        {
            size_t mnSampleCount = 0;

            const int mParamBaseID;
            EnvTimeParam mDelayTime;//;{ 0 };
            EnvTimeParam mAttackTime;//;{ 0 };
            CurveParam mAttackCurve;//;{ 0 };
            EnvTimeParam mHoldTime;//;{ 0 };
            EnvTimeParam mDecayTime;//;{ 0.5f };
            CurveParam mDecayCurve;//;{ 0 };
            Float01Param mSustainLevel;//;{ 0.4f };
            EnvTimeParam mReleaseTime;//;{ 0.2f };
            CurveParam mReleaseCurve;//;{ 0 };
            BoolParam mLegatoRestart;// { false };
            
            ModSource mMyModSource; // not used by this object, but useful for mappings.

            explicit EnvelopeNode(ModMatrixNode& modMatrix, ModDestination modDestIDDelayTime, real_t* paramCache, int paramBaseID, ModSource myModSource);

            enum class EnvelopeStage : uint8_t
            {
                Idle,    // output = 0
                Delay,   // output = 0
                Attack,  // output = curve from 0 to 1
                Hold,    // output = 1
                Decay,   // output = curve from 1 to 0
                Sustain, // output = SustainLevel
                Release, // output = curve from Y to 0, based on when release occurred
            };

            // advances to a specified stage. The point of this is to advance through 0-length stages so we don't end up with
            // >=1sample per stage.
            void AdvanceToStage(EnvelopeStage stage);

            void noteOn(bool isLegato);

            void noteOff();

            void kill();

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

            float ProcessSample();
        private:
            void RecalcState();

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
            const int mModDestBase;// delay time

        };

    } // namespace M7


} // namespace WaveSabreCore








