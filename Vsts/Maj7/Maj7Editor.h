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
			Token(ImVec4* newColors) {
				isSet = !!newColors;
				if (isSet) {
					memcpy(mOldColors, ImGui::GetStyle().Colors, sizeof(mOldColors));
					memcpy(ImGui::GetStyle().Colors, newColors, sizeof(mOldColors));
				}
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
	ColorMod mOscDisabledColors{ 0, 0, .6f, 0.9f, 0.0f };

	ColorMod mCyanColors{ 0.92f, 0.6f, 0.75f, 0.9f, 0.0f };
	ColorMod mPinkColors{ 0.40f, 0.6f, 0.75f, 0.9f, 0.0f };

	ColorMod mNopColors;

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

		//// color explorer
		//static float colorHueVarAmt = 0;
		//static float colorSaturationVarAmt = 1;
		//static float colorValueVarAmt = 1;
		//static float colorTextVal = 0.9f;
		//static float colorTextDisabledVal = 0.5f;
		//bool b1 = ImGuiKnobs::Knob("hue", &colorHueVarAmt, -1.0f, 1.0f, 0.0f, gNormalKnobSpeed, gSlowKnobSpeed);
		//ImGui::SameLine(); bool b2 = ImGuiKnobs::Knob("sat", &colorSaturationVarAmt, 0.0f, 1.0f, 0.0f, gNormalKnobSpeed, gSlowKnobSpeed);
		//ImGui::SameLine(); bool b3 = ImGuiKnobs::Knob("val", &colorValueVarAmt, 0.0f, 1.0f, 0.0f, gNormalKnobSpeed, gSlowKnobSpeed);
		//ImGui::SameLine(); bool b4 = ImGuiKnobs::Knob("txt", &colorTextVal, 0.0f, 1.0f, 0.0f, gNormalKnobSpeed, gSlowKnobSpeed);
		//ImGui::SameLine(); bool b5 = ImGuiKnobs::Knob("txtD", &colorTextDisabledVal, 0.0f, 1.0f, 0.0f, gNormalKnobSpeed, gSlowKnobSpeed);
		//ColorMod xyz{ colorHueVarAmt , colorSaturationVarAmt, colorValueVarAmt, colorTextVal, colorTextDisabledVal };
		//xyz.EnsureInitialized();
		//auto xyzt = xyz.Push();


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
		if (ImGui::BeginTabBar("osc", ImGuiTabBarFlags_None))
		{
			Oscillator("Oscillator A", (int)M7::ParamIndices::Osc1Enabled, 0);
			Oscillator("Oscillator B", (int)M7::ParamIndices::Osc2Enabled, 1);
			Oscillator("Oscillator C", (int)M7::ParamIndices::Osc3Enabled, 2);
			ImGui::EndTabBar();
		}

		if (ImGui::BeginTabBar("FM", ImGuiTabBarFlags_None))
		{
			auto colorModToken = mCyanColors.Push();
			if (ImGui::BeginTabItem("Phase Modulation")) {
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
			ImGui::EndTabBar();
		}

		// aux
		if (ImGui::BeginTabBar("aux", ImGuiTabBarFlags_None))
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
			ModulationSection("Mod 1", this->pMaj7->mModulations[0], (int)M7::ParamIndices::Mod1Enabled);
			ModulationSection("Mod 2", this->pMaj7->mModulations[1], (int)M7::ParamIndices::Mod2Enabled);
			ModulationSection("Mod 3", this->pMaj7->mModulations[2], (int)M7::ParamIndices::Mod3Enabled);
			ModulationSection("Mod 4", this->pMaj7->mModulations[3], (int)M7::ParamIndices::Mod4Enabled);
			ModulationSection("Mod 5", this->pMaj7->mModulations[4], (int)M7::ParamIndices::Mod5Enabled);
			ModulationSection("Mod 6", this->pMaj7->mModulations[5], (int)M7::ParamIndices::Mod6Enabled);
			ModulationSection("Mod 7", this->pMaj7->mModulations[6], (int)M7::ParamIndices::Mod7Enabled);
			ModulationSection("Mod 8", this->pMaj7->mModulations[7], (int)M7::ParamIndices::Mod8Enabled);
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

			ImGui::SameLine(0, 60); Maj7ImGuiParamFloatN11(enabledParamID + (int)M7::OscParamIndexOffsets::AuxMix, "Aux pan", 0);

			//static constexpr char const* const oscAmpEnvSourceCaptions[M7::Maj7::gOscillatorCount] = { "Amp Env 1", "Amp Env 2", "Amp Env 3" };
			//Maj7ImGuiParamEnumCombo(enabledParamID + (int)M7::OscParamIndexOffsets::AmpEnvSource, "Amp env", M7::Maj7::gOscillatorCount, oscID, oscAmpEnvSourceCaptions);

			//M7::IntParam ampSourceParam{ pMaj7->mParamCache[enabledParamID + (int)M7::OscParamIndexOffsets::AmpEnvSource], 0, M7::Maj7::gOscillatorCount - 1 };
			M7::ParamIndices ampEnvSources[M7::Maj7::gOscillatorCount] = {
				M7::ParamIndices::Osc1AmpEnvDelayTime,
				M7::ParamIndices::Osc2AmpEnvDelayTime,
				M7::ParamIndices::Osc3AmpEnvDelayTime,
			};
			auto ampEnvSource = ampEnvSources[oscID];// ampSourceParam.GetIntValue()];

			//ImGui::SameLine();

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
		return { auxInfo.mSelfLink, paramValues, (int)auxInfo.mModParam2ID };
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
		switch (a.mEffectType.GetEnumValue())
		{
		case M7::AuxEffectType::BigFilter:
		{
			FILTER_AUX_MOD_SUFFIXES(suffixes);
			labels[0] += suffixes[0];
			labels[1] += suffixes[1];
			labels[2] += suffixes[2];
			labels[3] += suffixes[3];
			break;
		}
		default:
		{
			labels[0] += " (n/a)";
			labels[1] += " (n/a)";
			labels[2] += " (n/a)";
			labels[3] += " (n/a)";
			break;
		}
		}
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

		if (ImGui::BeginTabItem(labelWithID))
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

		if (ImGui::BeginTabItem(labelWithID.c_str()))
		{
			ImGui::PushID(iaux);
			WSImGuiParamCheckbox((int)auxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Enabled, "Enabled");

			ImGui::SameLine(); Maj7ImGuiParamEnumCombo((int)auxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Link, "Link", (int)M7::AuxLink::Count, auxInfo.mSelfLink, auxLinkCaptions);

			ImGui::SameLine();
			ImGui::BeginGroup();

			if (ImGui::Button("Swap with...")) {
				ImGui::OpenPopup("selectAuxSwap");
			}

			//ImGui::SameLine();
			if (ImGui::Button("Copy from...")) {
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

				ImGui::SameLine(); Maj7ImGuiParamEnumCombo((int)auxInfo.mEnabledParamID + (int)M7::AuxParamIndexOffsets::Type, "Effect", (int)M7::AuxEffectType::Count, M7::AuxEffectType::None, auxEffectTypeCaptions);

				switch (a.mEffectType.GetEnumValue())
				{
				default:
					ImGui::TextUnformatted("Nothing to see.");
					break;
				case M7::AuxEffectType::BigFilter:
					AuxFilter(auxInfo);
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
