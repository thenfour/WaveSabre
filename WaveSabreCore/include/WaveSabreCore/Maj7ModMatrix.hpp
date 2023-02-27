#pragma once

#include <WaveSabreCore/Maj7Basic.hpp>

namespace WaveSabreCore
{
	namespace M7
	{

		enum class ModulationRate : uint8_t
		{
			Disabled,
			KRate, // evaluated each buffer.
			ARate,
			Count,
		};

		enum class ModulationPolarity : uint8_t
		{
			Positive01,
			N11,
			Count,
		};

		enum class ModSource : uint8_t
		{
			None,
			AmpEnv, // arate, 01
			ModEnv1, // arate, 01
			ModEnv2, // arate, 01
			LFO1, // arate, N11
			LFO2, // arate, N11
			PitchBend, // krate, N11
			Velocity, // krate, 01
			NoteValue, // krate, 01
			RandomTrigger, // krate, 01
			SustainPedal, // krate, 01
			Macro1, // krate, 01
			Macro2, // krate, 01
			Macro3, // krate, 01
			Macro4, // krate, 01

			Count,
		};

		// size-optimize using macro
		#define MOD_SOURCE_CAPTIONS { \
			"None", \
			"AmpEnv", \
			"ModEnv1", \
			"ModEnv2", \
			"LFO1", \
			"LFO2", \
			"PitchBend", \
			"Velocity", \
			"NoteValue", \
			"RandomTrigger", \
			"SustainPedal", \
			"Macro1", \
			"Macro2", \
			"Macro3", \
			"Macro4", \
		}

		enum class ModDestination : uint8_t
		{
			None,
			UnisonoDetune, // krate, 01
			UnisonoStereoSpread, // krate, 01
			OscillatorDetune, // krate, 01
			OscillatorStereoSpread, // krate, 01
			FMBrightness, // krate, 01

			PortamentoTime, // krate, 01
			PortamentoCurve, // krate, N11

			// NB!! the order of these must 1) be the same for all envelopes, and 2) stay in sync with the order expected by EnvelopeModulationValues::Fill
			Osc1Volume, // arate, 01
			Osc1Waveshape, // arate, 01
			Osc1SyncFrequency, // arate, 01
			Osc1FrequencyParam, // arate, 01
			Osc1FMFeedback, // arate, 01

			Osc2Volume,
			Osc2Waveshape,
			Osc2SyncFrequency,
			Osc2FrequencyParam,
			Osc2FMFeedback,

			Osc3Volume,
			Osc3Waveshape,
			Osc3SyncFrequency,
			Osc3FrequencyParam,
			Osc3FMFeedback,

			// NB!! the order of these must 1) be the same for all envelopes, and 2) stay in sync with the order expected by EnvelopeModulationValues::Fill
			AmpEnvDelayTime, 
			AmpEnvAttackTime,
			AmpEnvAttackCurve,
			AmpEnvHoldTime,
			AmpEnvDecayTime,
			AmpEnvDecayCurve,
			AmpEnvSustainLevel,
			AmpEnvReleaseTime,
			AmpEnvReleaseCurve,

			Env1DelayTime, // krate, 01
			Env1AttackTime, // krate, 01
			Env1AttackCurve, // krate, N11
			Env1HoldTime, // krate, 01
			Env1DecayTime, // krate, 01
			Env1DecayCurve, // krate, N11
			Env1SustainLevel, // krate, 01
			Env1ReleaseTime, // krate, 01
			Env1ReleaseCurve, // krate, N11

			Env2DelayTime,
			Env2AttackTime,
			Env2AttackCurve,
			Env2HoldTime,
			Env2DecayTime,
			Env2DecayCurve,
			Env2SustainLevel,
			Env2ReleaseTime,
			Env2ReleaseCurve,

			LFO1Waveshape,
			LFO1FrequencyParam,

			LFO2Waveshape, // krate, 01
			LFO2FrequencyParam, // krate, 01

			FilterQ, // krate, 01
			FilterSaturation, // krate, 01
			FilterFrequency, // krate, 01

