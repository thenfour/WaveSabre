
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
			return 0; // unknown param name.
		}
		if (it->second.first) {
			return 0; // already set. is this a duplicate?
		}

		//char str[200];
		//sprintf(str, "setting param %s = %f\r\n", ch.mKeyName.c_str(), ch.mNumericValue.Get<float>());
		//::OutputDebugStringA(str);

		p->SetParam((VstInt32)it->second.second, ch.mNumericValue.Get<float>());
		it->second.first = true;
	}

	//for (auto& kv : paramMap) {
	//	if (!kv.second.first) {
	//		return 0; // a parameter was not set. well no big deal tbh; worth a warning?
	//	}
	//}

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
		M7::Deserializer ds{ data.data(), data.size() };
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

		//if (!byteSize) return byteSize;
		//const char* pstr = (const char *)data;
		//if (strnlen(pstr, byteSize - 1) >= byteSize) return byteSize;

		//using vstn = const char[kVstMaxParamStrLen];
		//static constexpr vstn paramNames[(int)M7::ParamIndices::NumParams] = MAJ7_PARAM_VST_NAMES;

		//clarinoid::MemoryStream memStream {(const uint8_t*)data, (size_t)byteSize};
		//clarinoid::BufferedStream buffering{ memStream };
		//clarinoid::TextStream textStream{ buffering };
		//clarinoid::JsonVariantReader doc{ textStream };

		//// @ root there is exactly 1 KV object.
		//auto maj7Obj = doc.GetNextObjectItem();
		//if (maj7Obj.IsEOF()) {
		//	return 0; // empty doc?
		//}
		//if (maj7Obj.mParseResult.IsFailure()) {
		//	return 0;//return ch.mParseResult;
		//}
		//if (maj7Obj.mKeyName != "Maj7") {
		//	return 0;
		//}

		//bool tagSatisfied = false;
		//bool versionSatisfied = false;
		//while (true) {
		//	auto ch = maj7Obj.GetNextObjectItem();
		//	if (ch.mKeyName == "Tag") {
		//		if (ch.mNumericValue.Get<DWORD>() != M7::Maj7::gChunkTag) {
		//			return 0; //invalid tag
		//		}
		//		tagSatisfied = true;
		//	}
		//	if (ch.mKeyName == "Version") {
		//		if (ch.mNumericValue.Get<uint8_t>() != M7::Maj7::gChunkVersion) {
		//			return 0; // unknown version
		//		}
		//		versionSatisfied = true;
		//	}
		//	if (ch.IsEOF())
		//		break;
		//}
		//if (!tagSatisfied || !versionSatisfied)
		//	return 0;

		//auto paramsObj = doc.GetNextObjectItem(); // assumes these are in this order. ya probably should not.
		//if (paramsObj.IsEOF()) {
		//	return 0;
		//}
		//if (paramsObj.mParseResult.IsFailure()) {
		//	return 0;
		//}
		//if (paramsObj.mKeyName != "params") {
		//	return 0;
		//}

		//std::map<std::string, std::pair<bool, size_t>> paramMap; // maps string name to whether it's been set + which param index is it.
		//for (size_t i = 0; i < (int)M7::ParamIndices::NumParams; ++i) {
		//	paramMap[paramNames[i]] = std::make_pair(false, i);
		//}

		//while (true) {
		//	auto ch = paramsObj.GetNextObjectItem();
		//	if (ch.IsEOF())
		//		break;
		//	if (ch.mParseResult.IsFailure()) {
		//		return 0;
		//	}
		//	auto it = paramMap.find(ch.mKeyName);
		//	if (it == paramMap.end()) {
		//		return 0; // unknown param name.
		//	}
		//	if (it->second.first) {
		//		return 0; // already set. is this a duplicate?
		//	}

		//	//char str[200];
		//	//sprintf(str, "setting param %s = %f\r\n", ch.mKeyName.c_str(), ch.mNumericValue.Get<float>());
		//	//::OutputDebugStringA(str);

		//	setParameter((VstInt32)it->second.second, ch.mNumericValue.Get<float>());
		//	it->second.first = true;
		//}

		////for (auto& kv : paramMap) {
		////	if (!kv.second.first) {
		////		return 0; // a parameter was not set. well no big deal tbh; worth a warning?
		////	}
		////}

		//auto samplersArr = doc.GetNextObjectItem(); // assumes these are in this order. ya probably should not.
		//if (samplersArr.IsEOF()) {
		//	return 0;
		//}
		//if (samplersArr.mParseResult.IsFailure()) {
		//	return 0;
		//}
		//if (samplersArr.mKeyName != "samplers") {
		//	return 0;
		//}

		//for (auto& s : GetMaj7()->mSamplerDevices) {
		//	auto b64 = samplersArr.GetNextArrayItem();
		//	if (b64.IsEOF()) break;
		//	if (b64.mParseResult.IsFailure()) break;
		//	auto data = clarinoid::base64_decode(b64.mStringValue);
		//	M7::Deserializer ds{data.data(), data.size()};
		//	s.Deserialize(ds);
		//}

		//return byteSize;
	}

	virtual VstInt32 getChunk(void** data, bool isPreset) override
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

		auto paramsElement = doc.Object_MakeKey("params");
		paramsElement.BeginObject();

		for (size_t i = 0; i < (int)M7::ParamIndices::NumParams; ++i)
		{
			paramsElement.Object_MakeKey(paramNames[i]).WriteNumberValue(getParameter((VstInt32)i));
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
			m->mEnabled.SetBoolValue(false);
			m->mSource.SetEnumValue(ModSource::None);
			for (auto& d : m->mDestinations) d.SetEnumValue(ModDestination::None);
			m->mCurve.SetN11Value(0);
			m->mScale.SetN11Value(0.6f);
			m->mAuxEnabled.SetBoolValue(false);
			m->mAuxSource.SetEnumValue(ModSource::None);
			m->mAuxCurve.SetN11Value(0);
			m->mAuxAttenuation.SetParamValue(1);
			m->mInvert.SetBoolValue(false);
			m->mAuxInvert.SetBoolValue(false);
		}

		static inline void GenerateDefaults_Env(EnvelopeNode* n)
		{
			n->mDelayTime.SetParamValue(0);
			n->mAttackTime.SetParamValue(0.05f);
			n->mAttackCurve.SetN11Value(0.5f);
			n->mHoldTime.SetParamValue(0);
			n->mDecayTime.SetParamValue(0.5f);
			n->mDecayCurve.SetN11Value(-0.5f);
			n->mSustainLevel.SetParamValue(0.4f);
			n->mReleaseTime.SetParamValue(0.2f);
			n->mReleaseCurve.SetN11Value(-0.5f);
			n->mLegatoRestart.SetBoolValue(true); // because for polyphonic, holding pedal and playing a note already playing is legato and should retrig. make this opt-out.
		}

		static inline void GenerateDefaults(AuxDevice* p)
		{
			p->mEnabledParam.SetBoolValue(false);
			p->mLink.SetEnumValue(p->mLinkToSelf);// (paramCache_Offset[(int)AuxParamIndexOffsets::Link], AuxLink::Count),
			p->mEffectType.SetEnumValue(AuxEffectType::None);// (paramCache_Offset[(int)AuxParamIndexOffsets::Type], AuxEffectType::Count),
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
			auto token = MutexHold{ p->mMutex };
			p->Reset();
			GenerateDefaults_Source(static_cast<ISoundSourceDevice*>(p));
			p->mLegatoTrig.SetBoolValue(true);
			p->mReverse.SetBoolValue(false);
			p->mReleaseExitsLoop.SetBoolValue(true);
			p->mSampleStart.SetParamValue(0);
			p->mFrequencyParam.mKTValue.SetParamValue(1);
			p->mLoopMode.SetEnumValue(LoopMode::Repeat);
			p->mLoopSource.SetEnumValue(LoopBoundaryMode::FromSample);
			p->mInterpolationMode.SetEnumValue(InterpolationMode::Linear);
			p->mLoopStart.SetParamValue(0);
			p->mLoopLength.SetParamValue(1);// (paramCache[(int)baseParamID + (int)SamplerParamIndexOffsets::LoopLength], 1),
			p->mBaseNote.SetIntValue(60);// (paramCache[(int)baseParamID + (int)SamplerParamIndexOffsets::BaseNote], 0, 127, 60),
			p->mSampleSource.SetEnumValue(SampleSource::Embed);
			p->mGmDlsIndex.SetIntValue(-1);
		}

		//enum class MainParamIndices : uint8_t
		static inline void GenerateMasterParamDefaults(Maj7* p)
		{
			p->mMasterVolume.SetDecibels(-6);//MasterVolume,
			p->mUnisonoVoicesParam.SetIntValue(1);
			p->mVoicingModeParam.SetEnumValue(VoiceMode::Polyphonic);
			// OscillatorDetune, = 0
			// UnisonoDetune, = 0
			// OscillatorSpread, = 0
			// UnisonoStereoSpread, = 0
			// OscillatorShapeSpread, = 0
			// UnisonoShapeSpread, = 0
			p->mFMBrightness.SetParamValue(0.5f);// FMBrightness,
			p->mAuxRoutingParam.SetEnumValue(AuxRoute::TwoTwo);// AuxRouting,
			p->mAuxWidth.SetN11Value(1);// AuxWidth
			p->mMaj7Voice[0]->mPortamento.mTime.SetParamValue(0.3f);// PortamentoTime,
			p->mMaj7Voice[0]->mPortamento.mCurve.SetN11Value(0);
			p->mPitchBendRange.SetIntValue(2);
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
			p->SetVoiceMode(p->mVoicingModeParam.GetEnumValue());
			p->SetUnisonoVoices(p->mUnisonoVoicesParam.GetIntValue());
			// NOTE: samplers will always be empty here
		}


		static inline bool IsEnvelopeInUse(Maj7* p, ModSource modSource)
		{
			// an envelope is in use if it's used as a modulation source, AND the modulation is enabled,
			// AND the modulation destination is not some disabled thing. The VAST majority of these "disabled destinations" would just be the hidden volume / pre-fm volume source param,
			// so to simplify the logic just check for that.
			for (auto& m : p->mModulations) {
				if (!m.mEnabled.GetBoolValue()) {
					continue;
				}
				if ((m.mSource.GetEnumValue() == modSource) || (m.mAuxEnabled.GetBoolValue() && (m.mAuxSource.GetEnumValue() == modSource))) {
					// it's referenced by aux or main source. check that the destination is enabled.
					for (size_t id = 0; id < M7::gModulationSpecDestinationCount; ++id)
					{
						auto dest = m.mDestinations[id].GetEnumValue();
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
				if (!m.mEnabled.GetBoolValue()) {
					continue;
				}
				if ((m.mSource.GetEnumValue() == modSource) || (m.mAuxEnabled.GetBoolValue() && (m.mAuxSource.GetEnumValue() == modSource))) {
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

		static inline void OptimizeParams(Maj7* p)
		{
			// samplers
			for (auto& s : p->mSamplerDevices)
			{
				if (!s.mEnabledParam.GetBoolValue()) {
					s.Reset();
					memcpy(p->mParamCache + (int)s.mBaseParamID, gDefaultSamplerParams, sizeof(gDefaultSamplerParams));
				}
			}

			// oscillators
			for (auto& s : p->mOscillatorDevices)
			{
				if (!s.mEnabledParam.GetBoolValue()) {
					memcpy(p->mParamCache + (int)s.mBaseParamID, gDefaultOscillatorParams, sizeof(gDefaultOscillatorParams));
				}
			}

			// LFO?
			if (!IsLFOInUse(p, ModSource::LFO1)) {
				memcpy(p->mParamCache + (int)ParamIndices::LFO1Waveform, gDefaultLFOParams, sizeof(gDefaultLFOParams));
			}
			if (!IsLFOInUse(p, ModSource::LFO2)) {
				memcpy(p->mParamCache + (int)ParamIndices::LFO2Waveform, gDefaultLFOParams, sizeof(gDefaultLFOParams));
			}
			if (!IsLFOInUse(p, ModSource::LFO3)) {
				memcpy(p->mParamCache + (int)ParamIndices::LFO3Waveform, gDefaultLFOParams, sizeof(gDefaultLFOParams));
			}
			if (!IsLFOInUse(p, ModSource::LFO4)) {
				memcpy(p->mParamCache + (int)ParamIndices::LFO4Waveform, gDefaultLFOParams, sizeof(gDefaultLFOParams));
			}

			// envelopes
			for (auto* env : p->mMaj7Voice[0]->mpAllModEnvelopes) {
				if (!IsEnvelopeInUse(p, env->mMyModSource)) {
					memcpy(p->mParamCache + (int)env->mParamBaseID, gDefaultEnvelopeParams, sizeof(gDefaultEnvelopeParams));
				}
			}
			for (auto* psv : p->mMaj7Voice[0]->mSourceVoices) {
				if (!IsEnvelopeInUse(p, psv->mpAmpEnv->mMyModSource)) {
					memcpy(p->mParamCache + (int)psv->mpAmpEnv->mParamBaseID, gDefaultEnvelopeParams, sizeof(gDefaultEnvelopeParams));
				}
			}

			// modulations 1-8 (gOptimizeableModulations)
			for (auto& m : p->mModulations) {
				if (m.mType != ModulationSpecType::General) continue; // don't try to optimize internal stuff. only stuff the user has likely messed with and disabled.
				if (m.mEnabled.GetBoolValue()) continue;
				memcpy(p->mParamCache + (int)m.mBaseParamID, gDefaultModSpecParams, sizeof(gDefaultModSpecParams));
			}
		}

	} // namespace M7

} // namespace WaveSabreCore
