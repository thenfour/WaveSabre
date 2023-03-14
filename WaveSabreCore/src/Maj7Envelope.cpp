
#include <WaveSabreCore/Maj7Envelope.hpp>

namespace WaveSabreCore
{
    namespace M7
    {
        EnvelopeNode::EnvelopeNode(ModMatrixNode& modMatrix, ModDestination modDestIDDelayTime, real_t* paramCache, int paramBaseID, ModSource myModSource) :
            mParamBaseID(paramBaseID),
            mModMatrix(modMatrix),
            mModDestBase((int)modDestIDDelayTime),
            mDelayTime(paramCache[paramBaseID + (int)EnvParamIndexOffsets::DelayTime]),
            mAttackTime(paramCache[paramBaseID + (int)EnvParamIndexOffsets::AttackTime]),
            mAttackCurve(paramCache[paramBaseID + (int)EnvParamIndexOffsets::AttackCurve]),
            mHoldTime(paramCache[paramBaseID + (int)EnvParamIndexOffsets::HoldTime]),
            mDecayTime(paramCache[paramBaseID + (int)EnvParamIndexOffsets::DecayTime]),
            mDecayCurve(paramCache[paramBaseID + (int)EnvParamIndexOffsets::DecayCurve]),
            mSustainLevel(paramCache[paramBaseID + (int)EnvParamIndexOffsets::SustainLevel]),
            mReleaseTime(paramCache[paramBaseID + (int)EnvParamIndexOffsets::ReleaseTime]),
            mReleaseCurve(paramCache[paramBaseID + (int)EnvParamIndexOffsets::ReleaseCurve]),
            mLegatoRestart(paramCache[paramBaseID + (int)EnvParamIndexOffsets::LegatoRestart]), // because for polyphonic, holding pedal and playing a note already playing is legato and should retrig. make this opt-out.
            mMyModSource(myModSource)
        {
        }