			FMAmt1to2, // arate, 01
			FMAmt1to3, // arate, 01
			FMAmt2to1, // arate, 01
			FMAmt2to3, // arate, 01
			FMAmt3to1, // arate, 01
			FMAmt3to2, // arate, 01

			Count,
		};
		enum class EnvModParamIndexOffsets : uint8_t // MUST BE IN SYNC WITH ABOVE
		{
			DelayTime,
			AttackTime,
			AttackCurve,
			HoldTime,
			DecayTime,
			DecayCurve,
			SustainLevel,
			ReleaseTime,
			ReleaseCurve,
		};
		enum class OscModParamIndexOffsets : uint8_t // MUST BE IN SYNC WITH ABOVE
		{
			Volume,
			Waveshape,
			SyncFrequency,
			FrequencyParam,
			FMFeedback,
		};
		enum class LFOModParamIndexOffsets : uint8_t // MUST BE IN SYNC WITH ABOVE
		{
			Waveshape,
			FrequencyParam,
		};

		// size-optimize using macro
		#define MOD_DEST_CAPTIONS { \
			"None", \
			"UnisonoDetune", \
			"UnisonoStereoSpread", \
			"OscillatorDetune", \
			"OscillatorStereoSpread", \
			"FMBrightness", \
			"PortamentoTime", \
			"PortamentoCurve", \
			"Osc1Volume", \
			"Osc1Waveshape", \
			"Osc1SyncFrequency", \
			"Osc1FrequencyParam", \
			"Osc1FMFeedback", \
			"Osc2Volume", \
			"Osc2Waveshape", \
			"Osc2SyncFrequency", \
			"Osc2FrequencyParam", \
			"Osc2FMFeedback", \
			"Osc3Volume", \
			"Osc3Waveshape", \
			"Osc3SyncFrequency", \
			"Osc3FrequencyParam", \
			"Osc3FMFeedback", \
			"AmpEnvDelayTime",  \
			"AmpEnvAttackTime", \
			"AmpEnvAttackCurve", \
			"AmpEnvHoldTime", \
			"AmpEnvDecayTime", \
			"AmpEnvDecayCurve", \
			"AmpEnvSustainLevel", \
			"AmpEnvReleaseTime", \
			"AmpEnvReleaseCurve", \
			"Env1DelayTime", \
			"Env1AttackTime", \
			"Env1AttackCurve", \
			"Env1HoldTime", \
			"Env1DecayTime", \
			"Env1DecayCurve", \
			"Env1SustainLevel", \
			"Env1ReleaseTime", \
			"Env1ReleaseCurve", \
			"Env2DelayTime", \
			"Env2AttackTime", \
			"Env2AttackCurve", \
			"Env2HoldTime", \
			"Env2DecayTime", \
			"Env2DecayCurve", \
			"Env2SustainLevel", \
			"Env2ReleaseTime", \
			"Env2ReleaseCurve", \
			"LFO1Waveshape", \
			"LFO1FrequencyParam", \
			"LFO2Waveshape", \
			"LFO2FrequencyParam", \
			"FilterQ", \
			"FilterSaturation", \
			"FilterFrequency", \
			"FMAmt1to2", \
			"FMAmt1to3", \
			"FMAmt2to1", \
			"FMAmt2to3", \
			"FMAmt3to1", \
			"FMAmt3to2", \
		}

		struct ModSourceInfo
		{
			ModSource mID;
			ModulationPolarity mPolarity;
			ModulationRate mRate;
		};
		static constexpr ModSourceInfo gModSourceInfo[] = {
			{
			ModSource::None, // arate, 01
			ModulationPolarity::Positive01,
			ModulationRate::Disabled,
			},
			{
			ModSource::AmpEnv, // arate, 01
			ModulationPolarity::Positive01,
			ModulationRate::ARate,
			},
			{
			ModSource::ModEnv1, // arate, 01
			ModulationPolarity::Positive01,
			ModulationRate::ARate,
			},
			{
			ModSource::ModEnv2, // arate, 01
			ModulationPolarity::Positive01,
			ModulationRate::ARate,
			},
			{
			ModSource::LFO1, // arate, N11
			ModulationPolarity::N11,
			ModulationRate::ARate,
			},
			{
			ModSource::LFO2, // arate, N11
			ModulationPolarity::N11,
			ModulationRate::ARate,
			},
			{
			ModSource::PitchBend, // krate, N11
			ModulationPolarity::N11,
			ModulationRate::KRate,
			},
			{
			ModSource::Velocity, // krate, 01
			ModulationPolarity::Positive01,
			ModulationRate::KRate,
			},
			{
			ModSource::NoteValue, // krate, 01
			ModulationPolarity::Positive01,
			ModulationRate::KRate,
			},
			{
			ModSource::RandomTrigger, // krate, 01
			ModulationPolarity::Positive01,
			ModulationRate::KRate,
			},
			{
			ModSource::SustainPedal, // krate, 01
			ModulationPolarity::Positive01,
			ModulationRate::KRate,
			},
			{
			ModSource::Macro1, // krate, 01
			ModulationPolarity::Positive01,
			ModulationRate::KRate,
			},
			{
			ModSource::Macro2, // krate, 01
			ModulationPolarity::Positive01,
			ModulationRate::KRate,
			},
			{
			ModSource::Macro3, // krate, 01
			ModulationPolarity::Positive01,
			ModulationRate::KRate,
			},
			{
			ModSource::Macro4, // krate, 01
			ModulationPolarity::Positive01,
			ModulationRate::KRate,
			},
		};

		struct ModulationSpec
		{
			BoolParam mEnabled;
			EnumParam<ModSource> mSource;
			EnumParam<ModDestination> mDestination;
			CurveParam mCurve;
			FloatN11Param mScale;

			ModulationSpec(real_t* paramCache, int baseParamID) :
				mEnabled(paramCache[baseParamID + (int)ModParamIndexOffsets::Enabled], false),
				mSource(paramCache[baseParamID + (int)ModParamIndexOffsets::Source], ModSource::Count, ModSource::None),
				mDestination(paramCache[baseParamID + (int)ModParamIndexOffsets::Destination], ModDestination::Count, ModDestination::None),
				mCurve(paramCache[baseParamID + (int)ModParamIndexOffsets::Curve], 0),
				mScale(paramCache[baseParamID + (int)ModParamIndexOffsets::Scale], 1)
			{}
		};

		struct ModDestinationInfo
		{
			ModDestination mID;
			ModulationPolarity mPolarity;
			ModulationRate mRate;
		};

		static constexpr ModDestinationInfo gModDestinationInfo[] = {
			{
				ModDestination::None, // krate, 01
				ModulationPolarity::Positive01,
				ModulationRate::Disabled,
			},
			{
				ModDestination::UnisonoDetune, // krate, 01
				ModulationPolarity::Positive01,
				ModulationRate::KRate,
			},
			{
				ModDestination::UnisonoStereoSpread, // krate, 01
				ModulationPolarity::Positive01,
				ModulationRate::KRate,
			},
			{
				ModDestination::OscillatorDetune, // krate, 01
				ModulationPolarity::Positive01,
				ModulationRate::KRate,
			},
			{
				ModDestination::OscillatorStereoSpread, // krate, 01
				ModulationPolarity::Positive01,
				ModulationRate::KRate,
			},{
				ModDestination::FMBrightness, // krate, 01
				ModulationPolarity::Positive01,
				ModulationRate::KRate,
			},{
				ModDestination::PortamentoTime, // krate, 01
				ModulationPolarity::Positive01,
				ModulationRate::KRate,
			},{
				ModDestination::PortamentoCurve, // krate, N11
				ModulationPolarity::Positive01,
				ModulationRate::KRate,
			},{
				ModDestination::Osc1Volume, // arate, 01
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::Osc1Waveshape, // arate, 01
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::Osc1SyncFrequency, // arate, 01
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::Osc1FrequencyParam, // arate, 01
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::Osc1FMFeedback, // arate, 01
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::Osc2Volume,
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::Osc2Waveshape,
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::Osc2SyncFrequency,
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::Osc2FrequencyParam,
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::Osc2FMFeedback,
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::Osc3Volume,
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::Osc3Waveshape,
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::Osc3SyncFrequency,
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::Osc3FrequencyParam,
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::Osc3FMFeedback,
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::AmpEnvDelayTime,
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::AmpEnvAttackTime,
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::AmpEnvAttackCurve,
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::AmpEnvHoldTime,
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::AmpEnvDecayTime,
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::AmpEnvDecayCurve,
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::AmpEnvSustainLevel,
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::AmpEnvReleaseTime,
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::AmpEnvReleaseCurve,
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::Env1DelayTime, // krate, 01
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::Env1AttackTime, // krate, 01
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::Env1AttackCurve, // krate, N11
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::Env1HoldTime, // krate, 01
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::Env1DecayTime, // krate, 01
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::Env1DecayCurve, // krate, N11
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::Env1SustainLevel, // krate, 01
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::Env1ReleaseTime, // krate, 01
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::Env1ReleaseCurve, // krate, N11
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::Env2DelayTime,
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::Env2AttackTime,
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::Env2AttackCurve,
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::Env2HoldTime,
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::Env2DecayTime,
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::Env2DecayCurve,
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::Env2SustainLevel,
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::Env2ReleaseTime,
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::Env2ReleaseCurve,
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::LFO1Waveshape,
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::LFO1FrequencyParam,
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::LFO2Waveshape, // krate, 01
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::LFO2FrequencyParam, // krate, 01
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::FilterQ, // krate, 01
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::FilterSaturation, // krate, 01
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::FilterFrequency, // krate, 01
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::FMAmt1to2, // arate, 01
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::FMAmt1to3, // arate, 01
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::FMAmt2to1, // arate, 01
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::FMAmt2to3, // arate, 01
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::FMAmt3to1, // arate, 01
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::FMAmt3to2, // arate, 01
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			}
		};
		// contiguous array holding multiple audio blocks. when we have 70 modulation destinations, 
