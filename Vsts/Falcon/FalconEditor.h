#ifndef __FALCONEDITOR_H__
#define __FALCONEDITOR_H__

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;
#include <WaveSabreCore.h>
using namespace WaveSabreCore;

class FalconEditor : public VstEditor
{
public:

	FalconEditor(AudioEffect* audioEffect)
		: VstEditor(audioEffect, 1000, 800)
	{
	}

	virtual void renderImgui() override
	{
		if (ImGui::BeginTable("mastertable", 1, ImGuiTableFlags_Borders))
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			ImGui::SeparatorText("Master");

			WSImGuiParamKnob((VstInt32)WaveSabreCore::Falcon::ParamIndices::MasterLevel, "Volume");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)Falcon::ParamIndices::VoicesPan, "PAN");

			ImGui::SameLine(0, 60);
			WSImGuiParamKnob((VstInt32)Falcon::ParamIndices::VoicesUnisono, "UNISONO", ParamBehavior::Unisono);
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)Falcon::ParamIndices::VoicesDetune, "DETUNE");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Falcon::ParamIndices::Rise, "RISE");

			ImGui::SameLine(0, 60);
			WSImGuiParamKnob((VstInt32)Falcon::ParamIndices::VibratoFreq, "VIB FREQ", ParamBehavior::VibratoFreq);
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)Falcon::ParamIndices::VibratoAmount, "VIB AMT");

			ImGui::SameLine(0, 60);
			WSImGuiParamVoiceMode((VstInt32)Falcon::ParamIndices::VoiceMode, "MODE");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)Falcon::ParamIndices::SlideTime, "SLIDE");


			ImGui::EndTable();
		}

		if (ImGui::BeginTable("osc1table", 1, ImGuiTableFlags_Borders))
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			ImGui::SeparatorText("Oscillator 1");

			WSImGuiParamKnob((VstInt32)WaveSabreCore::Falcon::ParamIndices::Osc1Waveform, "WAVEFORM##1");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Falcon::ParamIndices::Osc1RatioCoarse, "RATIO COARSE##1");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Falcon::ParamIndices::Osc1RatioFine, "RATIO SEMI##1");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Falcon::ParamIndices::Osc1Feedback, "FEEDBACK##1");

			ImGui::SameLine(0, 60);

			WSImGuiParamKnob((VstInt32)Falcon::ParamIndices::Osc1Attack, "ATTACK##1");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)Falcon::ParamIndices::Osc1Decay, "DECAY##1");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)Falcon::ParamIndices::Osc1Sustain, "SUSTAIN##1");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)Falcon::ParamIndices::Osc1Release, "RELEASE##1");

			ImGui::SameLine(0, 60);
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Falcon::ParamIndices::PitchEnvAmt1, "PitchMod AMT##1");

			ImGui::SameLine(0, 60);
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Falcon::ParamIndices::Osc1FeedForward, "FEEDFORWARD##1");

			ImGui::EndTable();
		}


		if (ImGui::BeginTable("osc2table", 1, ImGuiTableFlags_Borders))
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			ImGui::SeparatorText("Oscillator 2");

			WSImGuiParamKnob((VstInt32)WaveSabreCore::Falcon::ParamIndices::Osc2Waveform, "WAVEFORM##2");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Falcon::ParamIndices::Osc2RatioCoarse, "RATIO COARSE##2");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Falcon::ParamIndices::Osc2RatioFine, "RATIO SEMI##2");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Falcon::ParamIndices::Osc2Feedback, "FEEDBACK##2");

			ImGui::SameLine(0, 60);

			WSImGuiParamKnob((VstInt32)Falcon::ParamIndices::Osc2Attack, "ATTACK##2");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)Falcon::ParamIndices::Osc2Decay, "DECAY##2");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)Falcon::ParamIndices::Osc2Sustain, "SUSTAIN##2");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)Falcon::ParamIndices::Osc2Release, "RELEASE##2");

			ImGui::SameLine(0, 60);
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Falcon::ParamIndices::PitchEnvAmt2, "PitchMod AMT##2");

			ImGui::EndTable();
		}


		if (ImGui::BeginTable("pitchtable", 1, ImGuiTableFlags_Borders))
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			ImGui::SeparatorText("Pitch mod envelope");

			WSImGuiParamKnob((VstInt32)WaveSabreCore::Falcon::ParamIndices::PitchAttack, "P ATTACK");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Falcon::ParamIndices::PitchDecay, "P DECAY");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Falcon::ParamIndices::PitchSustain, "P SUSTAIN");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Falcon::ParamIndices::PitchRelease, "P RELEASE");

			ImGui::EndTable();
		}


	}


};

#endif
