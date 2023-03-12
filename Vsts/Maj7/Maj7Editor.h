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

template<typename T>
struct ComPtr
{
private:
	T* mp = nullptr;
	void Release() {
		if (mp) {
			mp->Release();
			mp = nullptr;
		}
	}
public:
	~ComPtr() {
		Release();
	}
	void Set(T* p, bool incrRef) {
		Release();
		if (p) {
			mp = p;
			if (incrRef) {
				mp->AddRef();
			}
		}
	}
	T** GetReceivePtr() { // many COM Calls return an interface with a reference for the caller. this receives the object and assumes a ref has already been added to keep.
		Release();
		return &mp;
	}
	T* get() {
		return mp;
	}
	T* operator ->() {
		return mp;
	}
	bool operator !() {
		return !!mp;
	}
};
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

	void CopyTextToClipboard(const std::string& s) {

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

	std::string GetClipboardText() {
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

	virtual void PopulateMenuBar() override
	{
		if (ImGui::BeginMenu("Maj7")) {
			bool b = false;
			if (ImGui::MenuItem("Panic", nullptr, false)) {
				pMaj7->AllNotesOff();
			}

			ImGui::Separator();
			ImGui::MenuItem("Show polyphonic inspector", nullptr, &mShowingInspector);
			ImGui::MenuItem("Show color expl", nullptr, &mShowingColorExp);

			ImGui::Separator();
			if (ImGui::MenuItem("Init patch (from core)")) {
				pMaj7->LoadDefaults();
			}

			if (ImGui::MenuItem("Copy patch to clipboard")) {
				CopyPatchToClipboard();
				::MessageBoxA(mCurrentWindow, "Copied to clipboard", "WaveSabre - Maj7", MB_OK);
			}

			if (ImGui::MenuItem("Paste patch from clipboard")) {
				std::string s = GetClipboardText();
				if (!s.empty()) {
					mMaj7VST->setChunk((void*)s.c_str(), VstInt32(s.size() + 1), false); // const cast oooooooh :/
				}
			}

			ImGui::Separator();
			if (ImGui::MenuItem("Init patch (from VST)")) {
				GenerateDefaults(pMaj7);
			}

			if (ImGui::MenuItem("Export as Maj7.cpp defaults to clipboard")) {
				if (IDYES != ::MessageBoxA(mCurrentWindow, "A new maj7.cpp will be copied to the clipboard, based on 1st item params.", "WaveSabre - Maj7", MB_YESNO | MB_ICONQUESTION)) {
					return;
				}
				CopyParamCache();
			}

			if (ImGui::MenuItem("Optimize")) {
				// the idea is to reset any unused parameters to default values, so they end up being 0 in the minified chunk.
				// that compresses better. this is a bit tricky though; i guess i should only do this for like, samplers, oscillators, modulations 1-8, and all envelopes.
				if (IDYES != ::MessageBoxA(mCurrentWindow, "Unused objects will be clobbered; are you sure? Do this as a post-processing step before rendering the minified song.", "WaveSabre - Maj7", MB_YESNO | MB_ICONQUESTION)) {
					return;
				}
				if (IDYES == ::MessageBoxA(mCurrentWindow, "Backup current patch to clipboard?", "WaveSabre - Maj7", MB_YESNO | MB_ICONQUESTION)) {
					CopyPatchToClipboard();
					::MessageBoxA(mCurrentWindow, "Copied to clipboard... click OK to continue to optimization", "WaveSabre - Maj7", MB_OK);
				}
				auto r1 = AnalyzeChunkMinification(pMaj7);
				OptimizeParams(pMaj7);
				auto r2 = AnalyzeChunkMinification(pMaj7);
				char msg[200];
				sprintf_s(msg, "Done!\r\nBefore: %d bytes; %d nondefaults\r\nAfter: %d bytes; %d nondefaults\r\nShrunk to %d %%",
					r1.compressedSize, r1.nonZeroParams,
					r2.compressedSize, r2.nonZeroParams,
					int(((float)r2.compressedSize / r1.compressedSize) * 100)
					);
				::MessageBoxA(mCurrentWindow, msg, "WaveSabre - Maj7", MB_OK);
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
				::MessageBoxA(mCurrentWindow, s.c_str() , "WaveSabre - Maj7", MB_OK);
			}
			ImGui::EndMenu();
		}
	}

	void CopyPatchToClipboard()
	{
		void* data;
		int size;
		size = mMaj7VST->getChunk(&data, false);
		if (data && size) {
			CopyTextToClipboard((const char*)data);
		}
		delete[] data;
		::MessageBoxA(mCurrentWindow, "Copied to clipboard", "WaveSabre - Maj7", MB_OK);
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
			ss << "    const float " << arrayName << "[" << count << "] = {" << std::endl;
			for (size_t i = 0; i < count; ++i) {
				size_t paramID = baseParamID + i;
				ss << std::setprecision(20) << "      " << GetEffectX()->getParameter((VstInt32)paramID) << ", // " << paramNames[paramID] << std::endl;
			}
			ss << "    };" << std::endl;
		};

		GenerateArray("gDefaultMasterParams", (int)M7::MainParamIndices::Count, "M7::MainParamIndices::Count", 0);
		GenerateArray("gDefaultSamplerParams", (int)M7::SamplerParamIndexOffsets::Count, "M7::SamplerParamIndexOffsets::Count", (int)pMaj7->mSamplerDevices[0].mBaseParamID);
		GenerateArray("gDefaultModSpecParams", (int)M7::ModParamIndexOffsets::Count, "M7::ModParamIndexOffsets::Count", (int)pMaj7->mModulations[0].mBaseParamID);
		GenerateArray("gDefaultLFOParams", (int)M7::LFOParamIndexOffsets::Count, "M7::LFOParamIndexOffsets::Count", (int)pMaj7->mLFO1Device.mBaseParamID);
		GenerateArray("gDefaultEnvelopeParams", (int)M7::EnvParamIndexOffsets::Count, "M7::EnvParamIndexOffsets::Count", (int)pMaj7->mMaj7Voice[0]->mOsc1AmpEnv.mParamBaseID);
		GenerateArray("gDefaultOscillatorParams", (int)M7::OscParamIndexOffsets::Count, "M7::OscParamIndexOffsets::Count", (int)pMaj7->mOscillatorDevices[0].mBaseParamID);
		GenerateArray("gDefaultAuxParams", (int)M7::AuxParamIndexOffsets::Count, "M7::AuxParamIndexOffsets::Count", (int)pMaj7->mMaj7Voice[0]->mAux1.mBaseParamID);

		ss << "  } // namespace M7" << std::endl;
		ss << "} // namespace WaveSabreCore" << std::endl;
		ImGui::SetClipboardText(ss.str().c_str());

		::MessageBoxA(mCurrentWindow, "Copied new contents of WaveSabreCore/Maj7.cpp", "WaveSabre Maj7", MB_OK);
	}

	virtual std::string GetMenuBarStatusText() override 
	{
		std::string ret = " Voices:";
		ret += std::to_string(pMaj7->GetCurrentPolyphony());
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

	struct ColorMod
	{
		float mHueAdd;
		float mSaturationMul;
		float mValueMul;
		float mTextValue;
		float mTextDisabledValue;

		bool mInitialized = false;
		ImVec4 mNewColors[ImGuiCol_COUNT];

		struct Token
		{
			ImVec4 mOldColors[ImGuiCol_COUNT];
			bool isSet = false;
			Token(ImVec4* newColors) {
				isSet = !!newColors;
				if (isSet) {
					memcpy(mOldColors, ImGui::GetStyle().Colors, sizeof(mOldColors));
					memcpy(ImGui::GetStyle().Colors, newColors, sizeof(mOldColors));
				}
			}
			Token(Token&& rhs)
			{
				isSet = rhs.isSet;
				rhs.isSet = false;
				memcpy(mOldColors, rhs.mOldColors, sizeof(mOldColors));
			}
			Token& operator =(Token&& rhs)
			{
				isSet = rhs.isSet;
				rhs.isSet = false;
				memcpy(mOldColors, rhs.mOldColors, sizeof(mOldColors));
				return *this;
			}
			Token() {

			}
			~Token() {
				if (isSet) {
					memcpy(ImGui::GetStyle().Colors, mOldColors, sizeof(mOldColors));
				}
			}
		};
		Token Push()
		{
			if (!this->mInitialized) {
				return { nullptr };
			}
			return { mNewColors };
		}

		void EnsureInitialized() {
			if (mInitialized) return;
			ImGuiStyle& style = ImGui::GetStyle();

			// correct some things in default style.
			{
				ImVec4 color = style.Colors[ImGuiCol_TabActive];
				float h, s, v;
				ImGui::ColorConvertRGBtoHSV(color.x, color.y, color.z, h, s, v);
				auto newcolor = ImColor::HSV(h, s, 1);
				style.Colors[ImGuiCol_TabActive] = newcolor.Value;
			}

			mInitialized = true;
			for (size_t i = 0; i < ImGuiCol_COUNT; ++i) {
				ImVec4 color = style.Colors[i];
				float h, s, v;
				ImGui::ColorConvertRGBtoHSV(color.x, color.y, color.z, h, s, v);
				auto newcolor = ImColor::HSV(h + mHueAdd, s * mSaturationMul, v * mValueMul);
				mNewColors[i] = newcolor.Value;
			}
			mNewColors[ImGuiCol_Text] = ImColor::HSV(0, 0, mTextValue);
			mNewColors[ImGuiCol_TextDisabled] = ImColor::HSV(0, 0, mTextDisabledValue);
		}

		ColorMod(float hueAdd, float satMul, float valMul, float textVal, float textDisabledVal) :
			mHueAdd(hueAdd),
			mSaturationMul(satMul),
			mValueMul(valMul),
			mTextValue(textVal),
			mTextDisabledValue(textDisabledVal)
		{
		}

		// really just for the NOP color
		ColorMod() :
			mHueAdd(0),
			mSaturationMul(1),
			mValueMul(1),
			mTextValue(.8f),
			mTextDisabledValue(.4f)
		{
		}
	};

	ColorMod mModulationsColors{ 0.15f, 0.6f, 0.65f, 0.9f, 0.0f };
	ColorMod mModulationDisabledColors{ 0.15f, 0.0f, 0.65f, 0.6f, 0.0f };

	ColorMod mAuxLeftColors{ 0.1f, 1, 1, .9f, 0.5f };
	ColorMod mAuxRightColors{ 0.4f, 1, 1, .9f, 0.5f };
	ColorMod mAuxLeftDisabledColors{ 0.1f, 0.25f, .4f, .5f, 0.1f };
	ColorMod mAuxRightDisabledColors{ 0.4f, 0.25f, .4f, .5f, 0.1f };

	ColorMod mOscColors{ 0, 1, 1, 0.9f, 0.0f };
	ColorMod mOscDisabledColors{ 0, .15f, .6f, 0.5f, 0.2f };

	ColorMod mSamplerColors{ 0.55f, 0.8f, .9f, 1.0f, 0.5f };
	ColorMod mSamplerDisabledColors{ 0.55f, 0.15f, .6f, 0.5f, 0.2f };

	ColorMod mCyanColors{ 0.92f, 0.6f, 0.75f, 0.9f, 0.0f };
	ColorMod mPinkColors{ 0.40f, 0.6f, 0.75f, 0.9f, 0.0f };

	ColorMod mNopColors;

	bool mShowingInspector = false;
	bool mShowingColorExp = false;

	bool BeginTabBar2(const char* str_id, ImGuiTabBarFlags flags, float columns = 1)
	{
		tabBarStoredSeparatorColor = ImGui::GetColorU32(ImGuiCol_TabActive);
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		ImGuiID id = window->GetID(str_id);
		ImGuiTabBar* tab_bar = g.TabBars.GetOrAddByKey(id);
		// Copied from imgui with changes:
		// 1. shrinking workrect max for column support
		// 2. adding X cursor pos for side-by-side positioning
		// 3. pushing empty separator color so BeginTabBarEx doesn't display a separator line (we'll do it later).
		ImRect tab_bar_bb = ImRect(window->DC.CursorPos.x, window->DC.CursorPos.y, window->DC.CursorPos.x + (window->WorkRect.Max.x / columns), window->DC.CursorPos.y + g.FontSize + g.Style.FramePadding.y * 2);
		tab_bar->ID = id;
		ImGui::PushStyleColor(ImGuiCol_TabActive, {});// : ImGuiCol_TabUnfocusedActive
		bool ret = ImGui::BeginTabBarEx(tab_bar, tab_bar_bb, flags | ImGuiTabBarFlags_IsFocused);
		ImGui::PopStyleColor(1);
		return ret;
	}

	ImRect tabBarBB;
	ImU32 tabBarStoredSeparatorColor;

	bool WSBeginTabItem(const char* label, bool* p_open = 0, ImGuiTabItemFlags flags = 0)
	{
		if (ImGui::BeginTabItem(label, p_open, flags)) {
			tabBarStoredSeparatorColor = ImGui::GetColorU32(ImGuiCol_TabActive);
			return true;
		}
		return false;
	}

	void EndTabBarWithColoredSeparator()
	{
		ImGuiContext& g = *GImGui;
		ImGuiWindow* window = g.CurrentWindow;
		ImGuiTabBar* tab_bar = g.CurrentTabBar;
		const ImU32 col = tabBarStoredSeparatorColor;
		const float y = tab_bar->BarRect.Max.y - 1.0f;

		const float separator_min_x = tab_bar->BarRect.Min.x - M7::math::floor(window->WindowPadding.x * 0.5f);
		const float separator_max_x = tab_bar->BarRect.Max.x + M7::math::floor(window->WindowPadding.x * 0.5f);

		ImRect body_bb;
		body_bb.Min = { separator_min_x, y };
		body_bb.Max = { separator_max_x, window->DC.CursorPos.y };

		ImColor c2 = { col };
		float h, s, v;
		ImGui::ColorConvertRGBtoHSV(c2.Value.x, c2.Value.y, c2.Value.z, h, s, v);
		auto halfAlpha = ImColor::HSV(h, s, v, c2.Value.w * 0.5f);
		auto lowAlpha = ImColor::HSV(h, s, v, c2.Value.w * 0.15f);

		//window->DrawList->AddLine(ImVec2(separator_min_x, y), ImVec2(separator_max_x, y), col, 1.0f);

		ImGui::GetBackgroundDrawList()->AddRectFilled(body_bb.Min, body_bb.Max, lowAlpha, 6, ImDrawFlags_RoundCornersAll);
		ImGui::GetBackgroundDrawList()->AddRect(body_bb.Min, body_bb.Max, halfAlpha, 6, ImDrawFlags_RoundCornersAll);

		ImGui::EndTabBar();

		// add some bottom margin.
		ImGui::Dummy({0, 7});
	}


	virtual void renderImgui() override
	{
		mModulationsColors.EnsureInitialized();
		mModulationDisabledColors.EnsureInitialized();
		mPinkColors.EnsureInitialized();
		mCyanColors.EnsureInitialized();
		mOscColors.EnsureInitialized();
		mOscDisabledColors.EnsureInitialized();
		mAuxLeftColors.EnsureInitialized();
		mAuxRightColors.EnsureInitialized();
		mAuxLeftDisabledColors.EnsureInitialized();
		mAuxRightDisabledColors.EnsureInitialized();
		mSamplerDisabledColors.EnsureInitialized();
		mSamplerColors.EnsureInitialized();

		{
			auto& style = ImGui::GetStyle();
			style.FramePadding.x = 7;
			style.FramePadding.y = 5;
			style.ItemSpacing.x = 6;
			style.TabRounding = 5;
			//style.WindowPadding.x = 10;
		}

		// color explorer
		ColorMod::Token colorExplorerToken;
		if (mShowingColorExp) {
			static float colorHueVarAmt = 0;
			static float colorSaturationVarAmt = 1;
			static float colorValueVarAmt = 1;
			static float colorTextVal = 0.9f;
			static float colorTextDisabledVal = 0.5f;
			bool b1 = ImGuiKnobs::Knob("hue", &colorHueVarAmt, -1.0f, 1.0f, 0.0f, gNormalKnobSpeed, gSlowKnobSpeed);
			ImGui::SameLine(); bool b2 = ImGuiKnobs::Knob("sat", &colorSaturationVarAmt, 0.0f, 1.0f, 0.0f, gNormalKnobSpeed, gSlowKnobSpeed);
			ImGui::SameLine(); bool b3 = ImGuiKnobs::Knob("val", &colorValueVarAmt, 0.0f, 1.0f, 0.0f, gNormalKnobSpeed, gSlowKnobSpeed);
			ImGui::SameLine(); bool b4 = ImGuiKnobs::Knob("txt", &colorTextVal, 0.0f, 1.0f, 0.0f, gNormalKnobSpeed, gSlowKnobSpeed);
			ImGui::SameLine(); bool b5 = ImGuiKnobs::Knob("txtD", &colorTextDisabledVal, 0.0f, 1.0f, 0.0f, gNormalKnobSpeed, gSlowKnobSpeed);
			ColorMod xyz{ colorHueVarAmt , colorSaturationVarAmt, colorValueVarAmt, colorTextVal, colorTextDisabledVal };
			xyz.EnsureInitialized();
			colorExplorerToken = std::move(xyz.Push());
		}


		Maj7ImGuiParamVolume((VstInt32)M7::ParamIndices::MasterVolume, "Volume##hc", M7::Maj7::gMasterVolumeMaxDb, 0.5f);
		ImGui::SameLine();
		Maj7ImGuiParamInt((VstInt32)M7::ParamIndices::Unisono, "Unison##mst", 1, M7::Maj7::gUnisonoVoiceMax, 1);

		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)M7::ParamIndices::OscillatorDetune, "OscDetune##mst");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)M7::ParamIndices::UnisonoDetune, "UniDetune##mst");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)M7::ParamIndices::OscillatorSpread, "OscPan##mst");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)M7::ParamIndices::UnisonoStereoSpread, "UniPan##mst");

		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)M7::ParamIndices::OscillatorShapeSpread, "OscShape##mst");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)M7::ParamIndices::UnisonoShapeSpread, "UniShape##mst");

		ImGui::SameLine(0, 60);
		Maj7ImGuiParamInt((VstInt32)M7::ParamIndices::PitchBendRange, "PB Range##mst", -M7::Maj7::gPitchBendMaxRange, M7::Maj7::gPitchBendMaxRange, 2);
		ImGui::SameLine();
		Maj7ImGuiParamEnvTime((VstInt32)M7::ParamIndices::PortamentoTime, "Port##mst", 0.4f);
		ImGui::SameLine();
		Maj7ImGuiParamCurve((VstInt32)M7::ParamIndices::PortamentoCurve, "##portcurvemst", 0.0f, M7CurveRenderStyle::Rising);

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
			Sampler("Sampler 1", pMaj7->mSamplerDevices[0], isrc ++);
			Sampler("Sampler 2", pMaj7->mSamplerDevices[1], isrc ++);
			Sampler("Sampler 3", pMaj7->mSamplerDevices[2], isrc ++);
			Sampler("Sampler 4", pMaj7->mSamplerDevices[3], isrc ++);
			EndTabBarWithColoredSeparator();
		}

		ImGui::BeginTable("##fmaux", 2);
		ImGui::TableNextRow();
		ImGui::TableNextColumn();

		//ImGui::BeginGroup();
		if (BeginTabBar2("FM", ImGuiTabBarFlags_None, 2.2f))
		{
			auto colorModToken = mCyanColors.Push();
			static_assert(M7::Maj7::gOscillatorCount == 4, "osc count");
			if (WSBeginTabItem("Phase Mod Matrix")) {
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
			auto modColorModToken = mModulationsColors.Push();
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
			if (WSBeginTabItem("LFO 1"))
			{
				LFO("LFO 1", (int)M7::ParamIndices::LFO1Waveform);
				ImGui::EndTabItem();
			}
			if (WSBeginTabItem("LFO 2"))
			{
				LFO("LFO 2", (int)M7::ParamIndices::LFO2Waveform);
				ImGui::EndTabItem();
			}
			EndTabBarWithColoredSeparator();
		}

		if (BeginTabBar2("modspectabs", ImGuiTabBarFlags_None))
		{
			ModulationSection("Mod 1", this->pMaj7->mModulations[0], (int)M7::ParamIndices::Mod1Enabled);
			ModulationSection("Mod 2", this->pMaj7->mModulations[1], (int)M7::ParamIndices::Mod2Enabled);
			ModulationSection("Mod 3", this->pMaj7->mModulations[2], (int)M7::ParamIndices::Mod3Enabled);
			ModulationSection("Mod 4", this->pMaj7->mModulations[3], (int)M7::ParamIndices::Mod4Enabled);
			ModulationSection("Mod 5", this->pMaj7->mModulations[4], (int)M7::ParamIndices::Mod5Enabled);
			ModulationSection("Mod 6", this->pMaj7->mModulations[5], (int)M7::ParamIndices::Mod6Enabled);
			ModulationSection("Mod 7", this->pMaj7->mModulations[6], (int)M7::ParamIndices::Mod7Enabled);
			ModulationSection("Mod 8", this->pMaj7->mModulations[7], (int)M7::ParamIndices::Mod8Enabled);
			ModulationSection("Mod 9", this->pMaj7->mModulations[8], (int)M7::ParamIndices::Mod9Enabled);
			ModulationSection("Mod 10", this->pMaj7->mModulations[9], (int)M7::ParamIndices::Mod10Enabled);
			ModulationSection("Mod 11", this->pMaj7->mModulations[10], (int)M7::ParamIndices::Mod11Enabled);
			ModulationSection("Mod 12", this->pMaj7->mModulations[11], (int)M7::ParamIndices::Mod12Enabled);
			ModulationSection("Mod 13", this->pMaj7->mModulations[12], (int)M7::ParamIndices::Mod13Enabled);
			ModulationSection("Mod 14", this->pMaj7->mModulations[13], (int)M7::ParamIndices::Mod14Enabled);
			ModulationSection("Mod 15", this->pMaj7->mModulations[14], (int)M7::ParamIndices::Mod15Enabled);
			ModulationSection("Mod 16", this->pMaj7->mModulations[15], (int)M7::ParamIndices::Mod16Enabled);

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
			ImGui::SeparatorText("Inspector");

			for (size_t i = 0; i < std::size(pMaj7->mVoices); ++i) {
				auto pv = (M7::Maj7::Maj7Voice*)pMaj7->mVoices[i];
				char txt[200];
				if (i & 0x1) ImGui::SameLine();
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
				ImGui::SameLine(); ImGui::ProgressBar(pv->mOsc1AmpEnv.GetLastOutputLevel(), ImVec2{ 50, 0 }, "Amp1");
				ImGui::SameLine(); ImGui::ProgressBar(pv->mOsc2AmpEnv.GetLastOutputLevel(), ImVec2{ 50, 0 }, "Amp2");
				ImGui::SameLine(); ImGui::ProgressBar(pv->mOsc3AmpEnv.GetLastOutputLevel(), ImVec2{ 50, 0 }, "Amp3");
				ImGui::SameLine(); ImGui::ProgressBar(pv->mOsc4AmpEnv.GetLastOutputLevel(), ImVec2{ 50, 0 }, "Amp4");
				ImGui::SameLine(); ImGui::ProgressBar(::fabsf(pv->mOscillator1.GetSample()), ImVec2{ 50, 0 }, "Osc1");
				ImGui::SameLine(); ImGui::ProgressBar(::fabsf(pv->mOscillator2.GetSample()), ImVec2{ 50, 0 }, "Osc2");
				ImGui::SameLine(); ImGui::ProgressBar(::fabsf(pv->mOscillator3.GetSample()), ImVec2{ 50, 0 }, "Osc3");
				ImGui::SameLine(); ImGui::ProgressBar(::fabsf(pv->mOscillator4.GetSample()), ImVec2{ 50, 0 }, "Osc4");
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
			ImGui::SameLine(); Maj7ImGuiParamVolume(enabledParamID + (int)M7::OscParamIndexOffsets::Volume, "Volume", 0, 0);

			//ImGui::SameLine(); Maj7ImGuiParamEnumCombo(enabledParamID + (int)M7::OscParamIndexOffsets::Waveform, "Waveform", M7::OscillatorWaveform::Count, 0, gWaveformCaptions);

			ImGui::SameLine(); WaveformParam(enabledParamID + (int)M7::OscParamIndexOffsets::Waveform, enabledParamID + (int)M7::OscParamIndexOffsets::Waveshape, enabledParamID + (int)M7::OscParamIndexOffsets::PhaseOffset);
			//ImGui::SameLine(); WaveformGraphic(waveformParam.GetEnumValue(), waveshapeParam.Get01Value());
			//ImGui::SameLine(); WSImGuiParamKnob(enabledParamID + (int)M7::OscParamIndexOffsets::Waveform, "Waveform");
			ImGui::SameLine(); WSImGuiParamKnob(enabledParamID + (int)M7::OscParamIndexOffsets::Waveshape, "Shape");
			ImGui::SameLine(0, 60); Maj7ImGuiParamFrequency(enabledParamID + (int)M7::OscParamIndexOffsets::FrequencyParam, enabledParamID + (int)M7::OscParamIndexOffsets::FrequencyParamKT, "Freq", M7::gSourceFrequencyCenterHz, M7::gSourceFrequencyScale, 0.4f);
			ImGui::SameLine(); WSImGuiParamKnob(enabledParamID + (int)M7::OscParamIndexOffsets::FrequencyParamKT, "KT");
			ImGui::SameLine(); Maj7ImGuiParamInt(enabledParamID + (int)M7::OscParamIndexOffsets::PitchSemis, "Transp", -M7::gSourcePitchSemisRange, M7::gSourcePitchSemisRange, 0);
			ImGui::SameLine(); Maj7ImGuiParamFloatN11(enabledParamID + (int)M7::OscParamIndexOffsets::PitchFine, "FineTune", 0);
			ImGui::SameLine(); Maj7ImGuiParamScaledFloat(enabledParamID + (int)M7::OscParamIndexOffsets::FreqMul, "FreqMul", 0, M7::OscillatorDevice::gFrequencyMulMax, 1);
			ImGui::SameLine(0, 60); WSImGuiParamCheckbox(enabledParamID + (int)M7::OscParamIndexOffsets::PhaseRestart, "PhaseRst");
			ImGui::SameLine(); Maj7ImGuiParamFloatN11(enabledParamID + (int)M7::OscParamIndexOffsets::PhaseOffset, "Phase", 0);
			ImGui::SameLine(0, 60); WSImGuiParamCheckbox(enabledParamID + (int)M7::OscParamIndexOffsets::SyncEnable, "Sync");
			ImGui::SameLine(); Maj7ImGuiParamFrequency(enabledParamID + (int)M7::OscParamIndexOffsets::SyncFrequency, enabledParamID + (int)M7::OscParamIndexOffsets::SyncFrequencyKT, "SyncFreq", M7::OscillatorDevice::gSyncFrequencyCenterHz, M7::OscillatorDevice::gSyncFrequencyScale, 0.4f);
			ImGui::SameLine(); WSImGuiParamKnob(enabledParamID + (int)M7::OscParamIndexOffsets::SyncFrequencyKT, "SyncKT");

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

			Maj7ImGuiParamInt(enabledParamID + (int)M7::OscParamIndexOffsets::KeyRangeMin, "KeyRangeMin", 0, 127, 0);
			ImGui::SameLine(); Maj7ImGuiParamInt(enabledParamID + (int)M7::OscParamIndexOffsets::KeyRangeMax, "KeyRangeMax", 0, 127, 127);

			ImGui::SameLine();
			{
				//ColorMod::Token ampEnvColorModToken{ (ampSourceParam.GetIntValue() != oscID) ? mPinkColors.mNewColors : nullptr };
				Envelope("Amplitude Envelope", (int)ampEnvSource);
			}
			ImGui::EndTabItem();
		}
	}

	void LFO(const char* labelWithID, int waveformParamID)
	{
		ImGui::PushID(labelWithID);
		//if (ImGui::CollapsingHeader(labelWithID)) {
		//WSImGuiParamKnob(waveformParamID + (int)M7::LFOParamIndexOffsets::Waveform, "Waveform");
		WaveformParam(waveformParamID + (int)M7::LFOParamIndexOffsets::Waveform, waveformParamID + (int)M7::LFOParamIndexOffsets::Waveshape, waveformParamID + (int)M7::LFOParamIndexOffsets::PhaseOffset);

		ImGui::SameLine(); WSImGuiParamKnob(waveformParamID + (int)M7::LFOParamIndexOffsets::Waveshape, "Shape");
		ImGui::SameLine(0, 60); Maj7ImGuiParamFrequency(waveformParamID + (int)M7::LFOParamIndexOffsets::FrequencyParam, -1, "Freq", M7::gLFOFrequencyCenterHz, M7::gLFOFrequencyScale, 0.4f);
		ImGui::SameLine(0, 60); WSImGuiParamKnob(waveformParamID + (int)M7::LFOParamIndexOffsets::PhaseOffset, "Phase");
		ImGui::SameLine(); WSImGuiParamCheckbox(waveformParamID + (int)M7::LFOParamIndexOffsets::Restart, "Restart");

		ImGui::SameLine(0, 60); Maj7ImGuiParamFrequency(waveformParamID + (int)M7::LFOParamIndexOffsets::Sharpness, -1, "Sharpness", M7::gLFOLPCenterFrequency, M7::gLFOLPFrequencyScale, 0.5f);

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
		WSImGuiParamCheckbox(delayTimeParamID + (int)M7::EnvParamIndexOffsets::LegatoRestart, "Leg.Restart");

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

	M7::AuxNode GetDummyAuxNode(float (&paramValues)[(int)M7::AuxParamIndexOffsets::Count], int iaux)
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
		return M7::AuxNode{ auxInfo.mSelfLink, 0, paramValues, (int)auxInfo.mModParam2ID };
	}

	std::string GetAuxName(int iaux, std::string idsuffix)
	{
		// (link:Aux1)
		// (Filter)
		float paramValues[(int)M7::AuxParamIndexOffsets::Count];
		M7::AuxNode a = GetDummyAuxNode(paramValues, iaux);
		auto ret = std::string{ "Aux " } + std::to_string(iaux + 1);
		if (a.IsLinkedExternally()) {
			ret += " (*Aux ";
			ret += std::to_string(a.mLink.GetIntValue() + 1);
			ret += ")###";
			ret += idsuffix;
			return ret;
		}
		switch (a.mEffectType.GetEnumValue())
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
		M7::AuxNode a = GetDummyAuxNode(paramValues, iaux);
		if (a.IsLinkedExternally()) {
			labels[0] += " (shadowed)";
			labels[1] += " (shadowed)";
			labels[2] += " (shadowed)";
			labels[3] += " (shadowed)";
			return;
		}
		char const* const* suffixes;
		switch (a.mEffectType.GetEnumValue())
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

	void ModulationSection(const char* labelWithID, M7::ModulationSpec& spec, int enabledParamID)
	{
		static constexpr char const* const modSourceCaptions[] = MOD_SOURCE_CAPTIONS;
		std::string modDestinationCaptions[(size_t)M7::ModDestination::Count] = MOD_DEST_CAPTIONS;
		char const* modDestinationCaptionsCstr[(size_t)M7::ModDestination::Count];

		// fix dynamic aux destination names
		FillAuxParamNames(&modDestinationCaptions[(int)M7::ModDestination::Aux1Param2], 0);
		FillAuxParamNames(&modDestinationCaptions[(int)M7::ModDestination::Aux2Param2], 1);
		FillAuxParamNames(&modDestinationCaptions[(int)M7::ModDestination::Aux3Param2], 2);
		FillAuxParamNames(&modDestinationCaptions[(int)M7::ModDestination::Aux4Param2], 3);

		for (size_t i = 0; i < (size_t)M7::ModDestination::Count; ++i)
		{
			modDestinationCaptionsCstr[i] = modDestinationCaptions[i].c_str();
		}

		bool isLocked = spec.mType != M7::ModulationSpecType::General;
		ColorMod& cm = spec.mEnabled.GetBoolValue() ? (isLocked ? mPinkColors : mModulationsColors) : mModulationDisabledColors;
		auto token = cm.Push();

		if (WSBeginTabItem(labelWithID))
		{
			ImGui::BeginDisabled(isLocked);
			WSImGuiParamCheckbox((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::Enabled, "Enabled");
			ImGui::EndDisabled();
			ImGui::SameLine();
			Maj7ImGuiParamEnumCombo((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::Source, "Source", (int)M7::ModSource::Count, M7::ModSource::None, modSourceCaptions);
			ImGui::SameLine();
			ImGui::BeginDisabled(isLocked);
			Maj7ImGuiParamEnumCombo((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::Destination, "Dest", (int)M7::ModDestination::Count, M7::ModDestination::None, modDestinationCaptionsCstr);
			ImGui::EndDisabled();
			ImGui::SameLine();
			Maj7ImGuiParamFloatN11(enabledParamID + (int)M7::ModParamIndexOffsets::Scale, "Scale", 1);
			ImGui::SameLine();
			WSImGuiParamCheckbox((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::Invert, "Invert");
			ImGui::SameLine();
			Maj7ImGuiParamCurve((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::Curve, "Curve", 0, M7CurveRenderStyle::Rising);

			ImGui::SameLine(0, 60); WSImGuiParamCheckbox((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::AuxEnabled, "SC Enable");
			ColorMod& cmaux = spec.mAuxEnabled.GetBoolValue() ? mNopColors : mModulationDisabledColors;
			auto auxToken = cmaux.Push();
			ImGui::SameLine();
			Maj7ImGuiParamEnumCombo((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::AuxSource, "SC Src", (int)M7::ModSource::Count, M7::ModSource::None, modSourceCaptions);
			ImGui::SameLine();
			WSImGuiParamKnob(enabledParamID + (int)M7::ModParamIndexOffsets::AuxAttenuation, "SC atten");
			ImGui::SameLine();
			WSImGuiParamCheckbox((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::AuxInvert, "SCInvert");
			ImGui::SameLine();
			Maj7ImGuiParamCurve((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::AuxCurve, "SC Curve", 0, M7CurveRenderStyle::Rising);
			ImGui::EndTabItem();

		}
	}

	void AuxFilter(const AuxInfo& auxInfo)
	{
		static constexpr char const* const filterModelCaptions[] = FILTER_MODEL_CAPTIONS;
		Maj7ImGuiParamEnumCombo((int)auxInfo.mEnabledParamID + (int)M7::FilterAuxParamIndexOffsets::FilterType, "Type##filt", (int)M7::FilterModel::Count, M7::FilterModel::LP_Moog4, filterModelCaptions);
		ImGui::SameLine(0, 60); Maj7ImGuiParamFrequency((int)auxInfo.mEnabledParamID + (int)M7::FilterAuxParamIndexOffsets::Freq, (int)auxInfo.mEnabledParamID + (int)M7::FilterAuxParamIndexOffsets::FreqKT, "Freq##filt", M7::gFilterCenterFrequency, M7::gFilterFrequencyScale, 0.4f);
		ImGui::SameLine(); WSImGuiParamKnob((int)auxInfo.mEnabledParamID + (int)M7::FilterAuxParamIndexOffsets::FreqKT, "KT##filt");
		ImGui::SameLine(); WSImGuiParamKnob((int)auxInfo.mEnabledParamID + (int)M7::FilterAuxParamIndexOffsets::Q, "Q##filt");
		ImGui::SameLine(0, 60); WSImGuiParamKnob((int)auxInfo.mEnabledParamID + (int)M7::FilterAuxParamIndexOffsets::Saturation, "Saturation##filt");
	}

	void AuxBitcrush(const AuxInfo& auxInfo)
	{
		Maj7ImGuiParamFrequency((int)auxInfo.mEnabledParamID + (int)M7::BitcrushAuxParamIndexOffsets::Freq, (int)auxInfo.mEnabledParamID + (int)M7::BitcrushAuxParamIndexOffsets::FreqKT, "Freq##filt", M7::gBitcrushFreqCenterFreq, M7::gBitcrushFreqRange, 0.4f);
		ImGui::SameLine(); WSImGuiParamKnob((int)auxInfo.mEnabledParamID + (int)M7::BitcrushAuxParamIndexOffsets::FreqKT, "KT##filt");
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
		M7::AuxNode a = GetDummyAuxNode(paramValues, iaux);

		ColorMod& cm = a.mEnabledParam.GetBoolValue() ? *auxTabColors[iaux] : *auxTabDisabledColors[iaux];
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
						M7::AuxNode srcNode = GetDummyAuxNode(srcParamValues, n);

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

						auto srcLink = srcNode.mLink.GetEnumValue();
						auto origLink = a.mLink.GetEnumValue();

						// if source links to itself, then we should now link to ourself.
						if (srcLink == srcNode.mLinkToSelf) {
							a.mLink.SetEnumValue(a.mLinkToSelf);
							GetEffectX()->setParameter((int)auxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Link, a.mLink.GetRawParamValue());
						}
						else if (srcLink == a.mLinkToSelf) {
							// if you are swapping ORIG with SRC and SRC links to ORIG, now ORIG will need to point to SRC
							a.mLink.SetEnumValue(srcNode.mLinkToSelf);
							GetEffectX()->setParameter((int)auxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Link, a.mLink.GetRawParamValue());
						}

						// if we linked to ourself, then source should now link to itself
						if (origLink == a.mLinkToSelf) {
							srcNode.mLink.SetEnumValue(srcNode.mLinkToSelf);
							GetEffectX()->setParameter((int)srcAuxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Link, srcNode.mLink.GetRawParamValue());
						}
						else if (origLink == srcNode.mLinkToSelf) {
							// similar logic as above.
							srcNode.mLink.SetEnumValue(a.mLinkToSelf);
							GetEffectX()->setParameter((int)srcAuxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Link, srcNode.mLink.GetRawParamValue());
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
						M7::AuxNode srcNode = GetDummyAuxNode(srcParamValues, n);

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
							a.mLink.SetEnumValue(a.mLinkToSelf);
							GetEffectX()->setParameter((int)auxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Link, a.mLink.GetRawParamValue());
						}
					}
					ImGui::PopID();
				}
				ImGui::EndPopup();
			}

			{
				ColorMod& cm = (a.IsLinkedExternally()) ? *auxTabDisabledColors[auxInfo.mIndex] : mNopColors;
				auto colorToken = cm.Push();

				switch (a.mEffectType.GetEnumValue())
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

	void WaveformGraphic(M7::OscillatorWaveform waveform, float waveshape01, float phaseOffsetN11, ImRect bb)
	{
		OSCILLATOR_WAVEFORM_CAPTIONS(gWaveformCaptions);
		std::unique_ptr<M7::IOscillatorWaveform> pWaveform;

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
		case M7::OscillatorWaveform::SineAsym:
			pWaveform.reset(new M7::SineAsymWaveform);
			break;
		case M7::OscillatorWaveform::SineClip:
			pWaveform.reset(new M7::SineClipWaveform);
			break;
		case M7::OscillatorWaveform::SineHarmTrunc:
			pWaveform.reset(new M7::SineHarmTruncWaveform);
			break;
		case M7::OscillatorWaveform::SineTrunc:
			pWaveform.reset(new M7::SineTruncWaveform);
			break;
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
			pWaveform.reset(new M7::WhiteNoiseWaveform);
			break;
		}

		float innerHeight = bb.GetHeight() - 4;

		// freq & samplerate should be set such that we have `width` samples per 1 cycle.
		// samples per cycle = srate / freq
		pWaveform->SetParams(1, 0, waveshape01, bb.GetWidth(), 1);
		//pWaveform->OSC_RESTART(0);

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
		drawList->AddLine({ outerTL.x, centerY }, {outerBR.x, centerY }, ImGui::GetColorU32(ImGuiCol_PlotLines), 2.0f);// center line
		for (size_t iSample = 0; iSample < bb.GetWidth(); ++iSample) {
			float sample = pWaveform->NaiveSample(M7::math::fract(float(iSample) / bb.GetWidth() + phaseOffsetN11));
			sample = (sample + pWaveform->mDCOffset) * pWaveform->mScale;
			drawList->AddLine({outerTL.x + iSample, centerY }, {outerTL.x + iSample, sampleToY(sample)}, ImGui::GetColorU32(ImGuiCol_PlotHistogram), 1);
		}

		drawList->AddText({ bb.Min.x + 1, bb.Min.y + 1 }, ImGui::GetColorU32(ImGuiCol_TextDisabled), gWaveformCaptions[(int)waveform]);
		drawList->AddText(bb.Min, ImGui::GetColorU32(ImGuiCol_Text), gWaveformCaptions[(int)waveform]);
	}

	bool WaveformButton(ImGuiID id, M7::OscillatorWaveform waveform, float waveshape01, float phaseOffsetN11)
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

		WaveformGraphic(waveform, waveshape01, phaseOffsetN11, bb);

		bool pressed = ImGui::ButtonBehavior(bb, id, &hovered, &held, flags);

		return pressed;
	}

	void WaveformParam(int waveformParamID, int waveshapeParamID, int phaseOffsetParamID)
	{
		OSCILLATOR_WAVEFORM_CAPTIONS(gWaveformCaptions);
		M7::EnumParam<M7::OscillatorWaveform> waveformParam(pMaj7->mParamCache[waveformParamID], M7::OscillatorWaveform::Count);
		M7::Float01Param waveshapeParam(pMaj7->mParamCache[waveshapeParamID]);
		M7::OscillatorWaveform selectedWaveform = waveformParam.GetEnumValue();
		float waveshape01 = waveshapeParam.Get01Value();
		M7::FloatN11Param phaseOffsetParam(pMaj7->mParamCache[phaseOffsetParamID]);
		float phaseOffsetN11 = phaseOffsetParam.GetN11Value();

		if (WaveformButton(waveformParamID, selectedWaveform, waveshape01, phaseOffsetN11)) {
			ImGui::OpenPopup("selectWaveformPopup");
		}
		ImGui::SameLine();
		if (ImGui::BeginPopup("selectWaveformPopup"))
		{
			for (int n = 0; n < (int)M7::OscillatorWaveform::Count; n++)
			{
				M7::OscillatorWaveform wf = (M7::OscillatorWaveform)n;
				ImGui::PushID(n);
				if (WaveformButton(n, wf, waveshapeParam.Get01Value(), phaseOffsetN11)) {
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

	void AuxDistortionGraphic(ImRect bb, int iaux)
	{
		float innerHeight = bb.GetHeight() - 4;

		ImVec2 outerTL = bb.Min;// ImGui::GetCursorPos();
		ImVec2 outerBR = { outerTL.x + bb.GetWidth(), outerTL.y + bb.GetHeight() };

		auto drawList = ImGui::GetWindowDrawList();

		auto sampleToY = [&](float sample) {
			float c = outerBR.y - float(bb.GetHeight()) * 0.5f;
			float h = float(innerHeight) * 0.5f * sample;
			return c - h;
		};

		float paramValues[(int)M7::AuxParamIndexOffsets::Count];
		M7::AuxNode an = GetDummyAuxNode(paramValues, iaux);
		auto pdist = an.CreateEffect(nullptr);
		if (pdist == nullptr)
		{
			//ImGui::Text("Error creating distortion node."); its normal when aux is disabled.
			return;
		}
		size_t nSamples = (size_t)bb.GetWidth();
		M7::ModMatrixNode mm;
		pdist->AuxBeginBlock(0, (int)nSamples, mm);

		std::vector<float> wave;
		wave.reserve(nSamples);
		float maxRectify = 0.001f;
		for (size_t iSample = 0; iSample < nSamples; ++iSample)
		{
			float s = M7::math::fract(float(iSample) / nSamples) * 2 - 1;
			s = pdist->AuxProcessSample(s);
			maxRectify = std::max(maxRectify, std::abs(s));
			wave.push_back(s);
		}

		ImGui::RenderFrame(outerTL, outerBR, ImGui::GetColorU32(ImGuiCol_FrameBg), true, 3.0f); // background
		float centerY = sampleToY(0);
		drawList->AddLine({ outerTL.x, centerY }, { outerBR.x, centerY }, ImGui::GetColorU32(ImGuiCol_PlotLines), 2.0f);// center line
		for (size_t iSample = 0; iSample < nSamples; ++iSample)
		{
			float s = wave[iSample] / maxRectify;
			drawList->AddLine({ outerTL.x + iSample, centerY }, { outerTL.x + iSample, sampleToY(s) }, ImGui::GetColorU32(ImGuiCol_PlotHistogram), 1);
		}
	}

	bool LoadSample(const char* path, M7::SamplerDevice& sampler, size_t isrc)
	{
		std::ifstream input(path, std::ios::in | std::ios::binary | std::ios::ate);
		if (!input.is_open()) return SetStatus(isrc, StatusStyle::Error, "Could not open file.");
		auto inputSize = input.tellg();
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
			while (inputIndex < input.size() && entryIndex < lowercaseEntry.size()) {
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

	void Sampler(const char* labelWithID, M7::SamplerDevice& sampler, size_t isrc)
	{
		ColorMod& cm = sampler.mEnabledParam.GetBoolValue() ? mSamplerColors : mSamplerDisabledColors;
		auto token = cm.Push();
		static constexpr char const* const interpModeNames[] = { "Nearest", "Linear" };
		static constexpr char const* const loopModeNames[] = { "Disabled", "Repeat", "Pingpong" };
		static constexpr char const* const loopBoundaryModeNames[] = { "FromSample", "Manual" };

		if (WSBeginTabItem(labelWithID)) {
			WSImGuiParamCheckbox((int)sampler.mEnabledParamID, "Enabled");
			ImGui::SameLine(); Maj7ImGuiParamVolume((int)sampler.mVolumeParamID, "Volume", 0, 0);

			ImGui::SameLine(0, 50); Maj7ImGuiParamFrequency((int)sampler.mFreqParamID, (int)sampler.mFreqKTParamID, "Freq", M7::gSourceFrequencyCenterHz, M7::gSourceFrequencyScale, 0.4f);
			ImGui::SameLine(); WSImGuiParamKnob((int)sampler.mFreqKTParamID, "KT");
			ImGui::SameLine(); Maj7ImGuiParamInt((int)sampler.mBaseParamID + (int)M7::SamplerParamIndexOffsets::TuneSemis, "Transp", -M7::gSourcePitchSemisRange, M7::gSourcePitchSemisRange, 0);
			ImGui::SameLine(); Maj7ImGuiParamFloatN11((int)sampler.mBaseParamID + (int)M7::SamplerParamIndexOffsets::TuneFine, "FineTune", 0);
			ImGui::SameLine(); Maj7ImGuiParamInt((int)sampler.mBaseParamID + (int)M7::SamplerParamIndexOffsets::BaseNote, "BaseNote", 0, 127, 60);

			ImGui::SameLine(0, 50); WSImGuiParamCheckbox((int)sampler.mBaseParamID + (int)M7::SamplerParamIndexOffsets::LegatoTrig, "Leg.Trig");
			ImGui::SameLine(); Maj7ImGuiParamInt((int)sampler.mBaseParamID + (int)M7::SamplerParamIndexOffsets::KeyRangeMin, "KeyMin", 0, 127, 0);
			ImGui::SameLine(); Maj7ImGuiParamInt((int)sampler.mBaseParamID + (int)M7::SamplerParamIndexOffsets::KeyRangeMax, "KeyMax", 0, 127, 127);

			ImGui::SameLine(0, 50); Maj7ImGuiParamEnumList<WaveSabreCore::LoopMode>((int)sampler.mBaseParamID + (int)M7::SamplerParamIndexOffsets::LoopMode, "LoopMode##mst", (int)WaveSabreCore::LoopMode::NumLoopModes, WaveSabreCore::LoopMode::Repeat, loopModeNames);
			ImGui::SameLine(); Maj7ImGuiParamEnumList<WaveSabreCore::LoopBoundaryMode>((int)sampler.mBaseParamID + (int)M7::SamplerParamIndexOffsets::LoopSource, "LoopSrc##mst", (int)WaveSabreCore::LoopBoundaryMode::NumLoopBoundaryModes, WaveSabreCore::LoopBoundaryMode::FromSample, loopBoundaryModeNames);
			ImGui::SameLine(); WSImGuiParamCheckbox((int)sampler.mBaseParamID + (int)M7::SamplerParamIndexOffsets::ReleaseExitsLoop, "Rel");
			ImGui::SameLine(); Maj7ImGuiParamEnumList<WaveSabreCore::InterpolationMode>((int)sampler.mBaseParamID + (int)M7::SamplerParamIndexOffsets::InterpolationType, "Interp.##mst", (int)WaveSabreCore::InterpolationMode::NumInterpolationModes, WaveSabreCore::InterpolationMode::Linear, interpModeNames);

			ImGui::SameLine(); Maj7ImGuiParamFloatN11((int)sampler.mBaseParamID + (int)M7::SamplerParamIndexOffsets::AuxMix, "Aux pan", 0);			

			ImGui::BeginGroup();
			WSImGuiParamCheckbox((int)sampler.mBaseParamID + (int)M7::SamplerParamIndexOffsets::Reverse, "Reverse");
			ImGui::SameLine(); WSImGuiParamKnob((int)sampler.mBaseParamID + (int)M7::SamplerParamIndexOffsets::SampleStart, "SampleStart");
			ImGui::SameLine(); WSImGuiParamKnob((int)sampler.mBaseParamID + (int)M7::SamplerParamIndexOffsets::LoopStart, "LoopBeg");
			ImGui::SameLine(); WSImGuiParamKnob((int)sampler.mBaseParamID + (int)M7::SamplerParamIndexOffsets::LoopLength, "LoopLen");
			ImGui::EndGroup();

			ImGui::SameLine();
			SamplerWaveformDisplay(sampler, isrc);

			if (ImGui::SmallButton("Load from file ...")) {
				OPENFILENAME ofn = { 0 };
				TCHAR szFile[MAX_PATH] = { 0 };
				ofn.lStructSize = sizeof(ofn);
				ofn.hwndOwner = this->mCurrentWindow;
				ofn.lpstrFilter = TEXT("All Files (*.*)\0*.*\0");
				ofn.lpstrFile = szFile;
				ofn.nMaxFile = MAX_PATH;
				ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST;
				if (GetOpenFileName(&ofn))
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
				int sampleIndex = (int)GetEffectX()->getParameter((int)sampler.mBaseParamID + (int)M7::SamplerParamIndexOffsets::GmDlsIndex);
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
			if (!sampler.mSample) {
				ImGui::Text("No sample loaded");
			}
			else if (sampler.mSampleSource.GetEnumValue() == M7::SampleSource::Embed) {
				auto* p = static_cast<WaveSabreCore::GsmSample*>(sampler.mSample);
				ImGui::Text("Uncompressed size: %d, compressed to %d (%d%%) / %d Samples / path:%s", p->UncompressedSize, p->CompressedSize, (p->CompressedSize * 100) / p->UncompressedSize, p->SampleLength, sampler.mSamplePath);
			}
			else if (sampler.mSampleSource.GetEnumValue() == M7::SampleSource::GmDls) {
				auto* p = static_cast<M7::GmDlsSample*>(sampler.mSample);
				const char* name = "(none)";
				if (p->mSampleIndex >= 0 && p->mSampleIndex < GmDls::NumSamples) {
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
				Envelope("Amplitude Envelope", (int)sampler.mBaseParamID + (int)M7::SamplerParamIndexOffsets::AmpEnvDelayTime);
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

		auto runningVoice = FindRunningVoice();
		float cursor = 0;
		if (runningVoice) {
			M7::SamplerVoice* sv = static_cast<M7::SamplerVoice*>(runningVoice->mSourceVoices[isrc]);
			cursor = (float)sv->mSamplePlayer.samplePos;
			cursor /= sampler.mSample->GetSampleLength();
		}
		WaveformGraphic(isrc, gSamplerWaveformHeight, peaks, sampler.mSampleStart.Get01Value(), sampler.mLoopStart.Get01Value(), sampler.mLoopLength.Get01Value(), cursor);
	}

}; // class maj7editor
