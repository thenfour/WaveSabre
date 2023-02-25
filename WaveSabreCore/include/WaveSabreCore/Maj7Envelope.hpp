#pragma once

#include <WaveSabreCore/Maj7Basic.hpp>
#include <WaveSabreCore/Maj7ModMatrix.hpp>

namespace WaveSabreCore
{
    namespace M7
    {
        struct EnvelopeNode
        {
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

            explicit EnvelopeNode(ModMatrixNode& modMatrix, ModDestination modDestIDDelayTime, real_t* paramCache, int paramBaseID) :
                mModMatrix(modMatrix),
                mModDestBase((int)modDestIDDelayTime),
                mDelayTime(paramCache[paramBaseID + gParamIDDelayTime], 0),
                mAttackTime(paramCache[paramBaseID + gParamIDAttackTime], 0),
                mAttackCurve(paramCache[paramBaseID + gParamIDAttackCurve], 0),
                mHoldTime(paramCache[paramBaseID + gParamIDHoldTime], 0),
                mDecayTime(paramCache[paramBaseID + gParamIDDecayTime], 0.5f),
                mDecayCurve(paramCache[paramBaseID + gParamIDDecayCurve], 0),
                mSustainLevel(paramCache[paramBaseID + gParamIDSustainLevel], 0.4f),
                mReleaseTime(paramCache[paramBaseID + gParamIDReleaseTime], 0.2f),
                mReleaseCurve(paramCache[paramBaseID + gParamIDReleaseCurve], 0),
                mLegatoRestart(paramCache[paramBaseID + gParamIDLegatoRestart], false)
            {
            }

            static constexpr int gParamIDDelayTime = 0;
            static constexpr int gParamIDAttackTime = 1;
            static constexpr int gParamIDAttackCurve = 2;
            static constexpr int gParamIDHoldTime = 3;
            static constexpr int gParamIDDecayTime = 4;
            static constexpr int gParamIDDecayCurve = 5;
            static constexpr int gParamIDSustainLevel = 6;
            static constexpr int gParamIDReleaseTime = 7;
            static constexpr int gParamIDReleaseCurve = 8;
            static constexpr int gParamIDLegatoRestart = 9;

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
            void AdvanceToStage(EnvelopeStage stage)
            {
                switch (stage)
                {
                case EnvelopeStage::Idle:
                    break;
                case EnvelopeStage::Delay:
                    if (FloatLessThanOrEquals(mDelayTime.GetMilliseconds(mModMatrix.GetKRateDestinationValue(mModDestBase + gModDestDelayTime)), 0))
                    {
                        AdvanceToStage(EnvelopeStage::Attack);
                        return;
                    }
                    break;
                case EnvelopeStage::Attack:
                    if (FloatLessThanOrEquals(mAttackTime.GetMilliseconds(mModMatrix.GetKRateDestinationValue(mModDestBase + gModDestAttackTime)), 0))
                    {
                        AdvanceToStage(EnvelopeStage::Hold);
                        return;
                    }
                    break;
                case EnvelopeStage::Hold:
                    if (FloatLessThanOrEquals(mHoldTime.GetMilliseconds(mModMatrix.GetKRateDestinationValue(mModDestBase + gModDestHoldTime)), 0))
                    {
                        AdvanceToStage(EnvelopeStage::Decay);
                        return;
                    }
                    break;
                case EnvelopeStage::Decay:
                    if (FloatLessThanOrEquals(mDecayTime.GetMilliseconds(mModMatrix.GetKRateDestinationValue(mModDestBase + gModDestDecayTime)), 0))
                    {
                        AdvanceToStage(EnvelopeStage::Decay);
                        return;
                    }
                    break;
                case EnvelopeStage::Sustain:
                    break;
                case EnvelopeStage::Release:
                    if (FloatLessThanOrEquals(mReleaseTime.GetMilliseconds(mModMatrix.GetKRateDestinationValue(mModDestBase + gModDestReleaseTime)), 0))
                    {
                        AdvanceToStage(EnvelopeStage::Idle);
                        return;
                    }
                    // here we must determine the value to release from, based on existing stage.
                    mReleaseFromValue01 = mLastOutputLevel;
                    break;
                }
                mStagePos01 = 0;
                mStage = stage;
                RecalcState();
            }

            void noteOn(bool isLegato)
            {
                if (isLegato && !mLegatoRestart.GetBoolValue()) return;
                // TODO: what if no legato restart, it's legato, but we're not actually in a playing stage?
                AdvanceToStage(EnvelopeStage::Delay);
            }

            void noteOff()
            {
                if (!IsPlaying()) return;
                AdvanceToStage(EnvelopeStage::Release);
            }

            void kill() {
                AdvanceToStage(EnvelopeStage::Idle);
            }

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

            void BeginBlock()
            {
                RecalcState();
            }

