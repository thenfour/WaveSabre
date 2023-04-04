
#pragma once

#define _7ZIP_ST // single-threaded LZMA lib

#include "LzmaEnc.h"

#include <WaveSabreVstLib.h>
#include "Serialization.hpp"


extern int GetMinifiedChunk(M7::Maj7* p, void** data);


inline int Maj7SetVstChunk(M7::Maj7* p, void* data, int byteSize)
{
	if (!byteSize) return byteSize;
	const char* pstr = (const char*)data;
	if (strnlen(pstr, byteSize - 1) >= (size_t)byteSize) return byteSize;

	using vstn = const char[kVstMaxParamStrLen];
	static constexpr vstn paramNames[(int)M7::ParamIndices::NumParams] = MAJ7_PARAM_VST_NAMES;

	clarinoid::MemoryStream memStream{ (const uint8_t*)data, (size_t)byteSize };
	clarinoid::BufferedStream buffering{ memStream };
	clarinoid::TextStream textStream{ buffering };
	clarinoid::JsonVariantReader doc{ textStream };

	// @ root there is exactly 1 KV object.
	auto maj7Obj = doc.GetNextObjectItem();
	if (maj7Obj.IsEOF()) {
		return 0; // empty doc?
	}
	if (maj7Obj.mParseResult.IsFailure()) {
		return 0;//return ch.mParseResult;
	}
	if (maj7Obj.mKeyName != "Maj7") {
		return 0;
	}

	bool tagSatisfied = false;
	bool versionSatisfied = false;
	while (true) {
		auto ch = maj7Obj.GetNextObjectItem();
		if (ch.mKeyName == "Tag") {
			if (ch.mNumericValue.Get<DWORD>() != M7::Maj7::gChunkTag) {
				return 0; //invalid tag
			}
			tagSatisfied = true;
		}
		if (ch.mKeyName == "Version") {
			if (ch.mNumericValue.Get<uint8_t>() != M7::Maj7::gChunkVersion) {
				return 0; // unknown version
			}
			versionSatisfied = true;
		}
		if (ch.IsEOF())
			break;
	}
	if (!tagSatisfied || !versionSatisfied)
		return 0;

	auto paramsObj = doc.GetNextObjectItem(); // assumes these are in this order. ya probably should not.
	if (paramsObj.IsEOF()) {
		return 0;
	}
	if (paramsObj.mParseResult.IsFailure()) {
		return 0;
	}
	if (paramsObj.mKeyName != "params") {
		return 0;
	}

	std::map<std::string, std::pair<bool, size_t>> paramMap; // maps string name to whether it's been set + which param index is it.
	for (size_t i = 0; i < (int)M7::ParamIndices::NumParams; ++i) {
		paramMap[paramNames[i]] = std::make_pair(false, i);
	}

	while (true) {
		auto ch = paramsObj.GetNextObjectItem();
		if (ch.IsEOF())
			break;
		if (ch.mParseResult.IsFailure()) {
			return 0;
		}
		auto it = paramMap.find(ch.mKeyName);
		if (it == paramMap.end()) {
			continue;// return 0; // unknown param name. just ignore and allow loading.
		}
		if (it->second.first) {
			continue;// return 0; // already set. is this a duplicate? just ignore.
		}

		p->SetParam((VstInt32)it->second.second, ch.mNumericValue.Get<float>());
		it->second.first = true;
	}

	auto samplersArr = doc.GetNextObjectItem(); // assumes these are in this order. ya probably should not.
	if (samplersArr.IsEOF()) {
		return 0;
	}
	if (samplersArr.mParseResult.IsFailure()) {
		return 0;
	}
	if (samplersArr.mKeyName != "samplers") {
		return 0;
	}

	for (auto& s : p->mSamplerDevices) {
		auto b64 = samplersArr.GetNextArrayItem();
		if (b64.IsEOF()) break;
		if (b64.mParseResult.IsFailure()) break;
		auto data = clarinoid::base64_decode(b64.mStringValue);
		M7::Deserializer ds{ data.data() };
		s.Deserialize(ds);
	}

	return byteSize;
}




class Maj7Vst : public WaveSabreVstLib::VstPlug
{
public:
	Maj7Vst(audioMasterCallback audioMaster);
	
	virtual void getParameterName(VstInt32 index, char *text);

	virtual bool getEffectName(char *name);
	virtual bool getProductString(char *text);

	virtual VstInt32 setChunk(void* data, VstInt32 byteSize, bool isPreset) override
	{
		return Maj7SetVstChunk(GetMaj7(), data, byteSize);
	}

