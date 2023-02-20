#ifndef __SLAUGHTEREDITOR_H__
#define __SLAUGHTEREDITOR_H__

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

class SlaughterEditor : public VstEditor
{
public:
	SlaughterEditor::SlaughterEditor(AudioEffect* audioEffect)
		: VstEditor(audioEffect, 1000, 800)
	{
	}

	virtual ~SlaughterEditor() {}

	virtual void renderImgui()
	{
		if (ImGui::BeginTable("mastertable", 1, ImGuiTableFlags_Borders))
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			ImGui::SeparatorText("Master");

			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::MasterLevel, "Volume##mst");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::VoicesUnisono, "UNISONO##pitchenv", ParamBehavior::Unisono);
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::VoicesDetune, "DETUNE##pitchenv");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::VoicesPan, "Voice panning##mst");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::VibratoFreq, "VibratoFreq##mst", ParamBehavior::VibratoFreq);
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::VibratoAmount, "VibratoAmount##mst");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::Rise, "Rise##mst");
			ImGui::SameLine();
			WSImGuiParamVoiceMode((VstInt32)WaveSabreCore::Slaughter::ParamIndices::VoiceMode, "VoiceMode##mst");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::SlideTime, "SlideTime##mst");

			ImGui::EndTable();
		}

		if (ImGui::BeginTable("osc12table", 2, ImGuiTableFlags_Borders))
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			ImGui::SeparatorText("Osc 1");
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::Osc1Waveform, "Saw-Pulse##1");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::Osc1PulseWidth, "PW##1");
			ImGui::SameLine(0, 60);
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::Osc1DetuneCoarse, "Det.Course##1");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::Osc1DetuneFine, "Det.Fine##1");
			ImGui::SameLine(0, 60);
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::Osc1Volume, "Volume##1");

			ImGui::TableNextColumn();

			ImGui::SeparatorText("Osc 2");
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::Osc2Waveform, "Saw-Pulse##2");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::Osc2PulseWidth, "PW##2");
			ImGui::SameLine(0, 60);
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::Osc2DetuneCoarse, "Det.Course##2");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::Osc2DetuneFine, "Det.Fine##2");
			ImGui::SameLine(0, 60);
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::Osc2Volume, "Volume##2");

			ImGui::EndTable();
		}

		if (ImGui::BeginTable("osc34table", 2, ImGuiTableFlags_Borders))
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			ImGui::SeparatorText("Osc 3");
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::Osc3Waveform, "Saw-Pulse##3");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::Osc3PulseWidth, "PW##3");
			ImGui::SameLine(0, 60);
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::Osc3DetuneCoarse, "Det.Course##3");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::Osc3DetuneFine, "Det.Fine##3");
			ImGui::SameLine(0, 60);
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::Osc3Volume, "Volume##3");

			ImGui::TableNextColumn();

			ImGui::SeparatorText("Noise osc");
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::NoiseVolume, "Volume##noise");

			ImGui::EndTable();
		}

		if (ImGui::BeginTable("filterTable", 2, ImGuiTableFlags_Borders))
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			ImGui::SeparatorText("Filter");
			WSImGuiParamFilterType((VstInt32)WaveSabreCore::Slaughter::ParamIndices::FilterType, "Type##flt");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::FilterFreq, "Freq##flt", ParamBehavior::Frequency, "%.0fHz");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::FilterResonance, "Q##flt");

			ImGui::TableNextColumn();

			ImGui::SeparatorText("Filter modulation");

			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::ModAttack, "Attack##flt");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::ModDecay, "Decay##flt");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::ModSustain, "Sustain##flt");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::ModRelease, "Release##flt");
			ImGui::SameLine(0, 60);
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::FilterModAmt, "Amt##flt");

			ImGui::EndTable();
		}

		if (ImGui::BeginTable("amppitchenvtable", 2, ImGuiTableFlags_Borders))
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			ImGui::SeparatorText("Amplitude envelope");
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::AmpAttack, "Attack##ampenv");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::AmpDecay, "Decay##ampenv");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::AmpSustain, "Sustain##ampenv");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::AmpRelease, "Release##ampenv");

			ImGui::TableNextColumn();

			ImGui::SeparatorText("Pitch envelope");
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::PitchAttack, "Attack##pitchenv");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::PitchDecay, "Decay##pitchenv");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::PitchSustain, "Sustain##pitchenv");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::PitchRelease, "Release##pitchenv");
			ImGui::SameLine(0, 60);
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Slaughter::ParamIndices::PitchEnvAmt, "Amount##pitchenv");

			ImGui::EndTable();
		}
	}
};

#endif
