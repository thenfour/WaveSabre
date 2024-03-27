
#include <WaveSabreCore/Maj7Sampler.hpp>

namespace WaveSabreCore
{
	namespace M7
	{

	GmDlsSample::GmDlsSample(int sampleIndex)
			{
				if (sampleIndex >= gGmDlsSampleCount) return;
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
						//mSampleData[j] = (float)((double)sample / 32768.0);
						mSampleData[j] = math::Sample16To32Bit(sample);// (float)((double)sample / 32768.0);
						wave += 2;
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

	GmDlsSample::~GmDlsSample() {
				delete[] mSampleData;
			}


			void SamplerDevice::Reset()
			{
				if (mSample) {
					delete mSample;
					mSample = nullptr;
				}
			}

			void SamplerDevice::Deserialize(Deserializer& ds)
			{
#ifdef MAJ7_INCLUDE_GSM_SUPPORT
				auto token = mMutex.Enter();
				SampleSource sampleSource = (SampleSource)ds.ReadUByte();
				mParams.SetEnumValue(SamplerParamIndexOffsets::SampleSource, sampleSource);
				//mSampleSource.SetIntValue(sampleSource);
				if (sampleSource != SampleSource::Embed) {
					LoadGmDlsSample(mParams.GetIntValue(SamplerParamIndexOffsets::GmDlsIndex, gGmDlsIndexParamCfg));// mGmDlsIndex.GetIntValue());
					return;
				}
				if (!ds.ReadUByte()) { // indicator whether there's a sample serialized or not.
					return;
				}
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
				LoadGmDlsSample(mParams.GetIntValue(SamplerParamIndexOffsets::GmDlsIndex, gGmDlsIndexParamCfg));// mGmDlsIndex.GetIntValue());
#endif // MAJ7_INCLUDE_GSM_SUPPORT
			}

#ifdef MAJ7_INCLUDE_GSM_SUPPORT
			void SamplerDevice::Serialize(Serializer& s)
			{
				auto token = mMutex.Enter();
				// params are already serialized. just serialize the non-param stuff (just sample data).
				// indicate sample source
				auto sampleSrc = mParams.GetEnumValue<SampleSource>(SamplerParamIndexOffsets::SampleSource);
				s.WriteUByte((uint8_t)sampleSrc);// mSampleSource.GetIntValue());
				if (sampleSrc != SampleSource::Embed) {
					return;
				}
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
			}
#endif // MAJ7_INCLUDE_GSM_SUPPORT


			//SamplerDevice::SamplerDevice(float* paramCache, ModulationSpec* ampEnvModulation,
			//	ParamIndices baseParamID, ParamIndices ampEnvBaseParamID, ModSource ampEnvModSourceID, ModDestination modDestBaseID
			//) :
			SamplerDevice::SamplerDevice(float* paramCache, ModulationList modulations, const SourceInfo& srcInfo) :
				ISoundSourceDevice(paramCache, modulations[srcInfo.mModulationIndex], srcInfo.mParamBase, //ampEnvBaseParamID,
					srcInfo.mAmpModSource,
					srcInfo.mModDestBase
				)//,
				//mParams(paramCache, sourceInfo.mParamBase)
			{
			}

			// called when loading chunk, or by VST
			void SamplerDevice::LoadSample(char* compressedDataPtr, int compressedSize, int uncompressedSize, WAVEFORMATEX* waveFormatPtr, const char *path)
			{
				auto token = mMutex.Enter();
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

			void SamplerDevice::LoadGmDlsSample(int sampleIndex)
			{
				auto token = mMutex.Enter();
				if (mSample) {
					delete mSample;
					mSample = nullptr;
				}
				if (sampleIndex < 0 || sampleIndex >= gGmDlsSampleCount) {
					return;
				}

				mParams.SetEnumValue(SamplerParamIndexOffsets::SampleSource, SampleSource::GmDls);
				//mSampleSource.SetEnumValue(SampleSource::GmDls);
				mParams.SetIntValue(SamplerParamIndexOffsets::GmDlsIndex, gGmDlsIndexParamCfg, sampleIndex);
				//mGmDlsIndex.SetIntValue(sampleIndex);
				//mSampleLoadSequence++;
				mSample = new GmDlsSample(sampleIndex);
			}

			void SamplerDevice::BeginBlock()
			{
				mMutex.ManualEnter();

				mEnabledCached = mParams.GetBoolValue(SamplerParamIndexOffsets::Enabled);

				ISoundSourceDevice::BeginBlock();

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
				float base_hz = math::MIDINoteToFreq((float)mParams.GetIntValue(SamplerParamIndexOffsets::BaseNote, gKeyRangeCfg));
				mSampleRateCorrectionFactor = mSample->GetSampleRate() / (2 * base_hz * Helpers::CurrentSampleRateF);// WHY * 2? because it corresponds more naturally to other synth octave ranges.
			}

			void SamplerDevice::EndBlock() {
				mMutex.ManualLeave();
			}


			SamplerVoice::SamplerVoice(ModMatrixNode& modMatrix, SamplerDevice* pDevice, EnvelopeNode* pAmpEnv) :
				ISoundSourceDevice::Voice(pDevice, &modMatrix, pAmpEnv),
				mpSamplerDevice(pDevice)
			{
			}

			void SamplerVoice::ConfigPlayer()
			{
				mSamplePlayer.SampleStart = mpSamplerDevice->mParams.Get01Value(SamplerParamIndexOffsets::SampleStart, mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID + (int)SamplerModParamIndexOffsets::SampleStart));// mpSamplerDevice->mSampleStart.Get01Value();
				mSamplePlayer.LoopStart = mpSamplerDevice->mParams.Get01Value(SamplerParamIndexOffsets::LoopStart); //mpSamplerDevice->mLoopStart.Get01Value();
				mSamplePlayer.LoopLength = mpSamplerDevice->mParams.Get01Value(SamplerParamIndexOffsets::LoopLength); //mpSamplerDevice->mLoopLength.Get01Value();
				if (!mNoteIsOn && mpSamplerDevice->mParams.GetBoolValue(SamplerParamIndexOffsets::ReleaseExitsLoop)) {// mReleaseExitsLoop.GetBoolValue()) {
				//if (!mNoteIsOn && mpSamplerDevice->mReleaseExitsLoop.GetBoolValue()) {
					mSamplePlayer.LoopMode = LoopMode::Disabled;
				}
				else
				{
					//mSamplePlayer.LoopMode = mpSamplerDevice->mLoopMode.GetEnumValue();
					mSamplePlayer.LoopMode = mpSamplerDevice->mParams.GetEnumValue<LoopMode>(SamplerParamIndexOffsets::LoopMode);
				}
				mSamplePlayer.LoopBoundaryMode = mpSamplerDevice->mParams.GetEnumValue<LoopBoundaryMode>(SamplerParamIndexOffsets::LoopSource); //mpSamplerDevice->mLoopSource.GetEnumValue();
				mSamplePlayer.InterpolationMode = mpSamplerDevice->mParams.GetEnumValue<InterpolationMode>(SamplerParamIndexOffsets::InterpolationType); //mpSamplerDevice->mInterpolationMode.GetEnumValue();
				mSamplePlayer.Reverse = mpSamplerDevice->mParams.GetBoolValue(SamplerParamIndexOffsets::Reverse);//mpSamplerDevice->mReverse.GetBoolValue();
			}

			void SamplerVoice::NoteOn(bool legato) 
			{
				if (!mpSamplerDevice->mSample) {
					return;
				}
				if (!mpSrcDevice->mParams.GetBoolValue(SamplerParamIndexOffsets::Enabled)) {
					return;
				}
				if (legato && !mpSamplerDevice->mParams.GetBoolValue(SamplerParamIndexOffsets::LegatoTrig)) {
					return;
				}

				auto token = mpSamplerDevice->mMutex.Enter();

				mSamplePlayer.SampleData = mpSamplerDevice->mSample->GetSampleData();
				mSamplePlayer.SampleLength = mpSamplerDevice->mSample->GetSampleLength();
				mSamplePlayer.SampleLoopStart = mpSamplerDevice->mSample->GetSampleLoopStart(); // used for boundary mode from sample
				mSamplePlayer.SampleLoopLength = mpSamplerDevice->mSample->GetSampleLoopLength(); // used for boundary mode from sample
				mNoteIsOn = true;
				// this is a subtle but important thing. DO NOT calculate the delay step yet here. on the 1st note (see #35)
				// the mod matrix hasn't run yet and will cause the delay stage to always be skipped. so we have to:
				// 1. set step to 0 to ensure a sample is always processed (and with it, the mod matrix)
				// 2. make sure we calculate delaystep *after* handling delay complete
				mDelayStep = 0;
				mDelayPos01 = 0;
				ConfigPlayer();
			}

			void SamplerVoice::NoteOff()
			{
				mNoteIsOn = false;
			}

			void SamplerVoice::BeginBlock(/*real_t midiNote, float detuneFreqMul, float fmScale,*/)
			{
				if (!mpSamplerDevice->mEnabledCached)// >mEnabledParam.mCachedVal)
					return;
				if (!mpSamplerDevice->mSample) {
					return;
				}

				ConfigPlayer();
				mSamplePlayer.RunPrep();

			}

			float SamplerVoice::ProcessSample(real_t midiNote, float detuneFreqMul, float fmScale, float ampEnvLin)
			{
				if (!mpSrcDevice->mParams.GetBoolValue(SamplerParamIndexOffsets::Enabled))
					return 0;

				// see above for subtle info about handling modulated delay
				if (mDelayPos01 < 1) {
					mDelayPos01 += mDelayStep;
					if (mDelayPos01 >= 1) {
						mSamplePlayer.InitPos(); // play.
					}
				}
				auto ms = mpSamplerDevice->mParams.GetPowCurvedValue(SamplerParamIndexOffsets::Delay, gEnvTimeCfg, mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID + (int)SamplerModParamIndexOffsets::Delay));
				mDelayStep = math::CalculateInc01PerSampleForMS(ms);

				float pitchFineMod = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID + (int)SamplerModParamIndexOffsets::PitchFine);
				float freqMod = mpModMatrix->GetDestinationValue((int)mpSrcDevice->mModDestBaseID + (int)SamplerModParamIndexOffsets::FrequencyParam);

				int pitchSemis = mpSamplerDevice->mParams.GetIntValue(SamplerParamIndexOffsets::TuneSemis, gSourcePitchSemisRange);
				float pitchFine = mpSamplerDevice->mParams.GetN11Value(SamplerParamIndexOffsets::TuneFine, pitchFineMod) * gSourcePitchFineRangeSemis;
				midiNote += pitchSemis + pitchFine;

				float noteHz = math::MIDINoteToFreq(midiNote);
				float freq = mpSamplerDevice->mParams.GetFrequency(SamplerParamIndexOffsets::FreqParam, SamplerParamIndexOffsets::FreqKT, gSourceFreqConfig, noteHz, freqMod);
				freq *= detuneFreqMul;

				float rate = freq * mpSamplerDevice->mSampleRateCorrectionFactor;

				mSamplePlayer.SetPlayRate(rate);

				return math::clampN11(mSamplePlayer.Next() * ampEnvLin); // clamp addresses craz glitch when changing samples.
			}


	} // namespace M7


} // namespace WaveSabreCore