	virtual VstInt32 getChunk(void** data, bool isPreset) override
	{
		// the default VST behavior is to output/set RAW values, not diff'd values.
		return getChunk2(data, isPreset, false);
	}
	VstInt32 getChunk2(void** data, bool isPreset, bool diff)
	{
		using vstn = const char[kVstMaxParamStrLen];
		static constexpr vstn paramNames[(int)M7::ParamIndices::NumParams] = MAJ7_PARAM_VST_NAMES;

		clarinoid::MemoryStream memStream;
		clarinoid::BufferedStream buffering{ memStream };
		clarinoid::TextStream textStream{ buffering };
		textStream.mMinified = false;
		clarinoid::JsonVariantWriter doc{ textStream };

		doc.BeginObject();

		auto maj7Element = doc.Object_MakeKey("Maj7");
		maj7Element.BeginObject();
		maj7Element.Object_MakeKey("Tag").WriteNumberValue(M7::Maj7::gChunkTag);
		maj7Element.Object_MakeKey("Version").WriteNumberValue(M7::Maj7::gChunkVersion);
		maj7Element.Object_MakeKey("Format").WriteStringValue(diff ? "DIFF values" : "Absolute values");

		auto paramsElement = doc.Object_MakeKey("params");
		paramsElement.BeginObject();

		for (size_t i = 0; i < (int)M7::ParamIndices::NumParams; ++i)
		{
			if (diff) {
				float def = GetMaj7()->mDefaultParamCache[i];
				paramsElement.Object_MakeKey(paramNames[i]).WriteNumberValue(getParameter((VstInt32)i) - def);
			}
			else {
				paramsElement.Object_MakeKey(paramNames[i]).WriteNumberValue(getParameter((VstInt32)i));
			}
		}

		auto samplersArray = doc.Object_MakeKey("samplers");
		samplersArray.BeginArray();
		for (auto& sampler : GetMaj7()->mSamplerDevices)
		{
			M7::Serializer samplerSerializer;
			sampler.Serialize(samplerSerializer);
			auto b64 = clarinoid::base64_encode(samplerSerializer.mBuffer, (unsigned int)samplerSerializer.mSize);
			samplersArray.Array_MakeValue().WriteStringValue(b64);
		}

		doc.EnsureClosed();
		buffering.flushWrite();

		size_t size = memStream.mBuffer.size() + 1;
		char* out = new char[size];
		strncpy(out, (const char *)memStream.mBuffer.data(), memStream.mBuffer.size());
		out[memStream.mBuffer.size()] = 0; // null term
		*data = out;
		return (VstInt32)size;
	}




	WaveSabreCore::M7::Maj7 *GetMaj7() const;
};


namespace WaveSabreCore
{
	namespace M7
	{
		static inline void GenerateDefaults(ModulationSpec* m)
		{
			m->mParams.SetBoolValue(ModParamIndexOffsets::Enabled, false);
			//m->mEnabled.SetBoolValue(false);
			m->mParams.SetEnumValue(ModParamIndexOffsets::Source, ModSource::None);
			//m->mSource.SetEnumValue(ModSource::None);

			for (size_t i = 0; i < std::size(m->mDestinations); ++i) {
				m->mParams.SetEnumValue((int)ModParamIndexOffsets::Destination1 + i, ModDestination::None);
			}

			//for (auto& d : m->mDestinations) d.SetEnumValue(ModDestination::None);
			
			for (size_t i = 0; i < std::size(m->mScales); ++i) {
				m->mParams.SetN11Value((int)ModParamIndexOffsets::Scale1 + i, 0.75f);
			}

			//for (auto& d : m->mScales) d.SetN11Value(0.75f);
			m->mCurve.SetN11Value(0);
			m->mParams.SetBoolValue(ModParamIndexOffsets::AuxEnabled, false);
			//m->mAuxEnabled.SetBoolValue(false);
			
			m->mParams.SetEnumValue(ModParamIndexOffsets::AuxSource, ModSource::None);
			//m->mAuxSource.SetEnumValue(ModSource::None);
			m->mAuxCurve.SetN11Value(0);
			m->mParams.Set01Val(ModParamIndexOffsets::AuxAttenuation, 1);
			//m->mAuxAttenuation.SetParamValue(1);

			m->mParams.SetEnumValue(ModParamIndexOffsets::ValueMapping, ModValueMapping::NoMapping);
			m->mParams.SetEnumValue(ModParamIndexOffsets::AuxValueMapping, ModValueMapping::NoMapping);
		}

		static inline void GenerateDefaults_Env(EnvelopeNode* n)
		{
			n->mParams.Set01Val(M7::EnvParamIndexOffsets::DelayTime, 0);
			n->mParams.Set01Val(M7::EnvParamIndexOffsets::AttackTime, 0.05f);
			n->mParams.SetN11Value(M7::EnvParamIndexOffsets::AttackCurve, 0.5f);
			n->mParams.Set01Val(M7::EnvParamIndexOffsets::HoldTime, 0);
			n->mParams.Set01Val(M7::EnvParamIndexOffsets::DecayTime, 0.5f);
			n->mParams.SetN11Value(M7::EnvParamIndexOffsets::DecayCurve, -0.5f);
			n->mParams.Set01Val(M7::EnvParamIndexOffsets::SustainLevel, 0.4f);
			n->mParams.Set01Val(M7::EnvParamIndexOffsets::ReleaseTime, 0.2f);
			n->mParams.SetN11Value(M7::EnvParamIndexOffsets::ReleaseCurve, -0.5f);
			n->mParams.SetBoolValue(M7::EnvParamIndexOffsets::LegatoRestart, true);

			//n->mDelayTime.SetParamValue(0);
			//n->mAttackTime.SetParamValue(0.05f);
			//n->mAttackCurve.SetN11Value(0.5f);
			//n->mHoldTime.SetParamValue(0);
			//n->mDecayTime.SetParamValue(0.5f);
			//n->mDecayCurve.SetN11Value(-0.5f);
			//n->mSustainLevel.SetParamValue(0.4f);
			//n->mReleaseTime.SetParamValue(0.2f);
			//n->mReleaseCurve.SetN11Value(-0.5f);
			//n->mLegatoRestart.SetBoolValue(true); // because for polyphonic, holding pedal and playing a note already playing is legato and should retrig. make this opt-out.
		}

