#pragma once

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

#include <WaveSabreCore/Maj7.hpp>
#include "Maj7Vst.h"

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

	virtual void renderImgui() override
	{
		if (ImGui::Button("panic")) {
			pMaj7->AllNotesOff();
		}

		Maj7ImGuiParamVolume((VstInt32)M7::ParamIndices::MasterVolume, "Output volume##hc", M7::Maj7::gMasterVolumeMaxDb, 0);
		ImGui::SameLine();
		Maj7ImGuiParamInt((VstInt32)M7::ParamIndices::Unisono, "UNISONO##mst", 1, M7::Maj7::gUnisonoVoiceMax, 1);
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)M7::ParamIndices::UnisonoDetune, "UnisonDetune##mst");
		ImGui::SameLine(0, 60);
		Maj7ImGuiParamInt((VstInt32)M7::ParamIndices::PitchBendRange, "PB Range##mst", -M7::Maj7::gPitchBendMaxRange, M7::Maj7::gPitchBendMaxRange, 2);

		static constexpr char const* const voiceModeCaptions[] = { "Poly", "Mono" };
		ImGui::SameLine(0, 60);
		Maj7ImGuiParamEnumList<WaveSabreCore::VoiceMode>((VstInt32)M7::ParamIndices::VoicingMode, "VoiceMode##mst", (int)WaveSabreCore::VoiceMode::Count, WaveSabreCore::VoiceMode::Polyphonic, voiceModeCaptions);

		// osc1
		Oscillator("Oscillator A", (int)M7::ParamIndices::Osc1Enabled);
		Oscillator("Oscillator B", (int)M7::ParamIndices::Osc2Enabled);
		Oscillator("Oscillator C", (int)M7::ParamIndices::Osc3Enabled);

		// LFO
		LFO("LFO 1", (int)M7::ParamIndices::LFO1Waveform);
		LFO("LFO 2", (int)M7::ParamIndices::LFO2Waveform);

		// ampenv
		Envelope("Amplitude Envelope", (int)M7::ParamIndices::AmpEnvDelayTime);
		Envelope("Modulation Envelope 1", (int)M7::ParamIndices::Env1DelayTime);
		Envelope("Modulation Envelope 2", (int)M7::ParamIndices::Env2DelayTime);

		ImGui::SeparatorText("FM");
		WSImGuiParamKnob((VstInt32)M7::ParamIndices::Osc1FMFeedback, "Osc A Feedback##osc1");
		ImGui::SameLine(); WSImGuiParamKnob((VstInt32)M7::ParamIndices::Osc2FMFeedback, "Osc B Feedback##osc2");
		ImGui::SameLine(); WSImGuiParamKnob((VstInt32)M7::ParamIndices::Osc3FMFeedback, "Osc C Feedback##osc2");

		ModulationSection("Modulation 1", (int)M7::ParamIndices::Mod1Enabled);
		ModulationSection("Modulation 2", (int)M7::ParamIndices::Mod2Enabled);
		ModulationSection("Modulation 3", (int)M7::ParamIndices::Mod3Enabled);

		ImGui::SeparatorText("Filter");
		static constexpr char const* const filterModelCaptions[] = FILTER_MODEL_CAPTIONS;
		Maj7ImGuiParamEnumCombo((VstInt32)M7::ParamIndices::FilterType, "Type##filt", (int)M7::FilterModel::Count, M7::FilterModel::LP_Moog4, filterModelCaptions);
		ImGui::SameLine(0, 60); Maj7ImGuiParamFrequency((VstInt32)M7::ParamIndices::FilterFrequency, (VstInt32)M7::ParamIndices::FilterFrequencyKT, "Freq##filt", M7::Maj7::gFilterCenterFrequency, 0.4f);
		ImGui::SameLine(); WSImGuiParamKnob((VstInt32)M7::ParamIndices::FilterFrequencyKT, "KT##filt");
		ImGui::SameLine(); WSImGuiParamKnob((VstInt32)M7::ParamIndices::FilterQ, "Q##filt");
		ImGui::SameLine(0, 60); WSImGuiParamKnob((VstInt32)M7::ParamIndices::FilterSaturation, "Saturation##filt");

		//Maj7ImGuiFilterGraphic(M7::Maj7::ParamIndices::FilterType, M7::Maj7::ParamIndices::FilterFrequency, M7::Maj7::ParamIndices::FilterQ, M7::Maj7::gFilterCenterFrequency);

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
			ImGui::SameLine(0, 60); Maj7ImGuiParamFrequency(enabledParamID + (int)M7::OscParamIndexOffsets::FrequencyParam, enabledParamID + (int)M7::OscParamIndexOffsets::FrequencyParamKT, "Freq", M7::OscillatorNode::gFrequencyCenterHz, 0.4f);
			ImGui::SameLine(); WSImGuiParamKnob(enabledParamID + (int)M7::OscParamIndexOffsets::FrequencyParamKT, "KT");
			ImGui::SameLine(); Maj7ImGuiParamInt(enabledParamID + (int)M7::OscParamIndexOffsets::PitchSemis, "Transp", -M7::OscillatorNode::gPitchSemisRange, M7::OscillatorNode::gPitchSemisRange, 0);
			ImGui::SameLine(); WSImGuiParamKnob(enabledParamID + (int)M7::OscParamIndexOffsets::PitchFine, "FineTune");
			ImGui::SameLine(); Maj7ImGuiParamScaledFloat(enabledParamID + (int)M7::OscParamIndexOffsets::FreqMul, "FreqMul", -M7::OscillatorNode::gFrequencyMulMax, M7::OscillatorNode::gFrequencyMulMax, 1);
			ImGui::SameLine(0, 60); WSImGuiParamCheckbox(enabledParamID + (int)M7::OscParamIndexOffsets::PhaseRestart, "PhaseRst");
			ImGui::SameLine(); WSImGuiParamKnob(enabledParamID + (int)M7::OscParamIndexOffsets::PhaseOffset, "Phase");
			ImGui::SameLine(0, 60); WSImGuiParamCheckbox(enabledParamID + (int)M7::OscParamIndexOffsets::SyncEnable, "Sync");
			ImGui::SameLine(); Maj7ImGuiParamFrequency(enabledParamID + (int)M7::OscParamIndexOffsets::SyncFrequency, enabledParamID + (int)M7::OscParamIndexOffsets::SyncFrequencyKT, "SyncFreq", M7::OscillatorNode::gSyncFrequencyCenterHz, 0.4f);
			ImGui::SameLine(); WSImGuiParamKnob(enabledParamID + (int)M7::OscParamIndexOffsets::SyncFrequencyKT, "SyncKT");
		}
		ImGui::PopID();
	}

	void LFO(const char* labelWithID, int waveformParamID)
	{
		ImGui::PushID(labelWithID);
		if (ImGui::CollapsingHeader(labelWithID)) {
			WSImGuiParamKnob(waveformParamID + (int)M7::LFOParamIndexOffsets::Waveform, "Waveform");
			ImGui::SameLine(); WSImGuiParamKnob(waveformParamID + (int)M7::LFOParamIndexOffsets::Waveshape, "Shape");
			ImGui::SameLine(0, 60); Maj7ImGuiParamFrequency(waveformParamID + (int)M7::LFOParamIndexOffsets::FrequencyParam, -1, "Freq", M7::OscillatorNode::gFrequencyCenterHz, 0.4f);
			ImGui::SameLine(0, 60); WSImGuiParamKnob(waveformParamID + (int)M7::LFOParamIndexOffsets::PhaseOffset, "Phase");
			ImGui::SameLine(); WSImGuiParamCheckbox(waveformParamID + (int)M7::LFOParamIndexOffsets::Restart, "Restart");
		}
		ImGui::PopID();
	}

	void Envelope(const char *labelWithID, int delayTimeParamID)
	{
		ImGui::PushID(labelWithID);
		if (ImGui::CollapsingHeader(labelWithID)) {
			Maj7ImGuiParamEnvTime(delayTimeParamID + (int)M7::EnvParamIndexOffsets::DelayTime, "Delay", 0);
			ImGui::SameLine();
			Maj7ImGuiParamEnvTime(delayTimeParamID + (int)M7::EnvParamIndexOffsets::AttackTime, "Attack", 0);
			ImGui::SameLine();
			Maj7ImGuiParamFloatN11(delayTimeParamID + (int)M7::EnvParamIndexOffsets::AttackCurve, "Curve##attack", 0);
			ImGui::SameLine();
			Maj7ImGuiParamEnvTime(delayTimeParamID + (int)M7::EnvParamIndexOffsets::HoldTime, "Hold", 0);
			ImGui::SameLine();
			Maj7ImGuiParamEnvTime(delayTimeParamID + (int)M7::EnvParamIndexOffsets::DecayTime, "Decay", .4f);
			ImGui::SameLine();
			Maj7ImGuiParamFloatN11(delayTimeParamID + (int)M7::EnvParamIndexOffsets::DecayCurve, "Curve##Decay", 0);
			ImGui::SameLine();
			WSImGuiParamKnob(delayTimeParamID + (int)M7::EnvParamIndexOffsets::SustainLevel, "Sustain");
			ImGui::SameLine();
			Maj7ImGuiParamEnvTime(delayTimeParamID + (int)M7::EnvParamIndexOffsets::ReleaseTime, "Release", 0);
			ImGui::SameLine();
			Maj7ImGuiParamFloatN11(delayTimeParamID + (int)M7::EnvParamIndexOffsets::ReleaseCurve, "Curve##Release", 0);
			ImGui::SameLine();
			WSImGuiParamCheckbox(delayTimeParamID + (int)M7::EnvParamIndexOffsets::LegatoRestart, "Leg.Restart");

			ImGui::SameLine();
			Maj7ImGuiEnvelopeGraphic("graph", delayTimeParamID);
		}
		ImGui::PopID();
	}

	void ModulationSection(const char* labelWithID, int enabledParamID)
	{
		static constexpr char const* const modSourceCaptions[] = MOD_SOURCE_CAPTIONS;
		static constexpr char const* const modDestinationCaptions[] = MOD_DEST_CAPTIONS;

		ImGui::PushID(labelWithID);

		if (ImGui::CollapsingHeader(labelWithID)) {
			WSImGuiParamCheckbox((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::Enabled, "Enabled");
			ImGui::SameLine();
			Maj7ImGuiParamEnumCombo((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::Source, "Source", (int)M7::ModSource::Count, M7::ModSource::None, modSourceCaptions);
			ImGui::SameLine();
			Maj7ImGuiParamEnumCombo((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::Destination, "Dest", (int)M7::ModDestination::Count, M7::ModDestination::None, modDestinationCaptions);
			ImGui::SameLine();
			Maj7ImGuiParamFloatN11(enabledParamID + (int)M7::ModParamIndexOffsets::Scale, "Scale", 1);
			ImGui::SameLine();
			Maj7ImGuiParamFloatN11((VstInt32)enabledParamID + (int)M7::ModParamIndexOffsets::Curve, "Curve", 0);
		}

		ImGui::PopID();
	}


};
