
#include <WaveSabreCore/Maj7ModMatrix.hpp>

namespace WaveSabreCore
{
	namespace M7
	{

			ModulationSpec::ModulationSpec(real_t* paramCache, int baseParamID) :
				mBaseParamID(baseParamID),
				mEnabled(paramCache[baseParamID + (int)ModParamIndexOffsets::Enabled]),
				mSource(paramCache[baseParamID + (int)ModParamIndexOffsets::Source], ModSource::Count),
				mDestination(paramCache[baseParamID + (int)ModParamIndexOffsets::Destination], ModDestination::Count),
				mCurve(paramCache[baseParamID + (int)ModParamIndexOffsets::Curve]),
				mScale(paramCache[baseParamID + (int)ModParamIndexOffsets::Scale]),
				mAuxEnabled(paramCache[baseParamID + (int)ModParamIndexOffsets::AuxEnabled]),
				mAuxSource(paramCache[baseParamID + (int)ModParamIndexOffsets::AuxSource], ModSource::Count),
				mAuxCurve(paramCache[baseParamID + (int)ModParamIndexOffsets::AuxCurve]),
				mAuxAttenuation(paramCache[baseParamID + (int)ModParamIndexOffsets::AuxAttenuation]),
				mInvert(paramCache[baseParamID + (int)ModParamIndexOffsets::Invert]),
				mAuxInvert(paramCache[baseParamID + (int)ModParamIndexOffsets::AuxInvert])
			{
			}



			float ModMatrixNode::InvertValue(float val, bool invertParam, const ModSource modSource)
			{
				if (invertParam) {
					switch (GetPolarity(modSource)) {
					case ModulationPolarity::N11:
						return -val;
					case ModulationPolarity::Positive01:
						return 1.0f - val;
					}
				}
				return val;
			}

			void ModMatrixNode::FullProcessSample(ModulationSpec(&modSpecs)[gModulationCount])
			{
				//for (ModulationSpec& spec : modSpecs)
				for (size_t imod = 0; imod < gModulationCount; ++ imod)
				{
					auto& spec = modSpecs[imod];
					mDestValueDeltas[imod] = 0;
					if (!spec.mEnabled.mCachedVal) continue;
					auto modSource = spec.mSource.mCachedVal;
					if (modSource == ModSource::None) continue;
					auto modDest = spec.mDestination.mCachedVal;
					if (modDest == ModDestination::None) continue;
					if (spec.mpDestSourceEnabledParam && !spec.mpDestSourceEnabledParam->mCachedVal) { // NB: requires that the source.enabled has cached its bool val
						continue;
					}

					real_t sourceVal = GetSourceValue(modSource);
					sourceVal = InvertValue(sourceVal, spec.mInvert.mCachedVal, modSource);
					sourceVal = spec.mCurve.ApplyToValue(sourceVal);
					sourceVal *= spec.mScale.mCachedVal;

					if (spec.mAuxEnabled.mCachedVal)
					{
						// attenuate the value
						auto auxSource = spec.mAuxSource.mCachedVal;
						if (auxSource != ModSource::None) {
							float auxVal = GetSourceValue(auxSource);
							auxVal = InvertValue(auxVal, spec.mAuxInvert.mCachedVal, auxSource);
							auxVal = spec.mAuxCurve.ApplyToValue(auxVal);
							// when auxAtten is 1.00, then auxVal will map from 0,1 to a scale factor of 1, 0
							// when auxAtten is 0.33, then auxVal will map from 0,1 to a scale factor of 1, .66
							float auxAtten = spec.mAuxAttenuation.Get01Value();
							float auxScale = math::lerp(1, 1.0f - auxAtten, auxVal);
							sourceVal *= auxScale;
						}
					}

					float orig = mDestValues[(size_t)modDest];
					mDestValueDeltas[imod] = (sourceVal - orig) / (gRecalcSampleMask + 1);

					//real_t destVal = GetDestinationValue(modDest);
					//mDestValues[(size_t)modDest] += sourceVal;
				} // for each mod
			} // full process

			//void ModMatrixNode::DeltaProcessSample(ModulationSpec(&modSpecs)[gModulationCount])
			//{
			//} // full process

			void ModMatrixNode::ProcessSample(ModulationSpec(&modSpecs)[gModulationCount])
			{
				//memset(mDestValues, 0, sizeof(mDestValues));// mDest.Reset();
				if (!mnSampleCount)
				{
					FullProcessSample(modSpecs); // populate modspec dest addend-per-sample
				}
				for (size_t imod = 0; imod < gModulationCount; ++imod)
				{
					auto& spec = modSpecs[imod];
					mDestValues[(size_t)spec.mDestination.mCachedVal] += mDestValueDeltas[imod];
				} // for each mod

				mnSampleCount = (mnSampleCount + 1) & gRecalcSampleMask;
			}

	} // namespace M7


} // namespace WaveSabreCore