		static inline void GenerateDefaults(AuxDevice* p)
		{
			p->mParams.SetBoolValue(AuxParamIndexOffsets::Enabled, false);
			//p->mEnabledParam.SetBoolValue(false);
			p->mParams.SetEnumValue(AuxParamIndexOffsets::Link, p->mLinkToSelf);
			//p->mLink.SetEnumValue(p->mLinkToSelf);// (paramCache_Offset[(int)AuxParamIndexOffsets::Link], AuxLink::Count),
			p->mParams.SetEnumValue(AuxParamIndexOffsets::Type, AuxEffectType::None);
			//);
			//p->mEffectType.SetEnumValue(AuxEffectType::None);// (paramCache_Offset[(int)AuxParamIndexOffsets::Type], AuxEffectType::Count),
		}

		static inline void GenerateDefaults_Source(ISoundSourceDevice* p)
		{
			p->mEnabledParam.SetBoolValue(false);
			p->mVolumeParam.SetDecibels(0);
			p->mAuxPanParam.SetN11Value(0);
			p->mFrequencyParam.mValue.SetParamValue(M7::gFreqParamKTUnity);//(paramCache[(int)freqParamID], paramCache[(int)freqKTParamID], gSourceFrequencyCenterHz, gSourceFrequencyScale, 0.4f, 1.0f),
			p->mFrequencyParam.mKTValue.SetParamValue(0);
			p->mPitchSemisParam.SetIntValue(0);// (paramCache[(int)tuneSemisParamID], -gSourcePitchSemisRange, gSourcePitchSemisRange, 0),
			p->mPitchFineParam.SetN11Value(0);// (paramCache[(int)tuneFineParamID], 0),
			p->mKeyRangeMin.SetIntValue(0);// (paramCache[(int)keyRangeMinParamID], 0, 127, 0),
			p->mKeyRangeMax.SetIntValue(127);// (paramCache[(int)keyRangeMaxParamID], 0, 127, 127)
		}

		static inline void GenerateDefaults_LFO(OscillatorDevice* p)
		{
			GenerateDefaults_Source(static_cast<ISoundSourceDevice*>(p));
			p->mWaveform.SetEnumValue(OscillatorWaveform::TriTrunc);
			p->mWaveshape.SetParamValue(0.5f);
			p->mPhaseOffset.SetN11Value(0);
			p->mSyncFrequency.mValue.SetParamValue(M7::gFreqParamKTUnity);
			p->mFrequencyMul.SetRangedValue(1);
			p->mIntention = OscillatorIntention::LFO;
			p->mEnabledParam.SetBoolValue(true);
			p->mFrequencyParam.mValue.SetParamValue(0.6f);
			p->mFrequencyParam.mKTValue.SetParamValue(0);
			p->mSyncFrequency.mKTValue.SetParamValue(0);
			p->mLPFFrequency.mValue.SetParamValue(0.5f);
		}

		static inline void GenerateDefaults_Audio(OscillatorDevice* p)
		{
			GenerateDefaults_Source(static_cast<ISoundSourceDevice*>(p));
			p->mWaveform.SetEnumValue(OscillatorWaveform::SineClip);
			p->mWaveshape.SetParamValue(0.5f);
			p->mPhaseOffset.SetN11Value(0);
			p->mSyncFrequency.mValue.SetParamValue(M7::gFreqParamKTUnity);
			p->mFrequencyMul.SetRangedValue(1);
			p->mIntention = OscillatorIntention::Audio;
			p->mEnabledParam.SetBoolValue(false);
			p->mFrequencyParam.mKTValue.SetParamValue(1);
			p->mSyncFrequency.mKTValue.SetParamValue(1);
		}

