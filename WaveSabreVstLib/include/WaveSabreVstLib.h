
#pragma once

#include <Windows.h>

#include <iostream>
#include <iomanip>
#include <array>

#include "WaveSabreVstLib/VstPlug.h"
#include "WaveSabreVstLib/VstEditor.h"

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
		//p->mParams.SetDecibels(LevellerParamIndices::MasterVolume, M7::gVolumeCfg12db, 0);

		//p->mParams.SetRawVal(LevellerParamIndices::LowShelfFreq, 0);
		//p->mParams.SetRawVal(LevellerParamIndices::LowShelfQ, 0);

		//p->mParams.SetFrequencyAssumingNoKeytracking(LevellerParamIndices::Peak1Freq, M7::gFilterFreqConfig, 650);
		//p->mParams.SetDecibels(LevellerParamIndices::Peak1Gain, M7::gVolumeCfg12db, 0);
		//p->mParams.SetRawVal(LevellerParamIndices::Peak1Q, 0);

		//p->mParams.SetFrequencyAssumingNoKeytracking(LevellerParamIndices::Peak2Freq, M7::gFilterFreqConfig, 2000);
		//p->mParams.SetDecibels(LevellerParamIndices::Peak2Gain, M7::gVolumeCfg12db, 0);
		//p->mParams.SetRawVal(LevellerParamIndices::Peak2Q, 0);

		//p->mParams.SetFrequencyAssumingNoKeytracking(LevellerParamIndices::Peak3Freq, M7::gFilterFreqConfig, 7000);
		//p->mParams.SetDecibels(LevellerParamIndices::Peak3Gain, M7::gVolumeCfg12db, 0);
		//p->mParams.SetRawVal(LevellerParamIndices::Peak3Q, 0);

		//p->mParams.SetRawVal(LevellerParamIndices::HighShelfFreq, 1);
		//p->mParams.SetRawVal(LevellerParamIndices::HighShelfQ, 0);
	}

	// take old-style params and adjust them into being compatible with M7 ParamAccessor-style params.
	static inline void AdjustLegacyParams(Leveller* p)
	{
		// in the case of Leveller, there's no adjustment necessary as it was already using ParamAccessor.
	}

	static inline void GenerateDefaults(Echo* p)
	{
		//p->mParams.SetIntValue(Echo::ParamIndices::LeftDelayCoarse, Echo::gDelayCoarseCfg, 3);
		//p->mParams.SetIntValue(Echo::ParamIndices::LeftDelayFine, Echo::gDelayFineCfg, 0);
		//p->mParams.SetIntValue(Echo::ParamIndices::RightDelayCoarse, Echo::gDelayCoarseCfg, 4);
		//p->mParams.SetIntValue(Echo::ParamIndices::RightDelayFine, Echo::gDelayFineCfg, 0);
		//p->mParams.SetFrequencyAssumingNoKeytracking(Echo::ParamIndices::LowCutFreq, M7::gFilterFreqConfig, 20);
		//p->mParams.SetFrequencyAssumingNoKeytracking(Echo::ParamIndices::HighCutFreq, M7::gFilterFreqConfig, 20000 - 20);
		//p->mParams.Set01Val(Echo::ParamIndices::Feedback, 0.5f);
		//p->mParams.Set01Val(Echo::ParamIndices::Cross, 0);
		//p->mParams.Set01Val(Echo::ParamIndices::DryWet, 0.5f);
	}

	//// take old-style params and adjust them into being compatible with M7 ParamAccessor-style params.
	//static inline void AdjustLegacyParams(Echo* p)
	//{
	//	p->mParams.SetIntValue(Echo::ParamIndices::LeftDelayCoarse, Echo::gDelayCoarseCfg, int(p->mParamCache[(int)Echo::ParamIndices::LeftDelayCoarse] * 16));
	//	p->mParams.SetIntValue(Echo::ParamIndices::LeftDelayFine, Echo::gDelayFineCfg, int(p->mParamCache[(int)Echo::ParamIndices::LeftDelayFine] * 200));
	//	p->mParams.SetIntValue(Echo::ParamIndices::RightDelayCoarse, Echo::gDelayCoarseCfg, int(p->mParamCache[(int)Echo::ParamIndices::RightDelayCoarse] * 16));
	//	p->mParams.SetIntValue(Echo::ParamIndices::RightDelayFine, Echo::gDelayFineCfg, int(p->mParamCache[(int)Echo::ParamIndices::RightDelayFine] * 200));

	//	float lowCut = Helpers::ParamToFrequency(p->mParamCache[(int)Echo::ParamIndices::LowCutFreq]);
	//	p->mParams.SetFrequencyAssumingNoKeytracking(Echo::ParamIndices::LowCutFreq, M7::gFilterFreqConfig, lowCut);

	//	float hiCut = Helpers::ParamToFrequency(p->mParamCache[(int)Echo::ParamIndices::HighCutFreq]);
	//	p->mParams.SetFrequencyAssumingNoKeytracking(Echo::ParamIndices::HighCutFreq, M7::gFilterFreqConfig, hiCut);
	//}

	static inline void GenerateDefaults(Maj7Comp* p) {
		// nop until needed.
	}

	static inline void GenerateDefaults(Maj7Sat* p) {
		// nop until needed.
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

	//// see impl of int Device::GetChunk(void **data)
	//template<typename TDevice, size_t paramCount>
	//inline int SetOldVstChunk(TDevice* pDevice, void* data, int byteSize, float(&paramCache)[paramCount])
	//{
	//	if (byteSize != sizeof(float) * paramCount + sizeof(int))
	//		return 0;

	//	auto src = (const float*)data;
	//	for (int i = 0; i < paramCount; ++i) {
	//		paramCache[i] = src[i];
	//	}
	//	AdjustLegacyParams(pDevice);
	//	return byteSize;
	//}


	// reads JSON and populates params. return bytes read.
	// we want to support the old chunk style as well.
	template<typename TDevice, size_t paramCount>
	inline int SetSimpleJSONVstChunk(TDevice* pDevice, const std::string& expectedKeyName, void* data, int byteSize, float(&paramCache)[paramCount], char const* const (&paramNames)[paramCount])
	{
		if (!byteSize) return byteSize;
		const char* pstr = (const char*)data;

		//if (strnlen(pstr, byteSize - 1) >= (size_t)byteSize) {
		//	return SetOldVstChunk(pDevice, data, byteSize, paramCache);
		//}

		clarinoid::MemoryStream memStream{ (const uint8_t*)data, (size_t)byteSize };
		clarinoid::BufferedStream buffering{ memStream };
		clarinoid::TextStream textStream{ buffering };
		clarinoid::JsonVariantReader doc{ textStream };

		// @ root there is exactly 1 KV object.
		auto maj7Obj = doc.GetNextObjectItem();
		//if (maj7Obj.IsEOF()) {
		//	return SetOldVstChunk(pDevice, data, byteSize, paramCache); // empty doc?
		//}
		//if (maj7Obj.mParseResult.IsFailure()) {
		//	return SetOldVstChunk(pDevice, data, byteSize, paramCache);//return ch.mParseResult;
		//}
		//if (expectedKeyName != maj7Obj.mKeyName) {
		//	return SetOldVstChunk(pDevice, data, byteSize, paramCache);
		//}

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

			paramCache[(int)it->second.second] = ch.mNumericValue.Get<float>();
			it->second.first = true;
		}

		// call SetParam so the device has the opportunity to react and recalc if needed.
		// but do it AFTER params have been set so there's not a bunch of undefined values.
		pDevice->SetParam(0, paramCache[0]);

		return byteSize;
	}

	template<size_t paramCount>
	inline int GetSimpleJSONVstChunk(const std::string& keyName, void** data, float const (&paramCache)[paramCount], char const* const (&paramNames)[paramCount])
	{
		clarinoid::MemoryStream memStream;
		clarinoid::BufferedStream buffering{ memStream };
		clarinoid::TextStream textStream{ buffering };
		textStream.mMinified = false;
		clarinoid::JsonVariantWriter doc{ textStream };

		doc.BeginObject();

		auto maj7Element = doc.Object_MakeKey(keyName);
		maj7Element.BeginObject();

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
		std::stringstream ss;

		ss << "static_assert((int)" << paramCountExpr << " == " << paramCount << ", \"param count probably changed and this needs to be regenerated.\");" << std::endl;
		ss << "static constexpr int16_t " << symbolName << "[" << paramCount << "] = {" << std::endl;
		for (size_t i = 0; i < paramCount; ++i) {
			float valf = paramCache[i];
			ss << std::setprecision(20) << "  " << M7::math::Sample32To16(valf) << ", // " << paramNames[i] << " = " << valf << std::endl;
		}
		ss << "};" << std::endl;

		ImGui::SetClipboardText(ss.str().c_str());

		::MessageBoxA(hWnd, "Code copied", "WaveSabre", MB_OK);
	}

	template<size_t paramCount, typename TDevice>
	inline void PopulateStandardMenuBar(HWND hWnd, const std::string& vstName, TDevice* pDevice, VstPlug* pVst, const std::string& defaultsArrayName, const std::string& paramCountExpr, const std::string& jsonKeyName, float const (&paramCache)[paramCount], char const* const (&paramNames)[paramCount])
	{
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

			// the idea of calculating the defaults made sense for things like megalithic Maj7 Synth. but for every other device it's unnecessary.
			//if (ImGui::MenuItem("Init patch (from VST)")) {
			//	GenerateDefaults(pDevice);
			//}

			if (ImGui::MenuItem("Export as C++ defaults to clipboard")) {
				if (IDYES == ::MessageBoxA(hWnd, "Code will be copied to the clipboard, based on 1st item params.", vstName.c_str(), MB_YESNO | MB_ICONQUESTION)) {
					CopyParamCache(hWnd, defaultsArrayName, paramCountExpr, paramCache, paramNames);
				}
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Optimize")) {
				// the idea is to reset any unused parameters to default values, so they end up being 0 in the minified chunk.
				// that compresses better. this is a bit tricky though; i guess i should only do this for like, samplers, oscillators, modulations 1-8, and all envelopes.
				if (IDYES == ::MessageBoxA(hWnd, "Unused objects will be clobbered; are you sure? Do this as a post-processing step before rendering the minified song.", "WaveSabre - Maj7", MB_YESNO | MB_ICONQUESTION)) {
					if (IDYES == ::MessageBoxA(hWnd, "Backup current patch to clipboard?", "WaveSabre - Maj7", MB_YESNO | MB_ICONQUESTION)) {
						CopyPatchToClipboard(hWnd, jsonKeyName, paramCache, paramNames);
						::MessageBoxA(hWnd, "Copied to clipboard... click OK to continue to optimization", "WaveSabre - Maj7", MB_OK);
					}
					auto r1 = pVst->AnalyzeChunkMinification();
					pVst->OptimizeParams();
					auto r2 = pVst->AnalyzeChunkMinification();
					char msg[200];
					sprintf_s(msg, "Done!\r\nBefore: %d bytes; %d nondefaults\r\nAfter: %d bytes; %d nondefaults\r\nShrunk to %d %%",
						r1.compressedSize, r1.nonZeroParams,
						r2.compressedSize, r2.nonZeroParams,
						int(((float)r2.compressedSize / r1.compressedSize) * 100)
					);
					::MessageBoxA(hWnd, msg, "WaveSabre - Maj7", MB_OK);
				}
			}


			if (ImGui::MenuItem("Test chunk roundtrip")) {
				if (IDYES == ::MessageBoxA(hWnd, "Make sure you save first. If there's any problem, it will ruin your patch. This optimizes & serializes the patch in the wavesabre format, then deserializes it, and compares the before/after param cache for errors.", "WaveSabre - Maj7", MB_YESNO | MB_ICONQUESTION))
				{
					if (IDYES == ::MessageBoxA(hWnd, "Backup current patch to clipboard?", "WaveSabre - Maj7", MB_YESNO | MB_ICONQUESTION)) {
						CopyPatchToClipboard(hWnd, jsonKeyName, paramCache, paramNames);
						::MessageBoxA(hWnd, "Copied to clipboard... click OK to continue to optimization", "WaveSabre - Maj7", MB_OK);
					}

					// make a copy of param cache.
					float orig[paramCount];
					for (size_t i = 0; i < paramCount; ++i) {
						orig[i] = paramCache[(int)i];
					}

					// get the wavesabre chunk
					void* data;
					pVst->OptimizeParams();
					int n = pVst->GetMinifiedChunk(&data);

					// apply it back (round trip)
					M7::Deserializer ds{ (const uint8_t*)data };
					pDevice->SetBinary16DiffChunk(ds);
					delete[] data;

					float after[paramCount];
					for (size_t i = 0; i < paramCount; ++i) {
						after[i] = paramCache[(int)i];
					}

					std::vector<std::string> paramReports;

					using vstn = const char[kVstMaxParamStrLen];
					static constexpr vstn paramNames[(int)M7::ParamIndices::NumParams] = MAJ7_PARAM_VST_NAMES;

					for (size_t i = 0; i < paramCount; ++i) {
						if (!M7::math::FloatEquals(orig[i], after[i])) {
							char msg[200];
							sprintf_s(msg, "%s before=%.2f after=%.2f", paramNames[i], orig[i], after[i]);
							paramReports.push_back(msg);
						}
					}

					char msg[200];
					sprintf_s(msg, "Done. %d bytes long. %d params have been messed up.\r\n", n, (int)paramReports.size());
					std::string smsg{ msg };
					smsg += "\r\n";
					for (auto& p : paramReports) {
						smsg += p;
					}
					::MessageBoxA(hWnd, smsg.c_str(), "WaveSabre - Maj7", MB_OK);
				}
			}



			if (ImGui::MenuItem("Analyze minified chunk")) {
				auto r = pVst->AnalyzeChunkMinification();
				std::string s = "uncompressed = ";
				s += std::to_string(r.uncompressedSize);
				s += " bytes.\r\nLZMA compressed this to ";
				s += std::to_string(r.compressedSize);
				s += " bytes.\r\nNon-default params set: ";
				s += std::to_string(r.nonZeroParams);
				s += "\r\nDefault params : ";
				s += std::to_string(r.defaultParams);
				::MessageBoxA(hWnd, s.c_str(), "WaveSabre - Maj7", MB_OK);
			}

			ImGui::EndMenu();
		} // debug menu
	}
} // namespace WaveSabreVstLib
