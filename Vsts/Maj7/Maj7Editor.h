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
		VstEditor(audioEffect, 1000, 800),
		mMaj7VST((Maj7Vst*)audioEffect),
		pMaj7(((Maj7Vst*)audioEffect)->GetMaj7())
	{
	}

	virtual void renderImgui() override
	{
		if (ImGui::Button("panic")) {
			pMaj7->AllNotesOff();
		}

		WSImGuiParamKnob((VstInt32)M7::Maj7::ParamIndices::MasterVolume, "Output volume##hc");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)M7::Maj7::ParamIndices::Unisono, "UNISONO##pitchenv");
		ImGui::SameLine();


		static constexpr char const* const voiceModeCaptions[] = { "Poly", "Mono" };
		Maj7ImGuiParamEnumList<WaveSabreCore::VoiceMode>((VstInt32)M7::Maj7::ParamIndices::VoicingMode, "VoiceMode##mst", (int)WaveSabreCore::VoiceMode::Count, WaveSabreCore::VoiceMode::Polyphonic, voiceModeCaptions);

		// ampenv
		ImGui::SeparatorText("Amplitude Envelope");
		WSImGuiParamKnob((VstInt32)M7::Maj7::ParamIndices::AmpEnvDelayTime, "DelayTime##ampenv");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)M7::Maj7::ParamIndices::AmpEnvAttackTime, "AttackTime##ampenv");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)M7::Maj7::ParamIndices::AmpEnvAttackCurve, "AttackCurve##ampenv");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)M7::Maj7::ParamIndices::AmpEnvHoldTime, "HoldTime##ampenv");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)M7::Maj7::ParamIndices::AmpEnvDecayTime, "DecayTime##ampenv");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)M7::Maj7::ParamIndices::AmpEnvDecayCurve, "DecayCurve##ampenv");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)M7::Maj7::ParamIndices::AmpEnvSustainLevel, "SustainLevel##ampenv");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)M7::Maj7::ParamIndices::AmpEnvReleaseTime, "ReleaseTime##ampenv");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)M7::Maj7::ParamIndices::AmpEnvReleaseCurve, "ReleaseCurve##ampenv");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)M7::Maj7::ParamIndices::AmpEnvLegatoRestart, "LegatoRestart##ampenv");

		// osc1
		ImGui::SeparatorText("Oscillator A");
		WSImGuiParamCheckbox((VstInt32)M7::Maj7::ParamIndices::Osc1Enabled, "Enabled");
		ImGui::SameLine(); WSImGuiParamKnob((VstInt32)M7::Maj7::ParamIndices::Osc1Volume, "Volume##osc1");
		ImGui::SameLine(); WSImGuiParamKnob((VstInt32)M7::Maj7::ParamIndices::Osc1Waveform, "Waveform##osc1");
		ImGui::SameLine(); WSImGuiParamKnob((VstInt32)M7::Maj7::ParamIndices::Osc1Waveshape, "Waveshape##osc1");
		ImGui::SameLine(0, 60); WSImGuiParamKnob((VstInt32)M7::Maj7::ParamIndices::Osc1FrequencyParam, "Freq##osc1");
		ImGui::SameLine(); WSImGuiParamKnob((VstInt32)M7::Maj7::ParamIndices::Osc1FrequencyParamKT, "KT##osc1");
		ImGui::SameLine(); WSImGuiParamKnob((VstInt32)M7::Maj7::ParamIndices::Osc1PitchSemis, "PitchSemis##osc1");
		ImGui::SameLine(); WSImGuiParamKnob((VstInt32)M7::Maj7::ParamIndices::Osc1PitchFine, "PitchFine##osc1");
		ImGui::SameLine(); Maj7ImGuiParamScaledFloat((VstInt32)M7::Maj7::ParamIndices::Osc1FreqMul, "FreqMul##osc1", -M7::OscillatorNode::gFrequencyMulMax, M7::OscillatorNode::gFrequencyMulMax, 1);
		WSImGuiParamCheckbox((VstInt32)M7::Maj7::ParamIndices::Osc1PhaseRestart, "PhaseRestart##osc1");
		ImGui::SameLine(); WSImGuiParamKnob((VstInt32)M7::Maj7::ParamIndices::Osc1PhaseOffset, "PhaseOffset##osc1");
		ImGui::SameLine(0, 60); WSImGuiParamCheckbox((VstInt32)M7::Maj7::ParamIndices::Osc1SyncEnable, "SyncEnable##osc1");
		ImGui::SameLine(); WSImGuiParamKnob((VstInt32)M7::Maj7::ParamIndices::Osc1SyncFrequency, "SyncFrequency##osc1");
		ImGui::SameLine(); WSImGuiParamKnob((VstInt32)M7::Maj7::ParamIndices::Osc1SyncFrequencyKT, "SyncFrequencyKT##osc1");

		ImGui::SameLine(0, 60); WSImGuiParamKnob((VstInt32)M7::Maj7::ParamIndices::Osc1FMFeedback, "FMFeedback##osc1");

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

};