		static inline void GenerateDefaults(SamplerDevice* p)
		{
			auto token = p->mMutex.Enter();
			p->Reset();
			GenerateDefaults_Source(static_cast<ISoundSourceDevice*>(p));
			p->mParams.SetBoolValue(SamplerParamIndexOffsets::LegatoTrig, true);
			//p->mLegatoTrig.SetBoolValue(true);
			p->mParams.SetBoolValue(SamplerParamIndexOffsets::Reverse, false);
			//p->mReverse.SetBoolValue(false);
			//p->mReleaseExitsLoop.SetBoolValue(true);
			p->mParams.SetBoolValue(SamplerParamIndexOffsets::ReleaseExitsLoop, true);
			p->mParams.Set01Val(SamplerParamIndexOffsets::SampleStart, 0);
			//p->mSampleStart.SetParamValue(0);
			//p->mFrequencyParam.mKTValue.SetParamValue(1);
			p->mParams.Set01Val(SamplerParamIndexOffsets::FreqKT, 1);
			p->mParams.SetEnumValue(SamplerParamIndexOffsets::LoopMode, LoopMode::Repeat);
			//p->mLoopMode.SetEnumValue(LoopMode::Repeat);
			p->mParams.SetEnumValue(SamplerParamIndexOffsets::LoopSource, LoopBoundaryMode::FromSample);
			//p->mLoopSource.SetEnumValue(LoopBoundaryMode::FromSample);
			p->mParams.SetEnumValue(SamplerParamIndexOffsets::InterpolationType, InterpolationMode::Linear);
			//p->mInterpolationMode.SetEnumValue(InterpolationMode::Linear);

			p->mParams.Set01Val(SamplerParamIndexOffsets::LoopStart, 0);
			p->mParams.Set01Val(SamplerParamIndexOffsets::LoopLength, 1);
			p->mParams.SetIntValue(SamplerParamIndexOffsets::BaseNote, M7::gKeyRangeCfg, 60);

			//p->mLoopStart.SetParamValue(0);
			//p->mLoopLength.SetParamValue(1);// (paramCache[(int)baseParamID + (int)SamplerParamIndexOffsets::LoopLength], 1),
			//p->mBaseNote.SetIntValue(60);// (paramCache[(int)baseParamID + (int)SamplerParamIndexOffsets::BaseNote], 0, 127, 60),
			p->mParams.SetEnumValue(SamplerParamIndexOffsets::SampleSource, SampleSource::Embed);
			//p->mSampleSource.SetEnumValue(SampleSource::Embed);
			p->mParams.SetIntValue(SamplerParamIndexOffsets::GmDlsIndex, gGmDlsIndexParamCfg, -1);
			//p->mGmDlsIndex.SetIntValue(-1);
		}

		//enum class MainParamIndices : uint8_t
		static inline void GenerateMasterParamDefaults(Maj7* p)
		{
			p->mParams.SetDecibels(ParamIndices::MasterVolume, gMasterVolumeCfg, -6);
			//p->mMasterVolume.SetDecibels(-6);//MasterVolume,
			p->mParams.SetIntValue(ParamIndices::Unisono, gUnisonoVoiceCfg, 1);//p->mUnisonoVoicesParam.SetIntValue(1);
			p->mParams.SetEnumValue(ParamIndices::VoicingMode, VoiceMode::Polyphonic);//p->mVoicingModeParam.SetEnumValue(VoiceMode::Polyphonic);
			// OscillatorDetune, = 0
			// UnisonoDetune, = 0
			// OscillatorSpread, = 0
			// UnisonoStereoSpread, = 0
			// OscillatorShapeSpread, = 0
			// UnisonoShapeSpread, = 0
			p->mParams.Set01Val(ParamIndices::FMBrightness, 0.5f);//p->mFMBrightness.SetParamValue(0.5f);// FMBrightness,
			p->mParams.SetEnumValue(ParamIndices::AuxRouting, AuxRoute::TwoTwo);//p->mAuxRoutingParam.SetEnumValue(AuxRoute::TwoTwo);// AuxRouting,
			p->mParams.SetN11Value(ParamIndices::AuxWidth, 1);//p->mAuxWidth.SetN11Value(1);// AuxWidth
			p->mParams.Set01Val(ParamIndices::PortamentoTime, 0.3f);//p->mMaj7Voice[0]->mPortamento.mTime.SetParamValue(0.3f);// PortamentoTime,
			p->mParams.SetN11Value(ParamIndices::PortamentoCurve, 0);////p->mMaj7Voice[0]->mPortamento.mCurve.SetN11Value(0);
			p->mParams.SetIntValue(ParamIndices::PitchBendRange, gPitchBendCfg, 2);//p->mPitchBendRange.SetIntValue(2);
			p->mParams.SetIntValue(ParamIndices::MaxVoices, gMaxVoicesCfg, 24);//p->mMaxVoicesParam.SetIntValue(24);
			// macros all 0
			// fm matrix all 0
		}

