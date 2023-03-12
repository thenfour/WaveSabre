#pragma once

// embedding own samples is kinda a luxury. if needed we can save code size
#define MAJ7_INCLUDE_GSM_SUPPORT // comment this and save about 1kb of minified code.
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
//#include "Specimen.h"
//#include "Adultery.h"
#include "SamplePlayer.h"
#include "Maj7Oscillator.hpp"


namespace WaveSabreCore
{
	namespace M7
	{
		enum class SampleSource
		{
			GmDls,
			Embed,
			Count,
		};

		struct MutexHold
		{
			HANDLE mMutex;
			MutexHold(HANDLE hMutex) : mMutex(hMutex)
			{
				WaitForSingleObject(mMutex, INFINITE);
			}
			~MutexHold() {
				ReleaseMutex(mMutex);
			}
		};

		struct GmDlsSample : ISampleSource
		{
			int mSampleIndex = 0;
			int mSampleLength = 0;
			int mSampleLoopStart = 0;
			int mSampleLoopLength = 0;
			float* mSampleData = nullptr;

			GmDlsSample(int sampleIndex)
			{
				if (sampleIndex >= GmDls::NumSamples) return;
				mSampleIndex = sampleIndex;
				auto gmDls = GmDls::Load();

				// Seek to wave pool chunk's data
				auto ptr = gmDls + GmDls::WaveListOffset;

				// Walk wave pool entries
				for (int i = 0; i <= sampleIndex; i++)
				{
					// Walk wave list
					auto waveListTag = *((unsigned int*)ptr); // Should be 'LIST'
					ptr += 4;
					auto waveListSize = *((unsigned int*)ptr);
					ptr += 4;

					// Skip entries until we've reached the index we're looking for
					if (i != sampleIndex)
					{
						ptr += waveListSize;
						continue;
					}

					// Walk wave entry
					auto wave = ptr;
					auto waveTag = *((unsigned int*)wave); // Should be 'wave'
					wave += 4;

					// Read fmt chunk
					WaveSabreCore::Fmt fmt;
					memcpy(&fmt, wave, sizeof(fmt));
					wave += fmt.size + 8; // size field doesn't account for tag or length fields

					// Read wsmp chunk
					WaveSabreCore::Wsmp wsmp;
					memcpy(&wsmp, wave, sizeof(wsmp));
					wave += wsmp.size + 8; // size field doesn't account for tag or length fields

					// Read data chunk
					auto dataChunkTag = *((unsigned int*)wave); // Should be 'data'
					wave += 4;
					auto dataChunkSize = *((unsigned int*)wave);
					wave += 4;

					// Data format is assumed to be mono 16-bit signed PCM
					mSampleLength = dataChunkSize / 2;
					mSampleData = new float[mSampleLength];
					for (int j = 0; j < mSampleLength; j++)
					{
						auto sample = *((short*)wave);
						wave += 2;
						mSampleData[j] = (float)((double)sample / 32768.0);
					}

					if (wsmp.loopCount)
					{
						mSampleLoopStart = wsmp.loopStart;
						mSampleLoopLength = wsmp.loopLength;
					}
					else
					{
						mSampleLoopStart = 0;
						mSampleLoopLength = mSampleLength;
					}

					delete[] gmDls;
				}
			}

			~GmDlsSample() {
				delete[] mSampleData;
			}

			virtual const float* GetSampleData() const override { return mSampleData; }
			virtual int GetSampleLength() const override { return mSampleLength; }
			virtual int GetSampleLoopStart() const override { return mSampleLoopStart; }
			virtual int GetSampleLoopLength() const override { return mSampleLoopLength; }
			virtual int GetSampleRate() const override { return 44100; }

		}; // struct GmDlsSample

		////////////////////////////////////////////////////////////////////////////////////////////////
		struct SamplerDevice : ISoundSourceDevice
		{
			BoolParam mLegatoTrig;
			BoolParam mReverse;
			BoolParam mReleaseExitsLoop;
			EnumParam<LoopMode> mLoopMode;
			EnumParam<LoopBoundaryMode> mLoopSource;
			EnumParam<InterpolationMode> mInterpolationMode;
			EnumParam<SampleSource> mSampleSource;

			IntParam mGmDlsIndex;
			IntParam mBaseNote;

			Float01Param mSampleStart;
			Float01Param mLoopStart;
			Float01Param mLoopLength; // 0-1 ?

			int mSampleLoadSequence = 0; // just an ID to let the VST editor know when the sample data has changed

