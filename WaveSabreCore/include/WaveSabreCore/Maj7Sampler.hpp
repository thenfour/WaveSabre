#pragma once

// embedding own samples is kinda a luxury. if needed we can save code size
//#define MAJ7_INCLUDE_GSM_SUPPORT // comment this and save about 1kb of minified code.
#include <Windows.h>
// correction for windows.h macros.
#undef min
#undef max

#include <WaveSabreCore/Maj7Basic.hpp>
#include <WaveSabreCore/Maj7ModMatrix.hpp>
#include <WaveSabreCore/GmDls.h>
#ifdef MAJ7_INCLUDE_GSM_SUPPORT
#include "GsmSample.h"
#endif // MAJ7_INCLUDE_GSM_SUPPORT
#include "SamplePlayer.h"
#include "Maj7Oscillator.hpp"


namespace WaveSabreCore
{
	namespace M7
	{
		enum class SampleSource : uint8_t
		{
			GmDls,
			Embed,
			Count,
		};

		struct GmDlsSample : ISampleSource
		{
			int mSampleIndex = 0;
			int mSampleLength = 0;
			int mSampleLoopStart = 0;
			int mSampleLoopLength = 0;
			float* mSampleData = nullptr;

			GmDlsSample(int sampleIndex);
			~GmDlsSample();
			virtual const float* GetSampleData() const override { return mSampleData; }
			virtual int GetSampleLength() const override { return mSampleLength; }
			virtual int GetSampleLoopStart() const override { return mSampleLoopStart; }
			virtual int GetSampleLoopLength() const override { return mSampleLoopLength; }
			virtual int GetSampleRate() const override { return 44100; }

		}; // struct GmDlsSample

		////////////////////////////////////////////////////////////////////////////////////////////////
		struct SamplerDevice : ISoundSourceDevice
		{
			ParamAccessor mParams;

			bool mEnabledCached;

			ISampleSource* mSample = nullptr;
			float mSampleRateCorrectionFactor = 0;
			WaveSabreCore::CriticalSection mMutex;
			char mSamplePath[MAX_PATH] = { 0 };

			void Reset();

			void Deserialize(Deserializer& ds);

			void Serialize(Serializer& s);

			explicit SamplerDevice(float* paramCache, ModulationSpec* ampEnvModulation,
				ParamIndices baseParamID, ParamIndices ampEnvBaseParamID, ModSource ampEnvModSourceID, ModDestination modDestBaseID
			);
			//virtual ~SamplerDevice();
			// called when loading chunk, or by VST
			void LoadSample(char* compressedDataPtr, int compressedSize, int uncompressedSize, WAVEFORMATEX* waveFormatPtr, const char* path);
			void LoadGmDlsSample(int sampleIndex);
			virtual void BeginBlock() override;

			virtual void EndBlock() override;

			virtual bool IsEnabled() const override {
				return mParams.GetBoolValue(OscParamIndexOffsets::Enabled);
			}
			virtual bool MatchesKeyRange(int midiNote) const override {
				if (mParams.GetIntValue(SamplerParamIndexOffsets::KeyRangeMin, gKeyRangeCfg) > midiNote)
					return false;
				if (mParams.GetIntValue(SamplerParamIndexOffsets::KeyRangeMax, gKeyRangeCfg) < midiNote)
					return false;
				return true;
			}

			virtual float GetAuxPan() const override
			{
				return mParams.GetN11Value(SamplerParamIndexOffsets::AuxMix, 0);
			}

			virtual float GetLinearVolume(float mod) const override
			{
				return mParams.GetLinearVolume(SamplerParamIndexOffsets::Volume, gUnityVolumeCfg, mod);
			}


		}; // struct SamplerDevice

		////////////////////////////////////////////////////////////////////////////////////////////////
		struct SamplerVoice : ISoundSourceDevice::Voice
		{
			SamplerDevice* mpSamplerDevice;
			SamplePlayer mSamplePlayer;
			bool mNoteIsOn = false;

			SamplerVoice(SamplerDevice* pDevice, ModMatrixNode& modMatrix, EnvelopeNode* pAmpEnv);
			void ConfigPlayer();

			virtual void NoteOn(bool legato) override;
			virtual void NoteOff() override;

			virtual void BeginBlock() override;

			virtual float GetLastSample() const override { return 0; } // not used

			float ProcessSample(real_t midiNote, float detuneFreqMul, float fmScale);

		}; // struct SamplerVoice

	} // namespace M7


} // namespace WaveSabreCore