		// here we generate the defaults for all params, without exceptional things. so all "nodes" / modules/whatever have identical baseline params.
		// this is to generate :
		//extern const float gDefaultSamplerParams[(int)SamplerParamIndexOffsets::Count];
		//extern const float gDefaultModSpecParams[(int)ModParamIndexOffsets::Count];
		//extern const float gDefaultLFOParams[(int)LFOParamIndexOffsets::Count];
		//extern const float gDefaultEnvelopeParams[(int)EnvParamIndexOffsets::Count];
		//extern const float gDefaultOscillatorParams[(int)OscParamIndexOffsets::Count];
		//extern const float gDefaultAuxParams[(int)AuxParamIndexOffsets::Count];
		// yea it's not strictly necessary to do this to *all* e.g. oscillators (only 1 is needed to generate the struct), but at least this is more predictable.
		static inline void GenerateDefaults(Maj7* p)
		{
			// set baseline 0
			for (int i = 0; i < (int)ParamIndices::NumParams; ++i) {
				p->mParamCache[i] = 0;
			}

			// generate defaults for the major nodes
			//GenerateDefaults_LFO(&p->mLFO1Device);
			//GenerateDefaults_LFO(&p->mLFO2Device);
			for (auto& m : p->mLFOs) {
				GenerateDefaults_LFO(&m.mDevice);
			}

			for (auto& m : p->mOscillatorDevices) {
				GenerateDefaults_Audio(&m);
			}
			for (auto& m : p->mSamplerDevices) {
				GenerateDefaults(&m);
			}
			for (auto& m : p->mModulations) {
				GenerateDefaults(&m);
			}
			for (auto& m : p->mAuxDevices) {
				GenerateDefaults(&m);
			}
			for (auto p : p->mMaj7Voice[0]->mpAllModEnvelopes)
			{
				GenerateDefaults_Env(p);
			}
			for (auto p : p->mMaj7Voice[0]->mSourceVoices)
			{
				GenerateDefaults_Env(p->mpAmpEnv);
			}

			GenerateMasterParamDefaults(p);

			// Apply dynamic state
			p->SetVoiceMode(p->mParams.GetEnumValue<VoiceMode>(ParamIndices::VoicingMode));// mVoicingModeParam.GetEnumValue());
			p->SetUnisonoVoices(p->mParams.GetIntValue(ParamIndices::Unisono, gUnisonoVoiceCfg));// p->mUnisonoVoicesParam.GetIntValue());
			// NOTE: samplers will always be empty here
		}


		static inline bool IsEnvelopeInUse(Maj7* p, ModSource modSource)
		{
			// an envelope is in use if it's used as a modulation source, AND the modulation is enabled,
			// AND the modulation destination is not some disabled thing. The VAST majority of these "disabled destinations" would just be the hidden volume / pre-fm volume source param,
			// so to simplify the logic just check for that.
			for (auto& m : p->mModulations) {
				if (!m.mParams.GetBoolValue(ModParamIndexOffsets::Enabled)) {// m.mEnabled.GetBoolValue()) {
					continue;
				}
				auto src__ = m.mParams.GetEnumValue<ModSource>(ModParamIndexOffsets::Source);
				auto auxEnabled__ = m.mParams.GetBoolValue(ModParamIndexOffsets::AuxEnabled);
				auto auxModSource__ = m.mParams.GetEnumValue<ModSource>(ModParamIndexOffsets::AuxSource);
				if ((src__ == modSource) || (auxEnabled__ && (auxModSource__ == modSource))) {
					// it's referenced by aux or main source. check that the destination is enabled.
					for (size_t id = 0; id < M7::gModulationSpecDestinationCount; ++id)
					{
						auto dest = m.mParams.GetEnumValue<ModDestination>((int)ModParamIndexOffsets::Destination1 + id); // m.mDestinations[id].GetEnumValue();
						bool isHiddenVolumeDest = false;
						for (auto& src : p->mSources)
						{
							if (src->mHiddenVolumeModDestID == dest) {
								isHiddenVolumeDest = true;
								// the mod spec is modulating from this env to a hidden volume control; likely a built-in modulationspec for osc vol.
								// if that source is enabled, then consider this used.
								// otherwise keep looking.
								if (src->mEnabledParam.GetBoolValue()) {
									return true;
								}
							}
						}
						if (!isHiddenVolumeDest) {
							// it's modulating something other than a built-in volume control; assume the use cares about this.
							// here's where ideally we'd check deeper but rather just assume it's in use. after all the user explicitly set this mod.
							return true;
						}
					}

				}
			}
			return false;
		}

		static inline bool IsLFOInUse(Maj7* p, ModSource modSource)
		{
			// an LFO is in use if it's used as a modulation source, AND the modulation is enabled.
			// that's it. we don't nede to be any more specific than that because there are no default modspecs that contain lfos.
			// therefore if it's enabled we assume the user means it.
			for (auto& m : p->mModulations) {
				auto menabled = m.mParams.GetBoolValue(ModParamIndexOffsets::Enabled);
				if (!menabled) {// m.mEnabled.GetBoolValue()) {
					continue;
				}
				auto src__ = m.mParams.GetEnumValue<ModSource>(ModParamIndexOffsets::Source);
				auto auxEnabled__ = m.mParams.GetBoolValue(ModParamIndexOffsets::AuxEnabled);
				auto auxModSource__ = m.mParams.GetEnumValue<ModSource>(ModParamIndexOffsets::AuxSource);
				if ((src__ == modSource) || (auxEnabled__ && (auxModSource__ == modSource))) {
					// it's referenced by aux or main source.
					return true;
				}
			}
			return false;
		}

		struct ChunkStats
		{
			int nonZeroParams = 0;
			int defaultParams = 0;
int uncompressedSize = 0;
int compressedSize = 0;
		};

