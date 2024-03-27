
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
	// converts a value to enum, via its integral value. todo: structured with a string value.
	template<typename Tenum>
	Tenum ToEnum(clarinoid::JsonVariantReader& value, Tenum count)
	{
		if (value.mNumericValue.Get<int>() < 0) return (Tenum)0;
		if (value.mNumericValue.Get<int>() >= (int)count) return (Tenum)0;
		return (Tenum)(value.mNumericValue.Get<int>());
	};

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

	// return bool if error
	inline bool VisitChildren(clarinoid::JsonVariantReader& parent, std::function<void(clarinoid::JsonVariantReader&)> handler) {
		while (true) {
			auto ch = parent.GetNextObjectItem();
			if (ch.IsEOF())
				return true; // done.
			if (ch.mParseResult.IsFailure()) {
				break;
			}
			handler(ch);
		}
		return false;
	}

	// reads JSON and populates params. return bytes read.
	// we want to support the old chunk style as well.
	template<typename TDevice, size_t paramCount>
	inline int SetSimpleJSONVstChunk(
		TDevice* pDevice,
		const std::string& expectedKeyName,
		void* data,
		int byteSize,
		float(&paramCache)[paramCount],
		char const* const (&paramNames)[paramCount],
		std::function<void(clarinoid::JsonVariantReader&)> setVSTParam
	)
	{
		if (!byteSize) return byteSize;
		const char* pstr = (const char*)data;

		clarinoid::MemoryStream memStream{ (const uint8_t*)data, (size_t)byteSize };
		clarinoid::BufferedStream buffering{ memStream };
		clarinoid::TextStream textStream{ buffering };
		clarinoid::JsonVariantReader doc{ textStream };

		auto handleM7Params = [&](clarinoid::JsonVariantReader& paramsObj)
		{
			std::map<std::string, std::pair<bool, size_t>> paramMap; // maps string name to whether it's been set + which param index is it.
			for (size_t i = 0; i < paramCount; ++i) {
				paramMap[paramNames[i]] = std::make_pair(false, i);
			}

			if (!VisitChildren(paramsObj, [&](clarinoid::JsonVariantReader& elem)
				{
					auto it = paramMap.find(elem.mKeyName);
					if (it == paramMap.end()) {
						return; // unknown param name. just ignore and allow loading.
					}
					if (it->second.first) {
						return;// return 0; // already set. is this a duplicate? just ignore.
					}

					paramCache[(int)it->second.second] = elem.mNumericValue.Get<float>();
					it->second.first = true;
				})) {
				return 0;
			}

			// call SetParam so the device has the opportunity to react and recalc if needed.
			// but do it AFTER params have been set so there's not a bunch of undefined values.
			pDevice->SetParam(0, paramCache[0]);

			return byteSize;
		};

		return VisitChildren(doc, [&](clarinoid::JsonVariantReader& elem)
			{
				if (elem.mKeyName == expectedKeyName.c_str()) {
					VisitChildren(elem, setVSTParam);
					//setVSTParam(elem);
				}
				else if (elem.mKeyName == "params") {
					handleM7Params(elem);
				}
			});
	}

	template<size_t paramCount>
	inline int GetSimpleJSONVstChunk(
		const std::string& keyName,
		void** data,
		float const (&paramCache)[paramCount],
		char const* const (&paramNames)[paramCount],
		std::function<void(clarinoid::JsonVariantWriter&)> getVSTParams
	)
	{
		clarinoid::MemoryStream memStream;
		clarinoid::BufferedStream buffering{ memStream };
		clarinoid::TextStream textStream{ buffering };
		textStream.mMinified = false;
		clarinoid::JsonVariantWriter doc{ textStream };

		doc.BeginObject();

		auto maj7Element = doc.Object_MakeKey(keyName);
		maj7Element.BeginObject();

		getVSTParams(maj7Element);

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

	

		inline void CopyPatchToClipboard(HWND hWnd, VstPlug* p/*, const std::string& keyName, float const (&paramCache)[paramCount], char const* const (&paramNames)[paramCount]*/)
		{
			void* data = nullptr;
			size_t size = (size_t)p->getChunk(&data, false);

			//void* data;
			//auto size = GetSimpleJSONVstChunk(keyName, &data, paramCache, paramNames);
			if (data && size) {
				CopyTextToClipboard((const char*)data);
				::MessageBoxA(hWnd, "Copied patch to clipboard", "WaveSabre", MB_OK | MB_ICONINFORMATION);
			}
			else {
				::MessageBoxA(hWnd, "Something went wrong... hm.", "WaveSabre", MB_OK | MB_ICONEXCLAMATION);
			}
			delete[] data;
		}

	template<size_t paramCount>
	void CopyParamCache(HWND hWnd, const std::string& symbolName, const std::string& paramCountExpr, float const (&paramCache)[paramCount], char const* const (&paramNames)[paramCount])
	{
		std::stringstream ss;

		ss << "static_assert((int)" << paramCountExpr << " == " << paramCount << ", \"param count probably changed and this needs to be regenerated.\");" << std::endl;
		ss << "static constexpr int16_t " << symbolName << "[(int)" << paramCountExpr << "] = {" << std::endl;
		for (size_t i = 0; i < paramCount; ++i) {
			float valf = paramCache[i];
			ss << std::setprecision(20) << "  " << M7::math::Sample32To16(valf) << ", // " << paramNames[i] << " = " << valf << std::endl;
		}
		ss << "};" << std::endl;

		ImGui::SetClipboardText(ss.str().c_str());

		::MessageBoxA(hWnd, "Code copied", "WaveSabre", MB_OK);
	}

	template<size_t paramCount, typename TDevice>
	inline void PopulateStandardMenuBar(HWND hWnd, const std::string& vstName, TDevice* pDevice, VstPlug* pVst, const std::string& defaultsArrayName, const std::string& paramCountExpr, float const (&paramCache)[paramCount], char const* const (&paramNames)[paramCount])
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
				CopyPatchToClipboard(hWnd, pVst);
				//CopyPatchToClipboard(hWnd, jsonKeyName, paramCache, paramNames);
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
						CopyPatchToClipboard(hWnd, pVst);
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
						CopyPatchToClipboard(hWnd, pVst);
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
					int n = pVst->GetMinifiedChunk(&data, true);

					// apply it back (round trip)
					M7::Deserializer ds{ (const uint8_t*)data };
					pDevice->SetBinary16DiffChunk(ds);
					delete[] data;

					float after[paramCount];
					for (size_t i = 0; i < paramCount; ++i) {
						after[i] = paramCache[(int)i];
					}

					std::vector<std::string> paramReports;

					//using vstn = const char[kVstMaxParamStrLen];
					//static constexpr vstn paramNames[(int)M7::ParamIndices::NumParams] = MAJ7_PARAM_VST_NAMES;

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


	template<int historyViewWidth, int historyViewHeight>
	struct CompressorVis
	{
		bool mShowInputHistory = true;
		bool mShowDetectorHistory = false;
		bool mShowOutputHistory = true;
		bool mShowAttenuationHistory = true;
		bool mShowThresh = true;
		bool mShowLeft = true;
		bool mShowRight = false;

		//static constexpr int historyViewHeight = 110;
		static constexpr float historyViewMinDB = -60;
		HistoryView<9, historyViewWidth, historyViewHeight> mHistoryView;

		void Render(bool enabled, bool showToggles, MonoCompressor& comp, AnalysisStream(&inputAnalysis)[2], AnalysisStream(&detectorAnalysis)[2], AnalysisStream(&attenuationAnalysis)[2], AnalysisStream(&outputAnalysis)[2])
		{
			//INLINE float LookupLUT1D(const float(&mpTable)[N], float x)

			ImGui::BeginGroup();
			static constexpr float lineWidth = 2.0f;

			// ... transfer curve.
			CompressorTransferCurve(comp, { historyViewHeight , historyViewHeight }, detectorAnalysis);

			ImGui::SameLine(); mHistoryView.Render({
				// input
				HistoryViewSeriesConfig{ColorFromHTML("666666", mShowLeft && mShowInputHistory ? 0.6f : 0), lineWidth},
				HistoryViewSeriesConfig{ColorFromHTML("444444", mShowRight && mShowInputHistory ? 0.6f : 0), lineWidth},

				HistoryViewSeriesConfig{ColorFromHTML("ffcc00", mShowLeft && mShowDetectorHistory ? 0.8f : 0), lineWidth},
				HistoryViewSeriesConfig{ColorFromHTML("aa8800", mShowRight && mShowDetectorHistory ? 0.8f : 0), lineWidth},

				HistoryViewSeriesConfig{ColorFromHTML("cc3333", mShowLeft && mShowAttenuationHistory ? 0.8f : 0), lineWidth},
				HistoryViewSeriesConfig{ColorFromHTML("881111", mShowRight && mShowAttenuationHistory ? 0.8f : 0), lineWidth},

				HistoryViewSeriesConfig{ColorFromHTML("4444ff", mShowLeft && mShowOutputHistory ? 0.9f : 0), lineWidth},
				HistoryViewSeriesConfig{ColorFromHTML("0000ff", mShowRight && mShowOutputHistory ? 0.9f : 0), lineWidth},

				HistoryViewSeriesConfig{ColorFromHTML("ffff00", mShowThresh ? 0.2f : 0), 1.0f},
				}, {
				float(inputAnalysis[0].mCurrentPeak),
				float(inputAnalysis[1].mCurrentPeak),
				float(detectorAnalysis[0].mCurrentPeak),
				float(detectorAnalysis[1].mCurrentPeak),
				float(attenuationAnalysis[0].mCurrentPeak),
				float(attenuationAnalysis[1].mCurrentPeak),
				float(outputAnalysis[0].mCurrentPeak),
				float(outputAnalysis[1].mCurrentPeak),
				M7::math::DecibelsToLinear(comp.mThreshold),
				});

			if (showToggles) {
				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, { 2, 0 });
				ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0);

				ToggleButton(&mShowInputHistory, "Input");
				ImGui::SameLine(); ToggleButton(&mShowDetectorHistory, "Detector");
				ImGui::SameLine(); ToggleButton(&mShowAttenuationHistory, "Attenuation");
				ImGui::SameLine(); ToggleButton(&mShowOutputHistory, "Output");

				ImGui::SameLine(0, 40); ToggleButton(&mShowLeft, "L");
				ImGui::SameLine(); ToggleButton(&mShowRight, "R");

				ImGui::PopStyleVar(2); // ImGuiStyleVar_ItemSpacing & ImGuiStyleVar_FrameRounding
			}
			ImGui::EndGroup();

			ImGui::SameLine();

			static const std::vector<VUMeterTick> tickSet = {
					{-3.0f, "3db"},
					{-6.0f, "6"},
					{-12.0f, "12"},
					{-20.0f, "20"},
					{-30.0f, "30"},
					{-40.0f, "40"},
					//{-50.0f, "50"},
			};

			VUMeterConfig mainCfg = {
				{20, historyViewHeight},
				VUMeterLevelMode::Audio,
				VUMeterUnits::Linear,
				-50, 6,
				tickSet,
			};

			VUMeterConfig attenCfg = mainCfg;
			attenCfg.levelMode = VUMeterLevelMode::Attenuation;

			VUMeterConfig disabledCfg = mainCfg;
			disabledCfg.levelMode = VUMeterLevelMode::Disabled;

			ImGui::SameLine(); VUMeter("vu_inp", inputAnalysis[0], inputAnalysis[1], mainCfg);

			ImGui::SameLine(); VUMeter("vu_atten", attenuationAnalysis[0], attenuationAnalysis[1], enabled ? attenCfg : disabledCfg);

			ImGui::SameLine(); VUMeter("vu_outp", outputAnalysis[0], outputAnalysis[1], mainCfg);


		}
	};

} // namespace WaveSabreVstLib
