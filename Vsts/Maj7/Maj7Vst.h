
#pragma once

#include <WaveSabreVstLib.h>

class Maj7Vst : public WaveSabreVstLib::VstPlug
{
public:
	std::string mMacroNames[M7::gMacroCount];
	std::string mSourceNames[M7::gSourceCount];
	std::string mLFONames[M7::gModLFOCount];
	std::string mModEnvNames[M7::gModEnvCount];
	bool mShowAdvancedModControls[M7::gModulationCount];
	bool mShowAdvancedModAuxControls[M7::gModulationCount];
	std::string mPatchName;
	int mSelectedSource = 0; // 0-7
	int mSelectedModEnvOrLFO = 0;
	int mSelectedFilter = 0;
	int mSelectedModulation = 0;

	void SetDefaultSettings()
	{
		for (auto& s : mMacroNames) {
			s.clear();
		}
		for (auto& s : mSourceNames) {
			s.clear();
		}
		for (auto& s : mLFONames) {
			s.clear();
		}
		for (auto& s : mModEnvNames) {
			s.clear();
		}
		for (auto& b : mShowAdvancedModControls) {
			b = false;
		}
		for (auto& b : mShowAdvancedModAuxControls) {
			b = false;
		}
		mPatchName.clear();
		mSelectedSource = 0; // 0-7
		mSelectedModEnvOrLFO = 0;
		mSelectedFilter = 0;
		mSelectedModulation = 0;
	}

	Maj7Vst(audioMasterCallback audioMaster);
	
	virtual void getParameterName(VstInt32 index, char *text);

	virtual bool getEffectName(char *name);
	virtual bool getProductString(char *text);

	virtual VstInt32 setChunk(void* data, VstInt32 byteSize, bool isPreset) override
	{
		//return Maj7SetVstChunk(this, GetMaj7(), data, byteSize);
		Maj7* p = GetMaj7();

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

		maj7Obj.EnsureClosed();

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
			auto keyName = ch.mKeyName;
			// #26 - redesign of aux devices means we should adjust some param key names.
			// aux params need to be converted to filter params.
			if (keyName == "X1En") keyName = "F1En";
			if (keyName == "X1P1") {
				keyName = "F1Type"; // model
			}
			if (keyName == "X1P2") keyName = "F1Q"; // Q
			if (keyName == "X1P3") keyName = "F1En"; // saturation
			if (keyName == "X1P4") keyName = "F1Freq"; // freq
			if (keyName == "X1P5") keyName = "F1FKT"; // kt

			if (keyName == "X2En") keyName = "F2En";
			if (keyName == "X2P1") {
				keyName = "F2Type"; // model
			}
			if (keyName == "X2P2") keyName = "F2Q"; // Q
			if (keyName == "X2P3") keyName = "F2En"; // saturation
			if (keyName == "X2P4") keyName = "F2Freq"; // freq
			if (keyName == "X2P5") keyName = "F2FKT"; // kt

			auto it = paramMap.find(keyName);
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

		for (auto* s : p->mpSamplerDevices) {
			auto b64 = samplersArr.GetNextArrayItem();
			if (b64.IsEOF()) break;
			if (b64.mParseResult.IsFailure()) break;
			auto data = clarinoid::base64_decode(b64.mStringValue);
			M7::Deserializer ds{ data.data() };
			s->Deserialize(ds);
		}

		samplersArr.EnsureClosed();

		SetDefaultSettings();

		auto settingsObj = doc.GetNextObjectItem(); // assumes these are in this order. ya probably should not.
		if (settingsObj.mParseResult.IsFailure()) {
			return 0;
		}
		if (settingsObj.mKeyName != "PluginSettings") {
			return 0;
		}
		auto pvst = this;
		while (true) {
			auto ch = settingsObj.GetNextObjectItem();
			if (ch.IsEOF())
				break;
			if (ch.mParseResult.IsFailure()) {
				break;
			}
			if (pvst) {
				static_assert(M7::gMacroCount == 7, "update this here shiz");
				if (ch.mKeyName == "MacroName0") pvst->mMacroNames[0] = ch.mStringValue;
				if (ch.mKeyName == "MacroName1") pvst->mMacroNames[1] = ch.mStringValue;
				if (ch.mKeyName == "MacroName2") pvst->mMacroNames[2] = ch.mStringValue;
				if (ch.mKeyName == "MacroName3") pvst->mMacroNames[3] = ch.mStringValue;
				if (ch.mKeyName == "MacroName4") pvst->mMacroNames[4] = ch.mStringValue;
				if (ch.mKeyName == "MacroName5") pvst->mMacroNames[5] = ch.mStringValue;
				if (ch.mKeyName == "MacroName6") pvst->mMacroNames[6] = ch.mStringValue;

				static_assert(M7::gSourceCount == 8, "update this here shiz");
				if (ch.mKeyName == "SourceName0") pvst->mSourceNames[0] = ch.mStringValue;
				if (ch.mKeyName == "SourceName1") pvst->mSourceNames[1] = ch.mStringValue;
				if (ch.mKeyName == "SourceName2") pvst->mSourceNames[2] = ch.mStringValue;
				if (ch.mKeyName == "SourceName3") pvst->mSourceNames[3] = ch.mStringValue;
				if (ch.mKeyName == "SourceName4") pvst->mSourceNames[4] = ch.mStringValue;
				if (ch.mKeyName == "SourceName5") pvst->mSourceNames[5] = ch.mStringValue;
				if (ch.mKeyName == "SourceName6") pvst->mSourceNames[6] = ch.mStringValue;
				if (ch.mKeyName == "SourceName7") pvst->mSourceNames[7] = ch.mStringValue;

				static_assert(M7::gModLFOCount == 4, "update this here shiz");
				if (ch.mKeyName == "LFOName0") pvst->mLFONames[0] = ch.mStringValue;
				if (ch.mKeyName == "LFOName1") pvst->mLFONames[1] = ch.mStringValue;
				if (ch.mKeyName == "LFOName2") pvst->mLFONames[2] = ch.mStringValue;
				if (ch.mKeyName == "LFOName3") pvst->mLFONames[3] = ch.mStringValue;

				static_assert(M7::gModEnvCount == 2, "update this here shiz");
				if (ch.mKeyName == "ModEnvName0") pvst->mModEnvNames[0] = ch.mStringValue;
				if (ch.mKeyName == "ModEnvName1") pvst->mModEnvNames[1] = ch.mStringValue;

				static_assert(M7::gModulationCount == 18, "update this here shiz");
				if (ch.mKeyName == "ShowAdvancedModControls0") pvst->mShowAdvancedModControls[0] = ch.mBooleanValue;
				if (ch.mKeyName == "ShowAdvancedModControls1") pvst->mShowAdvancedModControls[1] = ch.mBooleanValue;
				if (ch.mKeyName == "ShowAdvancedModControls2") pvst->mShowAdvancedModControls[2] = ch.mBooleanValue;
				if (ch.mKeyName == "ShowAdvancedModControls3") pvst->mShowAdvancedModControls[3] = ch.mBooleanValue;
				if (ch.mKeyName == "ShowAdvancedModControls4") pvst->mShowAdvancedModControls[4] = ch.mBooleanValue;
				if (ch.mKeyName == "ShowAdvancedModControls5") pvst->mShowAdvancedModControls[5] = ch.mBooleanValue;
				if (ch.mKeyName == "ShowAdvancedModControls6") pvst->mShowAdvancedModControls[6] = ch.mBooleanValue;
				if (ch.mKeyName == "ShowAdvancedModControls7") pvst->mShowAdvancedModControls[7] = ch.mBooleanValue;
				if (ch.mKeyName == "ShowAdvancedModControls8") pvst->mShowAdvancedModControls[8] = ch.mBooleanValue;
				if (ch.mKeyName == "ShowAdvancedModControls9") pvst->mShowAdvancedModControls[9] = ch.mBooleanValue;
				if (ch.mKeyName == "ShowAdvancedModControls10") pvst->mShowAdvancedModControls[10] = ch.mBooleanValue;
				if (ch.mKeyName == "ShowAdvancedModControls11") pvst->mShowAdvancedModControls[11] = ch.mBooleanValue;
				if (ch.mKeyName == "ShowAdvancedModControls12") pvst->mShowAdvancedModControls[12] = ch.mBooleanValue;
				if (ch.mKeyName == "ShowAdvancedModControls13") pvst->mShowAdvancedModControls[13] = ch.mBooleanValue;
				if (ch.mKeyName == "ShowAdvancedModControls14") pvst->mShowAdvancedModControls[14] = ch.mBooleanValue;
				if (ch.mKeyName == "ShowAdvancedModControls15") pvst->mShowAdvancedModControls[15] = ch.mBooleanValue;
				if (ch.mKeyName == "ShowAdvancedModControls16") pvst->mShowAdvancedModControls[16] = ch.mBooleanValue;
				if (ch.mKeyName == "ShowAdvancedModControls17") pvst->mShowAdvancedModControls[17] = ch.mBooleanValue;

				if (ch.mKeyName == "ShowAdvancedModAuxControls0") pvst->mShowAdvancedModAuxControls[0] = ch.mBooleanValue;
				if (ch.mKeyName == "ShowAdvancedModAuxControls1") pvst->mShowAdvancedModAuxControls[1] = ch.mBooleanValue;
				if (ch.mKeyName == "ShowAdvancedModAuxControls2") pvst->mShowAdvancedModAuxControls[2] = ch.mBooleanValue;
				if (ch.mKeyName == "ShowAdvancedModAuxControls3") pvst->mShowAdvancedModAuxControls[3] = ch.mBooleanValue;
				if (ch.mKeyName == "ShowAdvancedModAuxControls4") pvst->mShowAdvancedModAuxControls[4] = ch.mBooleanValue;
				if (ch.mKeyName == "ShowAdvancedModAuxControls5") pvst->mShowAdvancedModAuxControls[5] = ch.mBooleanValue;
				if (ch.mKeyName == "ShowAdvancedModAuxControls6") pvst->mShowAdvancedModAuxControls[6] = ch.mBooleanValue;
				if (ch.mKeyName == "ShowAdvancedModAuxControls7") pvst->mShowAdvancedModAuxControls[7] = ch.mBooleanValue;
				if (ch.mKeyName == "ShowAdvancedModAuxControls8") pvst->mShowAdvancedModAuxControls[8] = ch.mBooleanValue;
				if (ch.mKeyName == "ShowAdvancedModAuxControls9") pvst->mShowAdvancedModAuxControls[9] = ch.mBooleanValue;
				if (ch.mKeyName == "ShowAdvancedModAuxControls10") pvst->mShowAdvancedModAuxControls[10] = ch.mBooleanValue;
				if (ch.mKeyName == "ShowAdvancedModAuxControls11") pvst->mShowAdvancedModAuxControls[11] = ch.mBooleanValue;
				if (ch.mKeyName == "ShowAdvancedModAuxControls12") pvst->mShowAdvancedModAuxControls[12] = ch.mBooleanValue;
				if (ch.mKeyName == "ShowAdvancedModAuxControls13") pvst->mShowAdvancedModAuxControls[13] = ch.mBooleanValue;
				if (ch.mKeyName == "ShowAdvancedModAuxControls14") pvst->mShowAdvancedModAuxControls[14] = ch.mBooleanValue;
				if (ch.mKeyName == "ShowAdvancedModAuxControls15") pvst->mShowAdvancedModAuxControls[15] = ch.mBooleanValue;
				if (ch.mKeyName == "ShowAdvancedModAuxControls16") pvst->mShowAdvancedModAuxControls[16] = ch.mBooleanValue;
				if (ch.mKeyName == "ShowAdvancedModAuxControls17") pvst->mShowAdvancedModAuxControls[17] = ch.mBooleanValue;

				if (ch.mKeyName == "PatchName") pvst->mPatchName = ch.mStringValue;

				if (ch.mKeyName == "SelectedSource") pvst->mSelectedSource = ch.mNumericValue.Get<int>();
				if (ch.mKeyName == "SelectedModEnvOrLFO") pvst->mSelectedModEnvOrLFO = ch.mNumericValue.Get<int>();
				if (ch.mKeyName == "SelectedFilter") pvst->mSelectedFilter = ch.mNumericValue.Get<int>();
				if (ch.mKeyName == "SelectedModulation") pvst->mSelectedModulation = ch.mNumericValue.Get<int>();
			}
		}

		return byteSize;
	}