		static inline ChunkStats AnalyzeChunkMinification(Maj7* p)
		{
			ChunkStats ret;
			void* data;
			int size = GetMinifiedChunk(p, &data);
			//int size = p->GetChunk(&data);
			ret.uncompressedSize = size;

			//int numNonZeroParams = 0;
			for (size_t i = 0; i < (int)M7::ParamIndices::NumParams; ++i) {
				float d = p->mParamCache[i] - p->mDefaultParamCache[i];
				if (fabsf(d) > 0.00001f) ret.nonZeroParams++;
				else ret.defaultParams++;
			}

			std::vector<uint8_t> compressedData;
			compressedData.resize(size);
			std::vector<uint8_t> encodedProps;
			encodedProps.resize(size);
			SizeT compressedSize = size;
			SizeT encodedPropsSize = size;

			ISzAlloc alloc;
			alloc.Alloc = [](ISzAllocPtr p, SizeT s) {
				return malloc(s);
			};
			alloc.Free = [](ISzAllocPtr p, void* addr) {
				free(addr);
			};

			CLzmaEncProps props;
			LzmaEncProps_Init(&props);
			props.level = 5;

			int lzresult = LzmaEncode(&compressedData[0], &compressedSize, (const Byte*)data, size, &props, encodedProps.data(), &encodedPropsSize, 0, nullptr, &alloc, &alloc);
			ret.compressedSize = (int)compressedSize;

			delete[] data;
			return ret;
		}

		// for any param which can have multiple underlying values being effectively equal, make sure we are exactly
		// using the default value when the effective value is the same.
		template<typename T, typename Tbase, typename Toffset>
		static inline void OptimizeEnumParam(Maj7* p, EnumParam<T>& param, T itemCount, Tbase baseParam, Toffset paramOffset)
		{
			int paramID = (int)baseParam + (int)paramOffset;
			if (paramID < 0) return; // invalid IDs exist for example in LFo
			T liveEnumValue = param.GetEnumValue();
			float defaultParamVal = p->mDefaultParamCache[paramID];
			EnumParam<T> defParam{ defaultParamVal , itemCount };
			if (liveEnumValue == defParam.GetEnumValue()) {
				param.SetParamValue(defaultParamVal);
			}
		}

		template<typename Tenum, typename Toffset>
		static inline void OptimizeEnumParam(Maj7* p, ParamAccessor& params, Toffset offset)
		{
			int paramID = params.GetParamIndex(offset);// (int)baseParam + (int)paramOffset;
			if ((int)offset < 0 || paramID < 0) return; // invalid IDs exist for example in LFo

			ParamAccessor dp{ p->mDefaultParamCache, params.mBaseParamID };
			if (dp.GetEnumValue<Tenum>(offset) == params.GetEnumValue<Tenum>(offset)) {
				params.SetRawVal(offset, dp.GetRawVal(offset));
			}
		}

		// for any param which can have multiple underlying values being effectively equal, make sure we are exactly
		// using the default value when the effective value is the same.
		template<typename Tbase, typename Toffset>
		static inline void OptimizeIntParam(Maj7* p, IntParam& param, Tbase baseParam, Toffset paramOffset)
		{
			int paramID = (int)baseParam + (int)paramOffset;
			if (paramID < 0) return; // invalid IDs exist for example in LFo
			int liveIntValue = param.GetIntValue();
			float defaultParamVal = p->mDefaultParamCache[paramID];
			IntParam defParam{ defaultParamVal, param.mCfg };
			if (liveIntValue == defParam.GetIntValue()) {
				param.SetParamValue(defaultParamVal);
			}
		}


		template<typename Toffset>
		static inline void OptimizeIntParam(Maj7* p, ParamAccessor& params, const IntParamConfig& cfg, Toffset offset)
		{
			int paramID = params.GetParamIndex(offset);// (int)baseParam + (int)paramOffset;
			if ((int)offset < 0 || paramID < 0) return; // invalid IDs exist for example in LFo

			ParamAccessor dp{ p->mDefaultParamCache, params.mBaseParamID };
			if (dp.GetIntValue(offset, cfg) == params.GetIntValue(offset, cfg)) {
				params.SetRawVal(offset, dp.GetRawVal(offset));
			}
		}

		// for any param which can have multiple underlying values being effectively equal, make sure we are exactly
		// using the default value when the effective value is the same.
		template<typename Tbase, typename Toffset>
		static inline void OptimizeBoolParam(Maj7* p, BoolParam& param, Tbase baseParam, Toffset offset)
		{
			int paramID = (int)baseParam + (int)offset;
			if (paramID < 0) return; // invalid IDs exist for example in LFo
			bool liveBoolValue = param.GetBoolValue();
			float defaultParamVal = p->mDefaultParamCache[paramID];
			BoolParam defParam{ defaultParamVal };
			if (liveBoolValue == defParam.GetBoolValue()) {
				param.SetRawParamValue(defaultParamVal);
			}
		}

