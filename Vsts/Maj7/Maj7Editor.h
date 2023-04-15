#pragma once

#include <iostream>
#include <fstream>
#include <exception>
#include <Shlwapi.h>

#pragma comment(lib, "shlwapi.lib")

#include <WaveSabreVstLib.h>
#include <WaveSabreCore.h>
#include <WaveSabreCore/Maj7.hpp>

#include "Maj7Vst.h"

using namespace WaveSabreVstLib;
using namespace WaveSabreCore;
//
//template<typename T>
//struct ComPtr
//{
//private:
//	T* mp = nullptr;
//	void Release() {
//		if (mp) {
//			mp->Release();
//			mp = nullptr;
//		}
//	}
//public:
//	~ComPtr() {
//		Release();
//	}
//	void Set(T* p, bool incrRef) {
//		Release();
//		if (p) {
//			mp = p;
//			if (incrRef) {
//				mp->AddRef();
//			}
//		}
//	}
//	T** GetReceivePtr() { // many COM Calls return an interface with a reference for the caller. this receives the object and assumes a ref has already been added to keep.
//		Release();
//		return &mp;
//	}
//	T* get() {
//		return mp;
//	}
//	T* operator ->() {
//		return mp;
//	}
//	bool operator !() {
//		return !!mp;
//	}
//};
//
//// the "default param cache
//static inline void NewPatch(M7::Maj7* pmaj7)
//{
//	//
//}

class Maj7Editor : public VstEditor
{
	Maj7Vst* mMaj7VST;
	M7::Maj7* pMaj7;
	static constexpr int gSamplerWaveformWidth = 700;
	static constexpr int gSamplerWaveformHeight = 100;
	std::vector<std::pair<std::string, int>> mGmDlsOptions;
	bool mShowingGmDlsList = false;
	char mGmDlsFilter[100] = { 0 };


public:
	Maj7Editor(AudioEffect* audioEffect) :
		VstEditor(audioEffect, 1120, 950),
		mMaj7VST((Maj7Vst*)audioEffect),
		pMaj7(((Maj7Vst*)audioEffect)->GetMaj7())
	{
		mGmDlsOptions = LoadGmDlsOptions();
	}

	enum class StatusStyle
	{
		NoStyle,
		Green,
		Warning,
		Error,
	};

	struct SourceStatusText
	{
		std::string mStatus = "Ready.";
		StatusStyle mStatusStyle = StatusStyle::NoStyle;

		//int mSampleLoadSeq = -1;
	};

	SourceStatusText mSourceStatusText[M7::Maj7::gSourceCount];

	bool SetStatus(size_t isrc, StatusStyle style, const std::string& str) {
		mSourceStatusText[isrc].mStatusStyle = style;
		mSourceStatusText[isrc].mStatus = str;
		return false;
	}


	void OnLoadPatch()
	{
		OPENFILENAMEA ofn = { 0 };
		char szFile[MAX_PATH] = { 0 };
		ofn.lStructSize = sizeof(ofn);
		ofn.hwndOwner = this->mCurrentWindow;
		ofn.lpstrFilter = "Maj7 patches (*.majson;*.json;*.js;*.txt)\0*.majson;*.json;*.js;*.txt\0All Files (*.*)\0*.*\0";
		ofn.lpstrFile = szFile;
		ofn.nMaxFile = MAX_PATH;
		ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;
		if (GetOpenFileNameA(&ofn))
		{
			// load file contents
			std::string s = LoadContentsOfTextFile(szFile);
			if (!s.empty()) {
				// todo: use an internal setchunk and report load errors
				mMaj7VST->setChunk((void*)s.c_str(), VstInt32(s.size() + 1), false); // const cast oooooooh :/
				::MessageBoxA(mCurrentWindow, "Loaded successfully.", "WaveSabre - Maj7", MB_OK | MB_ICONINFORMATION);
			}
		}
	}

