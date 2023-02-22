#pragma once

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

#include <WaveSabreCore/Maj7.hpp>
#include "Maj7Vst.h"

class Maj7Editor : public VstEditor
{
	Maj7* pMaj7;
public:
	Maj7Editor(AudioEffect* audioEffect) : VstEditor(audioEffect, 1000, 800),
		pMaj7(((Maj7Vst*)audioEffect)->GetMaj7())
	{
	}

	virtual void renderImgui() override
	{
		if (ImGui::Button("panic")) {
			pMaj7->AllNotesOff();
		}

		WSImGuiParamVoiceMode((VstInt32)Maj7::ParamIndices::VoicingMode, "VoiceMode##mst");

		ImGui::Text("freq:%fHz note %d, current poly:%d", pMaj7->mLatestFreq, pMaj7->mLatestNote, pMaj7->GetCurrentPolyphony());
		WSImGuiParamKnob((VstInt32)Maj7::ParamIndices::Master, "Output volume##hc");

		for (size_t i = 0; i < std::size(pMaj7->mVoices); ++ i) {
			auto pv = pMaj7->mVoices[i];
			char txt[200];
			if (i & 0x7) ImGui::SameLine();
			auto color = ImColor::HSV(0, 0, .3f);
			if (pv->IsPlaying()) {
				auto& ns = pMaj7->mNoteStates[pv->mNoteInfo.MidiNoteValue];
				std::sprintf(txt, "%d:%d %c%c #%d", (int)i, ns.MidiNoteValue, ns.mIsPhysicallyHeld ? 'P' : ' ', ns.mIsMusicallyHeld ? 'M':' ', ns.mSequence);
				color = ImColor::HSV(2/7.0f, .8, .7f);
			}
			else {
				std::sprintf(txt, "%d:off", (int)i);
			}
			ImGui::PushStyleColor(ImGuiCol_Button, (ImVec4)color);
			ImGui::PushStyleColor(ImGuiCol_ButtonHovered, (ImVec4)color);
			ImGui::PushStyleColor(ImGuiCol_ButtonActive, (ImVec4)color);
			ImGui::Button(txt);
			ImGui::PopStyleColor(3);
		}
	}

};
