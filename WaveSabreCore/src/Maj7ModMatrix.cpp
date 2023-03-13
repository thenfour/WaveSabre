
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

			void ModMatrixBuffers::Reset(bool zero) {
				if (zero) {
					memset(mValues, 0, sizeof(mValues));
				}
				//size_t desiredElements = gEndpointCount * blockSize;
				//if (mElementsAllocated < desiredElements) {
				//	auto newBuffer = new real_t[desiredElements];
				//	memcpy(newBuffer, mpBuffer, sizeof(*mpBuffer) * mElementsAllocated);
				//	delete[] mpBuffer;
				//	mpBuffer = newBuffer;
				//	mElementsAllocated = desiredElements;
				//}
				//mBlockSize = blockSize;
				//if (zero) {
				//	memset(mpBuffer, 0, desiredElements * sizeof(mpBuffer[0]));
				//	memset(mpKRateValues, 0, std::size(mpKRateValues) * sizeof(mpKRateValues[0]));
				//	memset(mpARatePopulated, 0, std::size(mpARatePopulated) * sizeof(mpARatePopulated[0]));
				//}
				//memset(mpKRatePopulated, 0, std::size(mpKRatePopulated) * sizeof(mpKRatePopulated[0]));
			}

			//void ModMatrixBuffers::SetARateValue(size_t id, size_t iSample, real_t val) {
			//	//if (id >= gEndpointCount) return; // error
			//	size_t i = id * mBlockSize + iSample;
			//	//if (i >= mBlockSize) return; // error
			//	mpARatePopulated[id] = 1;
			//	mpBuffer[i] = val;
			//	//return mSource.SetSourceARateValue(iblock, iSample, val);
			//}

			//void ModMatrixBuffers::SetKRateValue(size_t id, real_t val)
			//{
			//	mpKRateValues[id] = val;
			//	//mpKRatePopulated[id] = 1;
			//}

			//real_t ModMatrixBuffers::GetValue(size_t id, size_t sample) const
			//{
			//	//if (!mpBuffer || id >= gEndpointCount) return 0;
			//	if (mpARatePopulated[id]) {
			//		size_t i = id * mBlockSize + sample;
			//		if (i >= mElementsAllocated) return 0;
			//		return mpBuffer[i];
			//	}
			//	//if (mpKRatePopulated[id]) 
			//	return mpKRateValues[(size_t)id];
			//	//return 0;
			//}
	/*
			ModMatrixNode::ModMatrixNode() :
				mSource((size_t)ModSource::Count),
				mDest((size_t)ModDestination::Count)
			{}*/


			void ModMatrixBuffers::SetValue(size_t id, real_t val)
			{
				mValues[id] = val;
			}

			real_t ModMatrixBuffers::GetValue(size_t id) const
			{
				return mValues[id];
			}


			// call at the beginning of audio block processing to allocate & zero all buffers, preparing for source value population.
			void ModMatrixNode::InitBlock()
			{
				//mSource.Reset(false);
				//mDest.Reset(true);
			}

			float ModMatrixNode::InvertValue(float val, const BoolParam& invertParam, const ModSource modSource)
			{
				if (invertParam.GetBoolValue()) {
					switch (GetPolarity(modSource)) {
					case ModulationPolarity::N11:
						return -val;
					case ModulationPolarity::Positive01:
						return 1.0f - val;
					}
				}
				return val;
			}

			void ModMatrixNode::ProcessSample(ModulationSpec(&modSpecs)[gModulationCount])
			{
				mDest.Reset(true);
				for (ModulationSpec& spec : modSpecs) {
					if (!spec.mEnabled.GetBoolValue()) continue;
					auto modSource = spec.mSource.GetEnumValue();
					//auto srcRate = GetRate(modSource);
					if (modSource == ModSource::None) continue;
					auto modDest = spec.mDestination.GetEnumValue();
					//auto destRate = GetRate(modSource);
					if (modDest == ModDestination::None) continue;

					real_t sourceVal = GetSourceValue(spec.mSource.GetEnumValue());
					sourceVal = InvertValue(sourceVal, spec.mInvert, modSource);
					sourceVal = spec.mCurve.ApplyToValue(sourceVal);
					sourceVal *= spec.mScale.GetN11Value();

					if (spec.mAuxEnabled.GetBoolValue())
					{
						// attenuate the value
						//const auto& auxSourceInfo = gModSourceInfo[(int)spec.mAuxSource.GetEnumValue()];
						auto auxSource = spec.mAuxSource.GetEnumValue();
						if (auxSource != ModSource::None) {
							float auxVal = GetSourceValue(spec.mAuxSource.GetEnumValue());
							auxVal = InvertValue(auxVal, spec.mAuxInvert, auxSource);
							auxVal = spec.mAuxCurve.ApplyToValue(auxVal);
							// when auxAtten is 1.00, then auxVal will map from 0,1 to a scale factor of 1, 0
							// when auxAtten is 0.33, then auxVal will map from 0,1 to a scale factor of 1, .66
							float auxAtten = spec.mAuxAttenuation.Get01Value();
							float auxScale = math::lerp(1, 1.0f - auxAtten, auxVal);
							sourceVal *= auxScale;
						}
					}

					real_t destVal = GetDestinationValue(spec.mDestination.GetEnumValue());
					mDest.SetValue((size_t)spec.mDestination.GetEnumValue(),destVal + sourceVal);
				}
			}
	} // namespace M7


} // namespace WaveSabreCore



