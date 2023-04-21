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

class Maj7Editor : public VstEditor
{
	Maj7Vst* mpMaj7VST;
	M7::Maj7* pMaj7;
	static constexpr int gSamplerWaveformWidth = 700;
	static constexpr int gSamplerWaveformHeight = 75;
	std::vector<std::pair<std::string, int>> mGmDlsOptions;
	bool mShowingGmDlsList = false;
	char mGmDlsFilter[100] = { 0 };

public:

	struct RenderContext
	{
		M7::Maj7* mpMaj7;
		M7::ModMatrixNode* mpModMatrix = nullptr;
		M7::Maj7::Maj7Voice* mpActiveVoice = nullptr;
		float mModSourceValues[(int)M7::ModSource::Count];
		ImGuiKnobs::ModInfo mModDestValues[(int)M7::ModDestination::Count];

		static M7::Maj7::Maj7Voice* FindRunningVoice(M7::Maj7* pMaj7) {
			M7::Maj7::Maj7Voice* playingVoiceToReturn = nullptr;// pMaj7->mMaj7Voice[0];
			for (auto* v : pMaj7->mMaj7Voice)
			{
				if (!v->IsPlaying())
					continue;
				if (!playingVoiceToReturn || (v->mNoteInfo.mSequence > playingVoiceToReturn->mNoteInfo.mSequence))
				{
					playingVoiceToReturn = v;
				}
			}
			return playingVoiceToReturn;
		}


		void Init(M7::Maj7* pMaj7)
		{
			mpMaj7 = pMaj7;
			mpActiveVoice = FindRunningVoice(pMaj7);
			// dont hate me
			memset(mModSourceValues, 0, sizeof(mModSourceValues));
			memset(mModDestValues, 0, sizeof(mModDestValues));
			if (mpActiveVoice) {
				mpModMatrix = &mpActiveVoice->mModMatrix;
				for (int i = 0; i < (int)M7::ModSource::Count; ++i)
				{
					mModSourceValues[i] = mpActiveVoice->mModMatrix.GetSourceValue(i);
				}
				for (int i = 0; i < (int)M7::ModDestination::Count; ++i)
				{
					mModDestValues[i].mValue = mpActiveVoice->mModMatrix.GetDestinationValue(i);
				}
			}
			else
			{
				mpModMatrix = &pMaj7->mMaj7Voice[0]->mModMatrix;
			}

			for (auto& m : pMaj7->mModulations)
			{
				if (!m.mEnabled)
					continue;
				if (m.mSource == M7::ModSource::None)
					continue;
				for (size_t i = 0; i < std::size(m.mDestinations); ++ i)
				{
					mModDestValues[(int)m.mDestinations[i]].mEnabled = true;
					// extents
					mModDestValues[(int)m.mDestinations[i]].mExtentMax += std::abs(m.mScales[i]);
					mModDestValues[(int)m.mDestinations[i]].mExtentMin = mModDestValues[(int)m.mDestinations[i]].mExtentMax;
				}
			}
		}
	};

	RenderContext mRenderContext;
	ImGuiTabSelectionHelper mSourceTabSelHelper;
	ImGuiTabSelectionHelper mModEnvOrLFOTabSelHelper;
	ImGuiTabSelectionHelper mFilterTabSelHelper;
	ImGuiTabSelectionHelper mModulationTabSelHelper;


	Maj7Editor(AudioEffect* audioEffect) :
		VstEditor(audioEffect, 1120, 950),
		mpMaj7VST((Maj7Vst*)audioEffect),
		pMaj7(((Maj7Vst*)audioEffect)->GetMaj7())
	{
		mGmDlsOptions = LoadGmDlsOptions();
		mSourceTabSelHelper.mpSelectedTabIndex = &mpMaj7VST->mSelectedSource;
		mModEnvOrLFOTabSelHelper.mpSelectedTabIndex = &mpMaj7VST->mSelectedModEnvOrLFO;
		mFilterTabSelHelper.mpSelectedTabIndex = &mpMaj7VST->mSelectedFilter;
		mModulationTabSelHelper.mpSelectedTabIndex = &mpMaj7VST->mSelectedModulation;
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
	};

	SourceStatusText mSourceStatusText[M7::Maj7::gSourceCount];

	bool SetStatus(size_t isrc, StatusStyle style, const std::string& str) {
		mSourceStatusText[isrc].mStatusStyle = style;
		mSourceStatusText[isrc].mStatus = str;
		return false;
	}


	void Sort(float& a, float& b)
	{
		float low = std::min(a, b);
		b = std::max(a, b);
		a = low;
	}

	std::tuple<bool, float, float> Intersect(float a1, float a2, float b1, float b2)
	{
		Sort(a1, a2);
		Sort(b1, b2);
		if (a1 <= b2 && a2 >= b1) {
			// there is an intersection.
			return std::make_tuple(true, std::max(b1, a1), std::min(b2, a2));
		}
		return std::make_tuple(false, 0.0f, 0.0f);
	}