            float ProcessSample()
            {
                float ret = 0;
                EnvelopeStage nextStage = EnvelopeStage::Idle;

                switch (mStage)
                {
                default:
                case EnvelopeStage::Idle: {
                    return 0;
                }
                case EnvelopeStage::Delay: {
                    ret = 0;
                    nextStage = EnvelopeStage::Attack;
                    break;
                }
                case EnvelopeStage::Attack: {
                    ret = mAttackCurve.ApplyToValue(mStagePos01, mModMatrix.GetKRateDestinationValue(mModDestBase + gModDestAttackCurve));// gModCurveLUT.Transfer32(mStagePos01, mpLutRow); // 0-1
                    nextStage = EnvelopeStage::Hold;
                    break;
                }
                case EnvelopeStage::Hold: {
                    ret = 1;
                    nextStage = EnvelopeStage::Decay;
                    break;
                }
                case EnvelopeStage::Decay: {
                    // 0-1 => 1 - sustainlevel
                    // curve contained within the stage, not the output 0-1 range.
                    float range = 1.0f - (mSustainLevel.Get01Value() + mModMatrix.GetKRateDestinationValue(mModDestBase + gModDestSustainLevel)); // could be precalculated
                    ret = 1.0f - mDecayCurve.ApplyToValue(1.0f - mStagePos01, mModMatrix.GetKRateDestinationValue(mModDestBase + gModDestDecayCurve));// gModCurveLUT.Transfer32(1.0f - mStagePos01, mpLutRow);   // 0-1
                    ret = 1.0f - range * ret;
                    nextStage = EnvelopeStage::Sustain;
                    break;
                }
                case EnvelopeStage::Sustain: {
                    return (mSustainLevel.Get01Value() + mModMatrix.GetKRateDestinationValue(mModDestBase + gModDestSustainLevel));
                }
                case EnvelopeStage::Release: {
                    // 0-1 => mReleaseFromValue01 - 0
                    // curve contained within the stage, not the output 0-1 range.
                    ret = mReleaseCurve.ApplyToValue(1.0f - mStagePos01, mModMatrix.GetKRateDestinationValue(mModDestBase + gModDestReleaseCurve));// gModCurveLUT.Transfer32(1.0f - mStagePos01, mpLutRow); // 1-0
                    ret = ret * mReleaseFromValue01;
                    nextStage = EnvelopeStage::Idle;
                    break;
                }
                }

                mStagePos01 += mStagePosIncPerSample;
                if (mStagePos01 >= 1.0f)
                {
                    AdvanceToStage(nextStage);
                }
                mLastOutputLevel = ret;
                return ret;
            }

        private:
            void RecalcState()
            {
                switch (mStage)
                {
                default:
                case EnvelopeStage::Idle:
                    return;
                case EnvelopeStage::Delay: {
                    auto ms = mDelayTime.GetMilliseconds(mModMatrix.GetKRateDestinationValue(mModDestBase + gModDestDelayTime));
                    mStagePosIncPerSample = CalculateInc01PerSampleForMS(ms);
                    return;
                }
                case EnvelopeStage::Attack: {
                    auto ms = mAttackTime.GetMilliseconds(mModMatrix.GetKRateDestinationValue(mModDestBase + gModDestAttackTime));
                    // Serial.println(String("attack param:") + mSpec.mAttackTime.GetValue() + ", mod:" + mModValues.attackTime +
                    //                " = " + ms + " ms");
                    mStagePosIncPerSample = CalculateInc01PerSampleForMS(ms);
                    //mpLutRow = mSpec.mAttackCurve.BeginLookup(mModValues.attackCurve);
                    return;
                }
                case EnvelopeStage::Hold: {
                    mStagePosIncPerSample = CalculateInc01PerSampleForMS(mHoldTime.GetMilliseconds(mModMatrix.GetKRateDestinationValue(mModDestBase + gModDestHoldTime)));
                    return;
                }
                case EnvelopeStage::Decay: {
                    mStagePosIncPerSample =
                        CalculateInc01PerSampleForMS(mDecayTime.GetMilliseconds(mModMatrix.GetKRateDestinationValue(mModDestBase + gModDestDecayTime)));
                    //mpLutRow = mSpec.mDecayCurve.BeginLookup(mModValues.decayCurve);
                    return;
                }
                case EnvelopeStage::Sustain: {
                    return;
                }
                case EnvelopeStage::Release: {
                    mStagePosIncPerSample =
                        CalculateInc01PerSampleForMS(mReleaseTime.GetMilliseconds(mModMatrix.GetKRateDestinationValue(mModDestBase + gModDestReleaseTime)));
                    //mpLutRow = mSpec.mReleaseCurve.BeginLookup(mModValues.releaseCurve);
                    return;
                }
                }
            }

            EnvelopeStage mStage = EnvelopeStage::Idle;
            real_t mStagePos01 = 0;      // where in the current stage are we?
            real_t mLastOutputLevel = 0; // used to calculated release from value.
            real_t mStagePosIncPerSample =
                0; // how much mStagePos01 changes per sample (recalculated when mStage changes, or when spec changes)

            real_t mReleaseFromValue01 = 0; // when release stage begins, what value is it releasing from?

            ModMatrixNode& mModMatrix;
            int mModDestBase;// delay time

            static constexpr int gModDestDelayTime = 0;
            static constexpr int gModDestAttackTime = 1;
            static constexpr int gModDestAttackCurve = 2;
            static constexpr int gModDestHoldTime = 3;
            static constexpr int gModDestDecayTime = 4;
            static constexpr int gModDestDecayCurve = 5;
            static constexpr int gModDestSustainLevel = 6;
            static constexpr int gModDestReleaseTime = 7;
            static constexpr int gModDestReleaseCurve = 8;
        };

    } // namespace M7


} // namespace WaveSabreCore