	virtual const char* GetJSONTagName() { return "Maj7Synth"; }

	virtual VstInt32 getChunk(void** data, bool isPreset) override
	{
		// the default VST behavior is to output/set RAW values, not diff'd values.
		return getChunk2(data, isPreset, false);
	}
	VstInt32 getChunk2(void** data, bool isPreset, bool diff)
	{
		using vstn = const char[kVstMaxParamStrLen];
		static constexpr vstn paramNames[(int)M7::ParamIndices::NumParams] = MAJ7_PARAM_VST_NAMES;
		
		//auto defaultParamCache = GetDefaultParamCache();// GenerateDefaultParamCache();
		clarinoid::MemoryStream memStream;
		clarinoid::BufferedStream buffering{ memStream };
		clarinoid::TextStream textStream{ buffering };
		textStream.mMinified = false;
		clarinoid::JsonVariantWriter doc{ textStream };

		doc.BeginObject();

		auto maj7Element = doc.Object_MakeKey("Maj7");
		maj7Element.BeginObject();
		maj7Element.Object_MakeKey("Format").WriteStringValue(diff ? "DIFF values" : "Absolute values");

		auto paramsElement = doc.Object_MakeKey("params");
		paramsElement.BeginObject();
		for (size_t i = 0; i < (int)M7::ParamIndices::NumParams; ++i)
		{
			if (diff) {
				float def = mDefaultParamCache[i];
				paramsElement.Object_MakeKey(paramNames[i]).WriteNumberValue(getParameter((VstInt32)i) - def);
			}
			else {
				paramsElement.Object_MakeKey(paramNames[i]).WriteNumberValue(getParameter((VstInt32)i));
			}
		}
		paramsElement.EnsureClosed();

		auto samplersArray = doc.Object_MakeKey("samplers");
		samplersArray.BeginArray();
		for (auto* sampler : GetMaj7()->mpSamplerDevices)
		{
			M7::Serializer samplerSerializer;
			sampler->Serialize(samplerSerializer);
			auto b64 = clarinoid::base64_encode(samplerSerializer.mBuffer, (unsigned int)samplerSerializer.mSize);
			samplersArray.Array_MakeValue().WriteStringValue(b64);
		}
		samplersArray.EnsureClosed();

		auto settingsObj = doc.Object_MakeKey("PluginSettings");
		settingsObj.BeginObject();
			
		static_assert(M7::gMacroCount == 7, "update this here shiz");
		settingsObj.Object_MakeKey("MacroName0").WriteStringValue(mMacroNames[0]);
		settingsObj.Object_MakeKey("MacroName1").WriteStringValue(mMacroNames[1]);
		settingsObj.Object_MakeKey("MacroName2").WriteStringValue(mMacroNames[2]);
		settingsObj.Object_MakeKey("MacroName3").WriteStringValue(mMacroNames[3]);
		settingsObj.Object_MakeKey("MacroName4").WriteStringValue(mMacroNames[4]);
		settingsObj.Object_MakeKey("MacroName5").WriteStringValue(mMacroNames[5]);
		settingsObj.Object_MakeKey("MacroName6").WriteStringValue(mMacroNames[6]);

		static_assert(M7::gSourceCount == 8, "update this here shiz");
		settingsObj.Object_MakeKey("SourceName0").WriteStringValue(mSourceNames[0]);
		settingsObj.Object_MakeKey("SourceName1").WriteStringValue(mSourceNames[1]);
		settingsObj.Object_MakeKey("SourceName2").WriteStringValue(mSourceNames[2]);
		settingsObj.Object_MakeKey("SourceName3").WriteStringValue(mSourceNames[3]);
		settingsObj.Object_MakeKey("SourceName4").WriteStringValue(mSourceNames[4]);
		settingsObj.Object_MakeKey("SourceName5").WriteStringValue(mSourceNames[5]);
		settingsObj.Object_MakeKey("SourceName6").WriteStringValue(mSourceNames[6]);
		settingsObj.Object_MakeKey("SourceName7").WriteStringValue(mSourceNames[7]);

		static_assert(M7::gModLFOCount == 4, "update this here shiz");
		settingsObj.Object_MakeKey("LFOName0").WriteStringValue(mLFONames[0]);
		settingsObj.Object_MakeKey("LFOName1").WriteStringValue(mLFONames[1]);
		settingsObj.Object_MakeKey("LFOName2").WriteStringValue(mLFONames[2]);
		settingsObj.Object_MakeKey("LFOName3").WriteStringValue(mLFONames[3]);

		static_assert(M7::gModEnvCount == 2, "update this here shiz");
		settingsObj.Object_MakeKey("ModEnvName0").WriteStringValue(mModEnvNames[0]);
		settingsObj.Object_MakeKey("ModEnvName1").WriteStringValue(mModEnvNames[1]);

		static_assert(M7::gModulationCount == 18, "update this here shiz");
		settingsObj.Object_MakeKey("ShowAdvancedModControls0").WriteBoolean(mShowAdvancedModControls[0]);
		settingsObj.Object_MakeKey("ShowAdvancedModControls1").WriteBoolean(mShowAdvancedModControls[1]);
		settingsObj.Object_MakeKey("ShowAdvancedModControls2").WriteBoolean(mShowAdvancedModControls[2]);
		settingsObj.Object_MakeKey("ShowAdvancedModControls3").WriteBoolean(mShowAdvancedModControls[3]);
		settingsObj.Object_MakeKey("ShowAdvancedModControls4").WriteBoolean(mShowAdvancedModControls[4]);
		settingsObj.Object_MakeKey("ShowAdvancedModControls5").WriteBoolean(mShowAdvancedModControls[5]);
		settingsObj.Object_MakeKey("ShowAdvancedModControls6").WriteBoolean(mShowAdvancedModControls[6]);
		settingsObj.Object_MakeKey("ShowAdvancedModControls7").WriteBoolean(mShowAdvancedModControls[7]);
		settingsObj.Object_MakeKey("ShowAdvancedModControls8").WriteBoolean(mShowAdvancedModControls[8]);
		settingsObj.Object_MakeKey("ShowAdvancedModControls9").WriteBoolean(mShowAdvancedModControls[9]);
		settingsObj.Object_MakeKey("ShowAdvancedModControls10").WriteBoolean(mShowAdvancedModControls[10]);
		settingsObj.Object_MakeKey("ShowAdvancedModControls11").WriteBoolean(mShowAdvancedModControls[11]);
		settingsObj.Object_MakeKey("ShowAdvancedModControls12").WriteBoolean(mShowAdvancedModControls[12]);
		settingsObj.Object_MakeKey("ShowAdvancedModControls13").WriteBoolean(mShowAdvancedModControls[13]);
		settingsObj.Object_MakeKey("ShowAdvancedModControls14").WriteBoolean(mShowAdvancedModControls[14]);
		settingsObj.Object_MakeKey("ShowAdvancedModControls15").WriteBoolean(mShowAdvancedModControls[15]);
		settingsObj.Object_MakeKey("ShowAdvancedModControls16").WriteBoolean(mShowAdvancedModControls[16]);
		settingsObj.Object_MakeKey("ShowAdvancedModControls17").WriteBoolean(mShowAdvancedModControls[17]);

		settingsObj.Object_MakeKey("ShowAdvancedModAuxControls0").WriteBoolean(mShowAdvancedModAuxControls[0]);
		settingsObj.Object_MakeKey("ShowAdvancedModAuxControls1").WriteBoolean(mShowAdvancedModAuxControls[1]);
		settingsObj.Object_MakeKey("ShowAdvancedModAuxControls2").WriteBoolean(mShowAdvancedModAuxControls[2]);
		settingsObj.Object_MakeKey("ShowAdvancedModAuxControls3").WriteBoolean(mShowAdvancedModAuxControls[3]);
		settingsObj.Object_MakeKey("ShowAdvancedModAuxControls4").WriteBoolean(mShowAdvancedModAuxControls[4]);
		settingsObj.Object_MakeKey("ShowAdvancedModAuxControls5").WriteBoolean(mShowAdvancedModAuxControls[5]);
		settingsObj.Object_MakeKey("ShowAdvancedModAuxControls6").WriteBoolean(mShowAdvancedModAuxControls[6]);
		settingsObj.Object_MakeKey("ShowAdvancedModAuxControls7").WriteBoolean(mShowAdvancedModAuxControls[7]);
		settingsObj.Object_MakeKey("ShowAdvancedModAuxControls8").WriteBoolean(mShowAdvancedModAuxControls[8]);
		settingsObj.Object_MakeKey("ShowAdvancedModAuxControls9").WriteBoolean(mShowAdvancedModAuxControls[9]);
		settingsObj.Object_MakeKey("ShowAdvancedModAuxControls10").WriteBoolean(mShowAdvancedModAuxControls[10]);
		settingsObj.Object_MakeKey("ShowAdvancedModAuxControls11").WriteBoolean(mShowAdvancedModAuxControls[11]);
		settingsObj.Object_MakeKey("ShowAdvancedModAuxControls12").WriteBoolean(mShowAdvancedModAuxControls[12]);
		settingsObj.Object_MakeKey("ShowAdvancedModAuxControls13").WriteBoolean(mShowAdvancedModAuxControls[13]);
		settingsObj.Object_MakeKey("ShowAdvancedModAuxControls14").WriteBoolean(mShowAdvancedModAuxControls[14]);
		settingsObj.Object_MakeKey("ShowAdvancedModAuxControls15").WriteBoolean(mShowAdvancedModAuxControls[15]);
		settingsObj.Object_MakeKey("ShowAdvancedModAuxControls16").WriteBoolean(mShowAdvancedModAuxControls[16]);
		settingsObj.Object_MakeKey("ShowAdvancedModAuxControls17").WriteBoolean(mShowAdvancedModAuxControls[17]);

		settingsObj.Object_MakeKey("PatchName").WriteStringValue(mPatchName);

		settingsObj.Object_MakeKey("SelectedSource").WriteNumberValue(mSelectedSource);
		settingsObj.Object_MakeKey("SelectedModEnvOrLFO").WriteNumberValue(mSelectedModEnvOrLFO);
		settingsObj.Object_MakeKey("SelectedFilter").WriteNumberValue(mSelectedFilter);
		settingsObj.Object_MakeKey("SelectedModulation").WriteNumberValue(mSelectedModulation);
			
		settingsObj.EnsureClosed();

		doc.EnsureClosed();
		buffering.flushWrite();

		size_t size = memStream.mBuffer.size() + 1;
		char* out = new char[size];
		strncpy(out, (const char *)memStream.mBuffer.data(), memStream.mBuffer.size());
		out[memStream.mBuffer.size()] = 0; // null term
		*data = out;
		return (VstInt32)size;
	}