		template<typename Toffset>
		static inline void OptimizeBoolParam(Maj7* p, ParamAccessor& params, Toffset offset)
		{
			int paramID = params.GetParamIndex(offset);// (int)baseParam + (int)paramOffset;
			if ((int)offset < 0 || paramID < 0) return; // invalid IDs exist for example in LFo
			//bool liveBoolValue = params.GetBoolValue(offset);// param.GetBoolValue();
			ParamAccessor dp{ p->mDefaultParamCache, params.mBaseParamID };
			if (dp.GetBoolValue(offset) == params.GetBoolValue(offset)) {
				params.SetRawVal(offset, dp.GetRawVal(offset));
			}
			////BoolParam defParam{ defaultParamVal };
			//if (liveBoolValue == defParam.GetBoolValue()) {
			//	float defaultParamVal = p->mDefaultParamCache[paramID];
			//	param.SetRawParamValue(defaultParamVal);
			//}
		}


		static inline void OptimizeEnvelope(Maj7* p, EnvelopeNode& env)
		{
			OptimizeBoolParam(p, env.mParams, EnvParamIndexOffsets::LegatoRestart);
			OptimizeEnumParam<EnvelopeMode>(p, env.mParams, EnvParamIndexOffsets::Mode);

			if (!IsEnvelopeInUse(p, env.mMyModSource)) {
				memcpy(env.mParams.GetOffsetParamCache(), gDefaultEnvelopeParams, sizeof(gDefaultEnvelopeParams));
				return;
			}
			if (env.mParams.GetEnumValue<EnvelopeMode>(EnvParamIndexOffsets::Mode) == EnvelopeMode::OneShot)
			{
				ParamAccessor defaults{ p->mDefaultParamCache, env.mParams.mBaseParamID };
				env.mParams.Set01Val(EnvParamIndexOffsets::SustainLevel, defaults.GetRawVal(EnvParamIndexOffsets::SustainLevel));
				env.mParams.Set01Val(EnvParamIndexOffsets::ReleaseTime, defaults.GetRawVal(EnvParamIndexOffsets::ReleaseTime));
				env.mParams.Set01Val(EnvParamIndexOffsets::ReleaseCurve, defaults.GetRawVal(EnvParamIndexOffsets::ReleaseCurve));
			}
		}

		static inline void OptimizeSource(Maj7* p, ISoundSourceDevice* psrc)
		{
			OptimizeBoolParam(p, psrc->mEnabledParam, psrc->mEnabledParamID, 0);
			OptimizeIntParam(p, psrc->mPitchSemisParam, psrc->mTuneSemisParamID, 0);
			OptimizeIntParam(p, psrc->mKeyRangeMin, psrc->mKeyRangeMinParamID, 0);
			OptimizeIntParam(p, psrc->mKeyRangeMax, psrc->mKeyRangeMaxParamID, 0);
		}