			ISampleSource* mSample = nullptr;
			float mSampleRateCorrectionFactor = 0;
			HANDLE mMutex;
			char mSamplePath[MAX_PATH] = { 0 };

			void Reset()
			{
				if (mSample) {
					delete mSample;
					mSample = nullptr;
				}
			}

			void Deserialize(Deserializer& ds)
			{
				auto token = MutexHold{ mMutex };
				int sampleSource = ds.ReadUByte();
				mSampleSource.SetIntValue(sampleSource);
				if (mSampleSource.GetEnumValue() != SampleSource::Embed) {
					return;
				}
				if (!ds.ReadUByte()) { // indicator whether there's a sample serialized or not.
					return;
				}
#ifdef MAJ7_INCLUDE_GSM_SUPPORT
				auto CompressedSize = ds.ReadUInt32();
				auto UncompressedSize = ds.ReadUInt32();

				WAVEFORMATEX wfx = { 0 };
				ds.ReadBuffer(&wfx, sizeof(wfx));
				auto waveFormatSize = sizeof(WAVEFORMATEX) + wfx.cbSize;
				// read the data after the WAVEFORMATEX
				auto pwfxComplete = new uint8_t[waveFormatSize];
				memcpy(pwfxComplete, &wfx, sizeof(wfx));
				ds.ReadBuffer(pwfxComplete + sizeof(wfx), wfx.cbSize);

				auto pCompressedData = new uint8_t[CompressedSize];
				ds.ReadBuffer(pCompressedData, CompressedSize);

				LoadSample((char*)pCompressedData, CompressedSize, UncompressedSize, (WAVEFORMATEX*)pwfxComplete, "");

				delete[] pCompressedData;
				delete[] pwfxComplete;
#else // MAJ7_INCLUDE_GSM_SUPPORT
				return;
#endif // MAJ7_INCLUDE_GSM_SUPPORT
			}

			void Serialize(Serializer& s) const
			{
				auto token = MutexHold{ mMutex };
				// params are already serialized. just serialize the non-param stuff (just sample data).
				// indicate sample source
				s.WriteUByte(mSampleSource.GetIntValue());
				if (mSampleSource.GetEnumValue() != SampleSource::Embed) {
					return;
				}
#ifdef MAJ7_INCLUDE_GSM_SUPPORT
				if (!mSample) {
					s.WriteUByte(0); // indicate there's no sample serialized.
					return;
				}
				s.WriteUByte(1); // indicate there's a sample serialized.

				auto pSample = static_cast<GsmSample*>(mSample);

				s.WriteUInt32(pSample->CompressedSize);
				s.WriteUInt32(pSample->UncompressedSize);

				auto waveFormatSize = sizeof(WAVEFORMATEX) + ((WAVEFORMATEX*)pSample->WaveFormatData)->cbSize;
				s.WriteBuffer((const uint8_t*)pSample->WaveFormatData, waveFormatSize);

				// Write compressed data
				s.WriteBuffer((const uint8_t*)pSample->CompressedData, pSample->CompressedSize);
#else // MAJ7_INCLUDE_GSM_SUPPORT
				s.WriteUByte(0); // indicate there's no sample serialized.
				return;
#endif // MAJ7_INCLUDE_GSM_SUPPORT
			}