	// assumes that p has had its default param cache filled.
	// takes the current patch, returns a binary blob containing the WaveSabre chunk.
	// this is where we serialize "diff" params, and there's the opportunity to append other things; in this case sampler devices.
	virtual int GetMinifiedChunk(void** data) override
	{
		// the base chunk is our big param dump, with sampler data appended. each sampler will append its data; we won't think too much about that data here.
		// the base chunk is a diff of default params, because that's way more compressible. the alternative would be to meticulously
		// serialize some optimal fields based on whether an object is in use or not.
		// if we just do diffs against defaults, then it's going to be almost all zeroes.
		// the DAW can also attempt to optimize by finding unused elements and setting them to defaults.
		//     0.20000000298023223877, // PortTm
		//     0.40000000596046447754, // O1ScFq
		//     0.0000001
		M7::Serializer s;

		//auto defaultParamCache = GetDefaultParamCache();// GenerateDefaultParamCache();

		for (int i = 0; i < (int)M7::ParamIndices::NumParams; ++i) {
			double f = getParameter((VstInt32)i);
			f -= mDefaultParamCache[i];
			static constexpr double eps = 0.000001;
			double af = f < 0 ? -f : f;
			if (af < eps) {
				f = 0;
			}
			//s.WriteFloat((float)f);
			s.WriteInt16NormalizedFloat((float)f);
		}

		for (auto* sd : GetMaj7()->mpSamplerDevices) {
			sd->Serialize(s);
		}

		auto ret = s.DetachBuffer();
		*data = ret.first;
		return (int)ret.second;
	}

