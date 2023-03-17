
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
            mLegatoRestart(paramCache[paramBaseID + (int)EnvParamIndexOffsets::LegatoRestart]),
            mMode(paramCache[paramBaseID + (int)EnvParamIndexOffsets::Mode], EnvelopeMode::Count),
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
                mLastOutputLevel = 0;
                break;
            case EnvelopeStage::Delay:
                if (math::FloatLessThanOrEquals(mDelayTime.GetMilliseconds(mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::DelayTime)), 0))
                {
                    AdvanceToStage(EnvelopeStage::Attack);
                    return;
                }
                // the delay stage functions the same as the release stage, using the release duration
                // for curving, but cutoff (or extended) to the delay length.
                mReleaseFromValue01 = mLastOutputLevel;
                mReleaseStagePos01 = 0;
                break;
            case EnvelopeStage::Attack:
                if (math::FloatLessThanOrEquals(mAttackTime.GetMilliseconds(mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::AttackTime)), 0))
                {
                    AdvanceToStage(EnvelopeStage::Hold);
                    return;
                }
                mAttackFromValue01 = mLastOutputLevel; // usually 0
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
                    AdvanceToStage(mMode.mCachedVal == EnvelopeMode::Sustain ? EnvelopeStage::Sustain : EnvelopeStage::Idle);
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
            if (mMode.mCachedVal == EnvelopeMode::OneShot) return; // in one-shot there's no way to cut off the note faster.
            EnvelopeNode::AdvanceToStage(EnvelopeStage::Release);
        }

        float EnvelopeNode::kill() {
            EnvelopeNode::AdvanceToStage(EnvelopeStage::Idle);
            return 0;
        }

        void EnvelopeNode::BeginBlock()
        {
            mnSampleCount = 0; // ensure reprocessing after setting these params to avoid corrupt state.
            mMode.CacheValue();
            RecalcState();
        }

        float EnvelopeNode::ProcessSample()
        {
            auto recalcMask = GetModulationRecalcSampleMask();
            bool calc = (mnSampleCount == 0);
            mnSampleCount = (mnSampleCount + 1) & recalcMask;
            if (!calc) {
                mLastOutputLevel += mOutputDeltaPerSample;
                return mLastOutputLevel;
            }

            // proceed to full calc
            mMode.CacheValue();

            float ret = 0;
            EnvelopeStage nextStage = EnvelopeStage::Idle;

            switch (mStage)
            {
            default:
            case EnvelopeStage::Idle: {
                mLastOutputLevel = 0;
                return 0;
            }
            case EnvelopeStage::Delay: {
                ret = mReleaseCurve.ApplyToValue(1.0f - mReleaseStagePos01, mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::ReleaseCurve));
                ret = ret * mReleaseFromValue01;
                mReleaseStagePos01 += mReleaseStagePosIncPerSample * (recalcMask + 1);
                nextStage = EnvelopeStage::Attack;
                break;
            }
            case EnvelopeStage::Attack: {
                ret = mAttackCurve.ApplyToValue(mStagePos01, mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::AttackCurve));
                ret = math::lerp(mAttackFromValue01, 1, ret);
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
                float susLevel = 0;
                nextStage = EnvelopeStage::Idle;
                if (mMode.mCachedVal == EnvelopeMode::Sustain) {
                    nextStage = EnvelopeStage::Sustain;
                    susLevel = mSustainLevel.Get01Value(mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::SustainLevel));
                }
                float range = 1.0f - (susLevel);
                ret = 1.0f - mDecayCurve.ApplyToValue(1.0f - mStagePos01, mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::DecayCurve));
                ret = 1.0f - range * ret;
                break;
            }
            case EnvelopeStage::Sustain: {
                if (mMode.mCachedVal == EnvelopeMode::OneShot) {
                    return kill();
                }
                float ret = (mSustainLevel.Get01Value(mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::SustainLevel)));
                mLastOutputLevel = ret;
                return ret;
            }
            case EnvelopeStage::Release: {
                if (mMode.mCachedVal == EnvelopeMode::OneShot) {
                    return kill();
                }
                // 0-1 => mReleaseFromValue01 - 0
                // curve contained within the stage, not the output 0-1 range.
                //ret = mReleaseCurve.ApplyToValue(1.0f - mStagePos01);
                ret = mReleaseCurve.ApplyToValue(1.0f - mStagePos01, mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::ReleaseCurve));
                ret = ret * mReleaseFromValue01;
                nextStage = EnvelopeStage::Idle;
                break;
            }
            }

            mStagePos01 += mStagePosIncPerSample * (recalcMask + 1);
            if (mStagePos01 >= 1.0f)
            {
                AdvanceToStage(nextStage);
            }

            mOutputDeltaPerSample = (ret - mLastOutputLevel) / (recalcMask + 1);
            mLastOutputLevel += mOutputDeltaPerSample;
            return mLastOutputLevel;
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

                mReleaseStagePosIncPerSample = math::CalculateInc01PerSampleForMS(
                    mReleaseTime.GetMilliseconds(mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::ReleaseTime)));
                return;
            }
            case EnvelopeStage::Attack: {
                auto ms = mAttackTime.GetMilliseconds(mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::AttackTime));
                mStagePosIncPerSample = math::CalculateInc01PerSampleForMS(ms);
                return;
            }
            case EnvelopeStage::Hold: {
                mStagePosIncPerSample = math::CalculateInc01PerSampleForMS(mHoldTime.GetMilliseconds(mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::HoldTime)));
                return;
            }
            case EnvelopeStage::Decay: {
                mStagePosIncPerSample =
                    math::CalculateInc01PerSampleForMS(mDecayTime.GetMilliseconds(mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::DecayTime)));
                return;
            }
            case EnvelopeStage::Sustain: {
                return;
            }
            case EnvelopeStage::Release: {
                mStagePosIncPerSample =
                    math::CalculateInc01PerSampleForMS(mReleaseTime.GetMilliseconds(mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::ReleaseTime)));
                return;
            }
            }
        }

    } // namespace M7


} // namespace WaveSabreCore








