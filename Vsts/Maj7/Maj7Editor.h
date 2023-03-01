#pragma once

#include <WaveSabreVstLib.h>
#include <WaveSabreCore.h>
#include <WaveSabreCore/Maj7.hpp>
#include "Maj7Vst.h"

using namespace WaveSabreVstLib;
using namespace WaveSabreCore;

class Maj7Editor : public VstEditor
{
	Maj7Vst* mMaj7VST;
	M7::Maj7* pMaj7;
public:
	Maj7Editor(AudioEffect* audioEffect) :
		VstEditor(audioEffect, 1100, 900),
		mMaj7VST((Maj7Vst*)audioEffect),
		pMaj7(((Maj7Vst*)audioEffect)->GetMaj7())
	{
	}

	virtual void PopulateMenuBar() override
	{
		if (ImGui::BeginMenu("Maj7")) {
			bool b = false;
			if (ImGui::MenuItem("Panic", nullptr, false)) {
				pMaj7->AllNotesOff();
			}
			ImGui::EndMenu();
		}
	}

	struct ColorMod
	{
		//bool b1 = ImGuiKnobs::Knob("hue", &colorHueVarAmt, -1.0f, 1.0f, 0.0f, gNormalKnobSpeed, gSlowKnobSpeed);
		//ImGui::SameLine(); bool b2 = ImGuiKnobs::Knob("sat", &colorSaturationVarAmt, 0.0f, 1.0f, 0.0f, gNormalKnobSpeed, gSlowKnobSpeed);
		//ImGui::SameLine(); bool b3 = ImGuiKnobs::Knob("val", &colorValueVarAmt, 0.0f, 1.0f, 0.0f, gNormalKnobSpeed, gSlowKnobSpeed);
		//ImGui::SameLine(); bool b4 = ImGuiKnobs::Knob("txt", &colorTextVal, 0.0f, 1.0f, 0.0f, gNormalKnobSpeed, gSlowKnobSpeed);
		//ImGui::SameLine(); bool b5 = ImGuiKnobs::Knob("txtD", &colorTextDisabledVal, 0.0f, 1.0f, 0.0f, gNormalKnobSpeed, gSlowKnobSpeed);
		//if (b1 || b2 || b3 || b4 || b5)
		//{
		//	float h, s, v;
		//	for (size_t i = 0; i < ImGuiCol_COUNT; ++i) {
		//		ImVec4 color = style.Colors[i];
		//		ImGui::ColorConvertRGBtoHSV(color.x, color.y, color.z, h, s, v);
		//		auto newcolor = ImColor::HSV(h + colorHueVarAmt, s * colorSaturationVarAmt, v * colorValueVarAmt);
		//		newColors[i] = newcolor.Value;
		//	}
		//	newColors[ImGuiCol_Text] = ImColor::HSV(0, 0, colorTextVal);
		//	newColors[ImGuiCol_TextDisabled] = ImColor::HSV(0, 0, colorTextDisabledVal);
		//}


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
			Token(ImVec4* newColors) {
				memcpy(mOldColors, ImGui::GetStyle().Colors, sizeof(mOldColors));
				memcpy(ImGui::GetStyle().Colors, newColors, sizeof(mOldColors));
			}
			~Token() {
				//ImGui::PopStyleColor(ImGuiCol_COUNT);
				memcpy(ImGui::GetStyle().Colors, mOldColors, sizeof(mOldColors));
			}
		};
		Token Push()
		{
			//for (size_t i = 0; i < ImGuiCol_COUNT; ++i) {
			//	ImGui::PushStyleColor((ImGuiCol)i, mNewColors[i]);
			//}
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
	};

	ColorMod mModulationsColors{ 0.15f, 0.6f, 0.65f, 0.9f, 0.0f };

	ColorMod mCyanColors{ 0.92f, 0.6f, 0.75f, 0.9f, 0.0f };
	ColorMod mPinkColors{ 0.40f, 0.6f, 0.75f, 0.9f, 0.0f };
	// other nice variations: disabled = saturation 0
	// muted cyan = H 0.922
	// pink = H 0.4