	virtual void OptimizeParams() override
	{
		auto p = GetMaj7();
		// samplers
		for (auto* ps : p->mpSamplerDevices)
		{
			auto& s = *ps;
			// enabled, pitch semis, keyrange min, keyrange max.
			OptimizeBoolParam(s.mParams, M7::SamplerParamIndexOffsets::Enabled);
			OptimizeIntParam(s.mParams, M7::gSourcePitchSemisRange, M7::SamplerParamIndexOffsets::TuneSemis);
			OptimizeIntParam(s.mParams, M7::gKeyRangeCfg, M7::SamplerParamIndexOffsets::KeyRangeMin);
			OptimizeIntParam(s.mParams, M7::gKeyRangeCfg, M7::SamplerParamIndexOffsets::KeyRangeMax);

			OptimizeEnumParam<LoopMode>(s.mParams, M7::SamplerParamIndexOffsets::LoopMode);

			//OptimizeEnumParam(p, s.mLoopMode, LoopMode::NumLoopModes, s.mBaseParamID, SamplerParamIndexOffsets::LoopMode);
			OptimizeEnumParam< LoopBoundaryMode>(s.mParams, M7::SamplerParamIndexOffsets::LoopSource);
			OptimizeEnumParam < InterpolationMode>(s.mParams, M7::SamplerParamIndexOffsets::InterpolationType);
			OptimizeEnumParam < M7::SampleSource>(s.mParams, M7::SamplerParamIndexOffsets::SampleSource);

			OptimizeBoolParam(s.mParams, M7::SamplerParamIndexOffsets::LegatoTrig);
			OptimizeBoolParam(s.mParams, M7::SamplerParamIndexOffsets::Reverse);
			OptimizeBoolParam(s.mParams, M7::SamplerParamIndexOffsets::ReleaseExitsLoop);

			OptimizeIntParam(s.mParams, M7::gGmDlsIndexParamCfg, M7::SamplerParamIndexOffsets::GmDlsIndex);
			OptimizeIntParam(s.mParams, M7::gKeyRangeCfg, M7::SamplerParamIndexOffsets::BaseNote);

			if (!s.IsEnabled()) {
				s.Reset();
				Copy16bitDefaults(s.mParams.GetOffsetParamCache(), M7::gDefaultSamplerParams);
				s.mParams.SetBoolValue(M7::SamplerParamIndexOffsets::Enabled, false);
			}
		}

		// oscillators
		bool oscEnabled[M7::gOscillatorCount];
		int iosc = 0;
		for (auto* ps : p->mpOscillatorDevices)
		{
			auto& s = *ps;
			// enabled, pitch semis, keyrange min, keyrange max.
			oscEnabled[iosc] = OptimizeBoolParam(s.mParams, M7::OscParamIndexOffsets::Enabled);
			OptimizeIntParam(s.mParams, M7::gSourcePitchSemisRange, M7::OscParamIndexOffsets::PitchSemis);
			OptimizeIntParam(s.mParams, M7::gKeyRangeCfg, M7::OscParamIndexOffsets::KeyRangeMin);
			OptimizeIntParam(s.mParams, M7::gKeyRangeCfg, M7::OscParamIndexOffsets::KeyRangeMax);

			//OptimizeSource(p, &s);
			OptimizeEnumParam<M7::OscillatorWaveform>(s.mParams, M7::OscParamIndexOffsets::Waveform);
			OptimizeBoolParam(s.mParams, M7::OscParamIndexOffsets::PhaseRestart);
			OptimizeBoolParam(s.mParams, M7::OscParamIndexOffsets::SyncEnable);

			if (!s.IsEnabled()) {
				//memcpy(p->mParams.GetOffsetParamCache(), gDefaultOscillatorParams, sizeof(gDefaultOscillatorParams));
				Copy16bitDefaults(s.mParams.GetOffsetParamCache(), M7::gDefaultOscillatorParams);
				s.mParams.SetBoolValue(M7::OscParamIndexOffsets::Enabled, false);
			}

			iosc++;
		}

		if (!oscEnabled[0]) {
			p->mParams.Set01Val(M7::ParamIndices::FMAmt1to2, 0);
			p->mParams.Set01Val(M7::ParamIndices::FMAmt1to3, 0);
			p->mParams.Set01Val(M7::ParamIndices::FMAmt1to4, 0);
			p->mParams.Set01Val(M7::ParamIndices::FMAmt2to1, 0);
			p->mParams.Set01Val(M7::ParamIndices::FMAmt3to1, 0);
			p->mParams.Set01Val(M7::ParamIndices::FMAmt4to1, 0);
		}
		if (!oscEnabled[1]) {
			p->mParams.Set01Val(M7::ParamIndices::FMAmt2to1, 0);
			p->mParams.Set01Val(M7::ParamIndices::FMAmt2to3, 0);
			p->mParams.Set01Val(M7::ParamIndices::FMAmt2to4, 0);
			p->mParams.Set01Val(M7::ParamIndices::FMAmt1to2, 0);
			p->mParams.Set01Val(M7::ParamIndices::FMAmt3to2, 0);
			p->mParams.Set01Val(M7::ParamIndices::FMAmt4to2, 0);
		}
		if (!oscEnabled[2]) {
			p->mParams.Set01Val(M7::ParamIndices::FMAmt3to2, 0);
			p->mParams.Set01Val(M7::ParamIndices::FMAmt3to1, 0);
			p->mParams.Set01Val(M7::ParamIndices::FMAmt3to4, 0);
			p->mParams.Set01Val(M7::ParamIndices::FMAmt1to3, 0);
			p->mParams.Set01Val(M7::ParamIndices::FMAmt2to3, 0);
			p->mParams.Set01Val(M7::ParamIndices::FMAmt4to3, 0);
		}
		if (!oscEnabled[3]) {
			p->mParams.Set01Val(M7::ParamIndices::FMAmt4to2, 0);
			p->mParams.Set01Val(M7::ParamIndices::FMAmt4to3, 0);
			p->mParams.Set01Val(M7::ParamIndices::FMAmt4to1, 0);
			p->mParams.Set01Val(M7::ParamIndices::FMAmt1to4, 0);
			p->mParams.Set01Val(M7::ParamIndices::FMAmt2to4, 0);
			p->mParams.Set01Val(M7::ParamIndices::FMAmt3to4, 0);
		}

		// LFO
		for (auto* lfo : p->mpLFOs) {
			OptimizeEnumParam<M7::OscillatorWaveform>(lfo->mDevice.mParams, M7::LFOParamIndexOffsets::Waveform);
			OptimizeEnumParam<M7::TimeBasis>(lfo->mDevice.mParams, M7::LFOParamIndexOffsets::FrequencyBasis);
			OptimizeBoolParam(lfo->mDevice.mParams, M7::LFOParamIndexOffsets::Restart);
		}

		// envelopes
		for (auto* env : p->mMaj7Voice[0]->mpEnvelopes) {
			OptimizeEnvelope(*env);
		}

		// envelopes
		for (auto& f : p->mMaj7Voice[0]->mpFilters) {
			OptimizeFilter(*f[0]);
		}

		// modulations.
		// optimize hard because there are so many modulations and there's always a lot of fiddling happening here.
		for (auto* pm : p->mpModulations)
		{
			auto& m = *pm;
			if (!m.mParams.GetBoolValue(M7::ModParamIndexOffsets::Enabled)) {
				Copy16bitDefaults(m.mParams.GetOffsetParamCache(), M7::gDefaultModSpecParams);
				m.mParams.SetBoolValue(M7::ModParamIndexOffsets::Enabled, false);
			}

			// still, try and optimize the aux part. 
			if (!m.mParams.GetBoolValue(M7::ModParamIndexOffsets::AuxEnabled)) {

				m.mParams.SetRawVal(M7::ModParamIndexOffsets::AuxAttenuation, M7::math::Sample16To32Bit(M7::gDefaultModSpecParams[(size_t)M7::ModParamIndexOffsets::AuxAttenuation]));
				m.mParams.SetRawVal(M7::ModParamIndexOffsets::AuxCurve, M7::math::Sample16To32Bit(M7::gDefaultModSpecParams[(size_t)M7::ModParamIndexOffsets::AuxCurve]));
				m.mParams.SetRawVal(M7::ModParamIndexOffsets::AuxSource, M7::math::Sample16To32Bit(M7::gDefaultModSpecParams[(size_t)M7::ModParamIndexOffsets::AuxSource]));
				m.mParams.SetRawVal(M7::ModParamIndexOffsets::AuxRangeMin, M7::math::Sample16To32Bit(M7::gDefaultModSpecParams[(size_t)M7::ModParamIndexOffsets::AuxRangeMin]));
				m.mParams.SetRawVal(M7::ModParamIndexOffsets::AuxRangeMax, M7::math::Sample16To32Bit(M7::gDefaultModSpecParams[(size_t)M7::ModParamIndexOffsets::AuxRangeMax]));
			}
			// set destination scales
			for (size_t id = 0; id < M7::gModulationSpecDestinationCount; ++id)
			{
				if (m.mParams.GetEnumValue<M7::ModDestination>((int)M7::ModParamIndexOffsets::Destination1 + id) != M7::ModDestination::None)
				{
					continue;
				}

				m.mParams.SetRawVal((int)M7::ModParamIndexOffsets::Destination1 + id, M7::math::Sample16To32Bit(M7::gDefaultModSpecParams[(int)M7::ModParamIndexOffsets::Destination1 + id]));
				m.mParams.SetRawVal((int)M7::ModParamIndexOffsets::Scale1 + id, M7::math::Sample16To32Bit(M7::gDefaultModSpecParams[(int)M7::ModParamIndexOffsets::Scale1 + id]));
			}

			OptimizeBoolParam(m.mParams, M7::ModParamIndexOffsets::Enabled);
			OptimizeBoolParam(m.mParams, M7::ModParamIndexOffsets::AuxEnabled);

			OptimizeEnumParam<M7::ModSource>(m.mParams, M7::ModParamIndexOffsets::Source);
			OptimizeEnumParam <M7::ModSource>(m.mParams, M7::ModParamIndexOffsets::AuxSource);

			OptimizeEnumParam< M7::ModDestination>(m.mParams, M7::ModParamIndexOffsets::Destination1);
			OptimizeEnumParam < M7::ModDestination>(m.mParams, M7::ModParamIndexOffsets::Destination2);
			OptimizeEnumParam < M7::ModDestination>(m.mParams, M7::ModParamIndexOffsets::Destination3);
			OptimizeEnumParam < M7::ModDestination>(m.mParams, M7::ModParamIndexOffsets::Destination4);
		}

		for (size_t i = 0; i < (size_t)M7::ParamIndices::NumParams; ++i) {
			if (M7::math::FloatEquals(p->mParamCache[i], mDefaultParamCache[i], 0.000001f)) {
				p->mParamCache[i] = mDefaultParamCache[i];
			}
		}

	}

