
#pragma once

#include "WaveSabreVstLib/VstPlug.h"
#include "WaveSabreVstLib/VstEditor.h"

#include "WaveSabreVstLib/ImageManager.h"
#include "WaveSabreVstLib/Serialization.hpp"

namespace WaveSabreVstLib
{

	inline void CopyTextToClipboard(const std::string& s) {

		if (!OpenClipboard(NULL)) return;
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, s.size() + 1);
		if (!hMem) {
			CloseClipboard();
			return;
		}
		char* pMem = (char*)GlobalLock(hMem);
		if (!pMem) {
			GlobalFree(hMem);
			CloseClipboard();
			return;
		}
		strcpy(pMem, s.c_str());
		GlobalUnlock(hMem);

		EmptyClipboard();
		SetClipboardData(CF_TEXT, hMem);

		CloseClipboard();
	}

	inline std::string GetClipboardText() {
		if (!OpenClipboard(NULL)) return {};
		HANDLE hData = GetClipboardData(CF_TEXT);
		if (!hData) {
			CloseClipboard();
			return {};
		}

		char* pMem = (char*)GlobalLock(hData);
		if (pMem == NULL) {
			CloseClipboard();
			return {};
		}

		std::string text = pMem;

		// Unlock the global memory and close the clipboard
		GlobalUnlock(hData);
		CloseClipboard();

		return text;
	}


	inline std::string LoadContentsOfTextFile(const std::string& path)
	{
		HANDLE hFile = CreateFileA(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
			return "";

		LARGE_INTEGER fileSize;
		if (!GetFileSizeEx(hFile, &fileSize))
		{
			CloseHandle(hFile);
			return "";
		}

		char* fileContents = new char[(int)(fileSize.QuadPart + 1)];

		DWORD bytesRead;
		if (!ReadFile(hFile, fileContents, (DWORD)fileSize.QuadPart, &bytesRead, NULL))
		{
			delete[] fileContents;
			CloseHandle(hFile);
			return "";
		}

		fileContents[fileSize.QuadPart] = '\0';

		CloseHandle(hFile);
		std::string result(fileContents);
		delete[] fileContents;
		return result;
	}




	static inline void GenerateDefaults(Leveller* p)
	{
		p->mParams.SetDecibels(LevellerParamIndices::MasterVolume, gLevellerVolumeCfg, 0);

		p->mParams.SetRawVal(LevellerParamIndices::LowCutFreq, 0);
		p->mParams.SetRawVal(LevellerParamIndices::LowCutQ, 0);

		p->mParams.SetFrequencyAssumingNoKeytracking(LevellerParamIndices::Peak1Freq, M7::gFilterFreqConfig, 650);
		p->mParams.SetDecibels(LevellerParamIndices::Peak1Gain, gLevellerBandVolumeCfg, 0);
		p->mParams.SetRawVal(LevellerParamIndices::Peak1Q, 0);

		p->mParams.SetFrequencyAssumingNoKeytracking(LevellerParamIndices::Peak2Freq, M7::gFilterFreqConfig, 2000);
		p->mParams.SetDecibels(LevellerParamIndices::Peak2Gain, gLevellerBandVolumeCfg, 0);
		p->mParams.SetRawVal(LevellerParamIndices::Peak2Q, 0);

		p->mParams.SetFrequencyAssumingNoKeytracking(LevellerParamIndices::Peak3Freq, M7::gFilterFreqConfig, 7000);
		p->mParams.SetDecibels(LevellerParamIndices::Peak3Gain, gLevellerBandVolumeCfg, 0);
		p->mParams.SetRawVal(LevellerParamIndices::Peak3Q, 0);

		p->mParams.SetRawVal(LevellerParamIndices::HighCutFreq, 1);
		p->mParams.SetRawVal(LevellerParamIndices::HighCutQ, 0);
	}

	// take old-style params and adjust them into being compatible with M7 ParamAccessor-style params.
	// in the case of Leveller, there's no adjustment necessary as it was already using ParamAccessor.
	static inline void AdjustLegacyParams(Leveller* p)
	{
		//
	}

	static inline void GenerateDefaults(Echo* p)
	{
		p->mParams.SetIntValue(Echo::ParamIndices::LeftDelayCoarse, Echo::gDelayCoarseCfg, 3);
		p->mParams.SetIntValue(Echo::ParamIndices::LeftDelayFine, Echo::gDelayFineCfg, 0);
		p->mParams.SetIntValue(Echo::ParamIndices::RightDelayCoarse, Echo::gDelayCoarseCfg, 4);
		p->mParams.SetIntValue(Echo::ParamIndices::RightDelayFine, Echo::gDelayFineCfg, 0);
		p->mParams.SetFrequencyAssumingNoKeytracking(Echo::ParamIndices::LowCutFreq, M7::gFilterFreqConfig, 20);
		p->mParams.SetFrequencyAssumingNoKeytracking(Echo::ParamIndices::HighCutFreq, M7::gFilterFreqConfig, 20000 - 20);
		p->mParams.Set01Val(Echo::ParamIndices::Feedback, 0.5f);
		p->mParams.Set01Val(Echo::ParamIndices::Cross, 0);
		p->mParams.Set01Val(Echo::ParamIndices::DryWet, 0.5f);
	}

	// take old-style params and adjust them into being compatible with M7 ParamAccessor-style params.
	static inline void AdjustLegacyParams(Echo* p)
	{
		//void Echo::SetParam(int index, float value)
		//{
		//	switch ((ParamIndices)index)
		//	{
		//	case ParamIndices::LeftDelayCoarse: leftDelayCoarse = (int)(value * 16.0f); break;
		//	case ParamIndices::LeftDelayFine: leftDelayFine = (int)(value * 200.0f); break;
		//	case ParamIndices::RightDelayCoarse: rightDelayCoarse = (int)(value * 16.0f); break;
		//	case ParamIndices::RightDelayFine: rightDelayFine = (int)(value * 200.0f); break;
		//	case ParamIndices::LowCutFreq: lowCutFreq = Helpers::ParamToFrequency(value); break;
		//	case ParamIndices::HighCutFreq: highCutFreq = Helpers::ParamToFrequency(value); break;
		//	case ParamIndices::Feedback: feedback = value; break;
		//	case ParamIndices::Cross: cross = value; break;
		//	case ParamIndices::DryWet: dryWet = value; break;
		//	}
		//}

		p->mParams.SetIntValue(Echo::ParamIndices::LeftDelayCoarse, Echo::gDelayCoarseCfg, int(p->mParamCache[(int)Echo::ParamIndices::LeftDelayCoarse] * 16));
		p->mParams.SetIntValue(Echo::ParamIndices::LeftDelayFine, Echo::gDelayFineCfg, int(p->mParamCache[(int)Echo::ParamIndices::LeftDelayFine] * 200));
		p->mParams.SetIntValue(Echo::ParamIndices::RightDelayCoarse, Echo::gDelayCoarseCfg, int(p->mParamCache[(int)Echo::ParamIndices::RightDelayCoarse] * 16));
		p->mParams.SetIntValue(Echo::ParamIndices::RightDelayFine, Echo::gDelayFineCfg, int(p->mParamCache[(int)Echo::ParamIndices::RightDelayFine] * 200));

		float lowCut = Helpers::ParamToFrequency(p->mParamCache[(int)Echo::ParamIndices::LowCutFreq]);
		p->mParams.SetFrequencyAssumingNoKeytracking(Echo::ParamIndices::LowCutFreq, M7::gFilterFreqConfig, lowCut);

		float hiCut = Helpers::ParamToFrequency(p->mParamCache[(int)Echo::ParamIndices::HighCutFreq]);
		p->mParams.SetFrequencyAssumingNoKeytracking(Echo::ParamIndices::HighCutFreq, M7::gFilterFreqConfig, hiCut);
	}




	// returns params as a minified chunk
	template<size_t paramCount>
	inline int GetSimpleMinifiedChunk(const float(&paramCache)[paramCount], const int16_t(&defaults16)[paramCount], void** data)
	{
		M7::Serializer s;

		for (int i = 0; i < paramCount; ++i) {
			double f = paramCache[i];
			f -= M7::math::Sample16To32Bit(defaults16[i]);
			static constexpr double eps = 0.0000001;
			double af = f < 0 ? -f : f;
			if (af < eps) {
				f = 0;
			}
			s.WriteInt16NormalizedFloat((float)f);
		}

		auto ret = s.DetachBuffer();
		*data = ret.first;
		return (int)ret.second;
	}

	// see impl of int Device::GetChunk(void **data)
	template<typename TDevice, size_t paramCount>
	inline int SetOldVstChunk(TDevice* pDevice, void* data, int byteSize, float(&paramCache)[paramCount])
	{
		if (byteSize != sizeof(float) * paramCount + sizeof(int))
			return 0;

		auto src = (const float*)data;
		for (int i = 0; i < paramCount; ++i) {
			paramCache[i] = src[i];
		}
		AdjustLegacyParams(pDevice);
		return byteSize;
	}


	// reads JSON and populates params. return bytes read.
	// we want to support the old chunk style as well.
	template<typename TDevice, size_t paramCount>
	inline int SetSimpleJSONVstChunk(TDevice* pDevice, const std::string& expectedKeyName, void* data, int byteSize, float(&paramCache)[paramCount], char const* const (&paramNames)[paramCount])
	{
		if (!byteSize) return byteSize;
		const char* pstr = (const char*)data;

		if (strnlen(pstr, byteSize - 1) >= (size_t)byteSize) {
			return SetOldVstChunk(pDevice, data, byteSize, paramCache);
		}

		//using vstn = const char[kVstMaxParamStrLen];
		//static constexpr vstn paramNames[(int)M7::ParamIndices::NumParams] = MAJ7_PARAM_VST_NAMES;

		clarinoid::MemoryStream memStream{ (const uint8_t*)data, (size_t)byteSize };
		clarinoid::BufferedStream buffering{ memStream };
		clarinoid::TextStream textStream{ buffering };
		clarinoid::JsonVariantReader doc{ textStream };

		// @ root there is exactly 1 KV object.
		auto maj7Obj = doc.GetNextObjectItem();
		if (maj7Obj.IsEOF()) {
			return SetOldVstChunk(pDevice, data, byteSize, paramCache); // empty doc?
		}
		if (maj7Obj.mParseResult.IsFailure()) {
			return SetOldVstChunk(pDevice, data, byteSize, paramCache);//return ch.mParseResult;
		}
		if (expectedKeyName != maj7Obj.mKeyName) {
			return SetOldVstChunk(pDevice, data, byteSize, paramCache);
		}

		// todo: read meta data / version / format info?
		doc.Object_Close();

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
		for (size_t i = 0; i < paramCount; ++i) {
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

			paramCache[it->second.second] = ch.mNumericValue.Get<float>();
			it->second.first = true;
		}

		return byteSize;
	}

	template<size_t paramCount>
	inline int GetSimpleJSONVstChunk(const std::string& keyName, void** data, float const (&paramCache)[paramCount], char const* const (&paramNames)[paramCount])
	{
		//using vstn = const char[kVstMaxParamStrLen];
		//static constexpr vstn paramNames[(int)M7::ParamIndices::NumParams] = MAJ7_PARAM_VST_NAMES;

		//auto defaultParamCache = GenerateDefaultParamCache();
		clarinoid::MemoryStream memStream;
		clarinoid::BufferedStream buffering{ memStream };
		clarinoid::TextStream textStream{ buffering };
		textStream.mMinified = false;
		clarinoid::JsonVariantWriter doc{ textStream };

		doc.BeginObject();

		auto maj7Element = doc.Object_MakeKey(keyName);
		maj7Element.BeginObject();
		//maj7Element.Object_MakeKey("Info").WriteStringValue("WaveSabre - Maj7 fork by tenfour"); // mostly this is to avoid a bug in the serializer; it doesn't like empty objects

		auto paramsElement = doc.Object_MakeKey("params");
		paramsElement.BeginObject();

		for (size_t i = 0; i < paramCount; ++i)
		{
			paramsElement.Object_MakeKey(paramNames[i]).WriteNumberValue(paramCache[i]);
		}

		doc.EnsureClosed();
		buffering.flushWrite();

		size_t size = memStream.mBuffer.size() + 1;
		char* out = new char[size];
		strncpy(out, (const char*)memStream.mBuffer.data(), memStream.mBuffer.size());
		out[memStream.mBuffer.size()] = 0; // null term
		*data = out;
		return (VstInt32)size;
	}


	template<size_t paramCount>
	inline void CopyPatchToClipboard(HWND hWnd, const std::string& keyName, float const (&paramCache)[paramCount], char const* const (&paramNames)[paramCount])
	{
		void* data;
		auto size = GetSimpleJSONVstChunk(keyName, &data, paramCache, paramNames);
		if (data && size) {
			CopyTextToClipboard((const char*)data);
		}
		delete[] data;
		::MessageBoxA(hWnd, "Copied patch to clipboard", "WaveSabre", MB_OK | MB_ICONINFORMATION);
	}

	template<size_t paramCount>
	void CopyParamCache(HWND hWnd, const std::string& symbolName, const std::string& paramCountExpr, float const (&paramCache)[paramCount], char const* const (&paramNames)[paramCount])
	{
		//using vstn = const char[kVstMaxParamStrLen];
		//static constexpr vstn paramNames[(int)M7::ParamIndices::NumParams] = MAJ7_PARAM_VST_NAMES;
		std::stringstream ss;
		//ss << "#include <WaveSabreCore/Maj7.hpp>" << std::endl;
		//ss << "namespace WaveSabreCore {" << std::endl;
		//ss << "  namespace M7 {" << std::endl;

		//auto GenerateArray = [&](const std::string& arrayName, size_t count, const std::string& countExpr, int baseParamID) {
		ss << "static_assert((int)" << paramCountExpr << " == " << paramCount << ", \"param count probably changed and this needs to be regenerated.\");" << std::endl;
		ss << "static constexpr int16_t " << symbolName << "[" << paramCount << "] = {" << std::endl;
		for (size_t i = 0; i < paramCount; ++i) {
			//size_t paramID = baseParamID + i;
			float valf = paramCache[i];
			ss << std::setprecision(20) << "  " << M7::math::Sample32To16(valf) << ", // " << paramNames[i] << " = " << valf << std::endl;
		}
		ss << "};" << std::endl;
		//};

		//GenerateArray("gDefaultMasterParams", (int)M7::MainParamIndices::Count, "M7::MainParamIndices::Count", 0);
		//GenerateArray("gDefaultSamplerParams", (int)M7::SamplerParamIndexOffsets::Count, "M7::SamplerParamIndexOffsets::Count", (int)pMaj7->mSamplerDevices[0].mParams.mBaseParamID);
		//GenerateArray("gDefaultModSpecParams", (int)M7::ModParamIndexOffsets::Count, "M7::ModParamIndexOffsets::Count", (int)pMaj7->mModulations[0].mParams.mBaseParamID);
		//GenerateArray("gDefaultLFOParams", (int)M7::LFOParamIndexOffsets::Count, "M7::LFOParamIndexOffsets::Count", (int)pMaj7->mLFOs[0].mDevice.mParams.mBaseParamID);
		//GenerateArray("gDefaultEnvelopeParams", (int)M7::EnvParamIndexOffsets::Count, "M7::EnvParamIndexOffsets::Count", (int)pMaj7->mMaj7Voice[0]->mAllEnvelopes[M7::Maj7::Maj7Voice::Osc1AmpEnvIndex].mParams.mBaseParamID);
		//GenerateArray("gDefaultOscillatorParams", (int)M7::OscParamIndexOffsets::Count, "M7::OscParamIndexOffsets::Count", (int)pMaj7->mOscillatorDevices[0].mParams.mBaseParamID);
		//GenerateArray("gDefaultAuxParams", (int)M7::AuxParamIndexOffsets::Count, "M7::AuxParamIndexOffsets::Count", (int)pMaj7->mAuxDevices[0].mParams.mBaseParamID);

		//ss << "  } // namespace M7" << std::endl;
		//ss << "} // namespace WaveSabreCore" << std::endl;
		ImGui::SetClipboardText(ss.str().c_str());

		::MessageBoxA(hWnd, "Code copied", "WaveSabre", MB_OK);
	}

	template<size_t paramCount, typename TDevice>
	inline void PopulateStandardMenuBar(HWND hWnd, const std::string& vstName, TDevice* pDevice, VstPlug* pVst, const std::string& defaultsArrayName, const std::string& paramCountExpr, const std::string& jsonKeyName, float const (&paramCache)[paramCount], char const* const (&paramNames)[paramCount])
	{
		//Leveller* p = mpLeveller;
		//LevellerVst* pVST = mpLevellerVST;
		if (ImGui::BeginMenu(vstName.c_str())) 
		{
			bool b = false;

			ImGui::Separator();
			if (ImGui::MenuItem("Init patch")) {
				pDevice->LoadDefaults();
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Copy patch to clipboard")) {
				CopyPatchToClipboard(hWnd, jsonKeyName, paramCache, paramNames);
			}

			if (ImGui::MenuItem("Paste patch from clipboard")) {
				std::string s = GetClipboardText();
				if (!s.empty()) {
					pVst->setChunk((void*)s.c_str(), VstInt32(s.size() + 1), false); // const cast oooooooh :/
				}
			}

			ImGui::EndMenu();
		} // "public" menu

		if (ImGui::BeginMenu("Debug")) {

			if (ImGui::MenuItem("Init patch (from VST)")) {
				GenerateDefaults(pDevice);
			}

			if (ImGui::MenuItem("Export as C++ defaults to clipboard")) {
				if (IDYES == ::MessageBoxA(hWnd, "Code will be copied to the clipboard, based on 1st item params.", vstName.c_str(), MB_YESNO | MB_ICONQUESTION)) {
					CopyParamCache(hWnd, defaultsArrayName, paramCountExpr, paramCache, paramNames);
				}
			}

			ImGui::EndMenu();
		} // debug menu
	}






} // namespace WaveSabreVstLib