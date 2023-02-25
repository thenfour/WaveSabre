#pragma once

#include <WaveSabreCore/Maj7Basic.hpp>
//#include <memory>

namespace WaveSabreCore
{
	namespace M7
	{
		// contiguous array holding multiple audio blocks. when we have 70 modulation destinations, 
		// this means 1 single big allocation instead of 70 identically-sized small ones.
		struct ModulationBuffers
		{
			real_t* mpBuffer = nullptr;
			size_t mElementsAllocated = 0; // the backing buffer is allocated this big. may be bigger than blocks*blocksize.
			size_t mBlockSize = 0;
			size_t mBlockCount = 0; // how many blocks are we advertising.

			~ModulationBuffers() {
				delete[] mpBuffer;
			}
			void Reset(size_t count, size_t blockSize) {
				size_t desiredElements = count * blockSize;
				if (mElementsAllocated < desiredElements) {
					delete[] mpBuffer;
					mpBuffer = new real_t[desiredElements];
					mElementsAllocated = desiredElements;
				}
				mBlockSize = blockSize;
				mBlockCount = count;
				//mpKRateValues = GetARateBuffer(count);
				// zero
				for (size_t i = 0; i < mElementsAllocated; ++i) {
					mpBuffer[i] = 0;
				}
			}

			real_t* GetARateBuffer(size_t iblock) {
				return &mpBuffer[iblock * mBlockSize];
			}
		};


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
		enum class ModDestination : uint8_t
		{
			None,
			UnisonoDetune, // krate, 01
			OscillatorDetune, // krate, 01
			StereoSpread, // krate, 01
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
				mEnabled(paramCache[baseParamID + gParamIDEnabled], false),
				mSource(paramCache[baseParamID + gParamIDSource], ModSource::Count, ModSource::None),
				mDestination(paramCache[baseParamID + gParamIDDestination], ModDestination::Count, ModDestination::None),
				mCurve(paramCache[baseParamID + gParamIDCurve], 0),
				mScale(paramCache[baseParamID + gParamIDScale], 1)
			{}

			static constexpr int gParamIDEnabled = 0;
			static constexpr int gParamIDSource = 0;
			static constexpr int gParamIDDestination = 0;
			static constexpr int gParamIDCurve = 0;
			static constexpr int gParamIDScale = 0;
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
				ModulationRate::ARate,
			},{
				ModDestination::OscillatorDetune, // krate, 01
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::StereoSpread, // krate, 01
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::FMBrightness, // krate, 01
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::PortamentoTime, // krate, 01
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
			},{
				ModDestination::PortamentoCurve, // krate, N11
				ModulationPolarity::Positive01,
				ModulationRate::ARate,
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

		struct ModMatrixNode
		{
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

			// only krate indices are used; rest are never touched.
			real_t mKRateSourceValues[(size_t)M7::ModSource::Count] = { 0 };

			void SetKRateSourceValue(M7::ModSource id, real_t val)
			{
				// things like pitch bend, etc. set these once per block.
				//WSAssert(M7::gModSourceInfo[(size_t)id].mRate == M7::ModulationRate::KRate, "");
				mKRateSourceValues[(size_t)id] = val;
			}

			real_t GetKRateSourceValue(M7::ModSource id)
			{
				return mKRateSourceValues[(size_t)id];
			}


			real_t mKRateDestinationValues[(size_t)M7::ModDestination::Count] = { 0 };

			void SetKRateDestinationValue(M7::ModDestination id, real_t val)
			{
				// things like pitch bend, etc. set these once per block.
				//WSAssert(M7::gModDestinationInfo[(size_t)id].mRate == M7::ModulationRate::KRate, "");
				mKRateDestinationValues[(size_t)id] = val;
			}

			template<typename T> // because some enum values are calculated and this avoids constantly casting only to cast back.
			// i'm assuming this doesn't bloat code size
			inline real_t* GetARateDestinationBuffer(T id) {
				return mDestinationBuffers.GetARateBuffer((size_t)id);
			}

			template<typename T> // because some enum values are calculated and this avoids constantly casting only to cast back.
			// i'm assuming this doesn't bloat code size
			inline real_t GetKRateDestinationValue(T id)
			{
				return mKRateDestinationValues[(size_t)id];
			}

			ModulationBuffers mDestinationBuffers;

			// caller passes in
			// sourceValues_KRateOnly: a buffer indexed by (size_t)M7::ModSource. only krate values are used though.
			// sourceARateBuffers: a contiguous array of block-sized buffers. sequentially arranged indexed by (size_t)M7::ModSource.
			// the result will be placed 
			void ProcessBlock(ModulationBuffers& buffers)
			{
				mDestinationBuffers.Reset(buffers.mBlockCount, buffers.mBlockSize);
				//size_t bigBufferSize = blockSize * (size_t)ModDestination::Count;
				//destinationBuffers = std::make_unique<real_t[]>(bigBufferSize);
			}
		};
	} // namespace M7


} // namespace WaveSabreCore