			explicit SamplerDevice(float* paramCache, ModulationSpec* ampEnvModulation,
				ParamIndices baseParamID, ModSource ampEnvModSourceID, ModDestination modDestBaseID
			) :
				ISoundSourceDevice(paramCache, ampEnvModulation, baseParamID,
					(ParamIndices)(int(baseParamID) + int(SamplerParamIndexOffsets::Enabled)),
					(ParamIndices)(int(baseParamID) + int(SamplerParamIndexOffsets::Volume)),
					(ParamIndices)(int(baseParamID) + int(SamplerParamIndexOffsets::AuxMix)),
					(ParamIndices)(int(baseParamID) + int(SamplerParamIndexOffsets::TuneSemis)),
					(ParamIndices)(int(baseParamID) + int(SamplerParamIndexOffsets::TuneFine)),
					(ParamIndices)(int(baseParamID) + int(SamplerParamIndexOffsets::FreqParam)),
					(ParamIndices)(int(baseParamID) + int(SamplerParamIndexOffsets::FreqKT)),
					(ParamIndices)(int(baseParamID) + int(SamplerParamIndexOffsets::KeyRangeMin)),
					(ParamIndices)(int(baseParamID) + int(SamplerParamIndexOffsets::KeyRangeMax)),
					ampEnvModSourceID,
					modDestBaseID,
					(ModDestination)(int(modDestBaseID) + int(SamplerModParamIndexOffsets::Volume)),
					(ModDestination)(int(modDestBaseID) + int(SamplerModParamIndexOffsets::AuxMix)),
					(ModDestination)(int(modDestBaseID) + int(SamplerModParamIndexOffsets::HiddenVolume))
				),
				mLegatoTrig(paramCache[(int)baseParamID + (int)SamplerParamIndexOffsets::LegatoTrig]),
				mReverse(paramCache[(int)baseParamID + (int)SamplerParamIndexOffsets::Reverse]),
				mReleaseExitsLoop(paramCache[(int)baseParamID + (int)SamplerParamIndexOffsets::ReleaseExitsLoop]),
				mSampleStart(paramCache[(int)baseParamID + (int)SamplerParamIndexOffsets::SampleStart]),
				mLoopMode(paramCache[(int)baseParamID + (int)SamplerParamIndexOffsets::LoopMode], LoopMode::NumLoopModes),
				mLoopSource(paramCache[(int)baseParamID + (int)SamplerParamIndexOffsets::LoopSource], LoopBoundaryMode::NumLoopBoundaryModes),
				mInterpolationMode(paramCache[(int)baseParamID + (int)SamplerParamIndexOffsets::InterpolationType], InterpolationMode::NumInterpolationModes),
				mLoopStart(paramCache[(int)baseParamID + (int)SamplerParamIndexOffsets::LoopStart]),
				mLoopLength(paramCache[(int)baseParamID + (int)SamplerParamIndexOffsets::LoopLength]),
				mBaseNote(paramCache[(int)baseParamID + (int)SamplerParamIndexOffsets::BaseNote], 0, 127),
				mSampleSource(paramCache[(int)baseParamID + (int)SamplerParamIndexOffsets::SampleSource], SampleSource::Count),
				mGmDlsIndex(paramCache[(int)baseParamID + (int)SamplerParamIndexOffsets::GmDlsIndex], -1, WaveSabreCore::GmDls::NumSamples)
			{
				mMutex = ::CreateMutex(0, 0, 0);
			}

			virtual ~SamplerDevice()
			{
				CloseHandle(mMutex);
			}

			// called when loading chunk, or by VST
			void LoadSample(char* compressedDataPtr, int compressedSize, int uncompressedSize, WAVEFORMATEX* waveFormatPtr, const char *path)
			{
				auto token = MutexHold{ mMutex };
				if (mSample) {
					delete mSample;
					mSample = nullptr;
				}
#ifdef MAJ7_INCLUDE_GSM_SUPPORT
				mSampleSource.SetEnumValue(SampleSource::Embed);
				mSampleLoadSequence++;
				mSample = new GsmSample(compressedDataPtr, compressedSize, uncompressedSize, waveFormatPtr);
				strcpy(mSamplePath, path);
#endif // MAJ7_INCLUDE_GSM_SUPPORT
			}

			void LoadGmDlsSample(int sampleIndex)
			{
				auto token = MutexHold{ mMutex };
				if (mSample) {
					delete mSample;
					mSample = nullptr;
				}
				if (sampleIndex < 0 || sampleIndex >= GmDls::NumSamples) {
					return;
				}

				mSampleSource.SetEnumValue(SampleSource::GmDls);
				mGmDlsIndex.SetIntValue(sampleIndex);
				mSampleLoadSequence++;
				mSample = new GmDlsSample(sampleIndex);
			}

			virtual void BeginBlock(int samplesInBlock) override
			{
				WaitForSingleObject(mMutex, INFINITE);
				//auto token = MutexHold{ mMutex };
				// base note + original samplerate gives us a "native frequency" of the sample.
				// so let's say the sample is 22.05khz, base midi note 60 (261hz).
				// and you are requested to play it back at "1000hz" (or, midi note 88.7)
				// and imagine native sample rate is 44.1khz.
				// the base frequency is 261hz, and you're requesting to play 1000hz
				// so that's (play_hz / base_hz) = a rate of 3.83. plus factor the base samplerate, and it's
				// (base_sr / play_sr) = another rate of 0.5.
				// put together that's (play_hz / base_hz) * (base_sr / play_sr)
				// or, play_hz * (base_sr / (base_hz * play_sr))
				//               ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
				// i don't know play_hz yet at this stage, let's simplify & precalculate the rest
				if (!mSample) {
					return;
				}
				float base_hz = math::MIDINoteToFreq((float)mBaseNote.GetIntValue());
				mSampleRateCorrectionFactor = mSample->GetSampleRate() / (2 * base_hz * Helpers::CurrentSampleRateF);// WHY * 2? because it corresponds more naturally to other synth octave ranges.
			}