        // advances to a specified stage. The point of this is to advance through 0-length stages so we don't end up with
        // >=1sample per stage.
        void EnvelopeNode::AdvanceToStage(EnvelopeStage stage)
        {
            switch (stage)
            {
            case EnvelopeStage::Idle:
                break;
            case EnvelopeStage::Delay:
                if (math::FloatLessThanOrEquals(mDelayTime.GetMilliseconds(mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::DelayTime)), 0))
                {
                    AdvanceToStage(EnvelopeStage::Attack);
                    return;
                }
                break;
            case EnvelopeStage::Attack:
                if (math::FloatLessThanOrEquals(mAttackTime.GetMilliseconds(mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::AttackTime)), 0))
                {
                    AdvanceToStage(EnvelopeStage::Hold);
                    return;
                }
                break;
            case EnvelopeStage::Hold:
                if (math::FloatLessThanOrEquals(mHoldTime.GetMilliseconds(mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::HoldTime)), 0))
                {
                    AdvanceToStage(EnvelopeStage::Decay);
                    return;
                }
                break;
            case EnvelopeStage::Decay:
                if (math::FloatLessThanOrEquals(mDecayTime.GetMilliseconds(mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::DecayTime)), 0))
                {
                    AdvanceToStage(EnvelopeStage::Sustain);
                    return;
                }
                break;
            case EnvelopeStage::Sustain:
                break;
            case EnvelopeStage::Release:
                if (math::FloatLessThanOrEquals(mReleaseTime.GetMilliseconds(mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::ReleaseTime)), 0))
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

        void EnvelopeNode::noteOn(bool isLegato)
        {
            if (isLegato && !mLegatoRestart.GetBoolValue()) return;
            // TODO: what if no legato restart, it's legato, but we're not actually in a playing stage?
            AdvanceToStage(EnvelopeStage::Delay);
        }

        void EnvelopeNode::noteOff()
        {
            if (!IsPlaying()) return;
            EnvelopeNode::AdvanceToStage(EnvelopeStage::Release);
        }

        void EnvelopeNode::kill() {
            EnvelopeNode::AdvanceToStage(EnvelopeStage::Idle);
        }

        void EnvelopeNode::BeginBlock()
        {
            RecalcState();
        }

        float EnvelopeNode::ProcessSample()
        {
            bool calc = (mnSampleCount == 0);
            mnSampleCount = (mnSampleCount + 1) & gRecalcSampleMask;
            if (!calc) {
                mLastOutputLevel += mOutputDeltaPerSample;
                return mLastOutputLevel;
            }

            // proceed to full calc

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
                ret = mAttackCurve.ApplyToValue(mStagePos01, mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::AttackCurve));// gModCurveLUT.Transfer32(mStagePos01, mpLutRow); // 0-1
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
                float range = 1.0f - (mSustainLevel.Get01Value() + mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::SustainLevel)); // could be precalculated
                ret = 1.0f - mDecayCurve.ApplyToValue(1.0f - mStagePos01, mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::DecayCurve));// gModCurveLUT.Transfer32(1.0f - mStagePos01, mpLutRow);   // 0-1
                ret = 1.0f - range * ret;
                nextStage = EnvelopeStage::Sustain;
                break;
            }
            case EnvelopeStage::Sustain: {
                float ret = (mSustainLevel.Get01Value() + mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::SustainLevel));
                mLastOutputLevel = ret;
                return ret;
            }
            case EnvelopeStage::Release: {
                // 0-1 => mReleaseFromValue01 - 0
                // curve contained within the stage, not the output 0-1 range.
                //ret = mReleaseCurve.ApplyToValue(1.0f - mStagePos01);
                ret = mReleaseCurve.ApplyToValue(1.0f - mStagePos01, mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::ReleaseCurve));// gModCurveLUT.Transfer32(1.0f - mStagePos01, mpLutRow); // 1-0
                ret = ret * mReleaseFromValue01;
                nextStage = EnvelopeStage::Idle;
                break;
            }
            }

            mStagePos01 += mStagePosIncPerSample * gRecalcSamplePeriod;
            if (mStagePos01 >= 1.0f)
            {
                AdvanceToStage(nextStage);
            }

            mOutputDeltaPerSample = (ret - mLastOutputLevel) / gRecalcSamplePeriod;
            mLastOutputLevel += mOutputDeltaPerSample;

            //mLastOutputLevel = ret;
            return mOutputDeltaPerSample;
        }


        void EnvelopeNode::RecalcState()
        {
            switch (mStage)
            {
            default:
            case EnvelopeStage::Idle:
                return;
            case EnvelopeStage::Delay: {
                auto ms = mDelayTime.GetMilliseconds(mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::DelayTime));
                mStagePosIncPerSample = math::CalculateInc01PerSampleForMS(ms);
                return;
            }
            case EnvelopeStage::Attack: {
                auto ms = mAttackTime.GetMilliseconds(mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::AttackTime));
                // Serial.println(String("attack param:") + mSpec.mAttackTime.GetValue() + ", mod:" + mModValues.attackTime +
                //                " = " + ms + " ms");
                mStagePosIncPerSample = math::CalculateInc01PerSampleForMS(ms);
                //mpLutRow = mSpec.mAttackCurve.BeginLookup(mModValues.attackCurve);
                return;
            }
            case EnvelopeStage::Hold: {
                mStagePosIncPerSample = math::CalculateInc01PerSampleForMS(mHoldTime.GetMilliseconds(mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::HoldTime)));
                return;
            }
            case EnvelopeStage::Decay: {
                mStagePosIncPerSample =
                    math::CalculateInc01PerSampleForMS(mDecayTime.GetMilliseconds(mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::DecayTime)));
                //mpLutRow = mSpec.mDecayCurve.BeginLookup(mModValues.decayCurve);
                return;
            }
            case EnvelopeStage::Sustain: {
                return;
            }
            case EnvelopeStage::Release: {
                mStagePosIncPerSample =
                    math::CalculateInc01PerSampleForMS(mReleaseTime.GetMilliseconds(mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::ReleaseTime)));
                //mpLutRow = mSpec.mReleaseCurve.BeginLookup(mModValues.releaseCurve);
                return;
            }
            }
        }

    } // namespace M7


} // namespace WaveSabreCore








