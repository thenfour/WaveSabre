
#include <WaveSabreCore/Maj7ModMatrix.hpp>

namespace WaveSabreCore
{
	namespace M7
	{
		inline float MapValue(float x, ModValueMapping mapping)
		{
			switch (mapping) {
			default:
			case NoMapping:
				return x;
			case N11_1N1: // -1,1 => 1,-1 (bipolar invert)
				return -x;
			case N11_01: // -1,1 => 0,1 (bipolar to positive)
				return (x + 1) * 0.5f;
			case N11_10: // -1,1 => 1,0 (bipolar to invert positive)
				x = 1-x; // 2,0
				return x * 0.5f;
			case P01_10: // 0,1 => 1,0 (positive invert)
				return 1-x;
			case P01_N11: // 0,1 => -1,1 (positive to bipolar)
				return x * 2 - 1;
			case P01_1N1: // 0,1 => 1,-1 (positive to negative bipolar)
				x *= 2; // 0,2
				x = 1 - x; // 1,-1
				return x;
			}
		}

			void ModMatrixNode::ProcessSample(ModulationSpec(&modSpecs)[gModulationCount])
			{
				auto recalcMask = GetModulationRecalcSampleMask();
				if (!mnSampleCount)
				{
					mModulatedDestValueCount = 0;
					for (size_t imod = 0; imod < gModulationCount; ++imod)
					{
						auto& spec = modSpecs[imod];
						bool skip = false;
						if (!spec.mEnabled.mCachedVal) skip = true;
						auto modSource = spec.mSource.mCachedVal;
						if (modSource == ModSource::None) skip = true;
						bool anyDestsEnabled = false;
						if (!skip) {
							for (auto& d : spec.mDestinations) {
								if (d.mCachedVal != ModDestination::None) {
									anyDestsEnabled = true;
									break;
								}
							}
							if (!anyDestsEnabled) skip = true;
							if (spec.mpDestSourceEnabledParam && !spec.mpDestSourceEnabledParam->mCachedVal) {
								skip = true;
							}
						}

						for (size_t id = 0; id < gModulationSpecDestinationCount; ++id) {
							//mDestValueDeltas[imod][id] = 0;
							auto lastDest = this->mModSpecLastDestinations[imod][id];
							auto newDest = skip ? ModDestination::None : spec.mDestinations[id].mCachedVal;
							if (lastDest != newDest) { // if a mod destination changes, then need to reset the value to 0 to erase the effect of the modulation.
								mDestValues[(size_t)lastDest] = 0;
							}
							mModSpecLastDestinations[imod][id] = newDest;
						}

						if (skip)
						{
							continue;
						}

						real_t sourceVal = GetSourceValue(modSource);
						sourceVal = MapValue(sourceVal, spec.mValueMapping.mCachedVal);
						sourceVal = spec.mCurve.ApplyToValue(sourceVal);
						if (spec.mAuxEnabled.mCachedVal)
						{
							// attenuate the value
							auto auxSource = spec.mAuxSource.mCachedVal;
							if (auxSource != ModSource::None) {
								float auxVal = GetSourceValue(auxSource);
								auxVal = MapValue(auxVal, spec.mAuxValueMapping.mCachedVal);
								auxVal = spec.mAuxCurve.ApplyToValue(auxVal);
								// when auxAtten is 1.00, then auxVal will map from 0,1 to a scale factor of 1, 0
								// when auxAtten is 0.33, then auxVal will map from 0,1 to a scale factor of 1, .66
								float auxAtten = spec.mAuxAttenuation.Get01Value();
								float auxScale = math::lerp(1, 1.0f - auxAtten, auxVal);
								sourceVal *= auxScale;
							}
						}

						for (size_t id = 0; id < gModulationSpecDestinationCount; ++id) {
							const auto& d = spec.mDestinations[id];
							const ModDestination destid = d.mCachedVal;
							if (destid == ModDestination::None) continue;
							const float orig = mDestValues[(size_t)destid];
							const float amt = sourceVal * spec.mScales[id].mCachedVal;
							
							if (math::FloatEquals(amt, orig)) continue;

							const float deltaPerSample = (amt - orig) / (recalcMask + 1);

							// get this dest/delta combo in mModulatedDestValueDeltas somehow...
							bool added = false;
							for (size_t id = 0; id < mModulatedDestValueCount; ++id) {
								auto& dvd = mModulatedDestValueDeltas[id];
								if (dvd.mDest == d.mCachedVal) {
									// add to existing
									dvd.mDeltaPerSample += deltaPerSample;
									added = true;
									break;
								}
							}
							if (!added) {
								// create new dest delta thingy
								auto& dvd = mModulatedDestValueDeltas[mModulatedDestValueCount];
								mModulatedDestValueCount++;
								dvd.mDest = d.mCachedVal;
								dvd.mDeltaPerSample = deltaPerSample;
							}
						}

					} // for each mod
				} // if needs recalc

				//} // for each mod
				for (size_t id = 0; id < mModulatedDestValueCount; ++id) {
					auto& dvd = mModulatedDestValueDeltas[id];
					mDestValues[(int)dvd.mDest] += dvd.mDeltaPerSample;
				}

				mnSampleCount = (mnSampleCount + 1) & recalcMask;
			}

	} // namespace M7


} // namespace WaveSabreCore



