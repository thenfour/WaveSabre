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
			bool isSet;
			//Token(ImVec4* newColors) {
			//	memcpy(mOldColors, ImGui::GetStyle().Colors, sizeof(mOldColors));
			//	memcpy(ImGui::GetStyle().Colors, newColors, sizeof(mOldColors));
			//	isSet = true;
			//}
			Token(ImVec4* newColors) {
				isSet = !!newColors;
				if (isSet) {
					memcpy(mOldColors, ImGui::GetStyle().Colors, sizeof(mOldColors));
					memcpy(ImGui::GetStyle().Colors, newColors, sizeof(mOldColors));
				}
			}
			//Token() {
			//}
			~Token() {
				if (isSet) {
					memcpy(ImGui::GetStyle().Colors, mOldColors, sizeof(mOldColors));
				}
			}
			void Poke() {
				// NOP; hint that the object should remain alive.
			}
		};
		Token Push()
		{
			return { mNewColors };
		}

		//static Token Nop()
		//{
		//	return {};
		//}

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
	ColorMod mModulationDisabledColors{ 0.15f, 0.0f, 0.65f, 0.6f, 0.0f };

	ColorMod mOscColors{ 0, 1, 1, 0.9f, 0.0f };
	ColorMod mOscDisabledColors{ 0, 0, .6f, 0.9f, 0.0f };

	ColorMod mCyanColors{ 0.92f, 0.6f, 0.75f, 0.9f, 0.0f };
	ColorMod mPinkColors{ 0.40f, 0.6f, 0.75f, 0.9f, 0.0f };

	virtual void renderImgui() override
	{
		mModulationsColors.EnsureInitialized();
		mModulationDisabledColors.EnsureInitialized();
		mPinkColors.EnsureInitialized();
		mCyanColors.EnsureInitialized();
		mOscColors.EnsureInitialized();
		mOscDisabledColors.EnsureInitialized();
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
		Maj7ImGuiParamEnvTime((VstInt32)M7::ParamIndices::PortamentoTime, "Port time##mst", 0.4f);
		ImGui::SameLine();
		Maj7ImGuiParamCurve((VstInt32)M7::ParamIndices::PortamentoCurve, "Port curve##mst", 0.0f, M7CurveRenderStyle::Rising);

		static constexpr char const* const voiceModeCaptions[] = { "Poly", "Mono" };
		ImGui::SameLine(0, 60);
		Maj7ImGuiParamEnumList<WaveSabreCore::VoiceMode>((VstInt32)M7::ParamIndices::VoicingMode, "VoiceMode##mst", (int)WaveSabreCore::VoiceMode::Count, WaveSabreCore::VoiceMode::Polyphonic, voiceModeCaptions);

		// osc1
		if (ImGui::BeginTabBar("osc", ImGuiTabBarFlags_None))
		{
			Oscillator("Oscillator A", (int)M7::ParamIndices::Osc1Enabled, 0);
			Oscillator("Oscillator B", (int)M7::ParamIndices::Osc2Enabled, 1);
			Oscillator("Oscillator C", (int)M7::ParamIndices::Osc3Enabled, 2);
			ImGui::EndTabBar();
		}

		// filter
		if (ImGui::BeginTabBar("filter", ImGuiTabBarFlags_None))
		{
			if (ImGui::BeginTabItem("Filter")) {
				//ImGui::PushID("Filter");
			//if (ImGui::CollapsingHeader("Filter")) {
				static constexpr char const* const filterModelCaptions[] = FILTER_MODEL_CAPTIONS;
				Maj7ImGuiParamEnumCombo((VstInt32)M7::ParamIndices::FilterType, "Type##filt", (int)M7::FilterModel::Count, M7::FilterModel::LP_Moog4, filterModelCaptions);
				ImGui::SameLine(0, 60); Maj7ImGuiParamFrequency((VstInt32)M7::ParamIndices::FilterFrequency, (VstInt32)M7::ParamIndices::FilterFrequencyKT, "Freq##filt", M7::Maj7::gFilterCenterFrequency, M7::Maj7::gFilterFrequencyScale, 0.4f);
				ImGui::SameLine(); WSImGuiParamKnob((VstInt32)M7::ParamIndices::FilterFrequencyKT, "KT##filt");
				ImGui::SameLine(); WSImGuiParamKnob((VstInt32)M7::ParamIndices::FilterQ, "Q##filt");
				ImGui::SameLine(0, 60); WSImGuiParamKnob((VstInt32)M7::ParamIndices::FilterSaturation, "Saturation##filt");

				ImGui::EndTabItem();
			}
			ImGui::EndTabBar();
		}
		//ImGui::PopID();

		if (ImGui::BeginTabBar("FM", ImGuiTabBarFlags_None))
		{
			auto colorModToken = mCyanColors.Push();
			if (ImGui::BeginTabItem("FM")) {
				//ImGui::PushID("FM");
				//if (ImGui::CollapsingHeader("FM")) {
				//WSImGuiParamKnob((VstInt32)M7::ParamIndices::Osc1FMFeedback, "Osc A FB##osc1");
				//ImGui::SameLine(); WSImGuiParamKnob((VstInt32)M7::ParamIndices::Osc2FMFeedback, "Osc B FB##osc2");
				//ImGui::SameLine(); WSImGuiParamKnob((VstInt32)M7::ParamIndices::Osc3FMFeedback, "Osc C FB##osc2");
				//ImGui::SameLine(0, 60);	WSImGuiParamKnob((VstInt32)M7::ParamIndices::FMBrightness, "Brightness##mst");

				WSImGuiParamKnob((VstInt32)M7::ParamIndices::Osc1FMFeedback, "FB1");
				ImGui::SameLine(); WSImGuiParamKnob((VstInt32)M7::ParamIndices::FMAmt2to1, "2-1");
				ImGui::SameLine(); WSImGuiParamKnob((VstInt32)M7::ParamIndices::FMAmt3to1, "3-1");
				ImGui::SameLine(0, 60);	WSImGuiParamKnob((VstInt32)M7::ParamIndices::FMBrightness, "Brightness##mst");

				WSImGuiParamKnob((VstInt32)M7::ParamIndices::FMAmt1to2, "1-2");
				ImGui::SameLine(); WSImGuiParamKnob((VstInt32)M7::ParamIndices::Osc2FMFeedback, "FB2");
				ImGui::SameLine(); WSImGuiParamKnob((VstInt32)M7::ParamIndices::FMAmt3to2, "3-2");

				WSImGuiParamKnob((VstInt32)M7::ParamIndices::FMAmt1to3, "1-3");
				ImGui::SameLine(); WSImGuiParamKnob((VstInt32)M7::ParamIndices::FMAmt2to3, "2-3");
				ImGui::SameLine(); WSImGuiParamKnob((VstInt32)M7::ParamIndices::Osc3FMFeedback, "FB3");

				ImGui::EndTabItem();
			}
			//ImGui::PopID();
			ImGui::EndTabBar();
		}

		// modulation shapes
		if (ImGui::BeginTabBar("envelopetabs", ImGuiTabBarFlags_None))
		{
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
				std::sprintf(txt, "%d u:%d", (int)i, pv->mUnisonVoice);
				//std::sprintf(txt, "%d u:%d %d %c%c #%d", (int)i, pv->mUnisonVoice, ns.MidiNoteValue, ns.mIsPhysicallyHeld ? 'P' : ' ', ns.mIsMusicallyHeld ? 'M' : ' ', ns.mSequence);
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
			ImGui::SameLine(); ImGui::ProgressBar(pv->mOsc1AmpEnv.GetLastOutputLevel(), ImVec2{ 50, 0 }, "Amp1");
			ImGui::SameLine(); ImGui::ProgressBar(pv->mOsc2AmpEnv.GetLastOutputLevel(), ImVec2{ 50, 0 }, "Amp2");
			ImGui::SameLine(); ImGui::ProgressBar(pv->mOsc3AmpEnv.GetLastOutputLevel(), ImVec2{ 50, 0 }, "Amp3");
			ImGui::SameLine(); ImGui::ProgressBar(::fabsf(pv->mOscillator1.GetSample()), ImVec2{ 50, 0 }, "Osc1");
			ImGui::SameLine(); ImGui::ProgressBar(::fabsf(pv->mOscillator2.GetSample()), ImVec2{ 50, 0 }, "Osc2");
			ImGui::SameLine(); ImGui::ProgressBar(::fabsf(pv->mOscillator3.GetSample()), ImVec2{ 50, 0 }, "Osc3");
		}
	}

	void Oscillator(const char* labelWithID, int enabledParamID, int oscID)
	{
		float enabledBacking;
		M7::BoolParam bp{ enabledBacking };
		bp.SetParamValue(GetEffectX()->getParameter(enabledParamID));
		ColorMod& cm = bp.GetBoolValue() ? mOscColors : mOscDisabledColors;
		auto token = cm.Push();

		if (ImGui::BeginTabItem(labelWithID)) {
			WSImGuiParamCheckbox(enabledParamID + (int)M7::OscParamIndexOffsets::Enabled, "Enabled");
			ImGui::SameLine(); Maj7ImGuiParamVolume(enabledParamID + (int)M7::OscParamIndexOffsets::Volume, "Volume", M7::OscillatorNode::gVolumeMaxDb, 0);

			//ImGui::SameLine(); Maj7ImGuiParamEnumCombo(enabledParamID + (int)M7::OscParamIndexOffsets::Waveform, "Waveform", M7::OscillatorWaveform::Count, 0, gWaveformCaptions);

			ImGui::SameLine(); WaveformParam(enabledParamID + (int)M7::OscParamIndexOffsets::Waveform, enabledParamID + (int)M7::OscParamIndexOffsets::Waveshape, enabledParamID + (int)M7::OscParamIndexOffsets::PhaseOffset);
			//ImGui::SameLine(); WaveformGraphic(waveformParam.GetEnumValue(), waveshapeParam.Get01Value());
			//ImGui::SameLine(); WSImGuiParamKnob(enabledParamID + (int)M7::OscParamIndexOffsets::Waveform, "Waveform");
			ImGui::SameLine(); WSImGuiParamKnob(enabledParamID + (int)M7::OscParamIndexOffsets::Waveshape, "Shape");
			ImGui::SameLine(0, 60); Maj7ImGuiParamFrequency(enabledParamID + (int)M7::OscParamIndexOffsets::FrequencyParam, enabledParamID + (int)M7::OscParamIndexOffsets::FrequencyParamKT, "Freq", M7::OscillatorNode::gFrequencyCenterHz, M7::OscillatorNode::gFrequencyScale, 0.4f);
			ImGui::SameLine(); WSImGuiParamKnob(enabledParamID + (int)M7::OscParamIndexOffsets::FrequencyParamKT, "KT");
			ImGui::SameLine(); Maj7ImGuiParamInt(enabledParamID + (int)M7::OscParamIndexOffsets::PitchSemis, "Transp", -M7::OscillatorNode::gPitchSemisRange, M7::OscillatorNode::gPitchSemisRange, 0);
			ImGui::SameLine(); Maj7ImGuiParamFloatN11(enabledParamID + (int)M7::OscParamIndexOffsets::PitchFine, "FineTune", 0);
			ImGui::SameLine(); Maj7ImGuiParamScaledFloat(enabledParamID + (int)M7::OscParamIndexOffsets::FreqMul, "FreqMul", 0, M7::OscillatorNode::gFrequencyMulMax, 1);
			ImGui::SameLine(0, 60); WSImGuiParamCheckbox(enabledParamID + (int)M7::OscParamIndexOffsets::PhaseRestart, "PhaseRst");
			ImGui::SameLine(); Maj7ImGuiParamFloatN11(enabledParamID + (int)M7::OscParamIndexOffsets::PhaseOffset, "Phase", 0);
			ImGui::SameLine(0, 60); WSImGuiParamCheckbox(enabledParamID + (int)M7::OscParamIndexOffsets::SyncEnable, "Sync");
			ImGui::SameLine(); Maj7ImGuiParamFrequency(enabledParamID + (int)M7::OscParamIndexOffsets::SyncFrequency, enabledParamID + (int)M7::OscParamIndexOffsets::SyncFrequencyKT, "SyncFreq", M7::OscillatorNode::gSyncFrequencyCenterHz, M7::OscillatorNode::gSyncFrequencyScale, 0.4f);
			ImGui::SameLine(); WSImGuiParamKnob(enabledParamID + (int)M7::OscParamIndexOffsets::SyncFrequencyKT, "SyncKT");

			static constexpr char const* const oscAmpEnvSourceCaptions[M7::Maj7::gOscillatorCount] = { "Amp Env 1", "Amp Env 2", "Amp Env 3" };
			Maj7ImGuiParamEnumCombo(enabledParamID + (int)M7::OscParamIndexOffsets::AmpEnvSource, "Amp env", M7::Maj7::gOscillatorCount, oscID, oscAmpEnvSourceCaptions);

			M7::IntParam ampSourceParam{ pMaj7->mParamCache[enabledParamID + (int)M7::OscParamIndexOffsets::AmpEnvSource], 0, M7::Maj7::gOscillatorCount - 1 };
			M7::ParamIndices ampEnvSources[M7::Maj7::gOscillatorCount] = {
				M7::ParamIndices::Osc1AmpEnvDelayTime,
				M7::ParamIndices::Osc2AmpEnvDelayTime,
				M7::ParamIndices::Osc3AmpEnvDelayTime,
			};
			auto ampEnvSource = ampEnvSources[ampSourceParam.GetIntValue()];

			ImGui::SameLine();

			{
				ColorMod::Token ampEnvColorModToken{ (ampSourceParam.GetIntValue() != oscID) ? mPinkColors.mNewColors : nullptr };
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
		ImGui::SameLine(0, 60); Maj7ImGuiParamFrequency(waveformParamID + (int)M7::LFOParamIndexOffsets::FrequencyParam, -1, "Freq", M7::OscillatorNode::gLFOFrequencyCenterHz, M7::OscillatorNode::gLFOFrequencyScale, 0.4f);
		ImGui::SameLine(0, 60); WSImGuiParamKnob(waveformParamID + (int)M7::LFOParamIndexOffsets::PhaseOffset, "Phase");
		ImGui::SameLine(); WSImGuiParamCheckbox(waveformParamID + (int)M7::LFOParamIndexOffsets::Restart, "Restart");

		ImGui::SameLine(0, 60); Maj7ImGuiParamFrequency(waveformParamID + (int)M7::LFOParamIndexOffsets::Sharpness, -1, "Sharpness", M7::Maj7::gLFOLPCenterFrequency, M7::Maj7::gLFOLPFrequencyScale, 0.5f);
		//mLFO2FilterLP(owner->mParamCache[(int)ParamIndices::LFO2Sharpness], mAlways0, gLFOLPCenterFrequency, gLFOLPFrequencyScale)

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

		float enabledBacking;
		M7::BoolParam bp{ enabledBacking };
		bp.SetParamValue(GetEffectX()->getParameter(enabledParamID));
		ColorMod& cm = bp.GetBoolValue() ? mModulationsColors : mModulationDisabledColors;
		auto token = cm.Push();

		if (ImGui::BeginTabItem(labelWithID))
		{
			WSImGuiParamCheckbox((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::Enabled, "Enabled");
			ImGui::SameLine();
			Maj7ImGuiParamEnumCombo((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::Source, "Source", (int)M7::ModSource::Count, M7::ModSource::None, modSourceCaptions);
			ImGui::SameLine();
			Maj7ImGuiParamEnumCombo((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::Destination, "Dest", (int)M7::ModDestination::Count, M7::ModDestination::None, modDestinationCaptions);
			ImGui::SameLine();
			Maj7ImGuiParamFloatN11(enabledParamID + (int)M7::ModParamIndexOffsets::Scale, "Scale", 1);
			ImGui::SameLine();
			WSImGuiParamCheckbox((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::Invert, "Invert");
			ImGui::SameLine();
			Maj7ImGuiParamCurve((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::Curve, "Curve", 0, M7CurveRenderStyle::Rising);

			ImGui::SameLine(0, 60); WSImGuiParamCheckbox((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::AuxEnabled, "Aux Enable");
			ImGui::SameLine();
			Maj7ImGuiParamEnumCombo((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::AuxSource, "Aux Src", (int)M7::ModSource::Count, M7::ModSource::None, modSourceCaptions);
			ImGui::SameLine();
			WSImGuiParamKnob(enabledParamID + (int)M7::ModParamIndexOffsets::AuxAttenuation, "Aux atten");
			ImGui::SameLine();
			WSImGuiParamCheckbox((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::AuxInvert, "AuxInvert");
			ImGui::SameLine();
			Maj7ImGuiParamCurve((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::AuxCurve, "Aux Curve", 0, M7CurveRenderStyle::Rising);
			ImGui::EndTabItem();

		}
	}

	void WaveformGraphic(M7::OscillatorWaveform waveform, float waveshape01, float phaseOffsetN11, ImRect bb)
	{
		OSCILLATOR_WAVEFORM_CAPTIONS(gWaveformCaptions);
		std::unique_ptr<M7::IOscillatorWaveform> pWaveform;

		switch (waveform) {
		case M7::OscillatorWaveform::SawClip:
			pWaveform.reset(new M7::SawClipWaveform);
			break;
		case M7::OscillatorWaveform::Pulse:
			pWaveform.reset(new M7::PulsePWMWaveform);
			break;
		case M7::OscillatorWaveform::SineClip:
			pWaveform.reset(new M7::SineClipWaveform);
			break;
		case M7::OscillatorWaveform::SineHarmTrunc:
			pWaveform.reset(new M7::SineHarmTruncWaveform);
			break;
		case M7::OscillatorWaveform::WhiteNoiseSH:
			pWaveform.reset(new M7::WhiteNoiseWaveform);
			break;
		}

		float innerHeight = bb.GetHeight() - 4;

		// freq & samplerate should be set such that we have `width` samples per 1 cycle.
		// samples per cycle = srate / freq
		pWaveform->SetParams(1, 0, waveshape01, bb.GetWidth());
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
			float sample = pWaveform->NaiveSample(M7::Fract(float(iSample) / bb.GetWidth() + phaseOffsetN11));
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
					M7::EnumParam<M7::OscillatorWaveform> tp(t, M7::OscillatorWaveform::Count, wf);
					GetEffectX()->setParameter(waveformParamID, t);
				}
				ImGui::PopID();
			}
			ImGui::EndPopup();
		}
	}

}; // class maj7editor