// this means 1 single big allocation instead of 70 identically-sized small ones.
		struct ModMatrixBuffers
		{
		private:
			static constexpr size_t gMaxEndpointCount = std::max((size_t)ModDestination::Count, (size_t)ModSource::Count);
			const size_t gEndpointCount = false; // as in, the number of modulations. like ModSource::Count

			size_t mElementsAllocated = 0; // the backing buffer is allocated this big. may be bigger than blocks*blocksize.
			size_t mBlockSize = 0;
			real_t* mpBuffer = nullptr;

			real_t mpKRateValues[gMaxEndpointCount] = { 0 };
			bool mpARatePopulated[gMaxEndpointCount] = { false };
			bool mpKRatePopulated[gMaxEndpointCount] = { false };

		public:
			ModMatrixBuffers(size_t modulationEndpointCount) : gEndpointCount(modulationEndpointCount) {
			}

			~ModMatrixBuffers() {
				delete[] mpBuffer;
			}

			void Reset(size_t blockSize) {
				size_t desiredElements = gEndpointCount * blockSize;
				if (mElementsAllocated < desiredElements) {
					delete[] mpBuffer;
					mpBuffer = new real_t[desiredElements];
					mElementsAllocated = desiredElements;
				}
				mBlockSize = blockSize;
				memset(mpKRateValues, 0, std::size(mpKRateValues) * sizeof(mpKRateValues[0]));
				memset(mpARatePopulated, 0, std::size(mpARatePopulated) * sizeof(mpARatePopulated[0]));
				memset(mpKRatePopulated, 0, std::size(mpKRatePopulated) * sizeof(mpKRatePopulated[0]));
			}

			void SetARateValue(size_t id, size_t iSample, real_t val) {
				//if (id >= gEndpointCount) return; // error
				size_t i = id * mBlockSize + iSample;
				//if (i >= mBlockSize) return; // error
				mpARatePopulated[id] = 1;
				mpBuffer[i] = val;
				//return mSource.SetSourceARateValue(iblock, iSample, val);
			}

			void SetKRateValue(size_t id, real_t val)
			{
				mpKRateValues[id] = val;
				mpKRatePopulated[id] = 1;
			}

			real_t GetValue(size_t id, size_t sample) const
			{
				if (!mpBuffer || id >= gEndpointCount) return 0;
				if (mpARatePopulated[id]) {
					size_t i = id * mBlockSize + sample;
					if (i >= mElementsAllocated) return 0;
					return mpBuffer[i];
				}
				if (mpKRatePopulated[id]) return mpKRateValues[(size_t)id];
				return 0;
			}
		};


		struct ModMatrixNode
		{
		private:
			// krate values are a single scalar.
			// arate are block-sized buffers.
			// 
			// NOTE: because block sizes will vary constantly, these can only be used within the same processing block.
			// do not for example hold a-rate buffers to modulate the next block.
			// 
			// that means all A-Rate sources must be processed before you process the mod matrix.
			// then all A-Rate destinations must be processed after mod matrix.
			// that implies that no single "node" can both be an A-Rate source and an A-Rate destination.
			// example: you modulate waveshape with LFO (common for PWM)
			// 1. process LFO (all krate inputs, no problem. arate output)
			// 2. mod matrix pulls in LFO a-rate buffer, and generates the waveshape destination buffer applying curve and accumulating multiple mods to same dest.
			// 2. Oscillator arate Osc1Waveshape dest gets buffer from mod matrix

			ModMatrixBuffers mSource;
			ModMatrixBuffers mDest;

		public:
			ModMatrixNode() :
				mSource((size_t)ModSource::Count),
				mDest((size_t)ModDestination::Count)
			{}

			template<typename Tmodid>
			void SetSourceARateValue(Tmodid iblock, size_t iSample, real_t val) {
				return mSource.SetARateValue((size_t)iblock, iSample, val);
			}

			template<typename Tmodid>
			inline void SetSourceKRateValue(Tmodid id, real_t val)
			{
				mSource.SetKRateValue((size_t)id, val);
			}

			template<typename Tmodid>
			inline real_t GetSourceValue(Tmodid id, size_t sample) const
			{
				return mSource.GetValue((size_t)id, sample);
			}

			template<typename Tmodid>
			inline real_t GetDestinationValue(Tmodid id, size_t sample = 0) const
			{
				return mDest.GetValue((size_t)id, sample);
			}

			// call at the beginning of audio block processing to allocate & zero all buffers, preparing for source value population.
			void InitBlock(size_t nSamples)
			{
				mSource.Reset(nSamples);
				mDest.Reset(nSamples);
			}

			// caller passes in:
			// sourceValues_KRateOnly: a buffer indexed by (size_t)M7::ModSource. only krate values are used though.
			// sourceARateBuffers: a contiguous array of block-sized buffers. sequentially arranged indexed by (size_t)M7::ModSource.
			// the result will be placed 
			template<size_t NmodulationSpecs>
			void ProcessSample(ModulationSpec (&modSpecs)[NmodulationSpecs], size_t iSample)
			{
				float mCurveParam;
				CurveParam cp{ mCurveParam, 0 };
				// run mod specs
				for (ModulationSpec& spec : modSpecs) {
					if (!spec.mEnabled.GetBoolValue()) continue;
					const auto& sourceInfo = gModSourceInfo[(int)spec.mSource.GetEnumValue()];
					if (sourceInfo.mRate == ModulationRate::Disabled) continue;
					const auto& destInfo = gModDestinationInfo[(int)spec.mDestination.GetEnumValue()];
					if (destInfo.mRate == ModulationRate::Disabled) continue;

					real_t sourceVal = GetSourceValue(spec.mSource.GetEnumValue(), iSample);
					real_t destVal = GetDestinationValue(spec.mDestination.GetEnumValue(), iSample);
					cp.SetParamValue(spec.mCurve.GetN11Value());
					sourceVal = cp.ApplyToValue(sourceVal);
					sourceVal *= spec.mScale.GetN11Value();
					mDest.SetARateValue((size_t)spec.mDestination.GetEnumValue(), iSample, destVal + sourceVal);
				}
			}
		};
	} // namespace M7


} // namespace WaveSabreCore