	inline bool IsEnvelopeInUse(::WaveSabreCore::M7::ModSource modSource)
	{
		// an envelope is in use if it's used as a modulation source, AND the modulation is enabled,
		// AND the modulation destination is not some disabled thing. The VAST majority of these "disabled destinations" would just be the hidden volume / pre-fm volume source param,
		// so to simplify the logic just check for that.
		for (auto& m : GetMaj7()->mpModulations) {
			if (!m->mParams.GetBoolValue(M7::ModParamIndexOffsets::Enabled)) {// m.mEnabled.GetBoolValue()) {
				continue;
			}
			auto src__ = m->mParams.GetEnumValue<M7::ModSource>(M7::ModParamIndexOffsets::Source);
			auto auxEnabled__ = m->mParams.GetBoolValue(M7::ModParamIndexOffsets::AuxEnabled);
			auto auxModSource__ = m->mParams.GetEnumValue<M7::ModSource>(M7::ModParamIndexOffsets::AuxSource);
			if ((src__ == modSource) || (auxEnabled__ && (auxModSource__ == modSource))) {
				// it's referenced by aux or main source. check that the destination is enabled.
				for (size_t id = 0; id < M7::gModulationSpecDestinationCount; ++id)
				{
					auto dest = m->mParams.GetEnumValue<M7::ModDestination>((int)M7::ModParamIndexOffsets::Destination1 + id); // m.mDestinations[id].GetEnumValue();
					bool isHiddenVolumeDest = false;
					for (auto& src : GetMaj7()->mSources)
					{
						if (src->mHiddenVolumeModDestID == dest) {
							isHiddenVolumeDest = true;
							// the mod spec is modulating from this env to a hidden volume control; likely a built-in modulationspec for osc vol.
							// if that source is enabled, then consider this used.
							// otherwise keep looking.
							if (src->IsEnabled()) {
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

	inline bool IsLFOInUse(::WaveSabreCore::M7::ModSource modSource)
	{
		// an LFO is in use if it's used as a modulation source, AND the modulation is enabled.
		// that's it. we don't nede to be any more specific than that because there are no default modspecs that contain lfos.
		// therefore if it's enabled we assume the user means it.
		for (auto& m : GetMaj7()->mpModulations) {
			auto menabled = m->mParams.GetBoolValue(M7::ModParamIndexOffsets::Enabled);
			if (!menabled) {// m.mEnabled.GetBoolValue()) {
				continue;
			}
			auto src__ = m->mParams.GetEnumValue<M7::ModSource>(M7::ModParamIndexOffsets::Source);
			auto auxEnabled__ = m->mParams.GetBoolValue(M7::ModParamIndexOffsets::AuxEnabled);
			auto auxModSource__ = m->mParams.GetEnumValue<M7::ModSource>(M7::ModParamIndexOffsets::AuxSource);
			if ((src__ == modSource) || (auxEnabled__ && (auxModSource__ == modSource))) {
				// it's referenced by aux or main source.
				return true;
			}
		}
		return false;
	}

	inline void OptimizeEnvelope(::WaveSabreCore::M7::EnvelopeNode& env)
	{
		OptimizeBoolParam(env.mParams, M7::EnvParamIndexOffsets::LegatoRestart);
		OptimizeEnumParam<M7::EnvelopeMode>(env.mParams, M7::EnvParamIndexOffsets::Mode);

		//auto defaultParamCache = GenerateDefaultParamCache();

		if (!IsEnvelopeInUse(env.mMyModSource)) {
			//memcpy(env.mParams.GetOffsetParamCache(), gDefaultEnvelopeParams, sizeof(gDefaultEnvelopeParams));
			Copy16bitDefaults(env.mParams.GetOffsetParamCache(), M7::gDefaultEnvelopeParams);
			return;
		}
		if (env.mParams.GetEnumValue<M7::EnvelopeMode>(M7::EnvParamIndexOffsets::Mode) == M7::EnvelopeMode::OneShot)
		{
			ParamAccessor defaults{ &mDefaultParamCache[0], env.mParams.mBaseParamID };
			env.mParams.Set01Val(M7::EnvParamIndexOffsets::SustainLevel, defaults.GetRawVal(M7::EnvParamIndexOffsets::SustainLevel));
			env.mParams.Set01Val(M7::EnvParamIndexOffsets::ReleaseTime, defaults.GetRawVal(M7::EnvParamIndexOffsets::ReleaseTime));
			env.mParams.Set01Val(M7::EnvParamIndexOffsets::ReleaseCurve, defaults.GetRawVal(M7::EnvParamIndexOffsets::ReleaseCurve));
		}
	}

	inline void OptimizeFilter(M7::FilterAuxNode& f)
	{
		OptimizeBoolParam(f.mParams, M7::FilterParamIndexOffsets::Enabled);
		OptimizeEnumParam<M7::FilterModel>(f.mParams, M7::FilterParamIndexOffsets::FilterType);
		bool enabled = f.mParams.GetBoolValue(M7::FilterParamIndexOffsets::Enabled);
		M7::FilterModel model = f.mParams.GetEnumValue<M7::FilterModel>(M7::FilterParamIndexOffsets::FilterType);
		if (!enabled || model == M7::FilterModel::Disabled) {
			Copy16bitDefaults(f.mParams.GetOffsetParamCache(), M7::gDefaultFilterParams);
			return;
		}
	}

	// if aggressive, then round values which are very close to defaults back to default.
	//static inline void OptimizeParams(Maj7* p, bool aggressive, std::vector<float>& defaultParamCache)
	//{
		//auto defaultParamCache = GenerateDefaultParamCache();

		//// samplers
		//for (auto* ps : p->mpSamplerDevices)
		//{
		//	auto& s = *ps;
		//	// enabled, pitch semis, keyrange min, keyrange max.
		//	OptimizeBoolParam(p, s.mParams, SamplerParamIndexOffsets::Enabled, defaultParamCache);
		//	OptimizeIntParam(p, s.mParams, M7::gSourcePitchSemisRange, SamplerParamIndexOffsets::TuneSemis, defaultParamCache);
		//	OptimizeIntParam(p, s.mParams, M7::gKeyRangeCfg, SamplerParamIndexOffsets::KeyRangeMin, defaultParamCache);
		//	OptimizeIntParam(p, s.mParams, M7::gKeyRangeCfg, SamplerParamIndexOffsets::KeyRangeMax, defaultParamCache);

		//	OptimizeEnumParam<LoopMode>(p, s.mParams, SamplerParamIndexOffsets::LoopMode, defaultParamCache);

		//	//OptimizeEnumParam(p, s.mLoopMode, LoopMode::NumLoopModes, s.mBaseParamID, SamplerParamIndexOffsets::LoopMode);
		//	OptimizeEnumParam< LoopBoundaryMode>(p, s.mParams, SamplerParamIndexOffsets::LoopSource, defaultParamCache);
		//	OptimizeEnumParam < InterpolationMode>(p, s.mParams, SamplerParamIndexOffsets::InterpolationType, defaultParamCache);
		//	OptimizeEnumParam < SampleSource>(p, s.mParams, SamplerParamIndexOffsets::SampleSource, defaultParamCache);

		//	OptimizeBoolParam(p, s.mParams, SamplerParamIndexOffsets::LegatoTrig, defaultParamCache);
		//	OptimizeBoolParam(p, s.mParams, SamplerParamIndexOffsets::Reverse, defaultParamCache);
		//	OptimizeBoolParam(p, s.mParams, SamplerParamIndexOffsets::ReleaseExitsLoop, defaultParamCache);

		//	OptimizeIntParam(p, s.mParams, gGmDlsIndexParamCfg, SamplerParamIndexOffsets::GmDlsIndex, defaultParamCache);
		//	OptimizeIntParam(p, s.mParams, gKeyRangeCfg, SamplerParamIndexOffsets::BaseNote, defaultParamCache);

		//	if (!s.IsEnabled()) {
		//		s.Reset();
		//		Copy16bitDefaults(s.mParams.GetOffsetParamCache(), gDefaultSamplerParams);
		//		s.mParams.SetBoolValue(SamplerParamIndexOffsets::Enabled, false);
		//	}
		//}

		//// oscillators
		//bool oscEnabled[M7::gOscillatorCount];
		//int iosc = 0;
		//for (auto* ps : p->mpOscillatorDevices)
		//{
		//	auto& s = *ps;
		//	// enabled, pitch semis, keyrange min, keyrange max.
		//	oscEnabled[iosc] = OptimizeBoolParam(p, s.mParams, OscParamIndexOffsets::Enabled, defaultParamCache);
		//	OptimizeIntParam(p, s.mParams, M7::gSourcePitchSemisRange, OscParamIndexOffsets::PitchSemis, defaultParamCache);
		//	OptimizeIntParam(p, s.mParams, M7::gKeyRangeCfg, OscParamIndexOffsets::KeyRangeMin, defaultParamCache);
		//	OptimizeIntParam(p, s.mParams, M7::gKeyRangeCfg, OscParamIndexOffsets::KeyRangeMax, defaultParamCache);

		//	//OptimizeSource(p, &s);
		//	OptimizeEnumParam<M7::OscillatorWaveform>(p, s.mParams, OscParamIndexOffsets::Waveform, defaultParamCache);
		//	OptimizeBoolParam(p, s.mParams, OscParamIndexOffsets::PhaseRestart, defaultParamCache);
		//	OptimizeBoolParam(p, s.mParams, OscParamIndexOffsets::SyncEnable, defaultParamCache);

		//	if (!s.IsEnabled()) {
		//		//memcpy(p->mParams.GetOffsetParamCache(), gDefaultOscillatorParams, sizeof(gDefaultOscillatorParams));
		//		Copy16bitDefaults(s.mParams.GetOffsetParamCache(), gDefaultOscillatorParams);
		//		s.mParams.SetBoolValue(OscParamIndexOffsets::Enabled, false);
		//	}

		//	iosc++;
		//}

		//if (!oscEnabled[0]) {
		//	p->mParams.Set01Val(M7::ParamIndices::FMAmt1to2, 0);
		//	p->mParams.Set01Val(M7::ParamIndices::FMAmt1to3, 0);
		//	p->mParams.Set01Val(M7::ParamIndices::FMAmt1to4, 0);
		//	p->mParams.Set01Val(M7::ParamIndices::FMAmt2to1, 0);
		//	p->mParams.Set01Val(M7::ParamIndices::FMAmt3to1, 0);
		//	p->mParams.Set01Val(M7::ParamIndices::FMAmt4to1, 0);
		//}
		//if (!oscEnabled[1]) {
		//	p->mParams.Set01Val(M7::ParamIndices::FMAmt2to1, 0);
		//	p->mParams.Set01Val(M7::ParamIndices::FMAmt2to3, 0);
		//	p->mParams.Set01Val(M7::ParamIndices::FMAmt2to4, 0);
		//	p->mParams.Set01Val(M7::ParamIndices::FMAmt1to2, 0);
		//	p->mParams.Set01Val(M7::ParamIndices::FMAmt3to2, 0);
		//	p->mParams.Set01Val(M7::ParamIndices::FMAmt4to2, 0);
		//}
		//if (!oscEnabled[2]) {
		//	p->mParams.Set01Val(M7::ParamIndices::FMAmt3to2, 0);
		//	p->mParams.Set01Val(M7::ParamIndices::FMAmt3to1, 0);
		//	p->mParams.Set01Val(M7::ParamIndices::FMAmt3to4, 0);
		//	p->mParams.Set01Val(M7::ParamIndices::FMAmt1to3, 0);
		//	p->mParams.Set01Val(M7::ParamIndices::FMAmt2to3, 0);
		//	p->mParams.Set01Val(M7::ParamIndices::FMAmt4to3, 0);
		//}
		//if (!oscEnabled[3]) {
		//	p->mParams.Set01Val(M7::ParamIndices::FMAmt4to2, 0);
		//	p->mParams.Set01Val(M7::ParamIndices::FMAmt4to3, 0);
		//	p->mParams.Set01Val(M7::ParamIndices::FMAmt4to1, 0);
		//	p->mParams.Set01Val(M7::ParamIndices::FMAmt1to4, 0);
		//	p->mParams.Set01Val(M7::ParamIndices::FMAmt2to4, 0);
		//	p->mParams.Set01Val(M7::ParamIndices::FMAmt3to4, 0);
		//}

		//// LFO
		//for (auto* lfo : p->mpLFOs) {
		//	OptimizeEnumParam<M7::OscillatorWaveform>(p, lfo->mDevice.mParams, LFOParamIndexOffsets::Waveform, defaultParamCache);
		//	OptimizeEnumParam<M7::TimeBasis>(p, lfo->mDevice.mParams, LFOParamIndexOffsets::FrequencyBasis, defaultParamCache);
		//	OptimizeBoolParam(p, lfo->mDevice.mParams, LFOParamIndexOffsets::Restart, defaultParamCache);
		//}

		//// envelopes
		//for (auto* env : p->mMaj7Voice[0]->mpEnvelopes) {
		//	OptimizeEnvelope(p, *env, defaultParamCache);
		//}

		//// envelopes
		//for (auto& f : p->mMaj7Voice[0]->mpFilters) {
		//	OptimizeFilter(p, *f[0], defaultParamCache);
		//}

		//// modulations.
		//// optimize hard because there are so many modulations and there's always a lot of fiddling happening here.
		//for (auto* pm : p->mpModulations)
		//{
		//	auto& m = *pm;
		//	if (!m.mParams.GetBoolValue(ModParamIndexOffsets::Enabled)) {
		//		Copy16bitDefaults(m.mParams.GetOffsetParamCache(), gDefaultModSpecParams);
		//		m.mParams.SetBoolValue(ModParamIndexOffsets::Enabled, false);
		//	}

		//	// still, try and optimize the aux part. 
		//	if (!m.mParams.GetBoolValue(ModParamIndexOffsets::AuxEnabled)) {

		//		m.mParams.SetRawVal(ModParamIndexOffsets::AuxAttenuation, M7::math::Sample16To32Bit(gDefaultModSpecParams[(size_t)ModParamIndexOffsets::AuxAttenuation]));
		//		m.mParams.SetRawVal(ModParamIndexOffsets::AuxCurve, M7::math::Sample16To32Bit(gDefaultModSpecParams[(size_t)ModParamIndexOffsets::AuxCurve]));
		//		m.mParams.SetRawVal(ModParamIndexOffsets::AuxSource, M7::math::Sample16To32Bit(gDefaultModSpecParams[(size_t)ModParamIndexOffsets::AuxSource]));
		//		m.mParams.SetRawVal(ModParamIndexOffsets::AuxRangeMin, M7::math::Sample16To32Bit(gDefaultModSpecParams[(size_t)ModParamIndexOffsets::AuxRangeMin]));
		//		m.mParams.SetRawVal(ModParamIndexOffsets::AuxRangeMax, M7::math::Sample16To32Bit(gDefaultModSpecParams[(size_t)ModParamIndexOffsets::AuxRangeMax]));
		//	}
		//	// set destination scales
		//	for (size_t id = 0; id < M7::gModulationSpecDestinationCount; ++id)
		//	{
		//		if (m.mParams.GetEnumValue<ModDestination>((int)ModParamIndexOffsets::Destination1 + id) != ModDestination::None)
		//		{
		//			continue;
		//		}

		//		m.mParams.SetRawVal((int)ModParamIndexOffsets::Destination1 + id, M7::math::Sample16To32Bit(gDefaultModSpecParams[(int)ModParamIndexOffsets::Destination1 + id]));
		//		m.mParams.SetRawVal((int)ModParamIndexOffsets::Scale1 + id, M7::math::Sample16To32Bit(gDefaultModSpecParams[(int)ModParamIndexOffsets::Scale1 + id]));
		//	}

		//	OptimizeBoolParam(p, m.mParams, ModParamIndexOffsets::Enabled, defaultParamCache);
		//	OptimizeBoolParam(p, m.mParams, ModParamIndexOffsets::AuxEnabled, defaultParamCache);

		//	OptimizeEnumParam< ModSource>(p, m.mParams, ModParamIndexOffsets::Source, defaultParamCache);
		//	OptimizeEnumParam < ModSource>(p, m.mParams, ModParamIndexOffsets::AuxSource, defaultParamCache);

		//	OptimizeEnumParam< ModDestination>(p, m.mParams, ModParamIndexOffsets::Destination1, defaultParamCache);
		//	OptimizeEnumParam < ModDestination>(p, m.mParams, ModParamIndexOffsets::Destination2, defaultParamCache);
		//	OptimizeEnumParam < ModDestination>(p, m.mParams, ModParamIndexOffsets::Destination3, defaultParamCache);
		//	OptimizeEnumParam < ModDestination>(p, m.mParams, ModParamIndexOffsets::Destination4, defaultParamCache);
		//}

		//if (aggressive) {
		//	for (size_t i = 0; i < (size_t)ParamIndices::NumParams; ++i) {
		//		if (math::FloatEquals(p->mParamCache[i], defaultParamCache[i], 0.000001f)) {
		//			p->mParamCache[i] = defaultParamCache[i];
		//		}
		//	}
		//}


	//} // optimizeParams()





	WaveSabreCore::M7::Maj7 *GetMaj7() const;
}; // Maj7Vst


namespace WaveSabreCore
{
	namespace M7
	{
		static inline void GenerateDefaults(ModulationSpec* m)
		{
			m->mParams.SetBoolValue(ModParamIndexOffsets::Enabled, false);
			m->mParams.SetEnumValue(ModParamIndexOffsets::Source, ModSource::None);

			for (size_t i = 0; i < std::size(m->mDestinations); ++i) {
				m->mParams.SetEnumValue((int)ModParamIndexOffsets::Destination1 + i, ModDestination::None);
			}

			for (size_t i = 0; i < std::size(m->mScales); ++i) {
				m->mParams.SetN11Value((int)ModParamIndexOffsets::Scale1 + i, 0.75f);
			}

			m->mParams.SetN11Value(ModParamIndexOffsets::Curve, 0);// mCurve.SetN11Value(0);
			m->mParams.SetBoolValue(ModParamIndexOffsets::AuxEnabled, false);
			
			m->mParams.SetEnumValue(ModParamIndexOffsets::AuxSource, ModSource::None);
			m->mParams.SetN11Value(ModParamIndexOffsets::AuxCurve, 0); //m->mAuxCurve.SetN11Value(0);
			m->mParams.Set01Val(ModParamIndexOffsets::AuxAttenuation, 1);

			m->mParams.SetRangedValue(ModParamIndexOffsets::SrcRangeMin, -3, 3, -1);
			m->mParams.SetRangedValue(ModParamIndexOffsets::SrcRangeMax, -3, 3, 1);
			m->mParams.SetRangedValue(ModParamIndexOffsets::AuxRangeMin, -3, 3, 0);
			m->mParams.SetRangedValue(ModParamIndexOffsets::AuxRangeMax, -3, 3, 1);
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
		}

		static inline void GenerateDefaults(FilterAuxNode* p)
		{
			p->mParams.SetBoolValue(FilterParamIndexOffsets::Enabled, false);
			p->mParams.SetEnumValue<FilterModel>(FilterParamIndexOffsets::FilterType, FilterModel::LP_Moog4);
			p->mParams.Set01Val(FilterParamIndexOffsets::Freq, 0.3f);
			p->mParams.Set01Val(FilterParamIndexOffsets::FreqKT, 1.0f);
			p->mParams.Set01Val(FilterParamIndexOffsets::Q, 0.2f);
		}

		static inline void GenerateDefaults_LFO(OscillatorDevice* p)
		{
			p->mParams.Set01Val(LFOParamIndexOffsets::FrequencyParam, 0.6f);//p->mFrequencyParam.mValue.SetParamValue(M7::gFreqParamKTUnity);//(paramCache[(int)freqParamID], paramCache[(int)freqKTParamID], gSourceFrequencyCenterHz, gSourceFrequencyScale, 0.4f, 1.0f),

			p->mParams.SetEnumValue(LFOParamIndexOffsets::Waveform, OscillatorWaveform::TriTrunc);// p->mWaveform.SetEnumValue(OscillatorWaveform::TriTrunc);
			p->mParams.Set01Val(LFOParamIndexOffsets::Waveshape, 0.5f);// p->mWaveshape.SetParamValue(0.5f);
			p->mParams.SetN11Value(LFOParamIndexOffsets::PhaseOffset, 0);// p->mPhaseOffset.SetN11Value(0);
			p->mParams.SetEnumValue(LFOParamIndexOffsets::FrequencyBasis, TimeBasis::Frequency);
			p->mParams.Set01Val(LFOParamIndexOffsets::Sharpness, 0.5f);
			p->mParams.SetBoolValue(LFOParamIndexOffsets::Restart, false);
		}

		static inline void GenerateDefaults_Audio(OscillatorDevice* p)
		{
			p->mParams.SetBoolValue(OscParamIndexOffsets::Enabled, false);//p->mEnabledParam.SetBoolValue(false);
			p->mParams.SetDecibels(OscParamIndexOffsets::Volume, M7::gUnityVolumeCfg, 0);//p->mVolumeParam.SetDecibels(0);
			p->mParams.SetN11Value(OscParamIndexOffsets::AuxMix, 0);//p->mAuxPanParam.SetN11Value(0);
			p->mParams.Set01Val(OscParamIndexOffsets::FrequencyParam, M7::gFreqParamKTUnity);//p->mFrequencyParam.mValue.SetParamValue(M7::gFreqParamKTUnity);//(paramCache[(int)freqParamID], paramCache[(int)freqKTParamID], gSourceFrequencyCenterHz, gSourceFrequencyScale, 0.4f, 1.0f),
			p->mParams.Set01Val(OscParamIndexOffsets::FrequencyParamKT, 1);//p->mFrequencyParam.mKTValue.SetParamValue(0);
			p->mParams.SetIntValue(OscParamIndexOffsets::PitchSemis, M7::gSourcePitchSemisRange, 0);//p->mPitchSemisParam.SetIntValue(0);// (paramCache[(int)tuneSemisParamID], -gSourcePitchSemisRange, gSourcePitchSemisRange, 0),
			p->mParams.SetN11Value(OscParamIndexOffsets::PitchFine, 0);//p->mPitchFineParam.SetN11Value(0);// (paramCache[(int)tuneFineParamID], 0),
			p->mParams.SetIntValue(OscParamIndexOffsets::KeyRangeMin, M7::gKeyRangeCfg, 0);//p->mKeyRangeMin.SetIntValue(0);// (paramCache[(int)keyRangeMinParamID], 0, 127, 0),
			p->mParams.SetIntValue(OscParamIndexOffsets::KeyRangeMax, M7::gKeyRangeCfg, 127);//p->mKeyRangeMax.SetIntValue(127);// (paramCache[(int)keyRangeMaxParamID], 0, 127, 127)

			p->mParams.SetEnumValue(OscParamIndexOffsets::Waveform, OscillatorWaveform::SineClip);//p->mWaveform.SetEnumValue(OscillatorWaveform::SineClip);
			p->mParams.Set01Val(OscParamIndexOffsets::Waveshape, 0.5f);//p->mWaveshape.SetParamValue(0.5f);
			p->mParams.SetN11Value(OscParamIndexOffsets::PhaseOffset, 0);//p->mPhaseOffset.SetN11Value(0);
			p->mParams.Set01Val(OscParamIndexOffsets::SyncFrequency, M7::gFreqParamKTUnity);//p->mSyncFrequency.mValue.SetParamValue(M7::gFreqParamKTUnity);
			p->mParams.Set01Val(OscParamIndexOffsets::SyncFrequencyKT, 1);//p->mSyncFrequency.mValue.SetParamValue(M7::gFreqParamKTUnity);
			p->mParams.SetRangedValue(OscParamIndexOffsets::FreqMul, 0.0f, gFrequencyMulMax, 1);//p->mFrequencyMul.SetRangedValue(1);
		}

		static inline void GenerateDefaults(SamplerDevice* p)
		{
			auto token = p->mMutex.Enter();
			p->Reset();

			p->mParams.SetBoolValue(SamplerParamIndexOffsets::Enabled, false);//p->mEnabledParam.SetBoolValue(false);
			p->mParams.SetDecibels(SamplerParamIndexOffsets::Volume, M7::gUnityVolumeCfg, 0);//p->mVolumeParam.SetDecibels(0);
			p->mParams.SetN11Value(SamplerParamIndexOffsets::AuxMix, 0);//p->mAuxPanParam.SetN11Value(0);
			p->mParams.Set01Val(SamplerParamIndexOffsets::FreqParam, M7::gFreqParamKTUnity);//p->mFrequencyParam.mValue.SetParamValue(M7::gFreqParamKTUnity);//(paramCache[(int)freqParamID], paramCache[(int)freqKTParamID], gSourceFrequencyCenterHz, gSourceFrequencyScale, 0.4f, 1.0f),
			p->mParams.Set01Val(SamplerParamIndexOffsets::FreqKT, 0);//p->mFrequencyParam.mKTValue.SetParamValue(0);
			p->mParams.SetIntValue(SamplerParamIndexOffsets::TuneSemis, M7::gSourcePitchSemisRange, 0);//p->mPitchSemisParam.SetIntValue(0);// (paramCache[(int)tuneSemisParamID], -gSourcePitchSemisRange, gSourcePitchSemisRange, 0),
			p->mParams.SetN11Value(SamplerParamIndexOffsets::TuneFine, 0);//p->mPitchFineParam.SetN11Value(0);// (paramCache[(int)tuneFineParamID], 0),
			p->mParams.SetIntValue(SamplerParamIndexOffsets::KeyRangeMin, M7::gKeyRangeCfg, 0);//p->mKeyRangeMin.SetIntValue(0);// (paramCache[(int)keyRangeMinParamID], 0, 127, 0),
			p->mParams.SetIntValue(SamplerParamIndexOffsets::KeyRangeMax, M7::gKeyRangeCfg, 127);//p->mKeyRangeMax.SetIntValue(127);// (paramCache[(int)keyRangeMaxParamID], 0, 127, 127)

			p->mParams.SetBoolValue(SamplerParamIndexOffsets::LegatoTrig, true);
			p->mParams.SetBoolValue(SamplerParamIndexOffsets::Reverse, false);
			p->mParams.SetBoolValue(SamplerParamIndexOffsets::ReleaseExitsLoop, true);
			p->mParams.Set01Val(SamplerParamIndexOffsets::SampleStart, 0);
			p->mParams.Set01Val(SamplerParamIndexOffsets::FreqKT, 1);
			p->mParams.SetEnumValue(SamplerParamIndexOffsets::LoopMode, LoopMode::Repeat);
			p->mParams.SetEnumValue(SamplerParamIndexOffsets::LoopSource, LoopBoundaryMode::FromSample);
			p->mParams.SetEnumValue(SamplerParamIndexOffsets::InterpolationType, InterpolationMode::Linear);

			p->mParams.Set01Val(SamplerParamIndexOffsets::LoopStart, 0);
			p->mParams.Set01Val(SamplerParamIndexOffsets::LoopLength, 1);
			p->mParams.SetIntValue(SamplerParamIndexOffsets::BaseNote, M7::gKeyRangeCfg, 60);

			p->mParams.SetEnumValue(SamplerParamIndexOffsets::SampleSource, SampleSource::Embed);
			p->mParams.SetIntValue(SamplerParamIndexOffsets::GmDlsIndex, gGmDlsIndexParamCfg, -1);
			p->mParams.Set01Val(SamplerParamIndexOffsets::Delay, 0);
		}

		//enum class MainParamIndices : uint8_t
		static inline void GenerateMasterParamDefaults(Maj7* p)
		{
			p->mParams.SetDecibels(ParamIndices::MasterVolume, gMasterVolumeCfg, -6);
			p->mParams.SetIntValue(ParamIndices::Unisono, gUnisonoVoiceCfg, 1);//p->mUnisonoVoicesParam.SetIntValue(1);
			p->mParams.SetEnumValue(ParamIndices::VoicingMode, VoiceMode::Polyphonic);//p->mVoicingModeParam.SetEnumValue(VoiceMode::Polyphonic);
			p->mParams.Set01Val(ParamIndices::FMBrightness, 0.5f);//p->mFMBrightness.SetParamValue(0.5f);// FMBrightness,
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
			for (auto& m : p->mpLFOs) {
				GenerateDefaults_LFO(&m->mDevice);
			}

			for (auto& m : p->mpOscillatorDevices) {
				GenerateDefaults_Audio(m);
			}
			for (auto& m : p->mpSamplerDevices) {
				GenerateDefaults(m);
			}
			for (auto& m : p->mpModulations) {
				GenerateDefaults(m);
			}
			for (auto& m : p->mMaj7Voice[0]->mpFilters) {
				GenerateDefaults(m[0]);
			}
			GenerateDefaults_Env(p->mMaj7Voice[0]->mpEnvelopes[M7::gModEnv1Index]);
			GenerateDefaults_Env(p->mMaj7Voice[0]->mpEnvelopes[M7::gModEnv2Index]);
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

	} // namespace M7

} // namespace WaveSabreCore
