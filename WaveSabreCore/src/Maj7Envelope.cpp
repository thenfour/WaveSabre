
#include <WaveSabreCore/Maj7Envelope.hpp>

namespace WaveSabreCore
{
    namespace M7
    {
        EnvelopeNode::EnvelopeNode(ModMatrixNode& modMatrix, real_t* paramCache, const EnvelopeInfo& envInfo) : //ModDestination modDestIDDelayTime, , int paramBaseID, ModSource myModSource) :
        //EnvelopeNode::EnvelopeNode(ModMatrixNode& modMatrix, ModDestination modDestIDDelayTime, real_t* paramCache, int paramBaseID, ModSource myModSource) :
            mModMatrix(modMatrix),
            mParams(paramCache, envInfo.mParamBase),
            mModDestBase((int)envInfo.mModDestBase),
            mMyModSource(envInfo.mMyModSource)
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
            case EnvelopeStage::FixedDelay:
                // the delay stage functions the same as the release stage, using the release duration
                // for curving, but cutoff (or extended) to the delay length.
                mReleaseFromValue01 = mLastOutputLevel;
                mReleaseStagePos01 = 0;
                break;
            case EnvelopeStage::Delay:
                if (mParams.GetPowCurvedValue(EnvParamIndexOffsets::DelayTime, gEnvTimeCfg, mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::DelayTime)) <= 0)
                {
                    AdvanceToStage(EnvelopeStage::Attack);
                    return;
                }
                break;
            case EnvelopeStage::Attack:
                if (mParams.GetPowCurvedValue(EnvParamIndexOffsets::AttackTime, gEnvTimeCfg, mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::AttackTime)) <= 0)
                {
                    AdvanceToStage(EnvelopeStage::Hold);
                    return;
                }
                mAttackFromValue01 = mLastOutputLevel; // usually 0
                break;
            case EnvelopeStage::Hold:
                if (mParams.GetPowCurvedValue(EnvParamIndexOffsets::HoldTime, gEnvTimeCfg, mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::HoldTime)) <= 0)
                {
                    AdvanceToStage(EnvelopeStage::Decay);
                    return;
                }
                break;
            case EnvelopeStage::Decay:
                if (mParams.GetPowCurvedValue(EnvParamIndexOffsets::DecayTime, gEnvTimeCfg, mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::DecayTime)) <= 0)
                {
                    AdvanceToStage(mMode == EnvelopeMode::Sustain ? EnvelopeStage::Sustain : EnvelopeStage::ReleaseSilence);
                    return;
                }
                break;
            case EnvelopeStage::Sustain:
                break;
            case EnvelopeStage::Release:
                if (mParams.GetPowCurvedValue(EnvParamIndexOffsets::ReleaseTime, gEnvTimeCfg, mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::ReleaseTime)) <= 0)
                {
                    AdvanceToStage(EnvelopeStage::ReleaseSilence);
                    return;
                }
                // here we must determine the value to release from, based on existing stage.
                mReleaseFromValue01 = mLastOutputLevel;
                break;
            case EnvelopeStage::ReleaseSilence:
                break;
            }
            mStagePos01 = 0;
            mStage = stage;
            mnSampleCount = 0;
            RecalcState();
        }

        void EnvelopeNode::noteOn(bool isLegato)
        {
            if (isLegato && !mParams.GetBoolValue(EnvParamIndexOffsets::LegatoRestart)) {
                return;
            }
            // TODO: what if no legato restart, it's legato, but we're not actually in a playing stage?

            AdvanceToStage(EnvelopeStage::FixedDelay);
        }

        void EnvelopeNode::noteOff()
        {
            if (!IsPlaying()) return;
            if (mMode == EnvelopeMode::OneShot) return; // in one-shot there's no way to cut off the note faster.
            AdvanceToStage(EnvelopeStage::Release);
        }

        // call to forcibly kill the env and jump to silence
        float EnvelopeNode::kill() {
            mnSampleCount = 0;
            mStage = EnvelopeStage::Idle;
            mLastOutputLevel = 0;
            mOutputDeltaPerSample = 0;
            mStagePosIncPerSample = 0;
            mAttackFromValue01 = 0;
            return 0;
        }

        void EnvelopeNode::BeginBlock()
        {
            mnSampleCount = 0; // ensure reprocessing after setting these params to avoid corrupt state.
            RecalcState();
        }

        void EnvelopeNode::ProcessSampleFull()
        {
            // full calc
            mMode = mParams.GetEnumValue<EnvelopeMode>(EnvParamIndexOffsets::Mode);
            int recalcPeriod = (GetModulationRecalcSampleMask() + 1);

            float ret = 0;
            EnvelopeStage nextStage = EnvelopeStage::Idle;

            switch (mStage)
            {
            default:
            case EnvelopeStage::Idle: {
                mLastOutputLevel = 0;
                mOutputDeltaPerSample = 0;
                return;
            }
            case EnvelopeStage::FixedDelay: {
                ret = mReleaseFromValue01;
                mOutputDeltaPerSample = 0;
                break;
            }
            case EnvelopeStage::Delay: {
                ret = mParams.ApplyCurveToValue(EnvParamIndexOffsets::ReleaseCurve, 1.0f - mReleaseStagePos01, mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::ReleaseCurve));
                ret = ret * mReleaseFromValue01;
                mReleaseStagePos01 += mReleaseStagePosIncPerSample * recalcPeriod;
                nextStage = EnvelopeStage::Attack;
                break; // advance through stage.
            }
            case EnvelopeStage::Attack: {
                ret = mParams.ApplyCurveToValue(EnvParamIndexOffsets::AttackCurve, mStagePos01, mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::AttackCurve));
                ret = math::lerp(mAttackFromValue01, 1, ret);
                nextStage = EnvelopeStage::Hold;
                break; // advance through stage.
            }
            case EnvelopeStage::Hold: {
                ret = 1;
                nextStage = EnvelopeStage::Decay;
                break; // advance through stage.
            }
            case EnvelopeStage::Decay: {
                // 0-1 => 1 - sustainlevel
                // curve contained within the stage, not the output 0-1 range.
                float susLevel = 0;
                nextStage = EnvelopeStage::ReleaseSilence;
                if (mMode == EnvelopeMode::Sustain) {
                    nextStage = EnvelopeStage::Sustain;
                    susLevel = mParams.Get01Value(EnvParamIndexOffsets::SustainLevel, mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::SustainLevel));
                }
                float range = 1.0f - (susLevel);
                ret = 1.0f - mParams.ApplyCurveToValue(EnvParamIndexOffsets::DecayCurve, 1.0f - mStagePos01, mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::DecayCurve));
                ret = 1.0f - range * ret;
                break; // advance through stage.
            }
            case EnvelopeStage::Sustain: {
                if (mMode == EnvelopeMode::OneShot) {
                    kill();
                    return;
                }
                float ret = mParams.Get01Value(EnvParamIndexOffsets::SustainLevel, mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::SustainLevel));
                mLastOutputLevel = ret;
                return;
            }
            case EnvelopeStage::Release: {
                if (mMode == EnvelopeMode::OneShot) {
                    kill();
                    return;
                }
                // 0-1 => mReleaseFromValue01 - 0
                // curve contained within the stage, not the output 0-1 range.
                //ret = mReleaseCurve.ApplyToValue(1.0f - mStagePos01);
                ret = mParams.ApplyCurveToValue(EnvParamIndexOffsets::ReleaseCurve, 1.0f - mStagePos01, mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::ReleaseCurve));
                ret = ret * mReleaseFromValue01;
                nextStage = EnvelopeStage::ReleaseSilence;
                break; // advance through stage.
            }
            case EnvelopeStage::ReleaseSilence: {
                nextStage = EnvelopeStage::Idle;
                ret = 0;
                break;
            }
            }

            mStagePos01 += mStagePosIncPerSample * recalcPeriod;
            if (mStagePos01 >= 1.0f)
            {
                AdvanceToStage(nextStage);
            }

            mOutputDeltaPerSample = (ret - mLastOutputLevel) / recalcPeriod;
        }

        float EnvelopeNode::ProcessSample()
        {
            // for performance, because we process envelopes aggressively, check for the simplest & most common case.
            switch (mStage) {
            case EnvelopeStage::Idle:
                return 0;
            }
            auto recalcMask = GetModulationRecalcSampleMask();
            if (mnSampleCount == 0) {
                ProcessSampleFull();
            }
            mnSampleCount = (mnSampleCount + 1) & recalcMask;

            mLastOutputLevel += mOutputDeltaPerSample;

            if ((mStage == EnvelopeStage::FixedDelay) && (0 == --mFixedDelaySamplesRemaining)) {
                AdvanceToStage(EnvelopeStage::Delay);
            }

            return mLastOutputLevel;
        }

        // calculates stage increments per sample for times + modulations.
        void EnvelopeNode::RecalcState()
        {
            auto UpdateStagePosInc = [&](EnvParamIndexOffsets paramOffset, EnvModParamIndexOffsets modid) {
                auto ms = mParams.GetPowCurvedValue(paramOffset, gEnvTimeCfg, mModMatrix.GetDestinationValue(mModDestBase + (int)modid));
                mStagePosIncPerSample = math::CalculateInc01PerSampleForMS(ms);
            };

            switch (mStage)
            {
            default:
            case EnvelopeStage::Idle:
                return;
            case EnvelopeStage::FixedDelay:
                mFixedDelaySamplesRemaining = 3;
                return;
            case EnvelopeStage::Delay: {
                UpdateStagePosInc(EnvParamIndexOffsets::DelayTime, EnvModParamIndexOffsets::DelayTime);
                mReleaseStagePosIncPerSample = math::CalculateInc01PerSampleForMS(
                    mParams.GetPowCurvedValue(EnvParamIndexOffsets::ReleaseTime, gEnvTimeCfg, mModMatrix.GetDestinationValue(mModDestBase + (int)EnvModParamIndexOffsets::ReleaseTime)));
                return;
            }
            case EnvelopeStage::Attack: {
                UpdateStagePosInc(EnvParamIndexOffsets::AttackTime, EnvModParamIndexOffsets::AttackTime);
                return;
            }
            case EnvelopeStage::Hold: {
                UpdateStagePosInc(EnvParamIndexOffsets::HoldTime, EnvModParamIndexOffsets::HoldTime);
                return;
            }
            case EnvelopeStage::Decay: {
                UpdateStagePosInc(EnvParamIndexOffsets::DecayTime, EnvModParamIndexOffsets::DecayTime);
                return;
            }
            case EnvelopeStage::Sustain: {
                return;
            }
            case EnvelopeStage::Release: {
                UpdateStagePosInc(EnvParamIndexOffsets::ReleaseTime, EnvModParamIndexOffsets::ReleaseTime);
                return;
            }
            case EnvelopeStage::ReleaseSilence: {
                mLastOutputLevel = 0;
                mOutputDeltaPerSample = 0;
                mStagePosIncPerSample = 1.0f / (10 * (GetModulationRecalcSampleMask() + 1)); // last more than 2 recalc periods, in order for our zero to fully propagate, leaving no trailing crap around.
                return;
            }
            }
        }

    } // namespace M7


} // namespace WaveSabreCore