		// if aggressive, then round values which are very close to defaults back to default.
		static inline void OptimizeParams(Maj7* p, bool aggressive)
		{
			// samplers
			for (auto& s : p->mSamplerDevices)
			{
				OptimizeSource(p, &s);

				OptimizeEnumParam<LoopMode>(p, s.mParams,SamplerParamIndexOffsets::LoopMode);

				//OptimizeEnumParam(p, s.mLoopMode, LoopMode::NumLoopModes, s.mBaseParamID, SamplerParamIndexOffsets::LoopMode);
				OptimizeEnumParam< LoopBoundaryMode>(p, s.mParams, SamplerParamIndexOffsets::LoopSource);
				OptimizeEnumParam < InterpolationMode>(p, s.mParams, SamplerParamIndexOffsets::InterpolationType);
				OptimizeEnumParam < SampleSource>(p, s.mParams, SamplerParamIndexOffsets::SampleSource);

				OptimizeBoolParam(p, s.mParams, SamplerParamIndexOffsets::LegatoTrig);
				OptimizeBoolParam(p, s.mParams, SamplerParamIndexOffsets::Reverse);
				OptimizeBoolParam(p, s.mParams, SamplerParamIndexOffsets::ReleaseExitsLoop);

				OptimizeIntParam(p, s.mParams, gGmDlsIndexParamCfg, SamplerParamIndexOffsets::GmDlsIndex);
				OptimizeIntParam(p, s.mParams, gKeyRangeCfg, SamplerParamIndexOffsets::BaseNote);

				if (!s.mEnabledParam.GetBoolValue()) {
					s.Reset();
					memcpy(p->mParamCache + (int)s.mBaseParamID, gDefaultSamplerParams, sizeof(gDefaultSamplerParams));
				}
			}

			// oscillators
			for (auto& s : p->mOscillatorDevices)
			{
				OptimizeSource(p, &s);
				OptimizeEnumParam(p, s.mWaveform, OscillatorWaveform::Count, s.mBaseParamID, OscParamIndexOffsets::Waveform);
				OptimizeBoolParam(p, s.mPhaseRestart, s.mBaseParamID, OscParamIndexOffsets::PhaseRestart);
				OptimizeBoolParam(p, s.mSyncEnable, s.mBaseParamID, OscParamIndexOffsets::SyncEnable);

				if (!s.mEnabledParam.GetBoolValue()) {
					memcpy(p->mParamCache + (int)s.mBaseParamID, gDefaultOscillatorParams, sizeof(gDefaultOscillatorParams));
				}
			}

			// LFO
			for (auto& lfo : p->mLFOs) {
				OptimizeSource(p, &lfo.mDevice);
				OptimizeEnumParam(p, lfo.mDevice.mWaveform, OscillatorWaveform::Count, lfo.mDevice.mBaseParamID, OscParamIndexOffsets::Waveform);
				OptimizeBoolParam(p, lfo.mDevice.mPhaseRestart, lfo.mDevice.mBaseParamID, OscParamIndexOffsets::PhaseRestart);
				OptimizeBoolParam(p, lfo.mDevice.mSyncEnable, lfo.mDevice.mBaseParamID, OscParamIndexOffsets::SyncEnable);
			}

			// envelopes
			for (auto* env : p->mMaj7Voice[0]->mpAllModEnvelopes) {
				OptimizeEnvelope(p, *env);
			}
			for (auto* psv : p->mMaj7Voice[0]->mSourceVoices) {
				OptimizeEnvelope(p, *psv->mpAmpEnv);
			}

			// modulations.
			// optimize hard because there are so many modulations and there's always a lot of fiddling happening here.
			for (auto& m : p->mModulations)
			{
				if (!m.mParams.GetBoolValue(ModParamIndexOffsets::Enabled)) {
					memcpy(p->mParamCache + (int)m.mBaseParamID, gDefaultModSpecParams, sizeof(gDefaultModSpecParams));
				}

				// still, try and optimize the aux part. 
				if (!m.mParams.GetBoolValue(ModParamIndexOffsets::AuxEnabled)) {

					m.mParams.SetRawVal(ModParamIndexOffsets::AuxAttenuation, gDefaultModSpecParams[(size_t)ModParamIndexOffsets::AuxAttenuation]);
					m.mParams.SetRawVal(ModParamIndexOffsets::AuxCurve, gDefaultModSpecParams[(size_t)ModParamIndexOffsets::AuxCurve]);
					m.mParams.SetRawVal(ModParamIndexOffsets::AuxSource, gDefaultModSpecParams[(size_t)ModParamIndexOffsets::AuxSource]);
					m.mParams.SetRawVal(ModParamIndexOffsets::AuxValueMapping, gDefaultModSpecParams[(size_t)ModParamIndexOffsets::AuxValueMapping]);

					//m.mAuxAttenuation.SetParamValue(p->mParamCache[(size_t)m.mBaseParamID + (size_t)ModParamIndexOffsets::AuxAttenuation]);
					//m.mAuxCurve.SetParamValue(p->mParamCache[(size_t)m.mBaseParamID + (size_t)ModParamIndexOffsets::AuxCurve]);
					//m.mAuxSource.SetParamValue(p->mParamCache[(size_t)m.mBaseParamID + (size_t)ModParamIndexOffsets::AuxSource]);
					//m.mAuxValueMapping.SetParamValue(p->mParamCache[(size_t)m.mBaseParamID + (size_t)ModParamIndexOffsets::AuxValueMapping]);
				}
				// set destination scales
				for (size_t id = 0; id < M7::gModulationSpecDestinationCount; ++id)
				{
					if (m.mParams.GetEnumValue<ModDestination>((int)ModParamIndexOffsets::Destination1 + id) /* m.mDestinations[id].GetEnumValue() */ != ModDestination::None)
						//if (m.mDestinations[id].GetEnumValue() != ModDestination::None)
					{
						continue;
					}

					m.mParams.SetRawVal((int)ModParamIndexOffsets::Destination1 + id, gDefaultModSpecParams[(int)ModParamIndexOffsets::Destination1 + id]);

					//m.mScales[id].SetParamValue(p->mDefaultParamCache[id + (size_t)m.mBaseParamID + (size_t)ModParamIndexOffsets::Scale1]);
				}

				OptimizeBoolParam(p, m.mParams, ModParamIndexOffsets::Enabled);
				OptimizeBoolParam(p, m.mParams, ModParamIndexOffsets::AuxEnabled);

				OptimizeEnumParam< ModSource>(p, m.mParams, ModParamIndexOffsets::Source);
				OptimizeEnumParam < ModSource>(p, m.mParams, ModParamIndexOffsets::AuxSource);
				OptimizeEnumParam < ModValueMapping>(p, m.mParams, ModParamIndexOffsets::ValueMapping);
				OptimizeEnumParam < ModValueMapping>(p, m.mParams, ModParamIndexOffsets::AuxValueMapping);

				OptimizeEnumParam< ModDestination>(p, m.mParams, ModParamIndexOffsets::Destination1);
				OptimizeEnumParam < ModDestination>(p, m.mParams, ModParamIndexOffsets::Destination2);
				OptimizeEnumParam < ModDestination>(p, m.mParams, ModParamIndexOffsets::Destination3);
				OptimizeEnumParam < ModDestination>(p, m.mParams, ModParamIndexOffsets::Destination4);
			}

			if (aggressive) {
				for (size_t i = 0; i < (size_t)ParamIndices::NumParams; ++i) {
					if (math::FloatEquals(p->mParamCache[i], p->mDefaultParamCache[i], 0.000001f)) {
						p->mParamCache[i] = p->mDefaultParamCache[i];
					}
				}
			}
		} // optimizeParams()

	} // namespace M7

} // namespace WaveSabreCore
