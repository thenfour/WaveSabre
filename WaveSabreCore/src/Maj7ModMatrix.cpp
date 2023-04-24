
#include <WaveSabreCore/Maj7ModMatrix.hpp>

namespace WaveSabreCore
{
	namespace M7
	{
		ModulationSpec::ModulationSpec(real_t* paramCache, int baseParamID) :
			mParams(paramCache, baseParamID)
		{
		}

		// putting (nearly) empty ctor in the .cpp so the default ctor doesn't get inlined.
		ModMatrixNode::ModMatrixNode()
		{
			memset(mSourceValues, 0, sizeof(mSourceValues));
			memset(mDestValues, 0, sizeof(mDestValues)); // shouldn't have to do this; will be set on 1st process. but currently needed for some reason. no time to investigate.
			memset(mModulatedDestValueDeltas, 0, sizeof(mModulatedDestValueDeltas)); // same as above.
			memset(mModSpecLastDestinations, 0, sizeof(mModSpecLastDestinations));

			// this makes 0 difference in size optimizing whether fancy or whatev
			//static constexpr float consts[] = {
			//	1,0.5f,0,-0.5f, -1
			//};

			//memcpy(&mSourceValues[(int)ModSource::Const_1], consts, sizeof(consts));
			mSourceValues[(int)ModSource::Const_1] = 1;
			mSourceValues[(int)ModSource::Const_0_5] = 0.5f;
			mSourceValues[(int)ModSource::Const_0] = 0;
			mSourceValues[(int)ModSource::Const_N0_5] = -0.5f;
			mSourceValues[(int)ModSource::Const_N1] = -1;
		}

		// dest range 
		float ModMatrixNode::MapValue(ModulationSpec& spec, ModSource src, ModParamIndexOffsets curveParam, ModParamIndexOffsets srcRangeMinParam, ModParamIndexOffsets srcRangeMaxParam, bool isDestN11)
		{
			// make sourceVal from 0-1
			float val = GetSourceValue(src);
			float rangeMin = spec.mParams.GetScaledRealValue(srcRangeMinParam, -3, 3, 0);
			float rangeMax = spec.mParams.GetScaledRealValue(srcRangeMaxParam, -3, 3, 0);
			if (math::FloatEquals(rangeMin, rangeMax, 0.0001f)) {
				return 0;
			}

			val = math::lerp_rev(rangeMin, rangeMax, GetSourceValue(src));
			// but it should actually be -1 to 1.
			if (isDestN11) {
				val = val * 2 - 1;
				val = math::clampN11(val);
			}
			else {
				val = math::clamp01(val);
			}
			return spec.mParams.ApplyCurveToValue(curveParam, val);
		}

		//void ModMatrixNode::ProcessSample(ModulationSpec* modSpecs)
			void ModMatrixNode::ProcessSample(ModulationSpec* (&modSpecs)[gModulationCount])
			{
				auto recalcMask = GetModulationRecalcSampleMask();
				auto recalcSpan = recalcMask + 1;

				if (!mnSampleCount)
				{
					// begin by resetting our modulated deltas array to 0, effectively setting all deltas to 0.
					// we will populate this array in 2 stages:
					// 1. accumulate the theoretical dest value
					// 2. turn that dest value into a delta based on current val.
					mModulatedDestValueCount = 0;

					for (size_t imod = 0; imod < gModulationCount; ++imod)
					{
						auto& spec = *modSpecs[imod];

						// enabled...
						bool skip = false;
						if (!spec.mEnabled) skip = true;
						auto modSource = spec.mSource;
						if (modSource == ModSource::None) skip = true;
						bool anyDestsEnabled = false;
						if (!skip) {
							for (auto& d : spec.mDestinations) {
								if (d != ModDestination::None) {
									anyDestsEnabled = true;
									break;
								}
							}
							if (!anyDestsEnabled) skip = true;
							if (!(*spec.mpDestSourceEnabledCached)) { 
								// for source amp mods, don't run if the source is disabled. without this condition,
								// all source amp envelopes would run always. 
								skip = true;
							}
						}

						for (size_t id = 0; id < gModulationSpecDestinationCount; ++id) {
							auto lastDest = this->mModSpecLastDestinations[imod][id];
							auto newDest = skip ? ModDestination::None : spec.mDestinations[id];
							if (lastDest != newDest) { // if a mod destination changes, then need to reset the value to 0 to erase the effect of the modulation.
								mDestValues[(size_t)lastDest] = 0;
							}
							mModSpecLastDestinations[imod][id] = newDest;
						}

						if (skip)
						{
							continue;
						}

						float sourceVal = MapValue(spec, modSource, ModParamIndexOffsets::Curve, ModParamIndexOffsets::SrcRangeMin, ModParamIndexOffsets::SrcRangeMax, true);
						if (spec.mAuxEnabled)
						{
							// attenuate the value
							auto auxSource = spec.mAuxSource;
							if (auxSource != ModSource::None) {

								float auxVal = MapValue(spec, auxSource, ModParamIndexOffsets::AuxCurve, ModParamIndexOffsets::AuxRangeMin, ModParamIndexOffsets::AuxRangeMax, false);

								// when auxAtten is 1.00, then auxVal will map from 0,1 to a scale factor of 1, 0
								// when auxAtten is 0.33, then auxVal will map from 0,1 to a scale factor of 1, .66
								float auxAtten = spec.mAuxAttenuation;
								float auxScale = math::lerp(1, 1.0f - auxAtten, auxVal);
								sourceVal *= auxScale;
							}
						}

						for (size_t id = 0; id < gModulationSpecDestinationCount; ++id) {
							const auto& d = spec.mDestinations[id];
							const ModDestination destid = d;
							if (destid == ModDestination::None) continue;
							const float target = (sourceVal * spec.mScales[id]);
							
							// get this dest/delta combo in mModulatedDestValueDeltas somehow...
							bool added = false;
							for (size_t id = 0; id < mModulatedDestValueCount; ++id) {
								auto& dvd = mModulatedDestValueDeltas[id];
								if (dvd.mDest == d) {
									// add to existing
									dvd.mDeltaPerSample += target;
									added = true;
									break;
								}
							}
							if (!added) {
								// create new dest delta thingy
								auto& dvd = mModulatedDestValueDeltas[mModulatedDestValueCount];
								mModulatedDestValueCount++;
								dvd.mDest = d;
								dvd.mDeltaPerSample = target;
							}
						}

					} // for each mod

					// for each dest/delta combo calculated... calculate the deltas.
					for (size_t id = 0; id < mModulatedDestValueCount; ++id) {
						auto& dvd = mModulatedDestValueDeltas[id];
						// right now `mDeltaPerSample` represents the target abs values. turn them into deltas.
						dvd.mDeltaPerSample = (dvd.mDeltaPerSample - mDestValues[(size_t)dvd.mDest]) / recalcSpan;
					}
				} // if needs recalc

				//} // for each mod
				for (size_t id = 0; id < mModulatedDestValueCount; ++id) {
					auto& dvd = mModulatedDestValueDeltas[id];
					mDestValues[(int)dvd.mDest] += dvd.mDeltaPerSample;
				}

				mnSampleCount = (mnSampleCount + 1) & recalcMask;
			}


			float ModMatrixAccessor::GetDestValue__(int offset) const
			{
				return mModMatrix.GetDestinationValue(offset + mBase);
			}
	} // namespace M7


} // namespace WaveSabreCore