	virtual void renderImgui() override
	{
		mModulationsColors.EnsureInitialized();
		mPinkColors.EnsureInitialized();
		mCyanColors.EnsureInitialized();
		//ImGuiStyle& style = ImGui::GetStyle();
		//static ImVec4 newColors[ImGuiCol_COUNT];
		//static bool colorsInitialized = false;
		//if (!colorsInitialized) {
		//	colorsInitialized = true;
		//	for (size_t i = 0; i < ImGuiCol_COUNT; ++i) {
		//		ImVec4 color = style.Colors[i];
		//		float h, s, v;
		//		ImGui::ColorConvertRGBtoHSV(color.x, color.y, color.z, h, s, v);
		//		auto newcolor = ImColor::HSV(h, s, v);
		//		newColors[i] = newcolor.Value;
		//	}
		//}

		Maj7ImGuiParamVolume((VstInt32)M7::ParamIndices::MasterVolume, "Output volume##hc", M7::Maj7::gMasterVolumeMaxDb, 0);
		ImGui::SameLine();
		Maj7ImGuiParamInt((VstInt32)M7::ParamIndices::Unisono, "Unison##mst", 1, M7::Maj7::gUnisonoVoiceMax, 1);

		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)M7::ParamIndices::UnisonoDetune, "UniDetune##mst");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)M7::ParamIndices::OscillatorDetune, "OscDetune##mst");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)M7::ParamIndices::UnisonoStereoSpread, "UniSpread##mst");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)M7::ParamIndices::OscillatorSpread, "OscSpread##mst");

		ImGui::SameLine(0, 60);
		Maj7ImGuiParamInt((VstInt32)M7::ParamIndices::PitchBendRange, "PB Range##mst", -M7::Maj7::gPitchBendMaxRange, M7::Maj7::gPitchBendMaxRange, 2);
		ImGui::SameLine();
		Maj7ImGuiParamEnvTime((VstInt32)M7::ParamIndices::PortamentoTime, "Port time##mst", 0.4f);
		ImGui::SameLine();
		Maj7ImGuiParamCurve((VstInt32)M7::ParamIndices::PortamentoCurve, "Port curve##mst", 0.0f, M7CurveRenderStyle::Rising);

		static constexpr char const* const voiceModeCaptions[] = { "Poly", "Mono" };
		ImGui::SameLine(0, 60);
		Maj7ImGuiParamEnumList<WaveSabreCore::VoiceMode>((VstInt32)M7::ParamIndices::VoicingMode, "VoiceMode##mst", (int)WaveSabreCore::VoiceMode::Count, WaveSabreCore::VoiceMode::Polyphonic, voiceModeCaptions);

		// osc1
		Oscillator("Oscillator A", (int)M7::ParamIndices::Osc1Enabled);
		Oscillator("Oscillator B", (int)M7::ParamIndices::Osc2Enabled);
		Oscillator("Oscillator C", (int)M7::ParamIndices::Osc3Enabled);

		// filter
		ImGui::PushID("Filter");
		if (ImGui::CollapsingHeader("Filter")) {
			static constexpr char const* const filterModelCaptions[] = FILTER_MODEL_CAPTIONS;
			Maj7ImGuiParamEnumCombo((VstInt32)M7::ParamIndices::FilterType, "Type##filt", (int)M7::FilterModel::Count, M7::FilterModel::LP_Moog4, filterModelCaptions);
			ImGui::SameLine(0, 60); Maj7ImGuiParamFrequency((VstInt32)M7::ParamIndices::FilterFrequency, (VstInt32)M7::ParamIndices::FilterFrequencyKT, "Freq##filt", M7::Maj7::gFilterCenterFrequency, M7::Maj7::gFilterFrequencyScale, 0.4f);
			ImGui::SameLine(); WSImGuiParamKnob((VstInt32)M7::ParamIndices::FilterFrequencyKT, "KT##filt");
			ImGui::SameLine(); WSImGuiParamKnob((VstInt32)M7::ParamIndices::FilterQ, "Q##filt");
			ImGui::SameLine(0, 60); WSImGuiParamKnob((VstInt32)M7::ParamIndices::FilterSaturation, "Saturation##filt");
		}
		ImGui::PopID();

		{
			auto colorModToken = mCyanColors.Push();
			ImGui::PushID("FM");
			if (ImGui::CollapsingHeader("FM")) {
				WSImGuiParamKnob((VstInt32)M7::ParamIndices::Osc1FMFeedback, "Osc A FB##osc1");
				ImGui::SameLine(); WSImGuiParamKnob((VstInt32)M7::ParamIndices::Osc2FMFeedback, "Osc B FB##osc2");
				ImGui::SameLine(); WSImGuiParamKnob((VstInt32)M7::ParamIndices::Osc3FMFeedback, "Osc C FB##osc2");
				ImGui::SameLine(0, 60);	WSImGuiParamKnob((VstInt32)M7::ParamIndices::FMBrightness, "Brightness##mst");
			}
			ImGui::PopID();
		}

		// modulation shapes
		if (ImGui::BeginTabBar("envelopetabs", ImGuiTabBarFlags_None))
		{
			auto ampEnvColorModToken = mPinkColors.Push();
			if (ImGui::BeginTabItem("Amp env"))
			{
				Envelope("Amplitude Envelope", (int)M7::ParamIndices::AmpEnvDelayTime);
				ImGui::EndTabItem();
			}
			auto modColorModToken = mModulationsColors.Push();
			if (ImGui::BeginTabItem("Mod env 1"))
			{
				Envelope("Modulation Envelope 1", (int)M7::ParamIndices::Env1DelayTime);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("Mod env 2"))
			{
				Envelope("Modulation Envelope 2", (int)M7::ParamIndices::Env2DelayTime);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("LFO 1"))
			{
				LFO("LFO 1", (int)M7::ParamIndices::LFO1Waveform);
				ImGui::EndTabItem();
			}
			if (ImGui::BeginTabItem("LFO 2"))
			{
				LFO("LFO 2", (int)M7::ParamIndices::LFO2Waveform);
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

		if (ImGui::BeginTabBar("modspectabs", ImGuiTabBarFlags_None))
		{
			ModulationSection("Mod 1", (int)M7::ParamIndices::Mod1Enabled);
			ModulationSection("Mod 2", (int)M7::ParamIndices::Mod2Enabled);
			ModulationSection("Mod 3", (int)M7::ParamIndices::Mod3Enabled);
			ModulationSection("Mod 4", (int)M7::ParamIndices::Mod4Enabled);
			ModulationSection("Mod 5", (int)M7::ParamIndices::Mod5Enabled);
			ModulationSection("Mod 6", (int)M7::ParamIndices::Mod6Enabled);
			ModulationSection("Mod 7", (int)M7::ParamIndices::Mod7Enabled);
			ModulationSection("Mod 8", (int)M7::ParamIndices::Mod8Enabled);
			ImGui::EndTabBar();
		}

		if (ImGui::BeginTabBar("macroknobs", ImGuiTabBarFlags_None))
		{
			if (ImGui::BeginTabItem("Macros"))
			{
				WSImGuiParamKnob((int)M7::ParamIndices::Macro1, "Macro 1");
				ImGui::SameLine(); WSImGuiParamKnob((int)M7::ParamIndices::Macro2, "Macro 2");
				ImGui::SameLine(); WSImGuiParamKnob((int)M7::ParamIndices::Macro3, "Macro 3");
				ImGui::SameLine(); WSImGuiParamKnob((int)M7::ParamIndices::Macro4, "Macro 4");
				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}

		ImGui::SeparatorText("Inspector");

		ImGui::Text("current poly:%d", pMaj7->GetCurrentPolyphony());

		for (size_t i = 0; i < std::size(pMaj7->mVoices); ++ i) {
			auto pv = (M7::Maj7::Maj7Voice*)pMaj7->mVoices[i];
			char txt[200];
			if (i & 0x1) ImGui::SameLine();
			auto color = ImColor::HSV(0, 0, .3f);
			if (pv->IsPlaying()) {
				auto& ns = pMaj7->mNoteStates[pv->mNoteInfo.MidiNoteValue];
				std::sprintf(txt, "%d:%d %c%c #%d", (int)i, ns.MidiNoteValue, ns.mIsPhysicallyHeld ? 'P' : ' ', ns.mIsMusicallyHeld ? 'M':' ', ns.mSequence);
				color = ImColor::HSV(2/7.0f, .8f, .7f);
			}
			else {
				std::sprintf(txt, "%d:off", (int)i);
			}
			ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)color);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)color);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)color);
			ImGui::Button(txt);
			ImGui::PopStyleColor(3);
			ImGui::SameLine(); ImGui::ProgressBar(pv->mAmpEnv.GetLastOutputLevel(), ImVec2{ 50, 0 }, "Amp env");
			ImGui::SameLine(); ImGui::ProgressBar(::fabsf(pv->mOscillator1.GetLastSample()), ImVec2{ 50, 0 }, "Osc1");
			ImGui::SameLine(); ImGui::ProgressBar(::fabsf(pv->mOscillator2.GetLastSample()), ImVec2{ 50, 0 }, "Osc2");
			ImGui::SameLine(); ImGui::ProgressBar(::fabsf(pv->mOscillator3.GetLastSample()), ImVec2{ 50, 0 }, "Osc3");
		}
	}

	void Oscillator(const char* labelWithID, int enabledParamID)
	{
		ImGui::PushID(labelWithID);
		if (ImGui::CollapsingHeader(labelWithID)) {
			WSImGuiParamCheckbox(enabledParamID + (int)M7::OscParamIndexOffsets::Enabled, "Enabled");
			ImGui::SameLine(); Maj7ImGuiParamVolume(enabledParamID + (int)M7::OscParamIndexOffsets::Volume, "Volume", M7::OscillatorNode::gVolumeMaxDb, 0);
			ImGui::SameLine(); WSImGuiParamKnob(enabledParamID + (int)M7::OscParamIndexOffsets::Waveform, "Waveform");
			ImGui::SameLine(); WSImGuiParamKnob(enabledParamID + (int)M7::OscParamIndexOffsets::Waveshape, "Shape");
			ImGui::SameLine(0, 60); Maj7ImGuiParamFrequency(enabledParamID + (int)M7::OscParamIndexOffsets::FrequencyParam, enabledParamID + (int)M7::OscParamIndexOffsets::FrequencyParamKT, "Freq", M7::OscillatorNode::gFrequencyCenterHz, M7::OscillatorNode::gFrequencyScale, 0.4f);
			ImGui::SameLine(); WSImGuiParamKnob(enabledParamID + (int)M7::OscParamIndexOffsets::FrequencyParamKT, "KT");
			ImGui::SameLine(); Maj7ImGuiParamInt(enabledParamID + (int)M7::OscParamIndexOffsets::PitchSemis, "Transp", -M7::OscillatorNode::gPitchSemisRange, M7::OscillatorNode::gPitchSemisRange, 0);
			ImGui::SameLine(); Maj7ImGuiParamFloatN11(enabledParamID + (int)M7::OscParamIndexOffsets::PitchFine, "FineTune", 0);
			ImGui::SameLine(); Maj7ImGuiParamScaledFloat(enabledParamID + (int)M7::OscParamIndexOffsets::FreqMul, "FreqMul", -M7::OscillatorNode::gFrequencyMulMax, M7::OscillatorNode::gFrequencyMulMax, 1);
			ImGui::SameLine(0, 60); WSImGuiParamCheckbox(enabledParamID + (int)M7::OscParamIndexOffsets::PhaseRestart, "PhaseRst");
			ImGui::SameLine(); WSImGuiParamKnob(enabledParamID + (int)M7::OscParamIndexOffsets::PhaseOffset, "Phase");
			ImGui::SameLine(0, 60); WSImGuiParamCheckbox(enabledParamID + (int)M7::OscParamIndexOffsets::SyncEnable, "Sync");
			ImGui::SameLine(); Maj7ImGuiParamFrequency(enabledParamID + (int)M7::OscParamIndexOffsets::SyncFrequency, enabledParamID + (int)M7::OscParamIndexOffsets::SyncFrequencyKT, "SyncFreq", M7::OscillatorNode::gSyncFrequencyCenterHz, M7::OscillatorNode::gSyncFrequencyScale, 0.4f);
			ImGui::SameLine(); WSImGuiParamKnob(enabledParamID + (int)M7::OscParamIndexOffsets::SyncFrequencyKT, "SyncKT");
		}
		ImGui::PopID();
	}

	void LFO(const char* labelWithID, int waveformParamID)
	{
		ImGui::PushID(labelWithID);
		//if (ImGui::CollapsingHeader(labelWithID)) {
		WSImGuiParamKnob(waveformParamID + (int)M7::LFOParamIndexOffsets::Waveform, "Waveform");
		ImGui::SameLine(); WSImGuiParamKnob(waveformParamID + (int)M7::LFOParamIndexOffsets::Waveshape, "Shape");
		ImGui::SameLine(0, 60); Maj7ImGuiParamFrequency(waveformParamID + (int)M7::LFOParamIndexOffsets::FrequencyParam, -1, "Freq", M7::OscillatorNode::gLFOFrequencyCenterHz, M7::OscillatorNode::gLFOFrequencyScale, 0.4f);
		ImGui::SameLine(0, 60); WSImGuiParamKnob(waveformParamID + (int)M7::LFOParamIndexOffsets::PhaseOffset, "Phase");
		ImGui::SameLine(); WSImGuiParamCheckbox(waveformParamID + (int)M7::LFOParamIndexOffsets::Restart, "Restart");
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

	void ModulationSection(const char* labelWithID, int enabledParamID)
	{
		static constexpr char const* const modSourceCaptions[] = MOD_SOURCE_CAPTIONS;
		static constexpr char const* const modDestinationCaptions[] = MOD_DEST_CAPTIONS;

		if (ImGui::BeginTabItem(labelWithID))
		{
			//ImGui::PushID(labelWithID);
			//if (ImGui::CollapsingHeader(labelWithID)) {
			WSImGuiParamCheckbox((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::Enabled, "Enabled");
			ImGui::SameLine();
			Maj7ImGuiParamEnumCombo((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::Source, "Source", (int)M7::ModSource::Count, M7::ModSource::None, modSourceCaptions);
			ImGui::SameLine();
			Maj7ImGuiParamEnumCombo((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::Destination, "Dest", (int)M7::ModDestination::Count, M7::ModDestination::None, modDestinationCaptions);
			ImGui::SameLine();
			Maj7ImGuiParamFloatN11(enabledParamID + (int)M7::ModParamIndexOffsets::Scale, "Scale", 1);
			ImGui::SameLine();
			Maj7ImGuiParamCurve((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::Curve, "Curve", 0, M7CurveRenderStyle::Rising);
			//}
			ImGui::EndTabItem();
		}
		//ImGui::PopID();
	}


};