	bool SaveToFile(const std::string& path, const std::string& textContents)
	{
		if (!textContents.size()) return false;
		HANDLE fileHandle = CreateFileA(path.c_str(), GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (fileHandle == INVALID_HANDLE_VALUE) {
			return false;
		}

		DWORD bytesWritten;
		DWORD bytesToWrite =(DWORD)(textContents.size() * sizeof(textContents[0]));
		BOOL success = WriteFile(fileHandle, textContents.c_str(), bytesToWrite, &bytesWritten, NULL);

		CloseHandle(fileHandle);
		return (success != 0) && (bytesWritten == static_cast<DWORD>(bytesToWrite));
	}

	// Function to show a "Save As" dialog box and get the selected file path.
	std::string GetSaveFilePath(HWND hwnd)
	{
		static constexpr size_t gMaxPathLen = 1000;
		auto buf = std::make_unique<char[]>(gMaxPathLen);

		OPENFILENAMEA ofn = {};
		ofn.lStructSize = sizeof(OPENFILENAME);
		ofn.hwndOwner = hwnd;
		ofn.lpstrFilter = "Maj7 patches (*.majson;*.json;*.js;*.txt)\0*.majson;*.json;*.js;*.txt\0All Files (*.*)\0*.*\0";
		ofn.lpstrDefExt = "majson";
		ofn.lpstrFile = buf.get();
		ofn.nMaxFile = gMaxPathLen;
		ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST;

		//ofn.lpstrFileTitle = defaultFileName;
		//ofn.nMaxFileTitle = lstrlen(defaultFileName);

		if (GetSaveFileName(&ofn) == TRUE)
		{
			std::string ret = buf.get();
			return ret;
		}

		return "";
	}


	void OnSavePatchPatchAs()
	{
		auto path = GetSaveFilePath(this->mCurrentWindow);
		if (path.empty()) return;

		void* data;
		int size;
		std::string contents;
		size = mMaj7VST->getChunk2(&data, false, false);
		if (data && size) {
			contents = (const char*)data;
		}
		delete[] data;

		if (!contents.empty())
		{
			SaveToFile(path, contents);
			::MessageBoxA(mCurrentWindow, "Saved successfully.", "WaveSabre - Maj7", MB_OK | MB_ICONINFORMATION);
		}
	}


	virtual void PopulateMenuBar() override
	{
		if (ImGui::BeginMenu("Maj7")) {
			bool b = false;
			if (ImGui::MenuItem("Panic", nullptr, false)) {
				pMaj7->AllNotesOff();
			}

			ImGui::Separator();
			if (ImGui::BeginMenu("Performance"))
			{
				QUALITY_SETTING_CAPTIONS(captions);
				size_t currentSelectionID = (size_t)M7::GetQualitySetting();
				int newSelection = -1;
				for (size_t i = 0; i < (size_t)M7::QualitySetting::Count; ++i)
				{
					bool selected = (currentSelectionID == i);
					if (ImGui::MenuItem(captions[i], nullptr, &selected)) {
							newSelection = (int)i;
					}
				}
				if (newSelection >= 0) {
					M7::SetQualitySetting((M7::QualitySetting)newSelection);
				}
				ImGui::EndMenu();
			}

			ImGui::Separator();
			ImGui::MenuItem("Show internal modulations", nullptr, &mShowingLockedModulations);

			ImGui::Separator();
			if (ImGui::MenuItem("Init patch")) {
				pMaj7->LoadDefaults();
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Load patch...")) {
				OnLoadPatch();
			}
			if (ImGui::MenuItem("Save patch as...")) {
				OnSavePatchPatchAs();
			}

			ImGui::Separator();

			if (ImGui::MenuItem("Copy patch to clipboard")) {
				CopyPatchToClipboard(false);
			}

			if (ImGui::MenuItem("Paste patch from clipboard")) {
				std::string s = GetClipboardText();
				if (!s.empty()) {
					mMaj7VST->setChunk((void*)s.c_str(), VstInt32(s.size() + 1), false); // const cast oooooooh :/
				}
			}


			ImGui::EndMenu();
		} // maj7 menu

		if (ImGui::BeginMenu("Debug")) {

			ImGui::Separator();
			if (ImGui::MenuItem("Show polyphonic inspector", nullptr, &mShowingInspector)) {
				if (!mShowingInspector) mShowingModulationInspector = false;
			}
			if (ImGui::MenuItem("Show modulation inspector", nullptr, &mShowingModulationInspector)) {
				if (mShowingModulationInspector) mShowingInspector = true;
			}
			ImGui::MenuItem("Show color expl", nullptr, &mShowingColorExp);

			ImGui::Separator();

			if (ImGui::MenuItem("Init patch (from VST)")) {
				GenerateDefaults(pMaj7);
			}

			if (ImGui::MenuItem("Export as Maj7.cpp defaults to clipboard")) {
				if (IDYES == ::MessageBoxA(mCurrentWindow, "A new maj7.cpp will be copied to the clipboard, based on 1st item params.", "WaveSabre - Maj7", MB_YESNO | MB_ICONQUESTION)) {
					CopyParamCache();
				}
			}

			if (ImGui::MenuItem("Copy DIFF patch to clipboard")) {
				CopyPatchToClipboard(true);
			}


			if (ImGui::MenuItem("Test chunk roundtrip")) {
				if (IDYES == ::MessageBoxA(mCurrentWindow, "Sure? This could ruin your patch. But hopefully it doesn't?", "WaveSabre - Maj7", MB_YESNO | MB_ICONQUESTION)) {

					float orig[(size_t)M7::ParamIndices::NumParams];
					for (size_t i = 0; i < (size_t)M7::ParamIndices::NumParams; ++i) {
						orig[i] = pMaj7->GetParam((int)i);
					}

					void* data;
					//int n = pMaj7->GetChunk(&data);
					int n = GetMinifiedChunk(pMaj7, &data);
					pMaj7->SetChunk(data, n);
					delete[] data;


					float after[(size_t)M7::ParamIndices::NumParams];
					for (size_t i = 0; i < (size_t)M7::ParamIndices::NumParams; ++i) {
						after[i] = pMaj7->GetParam((int)i);
					}

					//float delta = 0;
					std::vector<std::string> paramReports;

					using vstn = const char[kVstMaxParamStrLen];
					static constexpr vstn paramNames[(int)M7::ParamIndices::NumParams] = MAJ7_PARAM_VST_NAMES;

					for (size_t i = 0; i < (size_t)M7::ParamIndices::NumParams; ++i) {
						if (!M7::math::FloatEquals(orig[i], after[i])) {
							//delta = abs(orig[i] - after[i]);
							//bdiff++;
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
					::MessageBoxA(mCurrentWindow, smsg.c_str(), "WaveSabre - Maj7", MB_OK);
				}
			}



			if (ImGui::MenuItem("Optimize")) {
				// the idea is to reset any unused parameters to default values, so they end up being 0 in the minified chunk.
				// that compresses better. this is a bit tricky though; i guess i should only do this for like, samplers, oscillators, modulations 1-8, and all envelopes.
				if (IDYES == ::MessageBoxA(mCurrentWindow, "Unused objects will be clobbered; are you sure? Do this as a post-processing step before rendering the minified song.", "WaveSabre - Maj7", MB_YESNO | MB_ICONQUESTION)) {
					if (IDYES == ::MessageBoxA(mCurrentWindow, "Backup current patch to clipboard?", "WaveSabre - Maj7", MB_YESNO | MB_ICONQUESTION)) {
						CopyPatchToClipboard(false);
						::MessageBoxA(mCurrentWindow, "Copied to clipboard... click OK to continue to optimization", "WaveSabre - Maj7", MB_OK);
					}
					auto r1 = AnalyzeChunkMinification(pMaj7);
					OptimizeParams(pMaj7, false);
					auto r2 = AnalyzeChunkMinification(pMaj7);
					char msg[200];
					sprintf_s(msg, "Done!\r\nBefore: %d bytes; %d nondefaults\r\nAfter: %d bytes; %d nondefaults\r\nShrunk to %d %%",
						r1.compressedSize, r1.nonZeroParams,
						r2.compressedSize, r2.nonZeroParams,
						int(((float)r2.compressedSize / r1.compressedSize) * 100)
					);
					::MessageBoxA(mCurrentWindow, msg, "WaveSabre - Maj7", MB_OK);
				}
			}
			if (ImGui::MenuItem("Optimize-Aggressive")) {
				// the idea is to reset any unused parameters to default values, so they end up being 0 in the minified chunk.
				// that compresses better. this is a bit tricky though; i guess i should only do this for like, samplers, oscillators, modulations 1-8, and all envelopes.
				if (IDYES == ::MessageBoxA(mCurrentWindow, "Unused objects will be clobbered; are you sure? Do this as a post-processing step before rendering the minified song.", "WaveSabre - Maj7", MB_YESNO | MB_ICONQUESTION)) {
					if (IDYES == ::MessageBoxA(mCurrentWindow, "Backup current patch to clipboard?", "WaveSabre - Maj7", MB_YESNO | MB_ICONQUESTION)) {
						CopyPatchToClipboard(false);
						::MessageBoxA(mCurrentWindow, "Copied to clipboard... click OK to continue to optimization", "WaveSabre - Maj7", MB_OK);
					}
					auto r1 = AnalyzeChunkMinification(pMaj7);
					OptimizeParams(pMaj7, true);
					auto r2 = AnalyzeChunkMinification(pMaj7);
					char msg[200];
					sprintf_s(msg, "Done!\r\nBefore: %d bytes; %d nondefaults\r\nAfter: %d bytes; %d nondefaults\r\nShrunk to %d %%",
						r1.compressedSize, r1.nonZeroParams,
						r2.compressedSize, r2.nonZeroParams,
						int(((float)r2.compressedSize / r1.compressedSize) * 100)
					);
					::MessageBoxA(mCurrentWindow, msg, "WaveSabre - Maj7", MB_OK);
				}
			}
			if (ImGui::MenuItem("Analyze minified chunk")) {
				auto r = AnalyzeChunkMinification(pMaj7);
				std::string s = "uncompressed = ";
				s += std::to_string(r.uncompressedSize);
				s += " bytes.\r\nLZMA compressed this to ";
				s += std::to_string(r.compressedSize);
				s += " bytes.\r\nNon-default params set: ";
				s += std::to_string(r.nonZeroParams);
				s += "\r\nDefault params : ";
				s += std::to_string(r.defaultParams);
				::MessageBoxA(mCurrentWindow, s.c_str(), "WaveSabre - Maj7", MB_OK);
			}

			ImGui::EndMenu();
		} // debug menu

	}

	void CopyPatchToClipboard(bool diff)
	{
		void* data;
		int size;
		size = mMaj7VST->getChunk2(&data, false, diff);
		if (data && size) {
			CopyTextToClipboard((const char*)data);
		}
		delete[] data;
		::MessageBoxA(mCurrentWindow, "Copied patch to clipboard", "WaveSabre - Maj7", MB_OK | MB_ICONINFORMATION);
	}

	void CopyParamCache()
	{
		using vstn = const char[kVstMaxParamStrLen];
		static constexpr vstn paramNames[(int)M7::ParamIndices::NumParams] = MAJ7_PARAM_VST_NAMES;
		std::stringstream ss;
		ss << "#include <WaveSabreCore/Maj7.hpp>" << std::endl;
		ss << "namespace WaveSabreCore {" << std::endl;
		ss << "  namespace M7 {" << std::endl;

		auto GenerateArray = [&](const std::string& arrayName, size_t count, const std::string& countExpr, int baseParamID) {
			ss << "    static_assert((int)" << countExpr << " == " << count << ", \"param count probably changed and this needs to be regenerated.\");" << std::endl;
			ss << "    const int16_t " << arrayName << "[" << count << "] = {" << std::endl;
			for (size_t i = 0; i < count; ++i) {
				size_t paramID = baseParamID + i;
				float valf = GetEffectX()->getParameter((VstInt32)paramID);
				ss << std::setprecision(20) << "      " << M7::math::Sample32To16(valf) << ", // " << paramNames[paramID] << " = " << valf << std::endl;
			}
			ss << "    };" << std::endl;
		};

		GenerateArray("gDefaultMasterParams", (int)M7::MainParamIndices::Count, "M7::MainParamIndices::Count", 0);
		GenerateArray("gDefaultSamplerParams", (int)M7::SamplerParamIndexOffsets::Count, "M7::SamplerParamIndexOffsets::Count", (int)pMaj7->mSamplerDevices[0].mParams.mBaseParamID);
		GenerateArray("gDefaultModSpecParams", (int)M7::ModParamIndexOffsets::Count, "M7::ModParamIndexOffsets::Count", (int)pMaj7->mModulations[0].mParams.mBaseParamID);
		GenerateArray("gDefaultLFOParams", (int)M7::LFOParamIndexOffsets::Count, "M7::LFOParamIndexOffsets::Count", (int)pMaj7->mLFOs[0].mDevice.mParams.mBaseParamID);
		GenerateArray("gDefaultEnvelopeParams", (int)M7::EnvParamIndexOffsets::Count, "M7::EnvParamIndexOffsets::Count", (int)pMaj7->mMaj7Voice[0]->mAllEnvelopes[M7::Maj7::Maj7Voice::Osc1AmpEnvIndex].mParams.mBaseParamID);
		GenerateArray("gDefaultOscillatorParams", (int)M7::OscParamIndexOffsets::Count, "M7::OscParamIndexOffsets::Count", (int)pMaj7->mOscillatorDevices[0].mParams.mBaseParamID);
		GenerateArray("gDefaultAuxParams", (int)M7::AuxParamIndexOffsets::Count, "M7::AuxParamIndexOffsets::Count", (int)pMaj7->mAuxDevices[0].mParams.mBaseParamID);

		ss << "  } // namespace M7" << std::endl;
		ss << "} // namespace WaveSabreCore" << std::endl;
		ImGui::SetClipboardText(ss.str().c_str());

		::MessageBoxA(mCurrentWindow, "Copied new contents of WaveSabreCore/Maj7.cpp", "WaveSabre Maj7", MB_OK);
	}

	virtual std::string GetMenuBarStatusText() override 
	{
		std::string ret = " Voices:";
		ret += std::to_string(pMaj7->GetCurrentPolyphony());

		auto* v = FindRunningVoice();
		if (v) {
			ret += " ";
			ret += midiNoteToString(v->mNoteInfo.MidiNoteValue);
		}

		return ret;
	};

	// always returns a voice. ideally we would look at envelope states to determine the most suitable, but let's just keep it simple and increase max poly
	M7::Maj7::Maj7Voice* FindRunningVoice() {
		M7::Maj7::Maj7Voice* playingVoiceToReturn = nullptr;// pMaj7->mMaj7Voice[0];
		for (auto* v : pMaj7->mMaj7Voice) {
			if (!v->IsPlaying())
				continue;
			if (!playingVoiceToReturn || (v->mNoteInfo.mSequence > playingVoiceToReturn->mNoteInfo.mSequence))
			{
				playingVoiceToReturn = v;
			}
		}
		return playingVoiceToReturn;
	}


	ColorMod mModulationsColors{ 0.15f, 0.6f, 0.65f, 0.9f, 0.0f };
	ColorMod mModulationDisabledColors{ 0.15f, 0.0f, 0.65f, 0.6f, 0.0f };
	ColorMod mLockedModulationsColors{ 0.50f, 0.6f, 0.75f, 0.9f, 0.0f };

	ColorMod mModEnvelopeColors{ 0.75f, 0.83f, 0.86f, 0.95f, 0.0f };

	ColorMod mLFOColors{ 0.4f, 0.6f, 0.65f, 0.9f, 0.0f };

	ColorMod mAuxLeftColors{ 0.1f, 1, 1, .9f, 0.5f };
	ColorMod mAuxRightColors{ 0.4f, 1, 1, .9f, 0.5f };
	ColorMod mAuxLeftDisabledColors{ 0.1f, 0.25f, .4f, .5f, 0.1f };
	ColorMod mAuxRightDisabledColors{ 0.4f, 0.25f, .4f, .5f, 0.1f };

	ColorMod mOscColors{ 0, 1, 1, 0.9f, 0.0f };
	ColorMod mOscDisabledColors{ 0, .15f, .6f, 0.5f, 0.2f };

	ColorMod mSamplerColors{ 0.55f, 0.8f, .9f, 1.0f, 0.5f };
	ColorMod mSamplerDisabledColors{ 0.55f, 0.15f, .6f, 0.5f, 0.2f };

	ColorMod mFMColors{ 0.92f, 0.6f, 0.75f, 0.9f, 0.0f };

	ColorMod mNopColors;

	bool mShowingInspector = false;
	bool mShowingModulationInspector = false;
	bool mShowingColorExp = false;
	bool mShowingLockedModulations = false;


	virtual void renderImgui() override
	{
		mModulationsColors.EnsureInitialized();
		mModulationDisabledColors.EnsureInitialized();
		mFMColors.EnsureInitialized();
		mLockedModulationsColors.EnsureInitialized();
		mOscColors.EnsureInitialized();
		mOscDisabledColors.EnsureInitialized();
		mAuxLeftColors.EnsureInitialized();
		mAuxRightColors.EnsureInitialized();
		mAuxLeftDisabledColors.EnsureInitialized();
		mAuxRightDisabledColors.EnsureInitialized();
		mSamplerDisabledColors.EnsureInitialized();
		mSamplerColors.EnsureInitialized();
		mLFOColors.EnsureInitialized();
		mModEnvelopeColors.EnsureInitialized();

		{
			auto& style = ImGui::GetStyle();
			style.FramePadding.x = 7;
			style.FramePadding.y = 5;
			style.ItemSpacing.x = 6;
			style.TabRounding = 5;
		}

		// color explorer
		ColorMod::Token colorExplorerToken;
		if (mShowingColorExp) {
			static float colorHueVarAmt = 0;
			static float colorSaturationVarAmt = 1;
			static float colorValueVarAmt = 1;
			static float colorTextVal = 0.9f;
			static float colorTextDisabledVal = 0.5f;
			bool b1 = ImGuiKnobs::Knob("hue", &colorHueVarAmt, -1.0f, 1.0f, 0.0f, 0, gNormalKnobSpeed, gSlowKnobSpeed);
			ImGui::SameLine(); bool b2 = ImGuiKnobs::Knob("sat", &colorSaturationVarAmt, 0.0f, 1.0f, 0.0f, 0, gNormalKnobSpeed, gSlowKnobSpeed);
			ImGui::SameLine(); bool b3 = ImGuiKnobs::Knob("val", &colorValueVarAmt, 0.0f, 1.0f, 0.0f, 0, gNormalKnobSpeed, gSlowKnobSpeed);
			ImGui::SameLine(); bool b4 = ImGuiKnobs::Knob("txt", &colorTextVal, 0.0f, 1.0f, 0.0f, 0, gNormalKnobSpeed, gSlowKnobSpeed);
			ImGui::SameLine(); bool b5 = ImGuiKnobs::Knob("txtD", &colorTextDisabledVal, 0.0f, 1.0f, 0.0f, 0, gNormalKnobSpeed, gSlowKnobSpeed);
			ColorMod xyz{ colorHueVarAmt , colorSaturationVarAmt, colorValueVarAmt, colorTextVal, colorTextDisabledVal };
			xyz.EnsureInitialized();
			colorExplorerToken = std::move(xyz.Push());
		}

		auto runningVoice = FindRunningVoice();

		Maj7ImGuiParamVolume((VstInt32)M7::ParamIndices::MasterVolume, "Volume##hc", M7::gMasterVolumeCfg, -6.0f);
		ImGui::SameLine();
		Maj7ImGuiParamInt((VstInt32)M7::ParamIndices::Unisono, "Unison##mst", M7::gUnisonoVoiceCfg, 1, 0);

		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)M7::ParamIndices::OscillatorDetune, "OscDetune##mst");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)M7::ParamIndices::UnisonoDetune, "UniDetune##mst");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)M7::ParamIndices::OscillatorSpread, "OscPan##mst");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)M7::ParamIndices::UnisonoStereoSpread, "UniPan##mst");

		//ImGui::SameLine();
		//WSImGuiParamKnob((VstInt32)M7::ParamIndices::OscillatorShapeSpread, "OscShape##mst");
		//ImGui::SameLine();
		//WSImGuiParamKnob((VstInt32)M7::ParamIndices::UnisonoShapeSpread, "UniShape##mst");

		ImGui::SameLine(0, 60);
		Maj7ImGuiParamInt((VstInt32)M7::ParamIndices::PitchBendRange, "PB Range##mst", M7::gPitchBendCfg, 2, 0);
		ImGui::SameLine();
		Maj7ImGuiParamEnvTime((VstInt32)M7::ParamIndices::PortamentoTime, "Port##mst", 0.4f);
		ImGui::SameLine();
		Maj7ImGuiParamCurve((VstInt32)M7::ParamIndices::PortamentoCurve, "##portcurvemst", 0.0f, M7CurveRenderStyle::Rising);
		ImGui::SameLine();
		Maj7ImGuiParamInt((VstInt32)M7::ParamIndices::MaxVoices, "MaxVox", M7::gMaxVoicesCfg, 24, 1);

		static constexpr char const* const voiceModeCaptions[] = { "Poly", "Mono" };
		ImGui::SameLine(0, 60);
		Maj7ImGuiParamEnumList<WaveSabreCore::VoiceMode>((VstInt32)M7::ParamIndices::VoicingMode, "VoiceMode##mst", (int)WaveSabreCore::VoiceMode::Count, WaveSabreCore::VoiceMode::Polyphonic, voiceModeCaptions);

		AUX_ROUTE_CAPTIONS(auxRouteCaptions);
		ImGui::SameLine(); Maj7ImGuiParamEnumCombo((VstInt32)M7::ParamIndices::AuxRouting, "AuxRoute", (int)M7::AuxRoute::Count, M7::AuxRoute::TwoTwo, auxRouteCaptions, 100);
		ImGui::SameLine(); Maj7ImGuiParamFloatN11((VstInt32)M7::ParamIndices::AuxWidth, "AuxWidth", 1);

		// osc1
		if (BeginTabBar2("osc", ImGuiTabBarFlags_None))
		{
			static_assert(M7::Maj7::gOscillatorCount == 4, "osc count");
			int isrc = 0;
			Oscillator("Oscillator A", (int)M7::ParamIndices::Osc1Enabled, isrc ++);
			Oscillator("Oscillator B", (int)M7::ParamIndices::Osc2Enabled, isrc ++);
			Oscillator("Oscillator C", (int)M7::ParamIndices::Osc3Enabled, isrc ++);
			Oscillator("Oscillator D", (int)M7::ParamIndices::Osc4Enabled, isrc ++);

			static_assert(M7::Maj7::gSamplerCount == 4, "sampler count");
			Sampler("Sampler 1", pMaj7->mSamplerDevices[0], isrc ++, runningVoice);
			Sampler("Sampler 2", pMaj7->mSamplerDevices[1], isrc ++, runningVoice);
			Sampler("Sampler 3", pMaj7->mSamplerDevices[2], isrc ++, runningVoice);
			Sampler("Sampler 4", pMaj7->mSamplerDevices[3], isrc ++, runningVoice);
			EndTabBarWithColoredSeparator();
		}

		ImGui::BeginTable("##fmaux", 2);
		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		//ImGui::BeginGroup();
		if (BeginTabBar2("FM", ImGuiTabBarFlags_None, 2.2f))
		{
			auto colorModToken = mFMColors.Push();
			static_assert(M7::Maj7::gOscillatorCount == 4, "osc count");
			if (WSBeginTabItem("\"Frequency Modulation\"")) {
				Maj7ImGuiParamFMKnob((VstInt32)M7::ParamIndices::Osc1FMFeedback, "FB1");
				ImGui::SameLine(); Maj7ImGuiParamFMKnob((VstInt32)M7::ParamIndices::FMAmt2to1, "2-1");
				ImGui::SameLine(); Maj7ImGuiParamFMKnob((VstInt32)M7::ParamIndices::FMAmt3to1, "3-1");
				ImGui::SameLine(); Maj7ImGuiParamFMKnob((VstInt32)M7::ParamIndices::FMAmt4to1, "4-1");
				ImGui::SameLine(0, 60);	Maj7ImGuiParamFMKnob((VstInt32)M7::ParamIndices::FMBrightness, "Brightness##mst");

				Maj7ImGuiParamFMKnob((VstInt32)M7::ParamIndices::FMAmt1to2, "1-2");
				ImGui::SameLine(); Maj7ImGuiParamFMKnob((VstInt32)M7::ParamIndices::Osc2FMFeedback, "FB2");
				ImGui::SameLine(); Maj7ImGuiParamFMKnob((VstInt32)M7::ParamIndices::FMAmt3to2, "3-2");
				ImGui::SameLine(); Maj7ImGuiParamFMKnob((VstInt32)M7::ParamIndices::FMAmt4to2, "4-2");

				Maj7ImGuiParamFMKnob((VstInt32)M7::ParamIndices::FMAmt1to3, "1-3");
				ImGui::SameLine(); Maj7ImGuiParamFMKnob((VstInt32)M7::ParamIndices::FMAmt2to3, "2-3");
				ImGui::SameLine(); Maj7ImGuiParamFMKnob((VstInt32)M7::ParamIndices::Osc3FMFeedback, "FB3");
				ImGui::SameLine(); Maj7ImGuiParamFMKnob((VstInt32)M7::ParamIndices::FMAmt4to3, "4-3");

				Maj7ImGuiParamFMKnob((VstInt32)M7::ParamIndices::FMAmt1to4, "1-4");
				ImGui::SameLine(); Maj7ImGuiParamFMKnob((VstInt32)M7::ParamIndices::FMAmt2to4, "2-4");
				ImGui::SameLine(); Maj7ImGuiParamFMKnob((VstInt32)M7::ParamIndices::FMAmt3to4, "3-4");
				ImGui::SameLine(); Maj7ImGuiParamFMKnob((VstInt32)M7::ParamIndices::Osc4FMFeedback, "FB4");

				ImGui::EndTabItem();
			}
			EndTabBarWithColoredSeparator();
		}
		//ImGui::EndGroup();

		ImGui::TableNextColumn();

		// aux
		//ImGui::SameLine();
		if (BeginTabBar2("aux", ImGuiTabBarFlags_None, 2.2f))
		{
			float routingBacking = GetEffectX()->getParameter((int)M7::ParamIndices::AuxRouting);
			M7::AuxRoute routing = M7::EnumParam<M7::AuxRoute>{ routingBacking, M7::AuxRoute::Count }.GetEnumValue();

			ColorMod* auxTabColors[M7::Maj7::gAuxNodeCount] = {
				&mAuxLeftColors,
				&mAuxLeftColors,
				&mAuxLeftColors,
				&mAuxLeftColors,
			};

			ColorMod* auxTabDisabledColors[M7::Maj7::gAuxNodeCount] = {
				&mAuxLeftDisabledColors,
				&mAuxLeftDisabledColors,
				&mAuxLeftDisabledColors,
				&mAuxLeftDisabledColors,
			};

			switch (routing) {
			default:
			case M7::AuxRoute::SerialMono:
			case M7::AuxRoute::FourZero:
				break;
			case M7::AuxRoute::ThreeOne:
				auxTabColors[3] = &mAuxRightColors;
				auxTabDisabledColors[3] = &mAuxRightDisabledColors;
				break;
			case M7::AuxRoute::TwoTwo:
				auxTabColors[2] = &mAuxRightColors;
				auxTabColors[3] = &mAuxRightColors;

				auxTabDisabledColors[2] = &mAuxRightDisabledColors;
				auxTabDisabledColors[3] = &mAuxRightDisabledColors;
				break;
			}

			AuxEffectTab("Aux1", 0, auxTabColors, auxTabDisabledColors);
			AuxEffectTab("Aux2", 1, auxTabColors, auxTabDisabledColors);
			AuxEffectTab("Aux3", 2, auxTabColors, auxTabDisabledColors);
			AuxEffectTab("Aux4", 3, auxTabColors, auxTabDisabledColors);

			EndTabBarWithColoredSeparator();
		}

		ImGui::EndTable();


		// modulation shapes
		if (BeginTabBar2("envelopetabs", ImGuiTabBarFlags_None))
		{
			auto modColorModToken = mModEnvelopeColors.Push();
			if (WSBeginTabItem("Mod env 1"))
			{
				Envelope("Modulation Envelope 1", (int)M7::ParamIndices::Env1DelayTime);
				ImGui::EndTabItem();
			}
			if (WSBeginTabItem("Mod env 2"))
			{
				Envelope("Modulation Envelope 2", (int)M7::ParamIndices::Env2DelayTime);
				ImGui::EndTabItem();
			}
			auto lfoColorModToken = mLFOColors.Push();
			if (WSBeginTabItem("LFO 1"))
			{
				LFO("LFO 1", (int)M7::ParamIndices::LFO1Waveform, 0);
				ImGui::EndTabItem();
			}
			if (WSBeginTabItem("LFO 2"))
			{
				LFO("LFO 2", (int)M7::ParamIndices::LFO2Waveform, 1);
				ImGui::EndTabItem();
			}
			if (WSBeginTabItem("LFO 3"))
			{
				LFO("LFO 3", (int)M7::ParamIndices::LFO3Waveform, 2);
				ImGui::EndTabItem();
			}
			if (WSBeginTabItem("LFO 4"))
			{
				LFO("LFO 4", (int)M7::ParamIndices::LFO4Waveform, 3);
				ImGui::EndTabItem();
			}
			EndTabBarWithColoredSeparator();
		}

		if (BeginTabBar2("modspectabs", ImGuiTabBarFlags_None))
		{
			ModulationSection(0, this->pMaj7->mModulations[0], (int)M7::ParamIndices::Mod1Enabled);
			ModulationSection(1, this->pMaj7->mModulations[1], (int)M7::ParamIndices::Mod2Enabled);
			ModulationSection(2, this->pMaj7->mModulations[2], (int)M7::ParamIndices::Mod3Enabled);
			ModulationSection(3, this->pMaj7->mModulations[3], (int)M7::ParamIndices::Mod4Enabled);
			ModulationSection(4, this->pMaj7->mModulations[4], (int)M7::ParamIndices::Mod5Enabled);
			ModulationSection(5, this->pMaj7->mModulations[5], (int)M7::ParamIndices::Mod6Enabled);
			ModulationSection(6, this->pMaj7->mModulations[6], (int)M7::ParamIndices::Mod7Enabled);
			ModulationSection(7, this->pMaj7->mModulations[7], (int)M7::ParamIndices::Mod8Enabled);
			ModulationSection(8, this->pMaj7->mModulations[8], (int)M7::ParamIndices::Mod9Enabled);
			ModulationSection(9, this->pMaj7->mModulations[9], (int)M7::ParamIndices::Mod10Enabled);
			ModulationSection(10, this->pMaj7->mModulations[10], (int)M7::ParamIndices::Mod11Enabled);
			ModulationSection(11, this->pMaj7->mModulations[11], (int)M7::ParamIndices::Mod12Enabled);
			ModulationSection(12, this->pMaj7->mModulations[12], (int)M7::ParamIndices::Mod13Enabled);
			ModulationSection(13, this->pMaj7->mModulations[13], (int)M7::ParamIndices::Mod14Enabled);
			ModulationSection(14, this->pMaj7->mModulations[14], (int)M7::ParamIndices::Mod15Enabled);
			ModulationSection(15, this->pMaj7->mModulations[15], (int)M7::ParamIndices::Mod16Enabled);
			ModulationSection(16, this->pMaj7->mModulations[16], (int)M7::ParamIndices::Mod17Enabled);
			ModulationSection(17, this->pMaj7->mModulations[17], (int)M7::ParamIndices::Mod18Enabled);

			EndTabBarWithColoredSeparator();
		}

		if (BeginTabBar2("macroknobs", ImGuiTabBarFlags_None))
		{
			if (WSBeginTabItem("Macros"))
			{
				WSImGuiParamKnob((int)M7::ParamIndices::Macro1, "Macro 1");
				ImGui::SameLine(); WSImGuiParamKnob((int)M7::ParamIndices::Macro2, "Macro 2");
				ImGui::SameLine(); WSImGuiParamKnob((int)M7::ParamIndices::Macro3, "Macro 3");
				ImGui::SameLine(); WSImGuiParamKnob((int)M7::ParamIndices::Macro4, "Macro 4");
				ImGui::SameLine(); WSImGuiParamKnob((int)M7::ParamIndices::Macro5, "Macro 5");
				ImGui::SameLine(); WSImGuiParamKnob((int)M7::ParamIndices::Macro6, "Macro 6");
				ImGui::SameLine(); WSImGuiParamKnob((int)M7::ParamIndices::Macro7, "Macro 7");
				ImGui::EndTabItem();
			}
			EndTabBarWithColoredSeparator();
		}

		if (mShowingInspector)
		{
			ImGui::SeparatorText("Voice inspector");

			for (size_t i = 0; i < std::size(pMaj7->mVoices); ++i) {
				auto pv = (M7::Maj7::Maj7Voice*)pMaj7->mVoices[i];
				char txt[200];
				//if (i & 0x1) ImGui::SameLine();
				auto color = ImColor::HSV(0, 0, .3f);
				if (pv->IsPlaying()) {
					auto& ns = pMaj7->mNoteStates[pv->mNoteInfo.MidiNoteValue];
					std::sprintf(txt, "%d u:%d", (int)i, pv->mUnisonVoice);
					//std::sprintf(txt, "%d u:%d %d %c%c #%d", (int)i, pv->mUnisonVoice, ns.MidiNoteValue, ns.mIsPhysicallyHeld ? 'P' : ' ', ns.mIsMusicallyHeld ? 'M' : ' ', ns.mSequence);
					color = ImColor::HSV(2 / 7.0f, .8f, .7f);
				}
				else {
					std::sprintf(txt, "%d:off", (int)i);
				}
				ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)color);
				ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)color);
				ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)color);
				ImGui::Button(txt);
				ImGui::PopStyleColor(3);
				static_assert(M7::Maj7::gOscillatorCount == 4, "osc count");
				//ImGui::SameLine(); ImGui::ProgressBar(pv->mOsc1AmpEnv.GetLastOutputLevel(), ImVec2{ 50, 0 }, "Amp1");
				//ImGui::SameLine(); ImGui::ProgressBar(pv->mOsc2AmpEnv.GetLastOutputLevel(), ImVec2{ 50, 0 }, "Amp2");
				//ImGui::SameLine(); ImGui::ProgressBar(pv->mOsc3AmpEnv.GetLastOutputLevel(), ImVec2{ 50, 0 }, "Amp3");
				//ImGui::SameLine(); ImGui::ProgressBar(pv->mOsc4AmpEnv.GetLastOutputLevel(), ImVec2{ 50, 0 }, "Amp4");
				ImGui::SameLine(); ImGui::ProgressBar(::fabsf(pv->mOscillator1.GetLastSample()), ImVec2{ 50, 0 }, "Osc1");
				ImGui::SameLine(); ImGui::ProgressBar(::fabsf(pv->mOscillator2.GetLastSample()), ImVec2{ 50, 0 }, "Osc2");
				ImGui::SameLine(); ImGui::ProgressBar(::fabsf(pv->mOscillator3.GetLastSample()), ImVec2{ 50, 0 }, "Osc3");
				ImGui::SameLine(); ImGui::ProgressBar(::fabsf(pv->mOscillator4.GetLastSample()), ImVec2{ 50, 0 }, "Osc4");

				if (mShowingModulationInspector) {
					//ImGui::SeparatorText("Modulation Inspector");
					for (size_t idest = 0; idest < std::size(pv->mModMatrix.mDestValues); ++idest)
					{
						char sz[10];
						sprintf_s(sz, "%d", (int)idest);
						if ((idest & 15) == 0) {
						}
						else {
							ImGui::SameLine();
						}
							ImGui::ProgressBar(::fabsf(pv->mModMatrix.mDestValues[idest]), ImVec2{ 50, 0 }, sz);
					}

					// todo...
					//for (size_t idest = 0; idest < std::size(pv->mModMatrix.mDestValueDeltas); ++idest)
					//{
					//	char sz[10];
					//	sprintf_s(sz, "+%d", (int)idest);
					//	if ((idest & 15) == 0) {
					//	}
					//	else {
					//		ImGui::SameLine();
					//	}
					//	float d = ::fabsf(pv->mModMatrix.mDestValueDeltas[idest]);
					//	// d is typically VERY small so 
					//	if (d > 0.00001f) {
					//		d = M7::math::modCurve_xN11_kN11(d + 0.001f, 0.9f);
					//	}
					//	ImGui::ProgressBar(d, ImVec2{50, 0}, sz);
					//}
				}
			}
		}
	}

	void Oscillator(const char* labelWithID, int enabledParamID, int oscID)
	{
		float enabledBacking;
		M7::BoolParam bp{ enabledBacking };
		bp.SetRawParamValue(GetEffectX()->getParameter(enabledParamID));
		ColorMod& cm = bp.GetBoolValue() ? mOscColors : mOscDisabledColors;
		auto token = cm.Push();

		if (WSBeginTabItem(labelWithID)) {
			WSImGuiParamCheckbox(enabledParamID + (int)M7::OscParamIndexOffsets::Enabled, "Enabled");
			ImGui::SameLine(); Maj7ImGuiParamVolume(enabledParamID + (int)M7::OscParamIndexOffsets::Volume, "Volume", M7::gUnityVolumeCfg, 0);

			//ImGui::SameLine(); Maj7ImGuiParamEnumCombo(enabledParamID + (int)M7::OscParamIndexOffsets::Waveform, "Waveform", M7::OscillatorWaveform::Count, 0, gWaveformCaptions);

			ImGui::SameLine(); WaveformParam(enabledParamID + (int)M7::OscParamIndexOffsets::Waveform, enabledParamID + (int)M7::OscParamIndexOffsets::Waveshape, enabledParamID + (int)M7::OscParamIndexOffsets::PhaseOffset, nullptr);
			//ImGui::SameLine(); WaveformGraphic(waveformParam.GetEnumValue(), waveshapeParam.Get01Value());
			//ImGui::SameLine(); WSImGuiParamKnob(enabledParamID + (int)M7::OscParamIndexOffsets::Waveform, "Waveform");
			ImGui::SameLine(); WSImGuiParamKnob(enabledParamID + (int)M7::OscParamIndexOffsets::Waveshape, "Shape");
			ImGui::SameLine(0, 60); Maj7ImGuiParamFrequency(enabledParamID + (int)M7::OscParamIndexOffsets::FrequencyParam, enabledParamID + (int)M7::OscParamIndexOffsets::FrequencyParamKT, "Freq", M7::gSourceFreqConfig, M7::gFreqParamKTUnity);
			ImGui::SameLine(); Maj7ImGuiParamScaledFloat(enabledParamID + (int)M7::OscParamIndexOffsets::FrequencyParamKT, "KT", 0, 1, 1, 1);
			ImGui::SameLine(); Maj7ImGuiParamInt(enabledParamID + (int)M7::OscParamIndexOffsets::PitchSemis, "Transp", M7::gSourcePitchSemisRange, 0, 0);
			ImGui::SameLine(); Maj7ImGuiParamFloatN11(enabledParamID + (int)M7::OscParamIndexOffsets::PitchFine, "FineTune", 0);
			ImGui::SameLine(); Maj7ImGuiParamScaledFloat(enabledParamID + (int)M7::OscParamIndexOffsets::FreqMul, "FreqMul", 0, M7::gFrequencyMulMax, 1, 0);
			ImGui::SameLine(0, 60); WSImGuiParamCheckbox(enabledParamID + (int)M7::OscParamIndexOffsets::PhaseRestart, "PhaseRst");
			ImGui::SameLine(); Maj7ImGuiParamFloatN11(enabledParamID + (int)M7::OscParamIndexOffsets::PhaseOffset, "Phase", 0);
			ImGui::SameLine(0, 60); WSImGuiParamCheckbox(enabledParamID + (int)M7::OscParamIndexOffsets::SyncEnable, "Sync");
			ImGui::SameLine(); Maj7ImGuiParamFrequency(enabledParamID + (int)M7::OscParamIndexOffsets::SyncFrequency, enabledParamID + (int)M7::OscParamIndexOffsets::SyncFrequencyKT, "SyncFreq", M7::gSyncFreqConfig, M7::gFreqParamKTUnity);
			ImGui::SameLine(); Maj7ImGuiParamScaledFloat(enabledParamID + (int)M7::OscParamIndexOffsets::SyncFrequencyKT, "SyncKT", 0, 1, 1, 1);

			ImGui::SameLine(0, 60); Maj7ImGuiParamFloatN11(enabledParamID + (int)M7::OscParamIndexOffsets::AuxMix, "Aux pan", 0);

			//static constexpr char const* const oscAmpEnvSourceCaptions[M7::Maj7::gOscillatorCount] = { "Amp Env 1", "Amp Env 2", "Amp Env 3" };
			//Maj7ImGuiParamEnumCombo(enabledParamID + (int)M7::OscParamIndexOffsets::AmpEnvSource, "Amp env", M7::Maj7::gOscillatorCount, oscID, oscAmpEnvSourceCaptions);

			//M7::IntParam ampSourceParam{ pMaj7->mParamCache[enabledParamID + (int)M7::OscParamIndexOffsets::AmpEnvSource], 0, M7::Maj7::gOscillatorCount - 1 };
			static_assert(M7::Maj7::gOscillatorCount == 4, "osc count");
			M7::ParamIndices ampEnvSources[M7::Maj7::gOscillatorCount] = {
				M7::ParamIndices::Osc1AmpEnvDelayTime,
				M7::ParamIndices::Osc2AmpEnvDelayTime,
				M7::ParamIndices::Osc3AmpEnvDelayTime,
				M7::ParamIndices::Osc4AmpEnvDelayTime,
			};
			auto ampEnvSource = ampEnvSources[oscID];// ampSourceParam.GetIntValue()];

			ImGui::BeginGroup();
			Maj7ImGuiParamMidiNote(enabledParamID + (int)M7::OscParamIndexOffsets::KeyRangeMin, "KeyRangeMin", 0, 0);
			Maj7ImGuiParamMidiNote(enabledParamID + (int)M7::OscParamIndexOffsets::KeyRangeMax, "KeyRangeMax", 127, 127);
			ImGui::EndGroup();

			ImGui::SameLine();
			{
				//ColorMod::Token ampEnvColorModToken{ (ampSourceParam.GetIntValue() != oscID) ? mPinkColors.mNewColors : nullptr };
				Envelope("Amplitude Envelope", (int)ampEnvSource);
			}
			ImGui::EndTabItem();
		}
	}

	void LFO(const char* labelWithID, int waveformParamID, int ilfo)
	{
		ImGui::PushID(labelWithID);
		//if (ImGui::CollapsingHeader(labelWithID)) {
		//WSImGuiParamKnob(waveformParamID + (int)M7::LFOParamIndexOffsets::Waveform, "Waveform");

		float phaseCursor = (float)(this->pMaj7->mLFOs[ilfo].mPhase.mPhase);

		WaveformParam(waveformParamID + (int)M7::LFOParamIndexOffsets::Waveform, waveformParamID + (int)M7::LFOParamIndexOffsets::Waveshape, waveformParamID + (int)M7::LFOParamIndexOffsets::PhaseOffset, &phaseCursor);

		ImGui::SameLine(); WSImGuiParamKnob(waveformParamID + (int)M7::LFOParamIndexOffsets::Waveshape, "Shape");
		ImGui::SameLine(0, 60); Maj7ImGuiParamFrequency(waveformParamID + (int)M7::LFOParamIndexOffsets::FrequencyParam, -1, "Freq", M7::gLFOFreqConfig, 0.4f);
		ImGui::SameLine(0, 60); WSImGuiParamKnob(waveformParamID + (int)M7::LFOParamIndexOffsets::PhaseOffset, "Phase");
		ImGui::SameLine(); WSImGuiParamCheckbox(waveformParamID + (int)M7::LFOParamIndexOffsets::Restart, "Restart");

		ImGui::SameLine(0, 60); Maj7ImGuiParamFrequency(waveformParamID + (int)M7::LFOParamIndexOffsets::Sharpness, -1, "Sharpness", M7::gLFOLPFreqConfig, 0.5f);

		//}
		ImGui::PopID();
	}

	void Envelope(const char* labelWithID, int delayTimeParamID)
	{
		ImGui::PushID(labelWithID);
		//if (ImGui::CollapsingHeader(labelWithID)) {
		Maj7ImGuiParamEnvTime(delayTimeParamID + (int)M7::EnvParamIndexOffsets::DelayTime, "Delay", 0);
		ImGui::SameLine();
		Maj7ImGuiParamEnvTime(delayTimeParamID + (int)M7::EnvParamIndexOffsets::AttackTime, "Attack", 0);
		ImGui::SameLine();
		Maj7ImGuiParamCurve(delayTimeParamID + (int)M7::EnvParamIndexOffsets::AttackCurve, "Curve##attack", 0, M7CurveRenderStyle::Rising);
		ImGui::SameLine();
		Maj7ImGuiParamEnvTime(delayTimeParamID + (int)M7::EnvParamIndexOffsets::HoldTime, "Hold", 0);
		ImGui::SameLine();
		Maj7ImGuiParamEnvTime(delayTimeParamID + (int)M7::EnvParamIndexOffsets::DecayTime, "Decay", .4f);
		ImGui::SameLine();
		Maj7ImGuiParamCurve(delayTimeParamID + (int)M7::EnvParamIndexOffsets::DecayCurve, "Curve##Decay", 0, M7CurveRenderStyle::Falling);
		ImGui::SameLine();
		WSImGuiParamKnob(delayTimeParamID + (int)M7::EnvParamIndexOffsets::SustainLevel, "Sustain");
		ImGui::SameLine();
		Maj7ImGuiParamEnvTime(delayTimeParamID + (int)M7::EnvParamIndexOffsets::ReleaseTime, "Release", 0);
		ImGui::SameLine();
		Maj7ImGuiParamCurve(delayTimeParamID + (int)M7::EnvParamIndexOffsets::ReleaseCurve, "Curve##Release", 0, M7CurveRenderStyle::Falling);
		ImGui::SameLine();
		ImGui::BeginGroup();
		WSImGuiParamCheckbox(delayTimeParamID + (int)M7::EnvParamIndexOffsets::LegatoRestart, "Leg.Restart");

		ImGui::Text("OneShot");
		float backing = GetEffectX()->getParameter(delayTimeParamID + (int)M7::EnvParamIndexOffsets::Mode);
		M7::EnumParam<M7::EnvelopeMode> modeParam{ backing, M7::EnvelopeMode::Count };
		bool boneshot = modeParam.GetEnumValue() == M7::EnvelopeMode::OneShot;
		bool r = ImGui::Checkbox("##cb", &boneshot);
		if (r) {
			modeParam.SetEnumValue(boneshot ? M7::EnvelopeMode::OneShot : M7::EnvelopeMode::Sustain);
			GetEffectX()->setParameterAutomated(delayTimeParamID + (int)M7::EnvParamIndexOffsets::Mode,
				backing);
		}

		ImGui::EndGroup();

		ImGui::SameLine();
		Maj7ImGuiEnvelopeGraphic("graph", delayTimeParamID);
		//}
		ImGui::PopID();
	}

	struct AuxInfo
	{
		int mIndex;
		M7::ParamIndices mEnabledParamID;
		M7::AuxLink mSelfLink;
		M7::ModDestination mModParam2ID;
	};

	static constexpr AuxInfo gAuxInfo[M7::Maj7::gAuxNodeCount] = {
		{0, M7::ParamIndices::Aux1Enabled, M7::AuxLink::Aux1,M7::ModDestination::Aux1Param2 },
		{1, M7::ParamIndices::Aux2Enabled, M7::AuxLink::Aux2,M7::ModDestination::Aux2Param2 },
		{2, M7::ParamIndices::Aux3Enabled, M7::AuxLink::Aux3,M7::ModDestination::Aux3Param2 },
		{3, M7::ParamIndices::Aux4Enabled, M7::AuxLink::Aux4,M7::ModDestination::Aux4Param2 },
	};

	M7::AuxDevice GetDummyAuxDevice(float (&paramValues)[(int)M7::AuxParamIndexOffsets::Count], int iaux)
	{
		auto& auxInfo = gAuxInfo[iaux];
		float tempParamValues[(int)M7::AuxParamIndexOffsets::Count] = {
			GetEffectX()->getParameter((int)auxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Enabled),
			GetEffectX()->getParameter((int)auxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Link),
			GetEffectX()->getParameter((int)auxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Type),
			GetEffectX()->getParameter((int)auxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Param1),
			GetEffectX()->getParameter((int)auxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Param2),
			GetEffectX()->getParameter((int)auxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Param3),
			GetEffectX()->getParameter((int)auxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Param4),
			GetEffectX()->getParameter((int)auxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Param5),
		};
		for (size_t i = 0; i < std::size(paramValues); ++i)
		{
			paramValues[i] = tempParamValues[i];
		}
		return M7::AuxDevice{ paramValues, nullptr, auxInfo.mSelfLink, 0, (int)auxInfo.mModParam2ID };
	}

	std::string GetAuxName(int iaux, std::string idsuffix)
	{
		// (link:Aux1)
		// (Filter)
		float paramValues[(int)M7::AuxParamIndexOffsets::Count];
		auto a = GetDummyAuxDevice(paramValues, iaux);
		auto ret = std::string{ "Aux " } + std::to_string(iaux + 1);
		if (a.IsLinkedExternally()) {
			ret += " (*Aux ";
			ret += std::to_string((int)a.mParams.GetEnumValue<M7::AuxLink>(M7::AuxParamIndexOffsets::Link) + 1);
			ret += ")###";
			ret += idsuffix;
			return ret;
		}
		switch (a.mParams.GetEnumValue<M7::AuxEffectType>(M7::AuxParamIndexOffsets::Type))// a.mEffectType.GetEnumValue())
		{
		case M7::AuxEffectType::BigFilter:
			ret += " (Filter)";
			break;
		//case M7::AuxEffectType::Distortion:
		//	ret += " (Distortion)";
		//	break;
		case M7::AuxEffectType::Bitcrush:
			ret += " (Bitcrush)";
			break;
		default:
			break;
		}
		ret += "###";
		ret += idsuffix;
		return ret;
	}

	// fills the labels with the names of the mod destination params for a given aux.
	// labels points to the 4 modulateable params
	void FillAuxParamNames(std::string* labels, int iaux)
	{
		auto& auxInfo = gAuxInfo[iaux];
		float paramValues[(int)M7::AuxParamIndexOffsets::Count];
		auto a = GetDummyAuxDevice(paramValues, iaux);
		if (a.IsLinkedExternally()) {
			labels[0] += " (shadowed)";
			labels[1] += " (shadowed)";
			labels[2] += " (shadowed)";
			labels[3] += " (shadowed)";
			return;
		}
		char const* const* suffixes;
		switch (a.mParams.GetEnumValue<M7::AuxEffectType>(M7::AuxParamIndexOffsets::Type))
		{
		case M7::AuxEffectType::BigFilter:
		{
			FILTER_AUX_MOD_SUFFIXES(xsuffixes);
			suffixes = xsuffixes;
			break;
		}
		//case M7::AuxEffectType::Distortion:
		//{
		//	DISTORTION_AUX_MOD_SUFFIXES(xsuffixes);
		//	suffixes = xsuffixes;
		//	break;
		//}
		case M7::AuxEffectType::Bitcrush:
		{
			BITCRUSH_AUX_MOD_SUFFIXES(xsuffixes);
			suffixes = xsuffixes;
			break;
		}
		default:
		{
			labels[0] += " (n/a)";
			labels[1] += " (n/a)";
			labels[2] += " (n/a)";
			labels[3] += " (n/a)";
			return;
		}
		} // switch

		labels[0] += suffixes[0];
		labels[1] += suffixes[1];
		labels[2] += suffixes[2];
		labels[3] += suffixes[3];
	}

	std::string GetModulationName(M7::ModulationSpec& spec, int imod)
	{
		char ret[200];

		float enabledBacking = GetEffectX()->getParameter(spec.mParams.GetParamIndex(M7::ModParamIndexOffsets::Enabled));
		M7::BoolParam mEnabled {enabledBacking};
		if (!mEnabled.GetBoolValue()) {
			sprintf_s(ret, "Mod %d###mod%d", imod, imod);
			return ret;
		}

		float srcBacking = GetEffectX()->getParameter(spec.mParams.GetParamIndex(M7::ModParamIndexOffsets::Source));
		M7::EnumParam<M7::ModSource> mSource{ srcBacking, M7::ModSource::Count };
		auto src = mSource.GetEnumValue();
		if (src == M7::ModSource::None) {
			sprintf_s(ret, "Mod %d###mod%d", imod, imod);
			return ret;
		}

		float destBacking = GetEffectX()->getParameter(spec.mParams.GetParamIndex(M7::ModParamIndexOffsets::Destination1));
		M7::EnumParam<M7::ModDestination> mDestination{ destBacking, M7::ModDestination::Count };
		auto dest = mDestination.GetEnumValue();
		if (dest == M7::ModDestination::None) {
			sprintf_s(ret, "Mod %d###mod%d", imod, imod);
			return ret;
		}

		MODSOURCE_SHORT_CAPTIONS(srcCaptions);

		if (spec.mType == M7::ModulationSpecType::SourceAmp) {
			sprintf_s(ret, "%s###mod%d", srcCaptions[(int)src], imod);
			return ret;
		}

		MODDEST_SHORT_CAPTIONS(destCaptions);

		sprintf_s(ret, "%s > %s###mod%d", srcCaptions[(int)src], destCaptions[(int)dest], imod);
		return ret;
	}

	void ModulationSection(int imod, M7::ModulationSpec& spec, int enabledParamID)
	{
		bool isLocked = spec.mType != M7::ModulationSpecType::General;
		if (isLocked && !mShowingLockedModulations) return;

		static constexpr char const* const modSourceCaptions[] = MOD_SOURCE_CAPTIONS;
		std::string modDestinationCaptions[(size_t)M7::ModDestination::Count] = MOD_DEST_CAPTIONS;
		char const* modDestinationCaptionsCstr[(size_t)M7::ModDestination::Count];

		MOD_VALUE_MAPPING_CAPTIONS(modValueMappingCaptions);

		// fix dynamic aux destination names
		FillAuxParamNames(&modDestinationCaptions[(int)M7::ModDestination::Aux1Param2], 0);
		FillAuxParamNames(&modDestinationCaptions[(int)M7::ModDestination::Aux2Param2], 1);
		FillAuxParamNames(&modDestinationCaptions[(int)M7::ModDestination::Aux3Param2], 2);
		FillAuxParamNames(&modDestinationCaptions[(int)M7::ModDestination::Aux4Param2], 3);

		for (size_t i = 0; i < (size_t)M7::ModDestination::Count; ++i)
		{
			modDestinationCaptionsCstr[i] = modDestinationCaptions[i].c_str();
		}

		ColorMod& cm = spec.mParams.GetBoolValue(M7::ModParamIndexOffsets::Enabled) ? (isLocked ? mLockedModulationsColors : mModulationsColors) : mModulationDisabledColors;
		auto token = cm.Push();

		if (WSBeginTabItem(GetModulationName(spec, imod).c_str()))
		{
			ImGui::BeginDisabled(isLocked);
			WSImGuiParamCheckbox((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::Enabled, "Enabled");
			ImGui::EndDisabled();
			ImGui::SameLine();
			Maj7ImGuiParamEnumCombo((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::Source, "Source", (int)M7::ModSource::Count, M7::ModSource::None, modSourceCaptions);
			ImGui::SameLine();
			ImGui::BeginDisabled(isLocked);
			{
				ImGui::BeginGroup();

				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0, 0});

				Maj7ImGuiParamEnumCombo((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::Destination1, "Dest##1", (int)M7::ModDestination::Count, M7::ModDestination::None, modDestinationCaptionsCstr);
				ImGui::SameLine();
				Maj7ImGuiParamFloatN11(enabledParamID + (int)M7::ModParamIndexOffsets::Scale1, "###Scale1", 1, 25.0f);

				Maj7ImGuiParamEnumCombo((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::Destination2, "###Dest2", (int)M7::ModDestination::Count, M7::ModDestination::None, modDestinationCaptionsCstr);
				ImGui::SameLine();
				Maj7ImGuiParamFloatN11(enabledParamID + (int)M7::ModParamIndexOffsets::Scale2, "###Scale2", 1, 25.0f);

				Maj7ImGuiParamEnumCombo((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::Destination3, "###Dest3", (int)M7::ModDestination::Count, M7::ModDestination::None, modDestinationCaptionsCstr);
				ImGui::SameLine();
				Maj7ImGuiParamFloatN11(enabledParamID + (int)M7::ModParamIndexOffsets::Scale3, "###Scale3", 1, 25.0f);

				Maj7ImGuiParamEnumCombo((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::Destination4, "###Dest4", (int)M7::ModDestination::Count, M7::ModDestination::None, modDestinationCaptionsCstr);
				ImGui::SameLine();
				Maj7ImGuiParamFloatN11(enabledParamID + (int)M7::ModParamIndexOffsets::Scale4, "###Scale4", 1, 25.0f);

				ImGui::PopStyleVar();
				ImGui::EndGroup();
			}
			ImGui::EndDisabled();
			ImGui::SameLine();
			//WSImGuiParamCheckbox((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::Invert, "Invert");
			Maj7ImGuiParamEnumCombo((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::ValueMapping, "Mapping", (int)M7::ModValueMapping::Count, M7::ModValueMapping::NoMapping, modValueMappingCaptions);
			ImGui::SameLine();
			Maj7ImGuiParamCurve((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::Curve, "Curve", 0, M7CurveRenderStyle::Rising);

			ImGui::SameLine(0, 60); WSImGuiParamCheckbox((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::AuxEnabled, "SC Enable");
			ColorMod& cmaux = spec.mParams.GetBoolValue(M7::ModParamIndexOffsets::AuxEnabled) ? mNopColors : mModulationDisabledColors;
			auto auxToken = cmaux.Push();
			ImGui::SameLine();
			Maj7ImGuiParamEnumCombo((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::AuxSource, "SC Src", (int)M7::ModSource::Count, M7::ModSource::None, modSourceCaptions);
			ImGui::SameLine();
			WSImGuiParamKnob(enabledParamID + (int)M7::ModParamIndexOffsets::AuxAttenuation, "SC atten");
			ImGui::SameLine();
			//WSImGuiParamCheckbox((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::AuxInvert, "SCInvert");
			Maj7ImGuiParamEnumCombo((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::AuxValueMapping, "AuxMapping", (int)M7::ModValueMapping::Count, M7::ModValueMapping::NoMapping, modValueMappingCaptions);
			ImGui::SameLine();
			Maj7ImGuiParamCurve((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::AuxCurve, "SC Curve", 0, M7CurveRenderStyle::Rising);


			ImGui::SameLine();
			if (ImGui::SmallButton("Copy to...")) {
				ImGui::OpenPopup("copymodto");
			}

			if (ImGui::BeginPopup("copymodto"))
			{
				for (int n = 0; n < (int)M7::gModulationCount; n++)
				{
					ImGui::PushID(n);

					char modName[100];
					sprintf_s(modName, "Mod %d", n);

					if (ImGui::Selectable(GetModulationName(this->pMaj7->mModulations[n], n).c_str()))
					{
						M7::ModulationSpec& srcSpec = spec;
						M7::ModulationSpec& destSpec = this->pMaj7->mModulations[n];
						for (int iparam = 0; iparam < (int)M7::ModParamIndexOffsets::Count; ++ iparam) {

							float p = GetEffectX()->getParameter(srcSpec.mParams.GetParamIndex(iparam));
							GetEffectX()->setParameter(destSpec.mParams.GetParamIndex(iparam), p);
						}
					}
					ImGui::PopID();
				}
				ImGui::EndPopup();
			}


			ImGui::EndTabItem();

		}
	}

	void AuxFilter(const AuxInfo& auxInfo)
	{
		static constexpr char const* const filterModelCaptions[] = FILTER_MODEL_CAPTIONS;
		Maj7ImGuiParamEnumCombo((int)auxInfo.mEnabledParamID + (int)M7::FilterAuxParamIndexOffsets::FilterType, "Type##filt", (int)M7::FilterModel::Count, M7::FilterModel::LP_Moog4, filterModelCaptions);
		ImGui::SameLine(0, 60); Maj7ImGuiParamFrequency((int)auxInfo.mEnabledParamID + (int)M7::FilterAuxParamIndexOffsets::Freq, (int)auxInfo.mEnabledParamID + (int)M7::FilterAuxParamIndexOffsets::FreqKT, "Freq##filt", M7::gFilterFreqConfig, M7::gFreqParamKTUnity);
		ImGui::SameLine(); Maj7ImGuiParamScaledFloat((int)auxInfo.mEnabledParamID + (int)M7::FilterAuxParamIndexOffsets::FreqKT, "KT##filt", 0, 1, 1, 1);
		ImGui::SameLine(); WSImGuiParamKnob((int)auxInfo.mEnabledParamID + (int)M7::FilterAuxParamIndexOffsets::Q, "Q##filt");
		//ImGui::SameLine(0, 60); WSImGuiParamKnob((int)auxInfo.mEnabledParamID + (int)M7::FilterAuxParamIndexOffsets::Saturation, "Saturation##filt");
	}

	void AuxBitcrush(const AuxInfo& auxInfo)
	{
		Maj7ImGuiParamFrequency((int)auxInfo.mEnabledParamID + (int)M7::BitcrushAuxParamIndexOffsets::Freq, (int)auxInfo.mEnabledParamID + (int)M7::BitcrushAuxParamIndexOffsets::FreqKT, "Freq##filt", M7::gBitcrushFreqConfig, M7::gFreqParamKTUnity);
		ImGui::SameLine(); Maj7ImGuiParamScaledFloat((int)auxInfo.mEnabledParamID + (int)M7::BitcrushAuxParamIndexOffsets::FreqKT, "KT##filt", 0, 1, 1, 1);
	}

	//void AuxDistortion(const AuxInfo& auxInfo)
	//{
	//	DISTORTION_STYLE_NAMES(styleNames);
	//	Maj7ImGuiParamEnumCombo((int)auxInfo.mEnabledParamID + (int)M7::DistortionAuxParamIndexOffsets::DistortionStyle, "Style##aux", (int)M7::DistortionStyle::Count, M7::DistortionStyle::Sine, styleNames);
	//	ImGui::SameLine(); Maj7ImGuiParamVolume((int)auxInfo.mEnabledParamID + (int)M7::DistortionAuxParamIndexOffsets::Drive, "Input vol##hc", M7::gDistortionAuxDriveMaxDb, 0.5f);
	//	ImGui::SameLine(); WSImGuiParamKnob((int)auxInfo.mEnabledParamID + (int)M7::DistortionAuxParamIndexOffsets::Threshold, "Threshold##aux");
	//	ImGui::SameLine(); WSImGuiParamKnob((int)auxInfo.mEnabledParamID + (int)M7::DistortionAuxParamIndexOffsets::Shape, "Shape##aux");
	//	
	//	ImGui::SameLine();
	//	ImGuiContext& g = *GImGui;
	//	const ImVec2 size = { 90,60 };
	//	ImGuiWindow* window = ImGui::GetCurrentWindow();
	//	const ImVec2 padding = g.Style.FramePadding;
	//	const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size + padding * 2.0f);
	//	
	//	ImGui::ItemSize(bb);
	//	if (!ImGui::ItemAdd(bb, window->GetID(auxInfo.mIndex))) {
	//		return;
	//	}

	//	AuxDistortionGraphic(bb, auxInfo.mIndex);

	//	ImGui::SameLine(); Maj7ImGuiParamVolume((int)auxInfo.mEnabledParamID + (int)M7::DistortionAuxParamIndexOffsets::OutputVolume, "Output vol##hc", 0, 0.5f);
	//}

	void AuxEffectTab(const char* idSuffix, int iaux, ColorMod* auxTabColors[], ColorMod* auxTabDisabledColors[])
	{
		AUX_LINK_CAPTIONS(auxLinkCaptions);
		AUX_EFFECT_TYPE_CAPTIONS(auxEffectTypeCaptions);
		auto& auxInfo = gAuxInfo[iaux];
		float paramValues[(int)M7::AuxParamIndexOffsets::Count];
		auto a = GetDummyAuxDevice(paramValues, iaux);

		ColorMod& cm = a.mParams.GetBoolValue(M7::AuxParamIndexOffsets::Enabled) ? *auxTabColors[iaux] : *auxTabDisabledColors[iaux];
		auto token = cm.Push();

		std::string labelWithID = GetAuxName(iaux, idSuffix);

		if (WSBeginTabItem(labelWithID.c_str()))
		{
			ImGui::PushID(iaux);

			Maj7ImGuiParamEnumCombo((int)auxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Link, "Link", (int)M7::AuxLink::Count, auxInfo.mSelfLink, auxLinkCaptions);

			{
				ColorMod& cm = (a.IsLinkedExternally()) ? *auxTabDisabledColors[auxInfo.mIndex] : mNopColors;
				auto colorToken = cm.Push();

				ImGui::SameLine(); WSImGuiParamCheckbox((int)auxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Enabled, "Enabled");
				ImGui::SameLine(); Maj7ImGuiParamEnumCombo((int)auxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Type, "Effect", (int)M7::AuxEffectType::Count, M7::AuxEffectType::None, auxEffectTypeCaptions);
			}

			ImGui::SameLine();
			ImGui::BeginGroup();

			if (ImGui::SmallButton("Swap with...")) {
				ImGui::OpenPopup("selectAuxSwap");
			}

			if (ImGui::SmallButton("Copy from...")) {
				ImGui::OpenPopup("selectAuxCopyFrom");
			}
			ImGui::EndGroup();

			if (ImGui::BeginPopup("selectAuxSwap"))
			{
				for (int n = 0; n < (int)M7::Maj7::gAuxNodeCount; n++)
				{
					ImGui::PushID(n);
					if (ImGui::Selectable(GetAuxName(n, "").c_str()))
					{
						// modulations: don't copy modulations because we can't guarantee you expect them to be clobbered, and there's a finite number so just avoid the headache.
						auto& srcAuxInfo = gAuxInfo[n];
						float srcParamValues[(int)M7::AuxParamIndexOffsets::Count];
						auto srcNode = GetDummyAuxDevice(srcParamValues, n);

						// copy from SRC to THIS
						GetEffectX()->setParameter((int)auxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Enabled, srcParamValues[(int)M7::AuxParamIndexOffsets::Enabled]);
						GetEffectX()->setParameter((int)auxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Type, srcParamValues[(int)M7::AuxParamIndexOffsets::Type]);
						GetEffectX()->setParameter((int)auxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Link, srcParamValues[(int)M7::AuxParamIndexOffsets::Link]);
						GetEffectX()->setParameter((int)auxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Param1, srcParamValues[(int)M7::AuxParamIndexOffsets::Param1]);
						GetEffectX()->setParameter((int)auxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Param2, srcParamValues[(int)M7::AuxParamIndexOffsets::Param2]);
						GetEffectX()->setParameter((int)auxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Param3, srcParamValues[(int)M7::AuxParamIndexOffsets::Param3]);
						GetEffectX()->setParameter((int)auxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Param4, srcParamValues[(int)M7::AuxParamIndexOffsets::Param4]);
						GetEffectX()->setParameter((int)auxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Param5, srcParamValues[(int)M7::AuxParamIndexOffsets::Param5]);

						// copy from THIS to SRC
						GetEffectX()->setParameter((int)srcAuxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Enabled, paramValues[(int)M7::AuxParamIndexOffsets::Enabled]);
						GetEffectX()->setParameter((int)srcAuxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Type, paramValues[(int)M7::AuxParamIndexOffsets::Type]);
						GetEffectX()->setParameter((int)srcAuxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Link, paramValues[(int)M7::AuxParamIndexOffsets::Link]);
						GetEffectX()->setParameter((int)srcAuxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Param1, paramValues[(int)M7::AuxParamIndexOffsets::Param1]);
						GetEffectX()->setParameter((int)srcAuxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Param2, paramValues[(int)M7::AuxParamIndexOffsets::Param2]);
						GetEffectX()->setParameter((int)srcAuxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Param3, paramValues[(int)M7::AuxParamIndexOffsets::Param3]);
						GetEffectX()->setParameter((int)srcAuxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Param4, paramValues[(int)M7::AuxParamIndexOffsets::Param4]);
						GetEffectX()->setParameter((int)srcAuxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Param5, paramValues[(int)M7::AuxParamIndexOffsets::Param5]);

						auto srcLink = srcNode.mParams.GetEnumValue<M7::AuxLink>(M7::AuxParamIndexOffsets::Link);// srcNode.mLink.GetEnumValue();
						auto origLink = a.mParams.GetEnumValue<M7::AuxLink>(M7::AuxParamIndexOffsets::Link); //a.mLink.GetEnumValue();

						// if source links to itself, then we should now link to ourself.
						if (srcLink == srcNode.mLinkToSelf) {
							//a.mLink.SetEnumValue(a.mLinkToSelf);
							a.mParams.SetEnumValue(M7::AuxParamIndexOffsets::Link, a.mLinkToSelf);
							GetEffectX()->setParameter((int)auxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Link, a.mParams.GetRawVal(M7::AuxParamIndexOffsets::Link) /*a.mLink.GetRawParamValue()*/);
						}
						else if (srcLink == a.mLinkToSelf) {
							// if you are swapping ORIG with SRC and SRC links to ORIG, now ORIG will need to point to SRC
							a.mParams.SetEnumValue(M7::AuxParamIndexOffsets::Link, a.mLinkToSelf);
							GetEffectX()->setParameter((int)auxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Link, a.mParams.GetRawVal(M7::AuxParamIndexOffsets::Link) /*a.mLink.GetRawParamValue()*/);
						}

						// if we linked to ourself, then source should now link to itself
						if (origLink == a.mLinkToSelf) {
							srcNode.mParams.SetEnumValue(M7::AuxParamIndexOffsets::Link, srcNode.mLinkToSelf);
							GetEffectX()->setParameter((int)srcAuxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Link, srcNode.mParams.GetRawVal(M7::AuxParamIndexOffsets::Link));
						}
						else if (origLink == srcNode.mLinkToSelf) {
							// similar logic as above.
							srcNode.mParams.SetEnumValue(M7::AuxParamIndexOffsets::Link, srcNode.mLinkToSelf);
							GetEffectX()->setParameter((int)srcAuxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Link, srcNode.mParams.GetRawVal(M7::AuxParamIndexOffsets::Link));
						}
					}
					ImGui::PopID();
				}
				ImGui::EndPopup();
			}

			if (ImGui::BeginPopup("selectAuxCopyFrom"))
			{
				for (int n = 0; n < (int)M7::Maj7::gAuxNodeCount; n++)
				{
					ImGui::PushID(n);
					if (ImGui::Selectable(GetAuxName(n, "").c_str()))
					{
						// modulations: don't copy modulations because we can't guarantee you expect them to be clobbered, and there's a finite number so just avoid the headache.
						auto& srcAuxInfo = gAuxInfo[n];
						float srcParamValues[(int)M7::AuxParamIndexOffsets::Count];
						auto srcNode = GetDummyAuxDevice(srcParamValues, n);

						GetEffectX()->setParameter((int)auxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Enabled, GetEffectX()->getParameter((int)srcAuxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Enabled));
						GetEffectX()->setParameter((int)auxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Type, GetEffectX()->getParameter((int)srcAuxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Type));
						GetEffectX()->setParameter((int)auxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Link, GetEffectX()->getParameter((int)srcAuxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Link));
						GetEffectX()->setParameter((int)auxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Param1, GetEffectX()->getParameter((int)srcAuxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Param1));
						GetEffectX()->setParameter((int)auxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Param2, GetEffectX()->getParameter((int)srcAuxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Param2));
						GetEffectX()->setParameter((int)auxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Param3, GetEffectX()->getParameter((int)srcAuxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Param3));
						GetEffectX()->setParameter((int)auxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Param4, GetEffectX()->getParameter((int)srcAuxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Param4));
						GetEffectX()->setParameter((int)auxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Param5, GetEffectX()->getParameter((int)srcAuxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Param5));

						// if the original aux was linking to itself, then we should now link to ourself.
						if (!srcNode.IsLinkedExternally()) {
							//a.mLink.SetEnumValue(a.mLinkToSelf);
							a.mParams.SetEnumValue<M7::AuxLink>(M7::AuxParamIndexOffsets::Link, a.mLinkToSelf);
							GetEffectX()->setParameter((int)auxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Link, a.mParams.GetRawVal(M7::AuxParamIndexOffsets::Link));// a.mLink.GetRawParamValue());
						}
					}
					ImGui::PopID();
				}
				ImGui::EndPopup();
			}

			{
				ColorMod& cm = (a.IsLinkedExternally()) ? *auxTabDisabledColors[auxInfo.mIndex] : mNopColors;
				auto colorToken = cm.Push();

				switch (a.mParams.GetEnumValue<M7::AuxEffectType>(M7::AuxParamIndexOffsets::Type))// a.mEffectType.GetEnumValue())
				{
				default:
					ImGui::TextUnformatted("Nothing to see.");
					break;
				case M7::AuxEffectType::BigFilter:
					AuxFilter(auxInfo);
					break;
				//case M7::AuxEffectType::Distortion:
				//	AuxDistortion(auxInfo);
				//	break;
				case M7::AuxEffectType::Bitcrush:
					AuxBitcrush(auxInfo);
					break;
				}
			}


			ImGui::PopID();
			ImGui::EndTabItem();
		}
	}

	void WaveformGraphic(M7::OscillatorWaveform waveform, float waveshape01, float phaseOffsetN11, ImRect bb, float* cursorPhase)
	{
		OSCILLATOR_WAVEFORM_CAPTIONS(gWaveformCaptions);
		std::unique_ptr<M7::IOscillatorWaveform> pWaveform;

		float freq = 1; // 1 hz at width-in-pixels samplerate will put 1 cycle in frame.
		switch (waveform) {
		default:
		case M7::OscillatorWaveform::Pulse:
			pWaveform.reset(new M7::PulsePWMWaveform);
			break;
		case M7::OscillatorWaveform::PulseTristate:
			pWaveform.reset(new M7::PulseTristateWaveform);
			break;
		case M7::OscillatorWaveform::SawClip:
			pWaveform.reset(new M7::SawClipWaveform);
			break;
		//case M7::OscillatorWaveform::SineAsym:
		//	pWaveform.reset(new M7::SineAsymWaveform);
		//	break;
		case M7::OscillatorWaveform::SineClip:
			pWaveform.reset(new M7::SineClipWaveform);
			break;
		case M7::OscillatorWaveform::SineHarmTrunc:
			pWaveform.reset(new M7::SineHarmTruncWaveform);
			break;
		//case M7::OscillatorWaveform::SineTrunc:
		//	pWaveform.reset(new M7::SineTruncWaveform);
		//	break;
		case M7::OscillatorWaveform::TriClip:
			pWaveform.reset(new M7::TriClipWaveform);
			break;
		case M7::OscillatorWaveform::TriSquare:
			pWaveform.reset(new M7::TriSquareWaveform);
			break;
		case M7::OscillatorWaveform::TriTrunc:
			pWaveform.reset(new M7::TriTruncWaveform);
			break;
		case M7::OscillatorWaveform::VarTrapezoidHard:
			pWaveform.reset(new M7::VarTrapezoidWaveform{ M7::gVarTrapezoidHardSlope });
			break;
		case M7::OscillatorWaveform::VarTrapezoidSoft:
			pWaveform.reset(new M7::VarTrapezoidWaveform { M7::gVarTrapezoidSoftSlope });
			break;
		case M7::OscillatorWaveform::VarTriangle:
			pWaveform.reset(new M7::VarTriWaveform);
			break;
		case M7::OscillatorWaveform::WhiteNoiseSH:
		{
			freq = 8;// 1.0f / 12; // white noise waveforms just have 1 sample level per cycle and always start at 0.
			auto p = new M7::WhiteNoiseWaveform;
			p->mCurrentLevel = p->mCurrentSample = M7::math::randN11();
			pWaveform.reset(p);
				break;
		}
		}

		float innerHeight = bb.GetHeight() - 4;

		// freq & samplerate should be set such that we have `width` samples per 1 cycle.
		// samples per cycle = srate / freq
		pWaveform->SetParams(freq, 0, waveshape01, bb.GetWidth(), M7::OscillatorIntention::LFO);

		ImVec2 outerTL = bb.Min;// ImGui::GetCursorPos();
		ImVec2 outerBR = { outerTL.x + bb.GetWidth(), outerTL.y + bb.GetHeight() };

		auto drawList = ImGui::GetWindowDrawList();

		auto sampleToY = [&](float sample) {
			float c = outerBR.y - float(bb.GetHeight()) * 0.5f;
			float h = float(innerHeight) * 0.5f * sample;
			return c - h;
		};
		auto phaseToX = [&](float phase) {
			return outerTL.x + M7::math::fract(phase) * bb.GetWidth();
		};

		ImGui::RenderFrame(outerTL, outerBR, ImGui::GetColorU32(ImGuiCol_FrameBg), true, 3.0f); // background
		float centerY = sampleToY(0);
		drawList->AddLine({ outerTL.x, centerY }, {outerBR.x, centerY }, ImGui::GetColorU32(ImGuiCol_PlotLines), 2.0f);// center line
		for (size_t iSample = 0; iSample < bb.GetWidth(); ++iSample) {
			pWaveform->OSC_ADVANCE(1, 0);
			float sample = pWaveform->NaiveSample(M7::math::fract(float(iSample) / bb.GetWidth() + phaseOffsetN11));
			sample = (sample + pWaveform->mDCOffset) * pWaveform->mScale;
			drawList->AddLine({outerTL.x + iSample, centerY }, {outerTL.x + iSample, sampleToY(sample)}, ImGui::GetColorU32(ImGuiCol_PlotHistogram), 1);
		}

		if (cursorPhase != nullptr)
		{
			float x = phaseToX(*cursorPhase - phaseOffsetN11);
			static constexpr float gHalfCursorWidth = 2.5f * 0.5f;
			drawList->AddLine({ x- gHalfCursorWidth, outerTL.y }, { x+ gHalfCursorWidth, outerBR.y }, ColorFromHTML("#ff0000"), 2.0f);// center line
		}

		drawList->AddText({ bb.Min.x + 1, bb.Min.y + 1 }, ImGui::GetColorU32(ImGuiCol_TextDisabled), gWaveformCaptions[(int)waveform]);
		drawList->AddText(bb.Min, ImGui::GetColorU32(ImGuiCol_Text), gWaveformCaptions[(int)waveform]);
	}

	bool WaveformButton(ImGuiID id, M7::OscillatorWaveform waveform, float waveshape01, float phaseOffsetN11, float* phaseCursor)
	{
		id += 10;// &= 0x8000; // just to comply with ImGui expectations of IDs never being 0. 
		ImGuiButtonFlags flags = 0;
		ImGuiContext& g = *GImGui;
		const ImVec2 size = {90,60};
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		if (window->SkipItems)
			return false;

		const ImVec2 padding = g.Style.FramePadding;
		const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size + padding * 2.0f);
		ImGui::ItemSize(bb);
		if (!ImGui::ItemAdd(bb, id))
			return false;

		bool hovered, held;

		WaveformGraphic(waveform, waveshape01, phaseOffsetN11, bb, phaseCursor);

		bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, flags);

		return pressed;
	}

	void WaveformParam(int waveformParamID, int waveshapeParamID, int phaseOffsetParamID, float* phaseCursor)
	{
		OSCILLATOR_WAVEFORM_CAPTIONS(gWaveformCaptions);
		M7::EnumParam<M7::OscillatorWaveform> waveformParam(pMaj7->mParamCache[waveformParamID], M7::OscillatorWaveform::Count);
		M7::Float01Param waveshapeParam(pMaj7->mParamCache[waveshapeParamID]);
		M7::OscillatorWaveform selectedWaveform = waveformParam.GetEnumValue();
		float waveshape01 = waveshapeParam.Get01Value();
		M7::FloatN11Param phaseOffsetParam(pMaj7->mParamCache[phaseOffsetParamID]);
		float phaseOffsetN11 = phaseOffsetParam.GetN11Value();

		if (WaveformButton(waveformParamID, selectedWaveform, waveshape01, phaseOffsetN11, phaseCursor)) {
			ImGui::OpenPopup("selectWaveformPopup");
		}
		ImGui::SameLine();
		if (ImGui::BeginPopup("selectWaveformPopup"))
		{
			for (int n = 0; n < (int)M7::OscillatorWaveform::Count; n++)
			{
				M7::OscillatorWaveform wf = (M7::OscillatorWaveform)n;
				ImGui::PushID(n);
				if (WaveformButton(n, wf, waveshapeParam.Get01Value(), phaseOffsetN11, phaseCursor)) {
					float t;
					M7::EnumParam<M7::OscillatorWaveform> tp(t, M7::OscillatorWaveform::Count);
					tp.SetEnumValue(wf);
					GetEffectX()->setParameter(waveformParamID, t);
				}
				ImGui::PopID();
			}
			ImGui::EndPopup();
		}
	} // waveform param

	//void AuxDistortionGraphic(ImRect bb, int iaux)
	//{
	//	float innerHeight = bb.GetHeight() - 4;

	//	ImVec2 outerTL = bb.Min;// ImGui::GetCursorPos();
	//	ImVec2 outerBR = { outerTL.x + bb.GetWidth(), outerTL.y + bb.GetHeight() };

	//	auto drawList = ImGui::GetWindowDrawList();

	//	auto sampleToY = [&](float sample) {
	//		float c = outerBR.y - float(bb.GetHeight()) * 0.5f;
	//		float h = float(innerHeight) * 0.5f * sample;
	//		return c - h;
	//	};

	//	float paramValues[(int)M7::AuxParamIndexOffsets::Count];
	//	M7::AuxNode an = GetDummyAuxNode(paramValues, iaux);
	//	auto pdist = an.CreateEffect(nullptr);
	//	if (pdist == nullptr)
	//	{
	//		//ImGui::Text("Error creating distortion node."); its normal when aux is disabled.
	//		return;
	//	}
	//	size_t nSamples = (size_t)bb.GetWidth();
	//	M7::ModMatrixNode mm;
	//	pdist->AuxBeginBlock(0, (int)nSamples, mm);

	//	std::vector<float> wave;
	//	wave.reserve(nSamples);
	//	float maxRectify = 0.001f;
	//	for (size_t iSample = 0; iSample < nSamples; ++iSample)
	//	{
	//		float s = M7::math::fract(float(iSample) / nSamples) * 2 - 1;
	//		s = pdist->AuxProcessSample(s);
	//		maxRectify = std::max(maxRectify, std::abs(s));
	//		wave.push_back(s);
	//	}

	//	ImGui::RenderFrame(outerTL, outerBR, ImGui::GetColorU32(ImGuiCol_FrameBg), true, 3.0f); // background
	//	float centerY = sampleToY(0);
	//	drawList->AddLine({ outerTL.x, centerY }, { outerBR.x, centerY }, ImGui::GetColorU32(ImGuiCol_PlotLines), 2.0f);// center line
	//	for (size_t iSample = 0; iSample < nSamples; ++iSample)
	//	{
	//		float s = wave[iSample] / maxRectify;
	//		drawList->AddLine({ outerTL.x + iSample, centerY }, { outerTL.x + iSample, sampleToY(s) }, ImGui::GetColorU32(ImGuiCol_PlotHistogram), 1);
	//	}
	//}

	bool LoadSample(const char* path, M7::SamplerDevice& sampler, size_t isrc)
	{
		std::ifstream input(path, std::ios::in | std::ios::binary | std::ios::ate);
		if (!input.is_open()) return SetStatus(isrc, StatusStyle::Error, "Could not open file.");
		size_t inputSize = (size_t)input.tellg();
		auto inputBuf = std::make_unique<char[]>(inputSize);// new unsigned char[(unsigned int)inputSize];
		input.seekg(0, std::ios::beg);
		input.read((char*)inputBuf.get(), inputSize);
		input.close();

		if (*((unsigned int*)inputBuf.get()) != 0x46464952) return SetStatus(isrc, StatusStyle::Error, "Input file missing RIFF header.");
		if (*((unsigned int*)(inputBuf.get() + 4)) != (unsigned int)inputSize - 8) return SetStatus(isrc, StatusStyle::Error, "Input file contains invalid RIFF header.");
		if (*((unsigned int*)(inputBuf.get() + 8)) != 0x45564157) return SetStatus(isrc, StatusStyle::Error, "Input file missing WAVE chunk.");

		if (*((unsigned int*)(inputBuf.get() + 12)) != 0x20746d66) return SetStatus(isrc, StatusStyle::Error, "Input file missing format sub-chunk.");
		if (*((unsigned int*)(inputBuf.get() + 16)) != 16) return SetStatus(isrc, StatusStyle::Error, "Input file is not a PCM waveform.");
		auto inputFormat = (LPWAVEFORMATEX)(inputBuf.get() + 20);
		if (inputFormat->wFormatTag != WAVE_FORMAT_PCM) return SetStatus(isrc, StatusStyle::Error, "Input file is not a PCM waveform.");
		if (inputFormat->nChannels != 1) return SetStatus(isrc, StatusStyle::Error, "Input file is not mono.");
		//if (inputFormat->nSamplesPerSec != Specimen::SampleRate) return SetStatus(isrc, StatusStyle::Error, ("Input file is not " + std::to_string(Specimen::SampleRate) + "hz.").c_str());
		if (inputFormat->wBitsPerSample != sizeof(short) * 8) return SetStatus(isrc, StatusStyle::Error, "Input file is not 16-bit.");

		int chunkPos = 36;
		int chunkSizeBytes;
		while (true)
		{
			if (chunkPos >= (int)inputSize)  return SetStatus(isrc, StatusStyle::Error, "Input file missing data sub-chunk.");
			chunkSizeBytes = *((unsigned int*)(inputBuf.get() + chunkPos + 4));
			if (*((unsigned int*)(inputBuf.get() + chunkPos)) == 0x61746164) break;
			else chunkPos += 8 + chunkSizeBytes;
		}
		int rawDataLength = chunkSizeBytes / 2;

		auto rawData = std::make_unique<short[]>(rawDataLength);

		memcpy(rawData.get(), inputBuf.get() + chunkPos + 8, chunkSizeBytes);

		auto compressedData = std::make_unique<char[]>(chunkSizeBytes);// new char[chunkSizeBytes];

		int waveFormatSize = 0;
		acmMetrics(NULL, ACM_METRIC_MAX_SIZE_FORMAT, &waveFormatSize);
		auto waveFormatBuf = std::make_unique<char[]>(waveFormatSize);
		auto waveFormat = (WAVEFORMATEX*)waveFormatBuf.get();// (new char[waveFormatSize]);
		memset(waveFormat, 0, waveFormatSize);
		waveFormat->wFormatTag = WAVE_FORMAT_GSM610;
		waveFormat->nSamplesPerSec = inputFormat->nSamplesPerSec;// Specimen::SampleRate;

		ACMFORMATCHOOSE formatChoose;
		memset(&formatChoose, 0, sizeof(formatChoose));
		formatChoose.cbStruct = sizeof(formatChoose);
		formatChoose.pwfx = waveFormat;
		formatChoose.cbwfx = waveFormatSize;
		formatChoose.pwfxEnum = waveFormat;
		formatChoose.fdwEnum = ACM_FORMATENUMF_WFORMATTAG | ACM_FORMATENUMF_NSAMPLESPERSEC;

		if (acmFormatChoose(&formatChoose))  return SetStatus(isrc, StatusStyle::Error, "acmFormatChoose failed");

		acmDriverEnum(driverEnumCallback, (DWORD_PTR)waveFormat, NULL);
		HACMDRIVER driver = NULL;
		if (acmDriverOpen(&driver, driverId, 0))  return SetStatus(isrc, StatusStyle::Error, "acmDriverOpen failed");

		HACMSTREAM stream = NULL;
		if (acmStreamOpen(&stream, driver, inputFormat, waveFormat, NULL, NULL, NULL, ACM_STREAMOPENF_NONREALTIME))  return SetStatus(isrc, StatusStyle::Error, "acmStreamOpen failed");

		ACMSTREAMHEADER streamHeader;
		memset(&streamHeader, 0, sizeof(streamHeader));
		streamHeader.cbStruct = sizeof(streamHeader);
		streamHeader.pbSrc = (LPBYTE)rawData.get();
		streamHeader.cbSrcLength = chunkSizeBytes;
		streamHeader.pbDst = (LPBYTE)compressedData.get();
		streamHeader.cbDstLength = chunkSizeBytes;
		if (acmStreamPrepareHeader(stream, &streamHeader, 0))  return SetStatus(isrc, StatusStyle::Error, "acmStreamPrepareHeader failed");
		if (acmStreamConvert(stream, &streamHeader, 0)) return SetStatus(isrc, StatusStyle::Error, "acmStreamConvert failed");

		acmStreamClose(stream, 0);
		acmDriverClose(driver, 0);

		sampler.LoadSample(compressedData.get(), streamHeader.cbDstLengthUsed, chunkSizeBytes, waveFormat, ::PathFindFileNameA(path));

		return SetStatus(isrc, StatusStyle::Green, "Sample loaded successfully.");
	}

	static BOOL __stdcall driverEnumCallback(HACMDRIVERID driverId, DWORD_PTR dwInstance, DWORD fdwSupport)
	{
		ACMDRIVERDETAILS driverDetails;
		driverDetails.cbStruct = sizeof(driverDetails);
		acmDriverDetails(driverId, &driverDetails, 0);

		HACMDRIVER driver = NULL;
		acmDriverOpen(&driver, driverId, 0);

		int waveFormatSize = 0;
		acmMetrics(NULL, ACM_METRIC_MAX_SIZE_FORMAT, &waveFormatSize);
		auto waveFormatBuf = std::make_unique<uint8_t[]>(waveFormatSize);
		auto pWaveFormat = (WAVEFORMATEX*)waveFormatBuf.get();
		ACMFORMATDETAILS formatDetails;
		memset(&formatDetails, 0, sizeof(formatDetails));
		formatDetails.cbStruct = sizeof(formatDetails);
		formatDetails.pwfx = pWaveFormat;
		formatDetails.cbwfx = waveFormatSize;
		formatDetails.dwFormatTag = WAVE_FORMAT_UNKNOWN;
		acmFormatEnum(driver, &formatDetails, formatEnumCallback, dwInstance, NULL);
		return 1;
	}

	static BOOL __stdcall formatEnumCallback(HACMDRIVERID __driverId, LPACMFORMATDETAILS formatDetails, DWORD_PTR dwInstance, DWORD fdwSupport)
	{
		auto waveFormat = (WAVEFORMATEX*)dwInstance;

		if (!memcmp(waveFormat, formatDetails->pwfx, sizeof(WAVEFORMATEX)))
		{
			driverId = __driverId;
			foundWaveFormat = formatDetails->pwfx;
		}

		return 1;
	}

	static HACMDRIVERID driverId;
	static WAVEFORMATEX* foundWaveFormat;


	std::vector<std::pair<std::string, int>> autocomplete(std::string input, const std::vector<std::pair<std::string, int>>& entries) {
		std::vector<std::pair<std::string, int>> suggestions;
		std::transform(input.begin(), input.end(), input.begin(), ::tolower); // convert input to lowercase
		for (auto entry : entries) {
			std::string lowercaseEntry = entry.first;
			std::transform(lowercaseEntry.begin(), lowercaseEntry.end(), lowercaseEntry.begin(), ::tolower); // convert entry to lowercase
			int inputIndex = 0, entryIndex = 0;
			while ((size_t)inputIndex < input.size() && (size_t)entryIndex < lowercaseEntry.size()) {
				if (input[inputIndex] == lowercaseEntry[entryIndex]) {
					inputIndex++;
				}
				entryIndex++;
			}
			if (inputIndex == input.size()) {
				suggestions.push_back(entry);
			}
		}
		return suggestions;
	}

	void Sampler(const char* labelWithID, M7::SamplerDevice& sampler, size_t isrc, Maj7::Maj7Voice* runningVoice)
	{
		ColorMod& cm = sampler.IsEnabled() ? mSamplerColors : mSamplerDisabledColors;
		auto token = cm.Push();
		static constexpr char const* const interpModeNames[] = { "Nearest", "Linear" };
		static constexpr char const* const loopModeNames[] = { "Disabled", "Repeat", "Pingpong" };
		static constexpr char const* const loopBoundaryModeNames[] = { "FromSample", "Manual" };

		if (WSBeginTabItem(labelWithID)) {
			WSImGuiParamCheckbox((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::Enabled), "Enabled");
			ImGui::SameLine(); Maj7ImGuiParamVolume((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::Volume), "Volume", M7::gUnityVolumeCfg, 0);

			ImGui::SameLine(0, 50); Maj7ImGuiParamFrequency((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::FreqParam), (int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::FreqKT), "Freq", M7::gSourceFreqConfig, M7::gFreqParamKTUnity);
			ImGui::SameLine(); Maj7ImGuiParamScaledFloat((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::FreqKT), "KT", 0, 1, 1, 1);
			ImGui::SameLine(); Maj7ImGuiParamInt((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::TuneSemis), "Transp", M7::gSourcePitchSemisRange, 0, 0);
			ImGui::SameLine(); Maj7ImGuiParamFloatN11((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::TuneFine), "FineTune", 0);
			ImGui::SameLine(); Maj7ImGuiParamInt((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::BaseNote), "BaseNote", M7::gKeyRangeCfg, 60, 60);

			ImGui::SameLine(0, 50); WSImGuiParamCheckbox((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::LegatoTrig), "Leg.Trig");

			ImGui::SameLine();
			ImGui::BeginGroup();
			Maj7ImGuiParamMidiNote((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::KeyRangeMin), "KeyMin", 0, 0);
			Maj7ImGuiParamMidiNote((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::KeyRangeMax), "KeyMax", 127, 127);
			ImGui::EndGroup();

			ImGui::SameLine(0, 50); Maj7ImGuiParamEnumList<WaveSabreCore::LoopMode>((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::LoopMode), "LoopMode##mst", (int)WaveSabreCore::LoopMode::NumLoopModes, WaveSabreCore::LoopMode::Repeat, loopModeNames);
			ImGui::SameLine(); Maj7ImGuiParamEnumList<WaveSabreCore::LoopBoundaryMode>((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::LoopSource), "LoopSrc##mst", (int)WaveSabreCore::LoopBoundaryMode::NumLoopBoundaryModes, WaveSabreCore::LoopBoundaryMode::FromSample, loopBoundaryModeNames);
			ImGui::SameLine(); WSImGuiParamCheckbox((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::ReleaseExitsLoop), "Rel");
			ImGui::SameLine(); Maj7ImGuiParamEnumList<WaveSabreCore::InterpolationMode>((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::InterpolationType), "Interp.##mst", (int)WaveSabreCore::InterpolationMode::NumInterpolationModes, WaveSabreCore::InterpolationMode::Linear, interpModeNames);

			ImGui::SameLine(); Maj7ImGuiParamFloatN11((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::AuxMix), "Aux pan", 0);

			ImGui::BeginGroup();
			WSImGuiParamCheckbox((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::Reverse), "Reverse");
			ImGui::SameLine(); WSImGuiParamKnob((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::SampleStart), "SampleStart");
			ImGui::SameLine(); WSImGuiParamKnob((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::LoopStart), "LoopBeg");
			ImGui::SameLine(); WSImGuiParamKnob((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::LoopLength), "LoopLen");
			ImGui::EndGroup();

			ImGui::SameLine();
			SamplerWaveformDisplay(sampler, isrc, runningVoice);

			if (ImGui::SmallButton("Load from file ...")) {
				OPENFILENAMEA ofn = { 0 };
				char szFile[MAX_PATH] = { 0 };
				ofn.lStructSize = sizeof(ofn);
				ofn.hwndOwner = this->mCurrentWindow;
				ofn.lpstrFilter = "All Files (*.*)\0*.*\0";
				ofn.lpstrFile = szFile;
				ofn.nMaxFile = MAX_PATH;
				ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;
				if (GetOpenFileNameA(&ofn))
				{
					mSourceStatusText[isrc].mStatus.clear();
					LoadSample(szFile, sampler, isrc);
				}
			}

			if (!mShowingGmDlsList)
			{
				ImGui::SameLine();
				if (ImGui::SmallButton("Load Gm.Dls sample...")) {
					mShowingGmDlsList = true;
				}
			}
			else
			{
				ImGui::InputText("sampleFilter", mGmDlsFilter, std::size(mGmDlsFilter));
				ImGui::BeginListBox("sample");
				int sampleIndex = (int)GetEffectX()->getParameter((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::GmDlsIndex));
				auto matches = (std::strlen(mGmDlsFilter) > 0) ? autocomplete(mGmDlsFilter, mGmDlsOptions) : mGmDlsOptions;
				for (auto& x : matches) {
					bool isSelected = sampleIndex == x.second;
					if (ImGui::Selectable(x.first.c_str(), &isSelected)) {
						if (isSelected) {
							sampler.LoadGmDlsSample(x.second);
						}
					}
				}
				ImGui::EndListBox();
				if (ImGui::SmallButton("Close")) {
					mShowingGmDlsList = false;
				}
			}

			//ImGui::SameLine();
			auto sampleSource = sampler.mParams.GetEnumValue<M7::SampleSource>(M7::SamplerParamIndexOffsets::SampleSource);
			if (!sampler.mSample) {
				ImGui::Text("No sample loaded");
			}
			else if (sampleSource == M7::SampleSource::Embed) {
				auto* p = static_cast<WaveSabreCore::GsmSample*>(sampler.mSample);
				ImGui::Text("Uncompressed size: %d, compressed to %d (%d%%) / %d Samples / path:%s", p->UncompressedSize, p->CompressedSize, (p->CompressedSize * 100) / p->UncompressedSize, p->SampleLength, sampler.mSamplePath);

				if (ImGui::SmallButton("Clear sample")) {
					sampler.Reset();
				}

			}
			else if (sampleSource == M7::SampleSource::GmDls) {
				auto* p = static_cast<M7::GmDlsSample*>(sampler.mSample);
				const char* name = "(none)";
				if (p->mSampleIndex >= 0 && p->mSampleIndex < M7::gGmDlsSampleCount) {
					name = mGmDlsOptions[p->mSampleIndex].first.c_str();
				}
				ImGui::Text("GmDls : %s (%d)", name, p->mSampleIndex);
			}
			else {
				ImGui::Text("--");
			}

			ImGui::SameLine();
			switch (mSourceStatusText[isrc].mStatusStyle)
			{
			case StatusStyle::NoStyle:
				ImGui::Text("%s", mSourceStatusText[isrc].mStatus.c_str());
				break;
			case StatusStyle::Error:
				ImGui::TextColored(ImColor::HSV(0 / 7.0f, 0.6f, 0.6f), "%s", mSourceStatusText[isrc].mStatus.c_str());
				break;
			case StatusStyle::Warning:
				ImGui::TextColored(ImColor::HSV(1 / 7.0f, 0.6f, 0.6f), "%s", mSourceStatusText[isrc].mStatus.c_str());
				break;
			default:
			case StatusStyle::Green:
				ImGui::TextColored(ImColor::HSV(2 / 7.0f, 0.6f, 0.6f), "%s", mSourceStatusText[isrc].mStatus.c_str());
				break;
			}

			{
				//ColorMod::Token ampEnvColorModToken{ (ampSourceParam.GetIntValue() != oscID) ? mPinkColors.mNewColors : nullptr };
				Envelope("Amplitude Envelope", (int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::AmpEnvDelayTime));
			}
			ImGui::EndTabItem();
		} // if begin tab item
	} // sampler()



	void WaveformGraphic(size_t isrc, float height, const std::vector<std::pair<float, float>>& peaks, float startPos01, float loopStart01, float loopLen01, float cursor)
	{
		ImGuiContext& g = *GImGui;
		size_t nSamples = peaks.size();
		const ImVec2 size = { (float)nSamples, height };
		ImGuiWindow* window = ImGui::GetCurrentWindow();
		const ImVec2 padding = g.Style.FramePadding;
		const ImRect bb(window->DC.CursorPos, window->DC.CursorPos + size + padding * 2.0f);

		ImGui::ItemSize(bb);
		if (!ImGui::ItemAdd(bb, window->GetID((int)isrc))) {
			return;
		}

		float innerHeight = bb.GetHeight() - 4;

		ImVec2 outerTL = bb.Min;// ImGui::GetCursorPos();
		ImVec2 outerBR = { outerTL.x + bb.GetWidth(), outerTL.y + bb.GetHeight() };

		auto drawList = ImGui::GetWindowDrawList();

		auto sampleToY = [&](float sample) {
			float c = outerBR.y - float(bb.GetHeight()) * 0.5f;
			float h = float(innerHeight) * 0.5f * sample;
			return c - h;
		};

		ImGui::RenderFrame(outerTL, outerBR, ImGui::GetColorU32(ImGuiCol_FrameBg), true, 3.0f); // background
		float centerY = sampleToY(0);
		drawList->AddLine({ outerTL.x, centerY }, { outerBR.x, centerY }, ImGui::GetColorU32(ImGuiCol_PlotLines), 2.0f);// center line
		for (size_t iSample = 0; iSample < nSamples; ++iSample)
		{
			auto minY = sampleToY(peaks[iSample].first) - 0.5f;
			auto maxY = sampleToY(peaks[iSample].second) + 0.5f;
			drawList->AddLine({ outerTL.x + iSample, minY }, { outerTL.x + iSample, maxY }, ImGui::GetColorU32(ImGuiCol_PlotHistogram), 1);
		}

		auto GetX = [&](float t01) {
			float x = M7::math::lerp(bb.Min.x, bb.Max.x, t01);
			x = M7::math::clamp(x, bb.Min.x, bb.Max.x);
			return x;
		};

		auto drawCursor = [&](float t01, ImU32 color) {
			auto x = GetX(t01);
			drawList->AddLine({ x, bb.Min.y }, { x, bb.Max.y }, color, 3.5f);
		};

		drawCursor(loopStart01, ColorFromHTML("a03030"));
		drawCursor(loopStart01 + loopLen01, ColorFromHTML("a03030"));
		drawList->AddRectFilled({ GetX(loopStart01), bb.Min.y }, { GetX(loopStart01 + loopLen01), bb.Max.y }, ColorFromHTML("a03030", 0.4f));
		drawList->AddRectFilled({ GetX(loopStart01), bb.Max.y - 10 }, { GetX(loopStart01 + loopLen01), bb.Max.y }, ColorFromHTML("a03030", 0.8f));

		drawCursor(startPos01, ColorFromHTML("30a030"));
		drawCursor(cursor, ColorFromHTML("3030aa"));
	}

	// TODO: cache this image in a texture.
	void SamplerWaveformDisplay(M7::SamplerDevice& sampler, size_t isrc, Maj7::Maj7Voice* runningVoice)
	{
		auto sourceInfo = this->mSourceStatusText[isrc];
		//ImGuiIO& io = ImGui::GetIO();
		//ImGui::Image(io.Fonts->TexID, { gSamplerWaveformWidth, gSamplerWaveformHeight });
		if (!sampler.mSample) return;

		auto sampleData = sampler.mSample->GetSampleData();

		std::vector<std::pair<float, float>> peaks;
		peaks.resize(gSamplerWaveformWidth);
		for (size_t i = 0; i < gSamplerWaveformWidth; ++i) {
			size_t sampleBegin = size_t((i * sampler.mSample->GetSampleLength()) / gSamplerWaveformWidth);
			size_t sampleEnd = size_t(((i + 1) * sampler.mSample->GetSampleLength()) / gSamplerWaveformWidth);
			sampleEnd = std::max(sampleEnd, sampleBegin + 1);
			peaks[i].first = 0;
			peaks[i].second = 0;
			for (size_t s = sampleBegin; s < sampleEnd; ++s) {
				peaks[i].first = std::min(peaks[i].first, sampleData[s]);
				peaks[i].second = std::max(peaks[i].second, sampleData[s]);
			}
		}

		float cursor = 0;
		if (runningVoice) {
			M7::SamplerVoice* sv = static_cast<M7::SamplerVoice*>(runningVoice->mSourceVoices[isrc]);
			cursor = (float)sv->mSamplePlayer.samplePos;
			cursor /= sampler.mSample->GetSampleLength();
		}

		auto sampleStart = sampler.mParams.Get01Value(M7::SamplerParamIndexOffsets::SampleStart, 0);
		auto loopStart = sampler.mParams.Get01Value(M7::SamplerParamIndexOffsets::LoopStart, 0);
		auto loopLength = sampler.mParams.Get01Value(M7::SamplerParamIndexOffsets::LoopLength, 0);
		WaveformGraphic(isrc, gSamplerWaveformHeight, peaks, sampleStart, loopStart, loopLength, cursor);
	}

}; // class maj7editor