	void TooltipF(const char* format, ...)
	{
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_DelayShort))
		{
			ImGui::BeginTooltip();
			ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);

			char buffer[1024];

			va_list args;
			va_start(args, format);
			vsnprintf(buffer, sizeof(buffer), format, args);
			va_end(args);

			ImGui::TextUnformatted(buffer);
			ImGui::PopTextWrapPos();
			ImGui::EndTooltip();
		}
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
				mpMaj7VST->setChunk((void*)s.c_str(), VstInt32(s.size() + 1), false); // const cast oooooooh :/
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
		size = mpMaj7VST->getChunk2(&data, false, false);
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

			//ImGui::Separator();
			//ImGui::MenuItem("Show internal modulations", nullptr, &mShowingLockedModulations);

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
					mpMaj7VST->setChunk((void*)s.c_str(), VstInt32(s.size() + 1), false); // const cast oooooooh :/
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
				if (IDYES == ::MessageBoxA(mCurrentWindow, "Sure? This will clobber your patch, and generate defaults, to be used with the next menu item to generate Maj7.cpp. ", "WaveSabre - Maj7", MB_YESNO | MB_ICONQUESTION)) {
					GenerateDefaults(pMaj7);
				}
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
		size = mpMaj7VST->getChunk2(&data, false, diff);
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
		GenerateArray("gDefaultFilterParams", (int)M7::FilterParamIndexOffsets::Count, "M7::FilterParamIndexOffsets::Count", (int)pMaj7->mMaj7Voice[0]->mFilters[0][0].mParams.mBaseParamID);

		ss << "  } // namespace M7" << std::endl;
		ss << "} // namespace WaveSabreCore" << std::endl;
		ImGui::SetClipboardText(ss.str().c_str());

		::MessageBoxA(mCurrentWindow, "Copied new contents of WaveSabreCore/Maj7.cpp", "WaveSabre Maj7", MB_OK);
	}

	virtual std::string GetMenuBarStatusText() override 
	{
		std::string ret = " Voices:";
		ret += std::to_string(pMaj7->GetCurrentPolyphony());

		//auto* v = FindRunningVoice();
		if (mRenderContext.mpActiveVoice) {
			ret += " ";
			ret += midiNoteToString(mRenderContext.mpActiveVoice->mNoteInfo.MidiNoteValue);
		}

		return ret;
	};

	void Maj7ImGuiParamMacro(int imacro) {
		int paramID = (int)M7::ParamIndices::Macro1 + imacro;
		float tempVal = 0;
		M7::Float01Param p{ tempVal };
		float defaultParamVal = p.Get01Value();
		p.SetParamValue(GetEffectX()->getParameter((VstInt32)paramID));
		float centerVal01 = 0;
		Float01Converter conv{ };

		std::string label;
		std::string id = "##macro";
		id += std::to_string(imacro);

		if (mpMaj7VST->mMacroNames[imacro].empty())
		{
			std::string defaultLabel = "Macro " + std::to_string(imacro + 1);
			label = defaultLabel;
		}
		else {
			label = mpMaj7VST->mMacroNames[imacro];
		}

		if (ImGuiKnobs::KnobWithEditableLabel(label, id.c_str(), &tempVal, 0, 1, defaultParamVal, centerVal01, {}, gNormalKnobSpeed, gSlowKnobSpeed, nullptr, ImGuiKnobVariant_WiperOnly, 0, ImGuiKnobFlags_CustomInput | ImGuiKnobFlags_EditableTitle, 10, &conv, this))
		{
			GetEffectX()->setParameterAutomated(paramID, Clamp01(tempVal));
		}
		if (label != mpMaj7VST->mMacroNames[imacro]) {
			mpMaj7VST->mMacroNames[imacro] = label;
		}
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
	ColorMod mFMFeedbackColors{ 0.92f - 0.5f, 0.6f, 0.75f, 0.9f, 0.0f };

	ColorMod mNopColors;

	bool mShowingInspector = false;
	bool mShowingModulationInspector = false;
	bool mShowingColorExp = false;
	//bool mShowingLockedModulations = false;


	virtual void renderImgui() override
	{
		mModulationsColors.EnsureInitialized();
		mModulationDisabledColors.EnsureInitialized();
		mFMColors.EnsureInitialized();
		mFMFeedbackColors.EnsureInitialized();
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

		mRenderContext.Init(this->pMaj7);

		{
			auto& style = ImGui::GetStyle();
			style.FramePadding.x = 7;
			style.FramePadding.y = 3;
			style.ItemSpacing.x = 6;
			style.TabRounding = 3;
		}

		// color explorer
		ColorMod::Token colorExplorerToken;
		if (mShowingColorExp) {
			static float colorHueVarAmt = 0;
			static float colorSaturationVarAmt = 1;
			static float colorValueVarAmt = 1;
			static float colorTextVal = 0.9f;
			static float colorTextDisabledVal = 0.5f;
			bool b1 = ImGuiKnobs::Knob("hue", &colorHueVarAmt, -1.0f, 1.0f, 0.0f, 0, ImGuiKnobs::ModInfo{}, gNormalKnobSpeed, gSlowKnobSpeed);
			ImGui::SameLine(); bool b2 = ImGuiKnobs::Knob("sat", &colorSaturationVarAmt, 0.0f, 1.0f, 0.0f, 0, ImGuiKnobs::ModInfo{}, gNormalKnobSpeed, gSlowKnobSpeed);
			ImGui::SameLine(); bool b3 = ImGuiKnobs::Knob("val", &colorValueVarAmt, 0.0f, 1.0f, 0.0f, 0, ImGuiKnobs::ModInfo{}, gNormalKnobSpeed, gSlowKnobSpeed);
			ImGui::SameLine(); bool b4 = ImGuiKnobs::Knob("txt", &colorTextVal, 0.0f, 1.0f, 0.0f, 0, ImGuiKnobs::ModInfo{}, gNormalKnobSpeed, gSlowKnobSpeed);
			ImGui::SameLine(); bool b5 = ImGuiKnobs::Knob("txtD", &colorTextDisabledVal, 0.0f, 1.0f, 0.0f, 0, ImGuiKnobs::ModInfo{}, gNormalKnobSpeed, gSlowKnobSpeed);
			ColorMod xyz{ colorHueVarAmt , colorSaturationVarAmt, colorValueVarAmt, colorTextVal, colorTextDisabledVal };
			xyz.EnsureInitialized();
			colorExplorerToken = std::move(xyz.Push());
		}

		//auto runningVoice = FindRunningVoice();

#ifdef MAJ7_SELECTABLE_OUTPUT_STREAM_SUPPORT
		MAJ7_OUTPUT_STREAM_CAPTIONS(outputStreamCaptions);
		auto elementCount = std::size(outputStreamCaptions);
		for (int iOutput = 0; iOutput < 2; ++iOutput)
		{
			ImGui::PushID(iOutput);
			if (iOutput > 0) ImGui::SameLine();
			ImGui::Text("Output stream %d", iOutput);
			ImGui::SameLine();
			ImGui::SetNextItemWidth(200);
			if (ImGui::BeginCombo("##outputStream", outputStreamCaptions[(int)pMaj7->mOutputStreams[iOutput]]))
			{
				auto selectedIndex = (int)pMaj7->mOutputStreams[iOutput];
				for (int n = 0; n < (int)elementCount; n++)
				{
					const bool is_selected = (selectedIndex == n);
					if (ImGui::Selectable(outputStreamCaptions[n], is_selected))
					{
						pMaj7->mOutputStreams[iOutput] = (decltype(pMaj7->mOutputStreams[iOutput]))n;
					}
				}
				ImGui::EndCombo();
			}
			ImGui::PopID();
		}
#endif // MAJ7_SELECTABLE_OUTPUT_STREAM_SUPPORT
		Maj7ImGuiParamVolume((VstInt32)M7::ParamIndices::MasterVolume, "Volume##hc", M7::gMasterVolumeCfg, -6.0f, {});
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

		ImGui::SameLine(0, 60);
		Maj7ImGuiParamInt((VstInt32)M7::ParamIndices::PitchBendRange, "PB Range##mst", M7::gPitchBendCfg, 2, 0);
		ImGui::SameLine();
		Maj7ImGuiParamEnvTime((VstInt32)M7::ParamIndices::PortamentoTime, "Port##mst", 0.4f, GetModInfo(M7::ModDestination::PortamentoTime));
		ImGui::SameLine();
		Maj7ImGuiParamCurve((VstInt32)M7::ParamIndices::PortamentoCurve, "##portcurvemst", 0.0f, M7CurveRenderStyle::Rising, {});
		ImGui::SameLine();
		Maj7ImGuiParamInt((VstInt32)M7::ParamIndices::MaxVoices, "MaxVox", M7::gMaxVoicesCfg, 24, 1);

		static constexpr char const* const voiceModeCaptions[] = { "Poly", "Mono" };
		ImGui::SameLine(0, 60);
		Maj7ImGuiParamEnumList<WaveSabreCore::VoiceMode>((VstInt32)M7::ParamIndices::VoicingMode, "VoiceMode##mst", (int)WaveSabreCore::VoiceMode::Count, WaveSabreCore::VoiceMode::Polyphonic, voiceModeCaptions);

		//AUX_ROUTE_CAPTIONS(auxRouteCaptions);
		//ImGui::SameLine(); Maj7ImGuiParamEnumCombo((VstInt32)M7::ParamIndices::AuxRouting, "AuxRoute", (int)M7::AuxRoute::Count, M7::AuxRoute::TwoTwo, auxRouteCaptions, 100);
		//ImGui::SameLine(); Maj7ImGuiParamFloatN11((VstInt32)M7::ParamIndices::AuxWidth, "AuxWidth", 1, 0, {});

		// osc1
		if (BeginTabBar2("osc", ImGuiTabBarFlags_None))
		{
			static_assert(M7::Maj7::gOscillatorCount == 4, "osc count");
			int isrc = 0;
			Oscillator("Oscillator 1", (int)M7::ParamIndices::Osc1Enabled, isrc ++, (int)M7::ModDestination::Osc1AmpEnvDelayTime);
			Oscillator("Oscillator 2", (int)M7::ParamIndices::Osc2Enabled, isrc ++, (int)M7::ModDestination::Osc2AmpEnvDelayTime);
			Oscillator("Oscillator 3", (int)M7::ParamIndices::Osc3Enabled, isrc ++, (int)M7::ModDestination::Osc3AmpEnvDelayTime);
			Oscillator("Oscillator 4", (int)M7::ParamIndices::Osc4Enabled, isrc ++, (int)M7::ModDestination::Osc4AmpEnvDelayTime);

			static_assert(M7::Maj7::gSamplerCount == 4, "sampler count");
			Sampler("Sampler 1", pMaj7->mSamplerDevices[0], isrc++, (int)M7::ModDestination::Sampler1AmpEnvDelayTime);
			Sampler("Sampler 2", pMaj7->mSamplerDevices[1], isrc++, (int)M7::ModDestination::Sampler2AmpEnvDelayTime);
			Sampler("Sampler 3", pMaj7->mSamplerDevices[2], isrc++, (int)M7::ModDestination::Sampler3AmpEnvDelayTime);
			Sampler("Sampler 4", pMaj7->mSamplerDevices[3], isrc++, (int)M7::ModDestination::Sampler4AmpEnvDelayTime);
			EndTabBarWithColoredSeparator();
		}

		ImGui::BeginTable("##fmaux", 2);
		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		//ImGui::BeginGroup();
		if (BeginTabBar2("FM", ImGuiTabBarFlags_None, 400))
		{
			auto colorModToken = mFMColors.Push();
			static_assert(M7::Maj7::gOscillatorCount == 4, "osc count");
			if (WSBeginTabItem("\"Frequency Modulation\"")) {
				ImGui::BeginGroup();
				ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, {0,0});
				{
					auto colorModToken = mFMFeedbackColors.Push();
					Maj7ImGuiParamFMKnob((VstInt32)M7::ParamIndices::Osc1FMFeedback, "FB1");
				}
				ImGui::SameLine(); Maj7ImGuiParamFMKnob((VstInt32)M7::ParamIndices::FMAmt2to1, "2-1");
				ImGui::SameLine(); Maj7ImGuiParamFMKnob((VstInt32)M7::ParamIndices::FMAmt3to1, "3-1");
				ImGui::SameLine(); Maj7ImGuiParamFMKnob((VstInt32)M7::ParamIndices::FMAmt4to1, "4-1");

				Maj7ImGuiParamFMKnob((VstInt32)M7::ParamIndices::FMAmt1to2, "1-2");
				{
					auto colorModToken = mFMFeedbackColors.Push();
					ImGui::SameLine(); Maj7ImGuiParamFMKnob((VstInt32)M7::ParamIndices::Osc2FMFeedback, "FB2");
				}
				ImGui::SameLine(); Maj7ImGuiParamFMKnob((VstInt32)M7::ParamIndices::FMAmt3to2, "3-2");
				ImGui::SameLine(); Maj7ImGuiParamFMKnob((VstInt32)M7::ParamIndices::FMAmt4to2, "4-2");

				Maj7ImGuiParamFMKnob((VstInt32)M7::ParamIndices::FMAmt1to3, "1-3");
				ImGui::SameLine(); Maj7ImGuiParamFMKnob((VstInt32)M7::ParamIndices::FMAmt2to3, "2-3");
				{
					auto colorModToken = mFMFeedbackColors.Push();
					ImGui::SameLine(); Maj7ImGuiParamFMKnob((VstInt32)M7::ParamIndices::Osc3FMFeedback, "FB3");
				}
				ImGui::SameLine(); Maj7ImGuiParamFMKnob((VstInt32)M7::ParamIndices::FMAmt4to3, "4-3");

				Maj7ImGuiParamFMKnob((VstInt32)M7::ParamIndices::FMAmt1to4, "1-4");
				ImGui::SameLine(); Maj7ImGuiParamFMKnob((VstInt32)M7::ParamIndices::FMAmt2to4, "2-4");
				ImGui::SameLine(); Maj7ImGuiParamFMKnob((VstInt32)M7::ParamIndices::FMAmt3to4, "3-4");
				{
					auto colorModToken = mFMFeedbackColors.Push();
					ImGui::SameLine(); Maj7ImGuiParamFMKnob((VstInt32)M7::ParamIndices::Osc4FMFeedback, "FB4");
				}
	
				ImGui::PopStyleVar();
				ImGui::EndGroup();

				ImGui::SameLine(0, 60);	Maj7ImGuiParamFloat01((VstInt32)M7::ParamIndices::FMBrightness, "Brightness##mst", 0.5f, 0.5f, 0, GetModInfo(M7::ModDestination::FMBrightness));

				ImGui::EndTabItem();
			}
			EndTabBarWithColoredSeparator();
		}
		//ImGui::EndGroup();

		ImGui::TableNextColumn();

		// aux
		if (BeginTabBar2("aux", ImGuiTabBarFlags_None, 400))
		{

			AuxEffectTab("Filter1", 0);
			AuxEffectTab("Filter2", 1);

			EndTabBarWithColoredSeparator();
		}

		ImGui::EndTable();


		// modulation shapes
		if (BeginTabBar2("envelopetabs", ImGuiTabBarFlags_None))
		{
			auto modColorModToken = mModEnvelopeColors.Push();
			if (WSBeginTabItemWithSel("Mod env 1", 0, mModEnvOrLFOTabSelHelper))
			{
				Envelope("Modulation Envelope 1", (int)M7::ParamIndices::Env1DelayTime, (int)M7::ModDestination::Env1DelayTime);
				ImGui::EndTabItem();
			}
			if (WSBeginTabItemWithSel("Mod env 2", 1, mModEnvOrLFOTabSelHelper))
			{
				Envelope("Modulation Envelope 2", (int)M7::ParamIndices::Env2DelayTime, (int)M7::ModDestination::Env2DelayTime);
				ImGui::EndTabItem();
			}
			auto lfoColorModToken = mLFOColors.Push();
			if (WSBeginTabItemWithSel("LFO 1", 2, mModEnvOrLFOTabSelHelper))
			{
				LFO("LFO 1", (int)M7::ParamIndices::LFO1Waveform, 0);
				ImGui::EndTabItem();
			}
			if (WSBeginTabItemWithSel("LFO 2", 3, mModEnvOrLFOTabSelHelper))
			{
				LFO("LFO 2", (int)M7::ParamIndices::LFO2Waveform, 1);
				ImGui::EndTabItem();
			}
			if (WSBeginTabItemWithSel("LFO 3", 4, mModEnvOrLFOTabSelHelper))
			{
				LFO("LFO 3", (int)M7::ParamIndices::LFO3Waveform, 2);
				ImGui::EndTabItem();
			}
			if (WSBeginTabItemWithSel("LFO 4", 5, mModEnvOrLFOTabSelHelper))
			{
				LFO("LFO 4", (int)M7::ParamIndices::LFO4Waveform, 3);
				ImGui::EndTabItem();
			}
			EndTabBarWithColoredSeparator();
		}

		if (BeginTabBar2("modspectabs", ImGuiTabBarFlags_TabListPopupButton))
		{
			static_assert(M7::gModulationCount == 18, "update this yo");
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
				static_assert(M7::Maj7::gMacroCount == 7, "update this yo");
				Maj7ImGuiParamMacro(0);
				ImGui::SameLine(); Maj7ImGuiParamMacro(1);
				ImGui::SameLine(); Maj7ImGuiParamMacro(2);
				ImGui::SameLine(); Maj7ImGuiParamMacro(3);
				ImGui::SameLine(); Maj7ImGuiParamMacro(4);
				ImGui::SameLine(); Maj7ImGuiParamMacro(5);
				ImGui::SameLine(); Maj7ImGuiParamMacro(6);
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
				}
			}
		}
	}

	ImGuiKnobs::ModInfo GetModInfo(M7::ModDestination d)
	{
		return mRenderContext.mModDestValues[(int)d];
	}

	void Oscillator(const char* labelWithID, int enabledParamID, int oscID, int ampEnvModDestBase)
	{
		float enabledBacking;
		M7::BoolParam bp{ enabledBacking };
		bp.SetRawParamValue(GetEffectX()->getParameter(enabledParamID));
		ColorMod& cm = bp.GetBoolValue() ? mOscColors : mOscDisabledColors;
		auto token = cm.Push();

		auto lGetModInfo = [&](M7::OscModParamIndexOffsets x) {
			return GetModInfo((M7::ModDestination)((int)pMaj7->mOscillatorDevices[oscID].mModDestBaseID + (int)x));
		};

		if (mpMaj7VST->mSelectedSource != 0)
		{
			MulDiv(1, 1, 1);
		}

		//sth.GetImGuiFlags(oscID, &mpMaj7VST->mSelectedSource)
		//if (WSBeginTabItem(labelWithID, 0, 0)) {
		if (WSBeginTabItemWithSel(labelWithID, oscID, mSourceTabSelHelper)) {
			//sth.OnSelectedTab();
			WSImGuiParamCheckbox(enabledParamID + (int)M7::OscParamIndexOffsets::Enabled, "Enabled");
			ImGui::SameLine(); Maj7ImGuiParamVolume(enabledParamID + (int)M7::OscParamIndexOffsets::Volume, "Volume", M7::gUnityVolumeCfg, 0, lGetModInfo(M7::OscModParamIndexOffsets::Volume ));

			ImGui::SameLine(); WaveformParam(enabledParamID + (int)M7::OscParamIndexOffsets::Waveform, enabledParamID + (int)M7::OscParamIndexOffsets::Waveshape, enabledParamID + (int)M7::OscParamIndexOffsets::PhaseOffset, nullptr);
			ImGui::SameLine(); Maj7ImGuiParamFloat01(enabledParamID + (int)M7::OscParamIndexOffsets::Waveshape, "Shape", 0.5f, 0.5f, 0, lGetModInfo(M7::OscModParamIndexOffsets::Waveshape));

			ImGui::SameLine(0, 60); Maj7ImGuiParamFrequency(enabledParamID + (int)M7::OscParamIndexOffsets::FrequencyParam, enabledParamID + (int)M7::OscParamIndexOffsets::FrequencyParamKT, "Freq", M7::gSourceFreqConfig, M7::gFreqParamKTUnity, lGetModInfo(M7::OscModParamIndexOffsets::FrequencyParam));
			ImGui::SameLine(); Maj7ImGuiParamScaledFloat(enabledParamID + (int)M7::OscParamIndexOffsets::FrequencyParamKT, "KT", 0, 1, 1, 1, 0, {});
			ImGui::SameLine(); Maj7ImGuiParamInt(enabledParamID + (int)M7::OscParamIndexOffsets::PitchSemis, "Transp", M7::gSourcePitchSemisRange, 0, 0);
			ImGui::SameLine(); Maj7ImGuiParamFloatN11(enabledParamID + (int)M7::OscParamIndexOffsets::PitchFine, "FineTune", 0, 0, lGetModInfo(M7::OscModParamIndexOffsets::PitchFine));
			ImGui::SameLine(); Maj7ImGuiParamScaledFloat(enabledParamID + (int)M7::OscParamIndexOffsets::FreqMul, "FreqMul", 0, M7::gFrequencyMulMax, 1, 0, 0, {});
			ImGui::SameLine(0, 60); WSImGuiParamCheckbox(enabledParamID + (int)M7::OscParamIndexOffsets::PhaseRestart, "PhaseRst");
			ImGui::SameLine(); Maj7ImGuiParamFloatN11(enabledParamID + (int)M7::OscParamIndexOffsets::PhaseOffset, "Phase", 0, 0, lGetModInfo(M7::OscModParamIndexOffsets::Phase));
			ImGui::SameLine(0, 60); WSImGuiParamCheckbox(enabledParamID + (int)M7::OscParamIndexOffsets::SyncEnable, "Sync");
			ImGui::SameLine(); Maj7ImGuiParamFrequency(enabledParamID + (int)M7::OscParamIndexOffsets::SyncFrequency, enabledParamID + (int)M7::OscParamIndexOffsets::SyncFrequencyKT, "SyncFreq", M7::gSyncFreqConfig, M7::gFreqParamKTUnity, lGetModInfo(M7::OscModParamIndexOffsets::SyncFrequency));
			ImGui::SameLine(); Maj7ImGuiParamScaledFloat(enabledParamID + (int)M7::OscParamIndexOffsets::SyncFrequencyKT, "SyncKT", 0, 1, 1, 1, 0, {});

			ImGui::SameLine(0, 60); Maj7ImGuiParamFloatN11(enabledParamID + (int)M7::OscParamIndexOffsets::AuxMix, "Aux pan", 0, 0, lGetModInfo(M7::OscModParamIndexOffsets::AuxMix));

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
				Envelope("Amplitude Envelope", (int)ampEnvSource, ampEnvModDestBase);
			}
			ImGui::EndTabItem();
		}
	}

	void LFO(const char* labelWithID, int waveformParamID, int ilfo)
	{
		ImGui::PushID(labelWithID);

		auto lGetModInfo = [&](M7::LFOModParamIndexOffsets x) {
			return GetModInfo((M7::ModDestination)((int)pMaj7->mLFOs[ilfo].mDevice.mModDestBaseID + (int)x));
		};

		float phaseCursor = (float)(this->pMaj7->mLFOs[ilfo].mPhase.mPhase);

		TIME_BASIS_CAPTIONS(timeBasisCaptions);

		WaveformParam(waveformParamID + (int)M7::LFOParamIndexOffsets::Waveform, waveformParamID + (int)M7::LFOParamIndexOffsets::Waveshape, waveformParamID + (int)M7::LFOParamIndexOffsets::PhaseOffset, &phaseCursor);

		ImGui::SameLine(); WSImGuiParamKnob(waveformParamID + (int)M7::LFOParamIndexOffsets::Waveshape, "Shape");
		// TODO: in the future,
		//ImGui::SameLine(); Maj7ImGuiParamEnumCombo(waveformParamID + (int)M7::LFOParamIndexOffsets::FrequencyBasis, "TimeBasis", M7::TimeBasis::Count, M7::TimeBasis::Frequency, timeBasisCaptions);
		ImGui::SameLine(); Maj7ImGuiParamFrequency(waveformParamID + (int)M7::LFOParamIndexOffsets::FrequencyParam, -1, "Freq", M7::gLFOFreqConfig, 0.4f, lGetModInfo(M7::LFOModParamIndexOffsets::FrequencyParam));
		ImGui::SameLine(); Maj7ImGuiParamFloatN11(waveformParamID + (int)M7::LFOParamIndexOffsets::PhaseOffset, "Phase", 0, 0, lGetModInfo(M7::LFOModParamIndexOffsets::Phase));
		ImGui::SameLine(); WSImGuiParamCheckbox(waveformParamID + (int)M7::LFOParamIndexOffsets::Restart, "Restart");

		ImGui::SameLine(0, 60); Maj7ImGuiParamFrequency(waveformParamID + (int)M7::LFOParamIndexOffsets::Sharpness, -1, "Sharpness", M7::gLFOLPFreqConfig, 0.5f, lGetModInfo(M7::LFOModParamIndexOffsets::Sharpness));

		ImGui::PopID();
	}

	void Envelope(const char* labelWithID, int delayTimeParamID, int modDestBaseID)
	{
		auto lGetModInfo = [&](M7::EnvModParamIndexOffsets x) {
			return GetModInfo((M7::ModDestination)(modDestBaseID + (int)x));
		};

		ImGui::PushID(labelWithID);
		Maj7ImGuiParamEnvTime(delayTimeParamID + (int)M7::EnvParamIndexOffsets::DelayTime, "Delay", 0, lGetModInfo(M7::EnvModParamIndexOffsets::DelayTime));
		ImGui::SameLine();
		Maj7ImGuiParamEnvTime(delayTimeParamID + (int)M7::EnvParamIndexOffsets::AttackTime, "Attack", 0, lGetModInfo(M7::EnvModParamIndexOffsets::AttackTime));
		ImGui::SameLine();
		Maj7ImGuiParamCurve(delayTimeParamID + (int)M7::EnvParamIndexOffsets::AttackCurve, "Curve##attack", 0, M7CurveRenderStyle::Rising, lGetModInfo(M7::EnvModParamIndexOffsets::AttackCurve));
		ImGui::SameLine();
		Maj7ImGuiParamEnvTime(delayTimeParamID + (int)M7::EnvParamIndexOffsets::HoldTime, "Hold", 0, lGetModInfo(M7::EnvModParamIndexOffsets::HoldTime));
		ImGui::SameLine();
		Maj7ImGuiParamEnvTime(delayTimeParamID + (int)M7::EnvParamIndexOffsets::DecayTime, "Decay", .4f, lGetModInfo(M7::EnvModParamIndexOffsets::DecayTime));
		ImGui::SameLine();
		Maj7ImGuiParamCurve(delayTimeParamID + (int)M7::EnvParamIndexOffsets::DecayCurve, "Curve##Decay", 0, M7CurveRenderStyle::Falling, lGetModInfo(M7::EnvModParamIndexOffsets::DecayCurve));
		ImGui::SameLine();
		Maj7ImGuiParamFloat01(delayTimeParamID + (int)M7::EnvParamIndexOffsets::SustainLevel, "Sustain", 0.4f, 0, 0, lGetModInfo(M7::EnvModParamIndexOffsets::SustainLevel));
		ImGui::SameLine();
		Maj7ImGuiParamEnvTime(delayTimeParamID + (int)M7::EnvParamIndexOffsets::ReleaseTime, "Release", 0, lGetModInfo(M7::EnvModParamIndexOffsets::ReleaseTime));
		ImGui::SameLine();
		Maj7ImGuiParamCurve(delayTimeParamID + (int)M7::EnvParamIndexOffsets::ReleaseCurve, "Curve##Release", 0, M7CurveRenderStyle::Falling, lGetModInfo(M7::EnvModParamIndexOffsets::ReleaseCurve));
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
		ImGui::PopID();
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

	struct ModMeterStyle
	{
		ImColor background;
		ImColor backgroundOob;
		ImColor foreground;
		ImColor foregorundOob;
		ImColor foregroundTick;
		ImColor boundaryIndicator;
		ImColor zeroTick;
		ImVec2 meterSize;
		float gHalfTickWidth;
		float gTickHeight;

		ModMeterStyle Modification(float saturationMul, float valMul, float width, float height) {
			ModMeterStyle ret {
				ColorMod::GetModifiedColor(background, 0, saturationMul, valMul),
				ColorMod::GetModifiedColor(backgroundOob, 0, saturationMul, valMul),
				ColorMod::GetModifiedColor(foreground, 0, saturationMul, valMul),
				ColorMod::GetModifiedColor(foregorundOob, 0, saturationMul, valMul),
				ColorMod::GetModifiedColor(foregroundTick, 0, saturationMul, valMul),
				ColorMod::GetModifiedColor(boundaryIndicator, 0, saturationMul, valMul),
				ColorMod::GetModifiedColor(zeroTick, 0, saturationMul, valMul),
				meterSize,
				gHalfTickWidth,
				gTickHeight,
			};
			ret.meterSize.x = width;
			ret.meterSize.y = height;
			return ret;
		}
	};

	ModMeterStyle mBaseModMeterStyle{
		ColorFromHTML("0d350d"),
		ColorFromHTML("404040"),
		ColorFromHTML("228822"),
		ColorFromHTML("882222"),
		ColorFromHTML("40ff40"),
		ColorFromHTML("854ecb"),
		ColorFromHTML("000000"),
		{ 175, 9 },
		0.5f,
		2.0f,
	};

	ModMeterStyle mIntermediateLargeModMeterStyle = mBaseModMeterStyle.Modification(1, 1, 175, 9);
	ModMeterStyle mIntermediateSmallModMeterStyle = mBaseModMeterStyle.Modification(.3f, 0.85f, 175, 6);

	ModMeterStyle mIntermediateSmallModMeterStyleDisabled = mBaseModMeterStyle.Modification(0, 0.7f, 175, 6);
	ModMeterStyle mIntermediateLargeModMeterStyleDisabled = mBaseModMeterStyle.Modification(0, 0.7f, 175, 9);

	ModMeterStyle mPrimaryModMeterStyle = mBaseModMeterStyle.Modification(1, 1, 250, 10);
	ModMeterStyle mPrimaryModMeterStyleDisabled = mBaseModMeterStyle.Modification(0, 0.7f, 250, 10);

	void AddRect(float x1, float x2, float y1, float y2, const ImColor& color)
	{
		ImGui::GetWindowDrawList()->AddRectFilled(ImVec2{ std::min(x1,x2), std::min(y1,y2) }, ImVec2{ std::max(x1,x2), std::max(y1,y2) }, color);
	}

	// rangemin/max are the window being shown.
	// boundmin/max are the area within the window which are colored green.
	void ModMeter_Horiz(M7::ModulationSpec& spec, ImVec2 orig, ImVec2 size, float windowMin, float windowMax, float srcBound1, float srcBound2, float highlightBound1, float highlightBound2, float x, const ModMeterStyle& style)
	{
		auto ValToX = [&](float x_) {
			// map x from [rangemin,rangemax] to rect.
			return M7::math::map_clamped(x_, windowMin, windowMax, orig.x, orig.x + size.x);
		};

		auto* dl = ImGui::GetWindowDrawList();

		// background oob
		dl->AddRectFilled(orig, orig + size, style.backgroundOob);

		// background, in src bound
		float xSrcBound1 = ValToX(srcBound1);
		float xSrcBound2 = ValToX(srcBound2);
		auto srcBoundIntersection = Intersect(xSrcBound1, xSrcBound2, orig.x, orig.x + size.x);
		if (std::get<0>(srcBoundIntersection)) {
			AddRect(std::get<1>(srcBoundIntersection), std::get<2>(srcBoundIntersection), orig.y, orig.y + size.y, style.background);
		}

		// oob area
		float xzero = ValToX(0);
		float xval = ValToX(x);
		AddRect(xzero, xval, orig.y, orig.y + size.y, style.foregorundOob);

		// in-bound highlight area
		float xHighlightBound1 = ValToX(highlightBound1);
		float xHighlightBound2 = ValToX(highlightBound2);
		auto highlightRangeIntersection = Intersect(xHighlightBound1, xHighlightBound2, xzero, xval);
		if (std::get<0>(highlightRangeIntersection)) {
			AddRect(std::get<1>(highlightRangeIntersection), std::get<2>(highlightRangeIntersection), orig.y, orig.y + size.y, style.foreground);
		}

		// foreground tick
		AddRect(xval - style.gHalfTickWidth, xval + style.gHalfTickWidth, orig.y, orig.y + size.y, style.foregroundTick);

		// zero tick
		AddRect(xzero - style.gHalfTickWidth, xzero + style.gHalfTickWidth, orig.y, orig.y + size.y, style.zeroTick);

		// .5 ticks
		float windowBoundLow = std::min(windowMax, windowMin);
		float windowBoundHigh = std::max(windowMax, windowMin);
		windowBoundLow = floorf(windowBoundLow * 2) * 0.5f;// round down to 0.5.
		for (float f = windowBoundLow; f < windowBoundHigh; f += 0.5f)
		{
			float xtick = ValToX(f);
			if (xtick <= (orig.x + 4.0f)) continue; // don't show ticks that are close to the edge; looks ugly.
			if (xtick >= (orig.x + size.x - 4.0f)) continue; // don't show ticks that are close to the edge; looks ugly.
			AddRect(xtick - style.gHalfTickWidth, xtick + style.gHalfTickWidth, orig.y, orig.y + style.gTickHeight, style.zeroTick);
			AddRect(xtick - style.gHalfTickWidth, xtick + style.gHalfTickWidth, orig.y + size.y - style.gTickHeight, orig.y + size.y, style.zeroTick);
		}

		ImGui::Dummy(size);
	}

	// draws:
	// - source value, colored by whether it falls in the source range
	// - range arrows
	// - result value scaled and curved -1,1
	// returns the resulting value.
	float ModMeter(bool visible, M7::ModulationSpec& spec, M7::ModSource modSource, M7::ModParamIndexOffsets curveParam, M7::ModParamIndexOffsets rangeMinParam, M7::ModParamIndexOffsets rangeMaxParam, bool isTargetN11, bool isEnabled)
	{
		float srcVal = mRenderContext.mModSourceValues[(int)modSource];
		float rangeMin = spec.mParams.GetScaledRealValue(rangeMinParam, -3, 3, 0);
		float rangeMax = spec.mParams.GetScaledRealValue(rangeMaxParam, -3, 3, 0);
		float resultingValue = mRenderContext.mpModMatrix->MapValue(spec, modSource, curveParam, rangeMinParam, rangeMaxParam, isTargetN11);
		if (!visible) {
			return resultingValue;
		}

		auto& smallStyle = *(isEnabled ? &mIntermediateSmallModMeterStyle : &mIntermediateSmallModMeterStyleDisabled);
		auto& largeStyle = *(isEnabled ? &mIntermediateLargeModMeterStyle : &mIntermediateLargeModMeterStyleDisabled);

		ImGuiWindow* window = ImGui::GetCurrentWindow();

		float sourceWindowMin = std::min(std::min(- 1.0f, rangeMin), rangeMax);
		float sourceWindowMax = std::max(std::max(1.0f, rangeMin), rangeMax);
		ModMeter_Horiz(spec, window->DC.CursorPos, smallStyle.meterSize, sourceWindowMin, sourceWindowMax, -1, 1, rangeMin, rangeMax, srcVal, smallStyle);
		TooltipF("Raw input signal");

		// get highlight area of the total source window
		float highlightWindowMin = M7::math::map_clamped(rangeMin, sourceWindowMin, sourceWindowMax, 0, smallStyle.meterSize.x);
		float highlightWindowMax = M7::math::map_clamped(rangeMax, sourceWindowMin, sourceWindowMax, 0, smallStyle.meterSize.x);

		// little handle indicators
		static constexpr ImVec2 gHandleSize{ 5, 5 };
		static constexpr float gHandleThickness = 2;
		static constexpr float gHandleMarginX = 2;

		auto pos = window->DC.CursorPos;
		ModMeter_Horiz(spec, { window->DC.CursorPos.x + highlightWindowMin, window->DC.CursorPos.y }, { highlightWindowMax - highlightWindowMin, smallStyle.meterSize.y },
			rangeMin, rangeMax, rangeMin, rangeMax, rangeMin, rangeMax, srcVal, smallStyle);
		TooltipF("Selected range to use for modulation");

		AddRect(
			pos.x + highlightWindowMin - gHandleSize.x - gHandleMarginX,
			pos.x + highlightWindowMin - gHandleMarginX,
			pos.y,
			pos.y + gHandleThickness,
			smallStyle.boundaryIndicator);

		AddRect(
			pos.x + highlightWindowMin - gHandleThickness - gHandleMarginX,
			pos.x + highlightWindowMin - gHandleMarginX,
			pos.y,
			pos.y + gHandleSize.y,
			smallStyle.boundaryIndicator);

		AddRect( // TOP
			pos.x + highlightWindowMax + gHandleMarginX,
			pos.x + highlightWindowMax + gHandleMarginX + gHandleSize.x,
			pos.y,
			pos.y + gHandleThickness,
			smallStyle.boundaryIndicator);

		AddRect( // BOTTOM
			pos.x + highlightWindowMax + gHandleMarginX,
			pos.x + highlightWindowMax + gHandleMarginX + gHandleThickness,
			pos.y,
			pos.y + gHandleSize.y,
			smallStyle.boundaryIndicator);

		// show output value.
		ModMeter_Horiz(spec, window->DC.CursorPos, largeStyle.meterSize, isTargetN11 ? -1.0f : 0.0f, 1, -1, 1, -1, 1, resultingValue, largeStyle);
		TooltipF("Value after selected range and curve applied");

		return resultingValue;
	}


	void ModulationSection(int imod, M7::ModulationSpec& spec, int enabledParamID)
	{
		bool isLocked = spec.mType != M7::ModulationSpecType::General;

		static constexpr char const* const modSourceCaptions[] = MOD_SOURCE_CAPTIONS;
		std::string modDestinationCaptions[(size_t)M7::ModDestination::Count] = MOD_DEST_CAPTIONS;
		char const* modDestinationCaptionsCstr[(size_t)M7::ModDestination::Count];

		for (size_t i = 0; i < (size_t)M7::ModDestination::Count; ++i)
		{
			modDestinationCaptionsCstr[i] = modDestinationCaptions[i].c_str();
		}

		bool isEnabled = spec.mParams.GetBoolValue(M7::ModParamIndexOffsets::Enabled);

		ColorMod& cm = isEnabled ? (isLocked ? mLockedModulationsColors : mModulationsColors) : mModulationDisabledColors;
		auto token = cm.Push();

		float modVal = 0;

		if (WSBeginTabItemWithSel(GetModulationName(spec, imod).c_str(), imod, mModulationTabSelHelper))
		{
			ImGui::BeginDisabled(isLocked);
			WSImGuiParamCheckbox((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::Enabled, "Enabled");
			ImGui::EndDisabled();
			ImGui::SameLine();
			{
				ImGui::BeginGroup();
				ImGui::PushID("main");

				Maj7ImGuiParamEnumCombo((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::Source, "Source", (int)M7::ModSource::Count, M7::ModSource::None, modSourceCaptions, 180);

				if (mpMaj7VST->mShowAdvancedModControls[imod]) {

					Maj7ImGuiParamScaledFloat(enabledParamID + (int)M7::ModParamIndexOffsets::SrcRangeMin, "RangeMin", -3, 3, -1, -1, 30, {});
					ImGui::SameLine(); Maj7ImGuiParamScaledFloat(enabledParamID + (int)M7::ModParamIndexOffsets::SrcRangeMax, "Max", -3, 3, 1, 1, 30, {});

					ImGui::SameLine(); Maj7ImGuiParamCurve((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::Curve, "Curve", 0, M7CurveRenderStyle::Rising, {});

					modVal = ModMeter(true, spec, spec.mSource, M7::ModParamIndexOffsets::Curve, M7::ModParamIndexOffsets::SrcRangeMin, M7::ModParamIndexOffsets::SrcRangeMax, true, isEnabled);

					if (ImGui::SmallButton("Reset")) {
						spec.mParams.SetRangedValue(M7::ModParamIndexOffsets::SrcRangeMin, -3, 3, -1);
						spec.mParams.SetRangedValue(M7::ModParamIndexOffsets::SrcRangeMax, -3, 3, 1);
					}
					ImGui::SameLine();
					if (ImGui::SmallButton("To pos")) {
						spec.mParams.SetRangedValue(M7::ModParamIndexOffsets::SrcRangeMin, -3, 3, -3);
						spec.mParams.SetRangedValue(M7::ModParamIndexOffsets::SrcRangeMax, -3, 3, 1);
					}
					ImGui::SameLine();
					if (ImGui::SmallButton("To bip.")) {
						spec.mParams.SetRangedValue(M7::ModParamIndexOffsets::SrcRangeMin, -3, 3, 0);
						spec.mParams.SetRangedValue(M7::ModParamIndexOffsets::SrcRangeMax, -3, 3, 1);
					}
					if (ImGui::SmallButton("hide advanced")) {
						mpMaj7VST->mShowAdvancedModControls[imod] = false;
					}
				}
				else {
					modVal = ModMeter(false, spec, spec.mSource, M7::ModParamIndexOffsets::Curve, M7::ModParamIndexOffsets::SrcRangeMin, M7::ModParamIndexOffsets::SrcRangeMax, true, isEnabled);
					if (ImGui::SmallButton("show advanced")) {
						mpMaj7VST->mShowAdvancedModControls[imod] = true;
					}
				}

				ImGui::PopID();
				ImGui::EndGroup();
			}
			ImGui::SameLine();
			ImGui::BeginDisabled(isLocked);
			{
				ImGui::BeginGroup();

				ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, {0, 0});

				Maj7ImGuiParamEnumCombo((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::Destination1, "Dest##1", (int)M7::ModDestination::Count, M7::ModDestination::None, modDestinationCaptionsCstr, 180);
				ImGui::SameLine();
				Maj7ImGuiParamFloatN11(enabledParamID + (int)M7::ModParamIndexOffsets::Scale1, "###Scale1", 1, 20.0f, {});

				Maj7ImGuiParamEnumCombo((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::Destination2, "###Dest2", (int)M7::ModDestination::Count, M7::ModDestination::None, modDestinationCaptionsCstr, 180);
				ImGui::SameLine();
				Maj7ImGuiParamFloatN11(enabledParamID + (int)M7::ModParamIndexOffsets::Scale2, "###Scale2", 1, 20.0f, {});

				Maj7ImGuiParamEnumCombo((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::Destination3, "###Dest3", (int)M7::ModDestination::Count, M7::ModDestination::None, modDestinationCaptionsCstr, 180);
				ImGui::SameLine();
				Maj7ImGuiParamFloatN11(enabledParamID + (int)M7::ModParamIndexOffsets::Scale3, "###Scale3", 1, 20.0f, {});

				Maj7ImGuiParamEnumCombo((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::Destination4, "###Dest4", (int)M7::ModDestination::Count, M7::ModDestination::None, modDestinationCaptionsCstr, 180);
				ImGui::SameLine();
				Maj7ImGuiParamFloatN11(enabledParamID + (int)M7::ModParamIndexOffsets::Scale4, "###Scale4", 1, 20.0f, {});

				ImGui::PopStyleVar();
				ImGui::EndGroup();
			}
			ImGui::EndDisabled();

			ImGui::SameLine(0, 60); WSImGuiParamCheckbox((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::AuxEnabled, "Aux Enable");
			bool isAuxEnabled = spec.mParams.GetBoolValue(M7::ModParamIndexOffsets::AuxEnabled);

			ColorMod& cmaux = isAuxEnabled ? mNopColors : mModulationDisabledColors;
			auto auxToken = cmaux.Push();
			ImGui::PushID("aux");
			ImGui::SameLine();
			{
				ImGui::BeginGroup();
				Maj7ImGuiParamEnumCombo((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::AuxSource, "Aux Src", (int)M7::ModSource::Count, M7::ModSource::None, modSourceCaptions, 180);
				float auxVal = 0;
				if (mpMaj7VST->mShowAdvancedModAuxControls[imod]) {

					Maj7ImGuiParamScaledFloat(enabledParamID + (int)M7::ModParamIndexOffsets::AuxRangeMin, "AuxRangeMin", -3, 3, 0, 0, 30, {});
					ImGui::SameLine(); Maj7ImGuiParamScaledFloat(enabledParamID + (int)M7::ModParamIndexOffsets::AuxRangeMax, "Max", -3, 3, 1, 1, 30, {});

					ImGui::SameLine(); Maj7ImGuiParamCurve((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::AuxCurve, "Aux Curve", 0, M7CurveRenderStyle::Rising, {});

					auxVal = ModMeter(true, spec, spec.mAuxSource, M7::ModParamIndexOffsets::AuxCurve, M7::ModParamIndexOffsets::AuxRangeMin, M7::ModParamIndexOffsets::AuxRangeMax, false, isEnabled && isAuxEnabled);

					if (ImGui::SmallButton("Reset")) {
						spec.mParams.SetRangedValue(M7::ModParamIndexOffsets::AuxRangeMin, -3, 3, 0);
						spec.mParams.SetRangedValue(M7::ModParamIndexOffsets::AuxRangeMax, -3, 3, 1);
					}
					ImGui::SameLine();
					if (ImGui::SmallButton("bip. to pos")) {
						spec.mParams.SetRangedValue(M7::ModParamIndexOffsets::AuxRangeMin, -3, 3, -3);
						spec.mParams.SetRangedValue(M7::ModParamIndexOffsets::AuxRangeMax, -3, 3, 1);
					}
					if (ImGui::SmallButton("hide advanced")) {
						mpMaj7VST->mShowAdvancedModAuxControls[imod] = false;
					}
				}
				else {
					auxVal = ModMeter(false, spec, spec.mAuxSource, M7::ModParamIndexOffsets::AuxCurve, M7::ModParamIndexOffsets::AuxRangeMin, M7::ModParamIndexOffsets::AuxRangeMax, false, isEnabled && isAuxEnabled);
					if (ImGui::SmallButton("show advanced")) {
						mpMaj7VST->mShowAdvancedModAuxControls[imod] = true;
					}
				}

				float auxAtten = spec.mParams.Get01Value(M7::ModParamIndexOffsets::AuxAttenuation, 0);
				float auxScale = M7::math::lerp(1, 1.0f - auxAtten, auxVal);
				modVal *= auxScale;

				ImGui::EndGroup();
			}
			ImGui::SameLine();
			WSImGuiParamKnob(enabledParamID + (int)M7::ModParamIndexOffsets::AuxAttenuation, "Atten");
			ImGui::PopID();


			ImGui::SameLine();
			ImGui::BeginGroup();
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

			ImGuiWindow* window = ImGui::GetCurrentWindow();
			ModMeter_Horiz(spec, window->DC.CursorPos, mPrimaryModMeterStyle.meterSize, -1, 1, -1, 1, -1, 1, modVal, isEnabled ? mPrimaryModMeterStyle : mPrimaryModMeterStyleDisabled);
			TooltipF("Modulation output value before it's scaled to the various destinations");

			ImGui::EndGroup();

			ImGui::EndTabItem();

		}
	}


	void AuxEffectTab(const char* labelID, int ifilter/*, ColorMod* auxTabColors[], ColorMod* auxTabDisabledColors[]*/)
	{
		auto& filter = pMaj7->mMaj7Voice[0]->mFilters[ifilter][0];

		ColorMod& cm = filter.mParams.GetBoolValue(M7::FilterParamIndexOffsets::Enabled) ? mAuxLeftColors : mAuxLeftDisabledColors;
		auto token = cm.Push();

		if (WSBeginTabItemWithSel(labelID, ifilter, mFilterTabSelHelper))
		{
			static constexpr char const* const filterModelCaptions[] = FILTER_MODEL_CAPTIONS;

			auto lGetModInfo = [&](M7::FilterAuxModDestOffsets x) {
				return GetModInfo((M7::ModDestination)((int)filter.mModDestBase + (int)x));
			};

			WSImGuiParamCheckbox(filter.mParams.GetParamIndex(M7::FilterParamIndexOffsets::Enabled), "Enabled");

			ImGui::SameLine(); Maj7ImGuiParamEnumCombo(filter.mParams.GetParamIndex(M7::FilterParamIndexOffsets::FilterType), "Type##filt", (int)M7::FilterModel::Count, M7::FilterModel::LP_Moog4, filterModelCaptions);
			ImGui::SameLine(); Maj7ImGuiParamFrequency(filter.mParams.GetParamIndex(M7::FilterParamIndexOffsets::Freq), filter.mParams.GetParamIndex(M7::FilterParamIndexOffsets::FreqKT), "Freq##filt", M7::gFilterFreqConfig, M7::gFreqParamKTUnity, lGetModInfo(M7::FilterAuxModDestOffsets::Freq));
			ImGui::SameLine(); Maj7ImGuiParamScaledFloat(filter.mParams.GetParamIndex(M7::FilterParamIndexOffsets::FreqKT), "KT##filt", 0, 1, 1, 1, 0, {});
			ImGui::SameLine(); Maj7ImGuiParamFloat01(filter.mParams.GetParamIndex(M7::FilterParamIndexOffsets::Q), "Q##filt", 0, 0, 0, lGetModInfo(M7::FilterAuxModDestOffsets::Q));

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

	void Sampler(const char* labelWithID, M7::SamplerDevice& sampler, size_t isrc, int ampEnvModDestBase)
	{
		ColorMod& cm = sampler.IsEnabled() ? mSamplerColors : mSamplerDisabledColors;
		auto token = cm.Push();
		static constexpr char const* const interpModeNames[] = { "Nearest", "Linear" };
		static constexpr char const* const loopModeNames[] = { "Disabled", "Repeat", "Pingpong" };
		static constexpr char const* const loopBoundaryModeNames[] = { "FromSample", "Manual" };

		auto lGetModInfo = [&](M7::SamplerModParamIndexOffsets x) {
			return GetModInfo((M7::ModDestination)((int)pMaj7->mSources[isrc]->mModDestBaseID + (int)x));
		};


		if (WSBeginTabItemWithSel(labelWithID, (int)isrc, mSourceTabSelHelper)) {
			WSImGuiParamCheckbox((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::Enabled), "Enabled");
			ImGui::SameLine(); Maj7ImGuiParamVolume((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::Volume), "Volume", M7::gUnityVolumeCfg, 0, lGetModInfo(M7::SamplerModParamIndexOffsets::Volume));

			ImGui::SameLine(0, 50); Maj7ImGuiParamFrequency((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::FreqParam),
				(int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::FreqKT), "Freq", M7::gSourceFreqConfig, M7::gFreqParamKTUnity,
				lGetModInfo(M7::SamplerModParamIndexOffsets::FrequencyParam));
			ImGui::SameLine(); Maj7ImGuiParamScaledFloat((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::FreqKT), "KT", 0, 1, 1, 1, 0, {});
			ImGui::SameLine(); Maj7ImGuiParamInt((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::TuneSemis), "Transp", M7::gSourcePitchSemisRange, 0, 0);
			ImGui::SameLine(); Maj7ImGuiParamFloatN11((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::TuneFine), "FineTune", 0, 0, lGetModInfo(M7::SamplerModParamIndexOffsets::PitchFine));
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

			ImGui::SameLine(); Maj7ImGuiParamFloatN11((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::AuxMix), "Aux pan", 0, 0, lGetModInfo(M7::SamplerModParamIndexOffsets::AuxMix));

			ImGui::BeginGroup();
			WSImGuiParamCheckbox((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::Reverse), "Reverse");
			
			ImGui::SameLine();
			Maj7ImGuiParamFloat01(sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::SampleStart), "SampleStart", 0, 0, 0, lGetModInfo(M7::SamplerModParamIndexOffsets::SampleStart));
			ImGui::SameLine();
			Maj7ImGuiParamEnvTime(sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::Delay), "SampleDelay", 0, lGetModInfo(M7::SamplerModParamIndexOffsets::Delay));

			ImGui::SameLine(); WSImGuiParamKnob((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::LoopStart), "LoopBeg");
			ImGui::SameLine(); WSImGuiParamKnob((int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::LoopLength), "LoopLen");
			ImGui::EndGroup();

			ImGui::SameLine();
			SamplerWaveformDisplay(sampler, isrc);

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
				ImGui::InputText("Filter#gmdlslist", mGmDlsFilter, std::size(mGmDlsFilter));
				ImGui::BeginListBox("sample");
				int sampleIndex = (int)sampler.mParams.GetIntValue(M7::SamplerParamIndexOffsets::GmDlsIndex, M7::gGmDlsIndexParamCfg);

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
					name = mGmDlsOptions[p->mSampleIndex + 1].first.c_str();
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
				Envelope("Amplitude Envelope", (int)sampler.mParams.GetParamIndex(M7::SamplerParamIndexOffsets::AmpEnvDelayTime), ampEnvModDestBase);
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
	void SamplerWaveformDisplay(M7::SamplerDevice& sampler, size_t isrc)
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
		if (mRenderContext.mpActiveVoice) {
			M7::SamplerVoice* sv = static_cast<M7::SamplerVoice*>(mRenderContext.mpActiveVoice->mSourceVoices[isrc]);
			cursor = (float)sv->mSamplePlayer.samplePos;
			cursor /= sampler.mSample->GetSampleLength();
		}

		auto sampleStart = sampler.mParams.Get01Value(M7::SamplerParamIndexOffsets::SampleStart, 0);
		auto loopStart = sampler.mParams.Get01Value(M7::SamplerParamIndexOffsets::LoopStart, 0);
		auto loopLength = sampler.mParams.Get01Value(M7::SamplerParamIndexOffsets::LoopLength, 0);
		WaveformGraphic(isrc, gSamplerWaveformHeight, peaks, sampleStart, loopStart, loopLength, cursor);
	}

}; // class maj7editor