			virtual void EndBlock() override {
				ReleaseMutex(mMutex);
			}

		}; // struct SamplerDevice

		////////////////////////////////////////////////////////////////////////////////////////////////
		struct SamplerVoice : ISoundSourceDevice::Voice
		{
			SamplerDevice* mpSamplerDevice;
			SamplePlayer mSamplePlayer;
			bool mNoteIsOn = false;

			SamplerVoice(SamplerDevice* pDevice, ModMatrixNode& modMatrix, EnvelopeNode* pAmpEnv) :
				ISoundSourceDevice::Voice(pDevice, modMatrix, pAmpEnv),
				mpSamplerDevice(pDevice)
			{
			}

			void ConfigPlayer()
			{
				mSamplePlayer.SampleStart = mpSamplerDevice->mSampleStart.Get01Value();
				mSamplePlayer.LoopStart = mpSamplerDevice->mLoopStart.Get01Value();
				mSamplePlayer.LoopLength = mpSamplerDevice->mLoopLength.Get01Value();
				if (!mNoteIsOn && mpSamplerDevice->mReleaseExitsLoop.GetBoolValue()) {
					mSamplePlayer.LoopMode = LoopMode::Disabled;
				}
				else
				{
					mSamplePlayer.LoopMode = mpSamplerDevice->mLoopMode.GetEnumValue();
				}
				mSamplePlayer.LoopBoundaryMode = mpSamplerDevice->mLoopSource.GetEnumValue();
				mSamplePlayer.InterpolationMode = mpSamplerDevice->mInterpolationMode.GetEnumValue();
				mSamplePlayer.Reverse = mpSamplerDevice->mReverse.GetBoolValue();
			}

			virtual void NoteOn(bool legato) override
			{
				if (!mpSamplerDevice->mSample) {
					return;
				}
				if (!mpSamplerDevice->mEnabledParam.GetBoolValue()) {
					return;
				}
				if (legato && !mpSamplerDevice->mLegatoTrig.GetBoolValue()) {
					return;
				}

				auto token = MutexHold{ mpSamplerDevice->mMutex };

				mSamplePlayer.SampleData = mpSamplerDevice->mSample->GetSampleData();
				mSamplePlayer.SampleLength = mpSamplerDevice->mSample->GetSampleLength();
				mSamplePlayer.SampleLoopStart = mpSamplerDevice->mSample->GetSampleLoopStart(); // used for boundary mode from sample
				mSamplePlayer.SampleLoopLength = mpSamplerDevice->mSample->GetSampleLoopLength(); // used for boundary mode from sample
				mNoteIsOn = true;

				ConfigPlayer();
				mSamplePlayer.InitPos();
			}

			virtual void NoteOff() override
			{
				mNoteIsOn = false;
			}

			virtual void BeginBlock(real_t midiNote, float voiceShapeMod, float detuneFreqMul, float fmScale, int samplesInBlock) override
			{
				if (!mpSamplerDevice->mEnabledParam.GetBoolValue()) {
					return;
				}
				if (!mpSamplerDevice->mSample) {
					return;
				}

				ConfigPlayer();
				mSamplePlayer.RunPrep();

				float pitchFineMod = mModMatrix.GetDestinationValue((int)mpSrcDevice->mModDestBaseID + (int)SamplerModParamIndexOffsets::PitchFine, 0);
				float freqMod = mModMatrix.GetDestinationValue((int)mpSrcDevice->mModDestBaseID + (int)SamplerModParamIndexOffsets::FrequencyParam, 0);

				midiNote += mpSamplerDevice->mPitchSemisParam.GetIntValue() + mpSamplerDevice->mPitchFineParam.GetN11Value(pitchFineMod) * gSourcePitchFineRangeSemis;
				float noteHz = math::MIDINoteToFreq(midiNote);
				float freq = mpSamplerDevice->mFrequencyParam.GetFrequency(noteHz, freqMod);
				freq *= detuneFreqMul;

				float rate = freq * mpSamplerDevice->mSampleRateCorrectionFactor;

				mSamplePlayer.SetPlayRate(rate);
			}

			float ProcessSample(size_t iSample)
			{
				return math::clamp(mSamplePlayer.Next(), -1, 1); // clamp addresses craz glitch when changing samples.
			}

		}; // struct SamplerVoice

	} // namespace M7


} // namespace WaveSabreCore




