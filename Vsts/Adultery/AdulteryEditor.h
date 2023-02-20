#ifndef __ADULTERYEDITOR_H__
#define __ADULTERYEDITOR_H__

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

class AdulteryEditor : public VstEditor
{
public:
	AdulteryEditor(AudioEffect *audioEffect, Adultery *adultery);
	virtual ~AdulteryEditor();
	virtual void renderImgui()
	{
		if (ImGui::BeginTable("sampletable", 1, ImGuiTableFlags_Borders))
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			ImGui::SeparatorText("Sample");

			if (ImGui::InputText("sampleFilter", mFilter, std::size(mFilter))) {
				char b[200] = { 0 };
				std::sprintf(b, "sample filter: %s", mFilter);
				OutputDebugStringA(b);
			}
			ImGui::BeginListBox("sample");
			int sampleIndex = (int)GetEffectX()->getParameter((VstInt32)Adultery::ParamIndices::SampleIndex);
			for (auto& x : mOptions) {
				if (std::strlen(mFilter) > 0 && x.first.find(mFilter) == std::string::npos) {
					continue;
				}
				bool isSelected = sampleIndex == x.second;
				if (ImGui::Selectable(x.first.c_str(), &isSelected)) {
					if (isSelected) {
						GetEffectX()->setParameterAutomated((VstInt32)Adultery::ParamIndices::SampleIndex, (float)x.second);
					}
				}
			}
			ImGui::EndListBox();

			WSImGuiParamKnob((VstInt32)Adultery::ParamIndices::SampleStart, "Sample start");

			ImGui::SameLine();
			static constexpr char const* const interpModeNames[] = { "Nearest", "Linear" };
			WSImGuiParamEnumList((VstInt32)Adultery::ParamIndices::InterpolationMode, "Interpolation", (int)InterpolationMode::NumInterpolationModes, interpModeNames);

			ImGui::SameLine();
			WSImGuiParamCheckbox((VstInt32)Adultery::ParamIndices::Reverse, "Reverse");

			static constexpr char const* const loopModeNames[] = { "Disabled", "Repeat", "Pingpong" };
			ImGui::SameLine();
			WSImGuiParamEnumList((VstInt32)Adultery::ParamIndices::LoopMode, "Loop mode", (int)LoopMode::NumLoopModes, loopModeNames);

			ImGui::SameLine();
			static constexpr char const* const boundaryModeNames[] = { "Builtin", "Manual" };
			WSImGuiParamEnumList((VstInt32)Adultery::ParamIndices::LoopBoundaryMode, "Loop boundary", (int)LoopBoundaryMode::NumLoopBoundaryModes, boundaryModeNames);

			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)Adultery::ParamIndices::LoopStart, "Loop start");

			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)Adultery::ParamIndices::LoopLength, "Loop len");

			ImGui::EndTable();
		}


		if (ImGui::BeginTable("mastertable", 1, ImGuiTableFlags_Borders))
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			ImGui::SeparatorText("Master");

			WSImGuiParamKnob((VstInt32)WaveSabreCore::Adultery::ParamIndices::Master, "Volume##mst");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Adultery::ParamIndices::VoicesUnisono, "UNISONO##pitchenv", ParamBehavior::Unisono);
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Adultery::ParamIndices::VoicesDetune, "DETUNE##pitchenv");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Adultery::ParamIndices::VoicesPan, "Voice panning##mst");

			ImGui::SameLine();
			WSImGuiParamVoiceMode((VstInt32)WaveSabreCore::Adultery::ParamIndices::VoiceMode, "VoiceMode##mst");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Adultery::ParamIndices::SlideTime, "SlideTime##mst");

			ImGui::EndTable();
		}

		if (ImGui::BeginTable("##amp", 1)) {
			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			ImGui::SeparatorText("Amplitude envelope");

			WSImGuiParamKnob((VstInt32)Adultery::ParamIndices::AmpAttack, "Attack");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)Adultery::ParamIndices::AmpDecay, "Decay");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)Adultery::ParamIndices::AmpSustain, "Sustain");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)Adultery::ParamIndices::AmpRelease, "Release");

			ImGui::EndTable();
		}

		if (ImGui::BeginTable("filterTable", 2, ImGuiTableFlags_Borders))
		{
			ImGui::TableNextRow();
			ImGui::TableNextColumn();

			ImGui::SeparatorText("Filter");
			WSImGuiParamFilterType((VstInt32)WaveSabreCore::Adultery::ParamIndices::FilterType, "Type##flt");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Adultery::ParamIndices::FilterFreq, "Freq##flt", ParamBehavior::Frequency, "%.0fHz");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Adultery::ParamIndices::FilterResonance, "Q##flt");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Adultery::ParamIndices::FilterModAmt, "Mod env amt##flt");

			ImGui::TableNextColumn();

			ImGui::SeparatorText("Filter modulation");

			WSImGuiParamKnob((VstInt32)WaveSabreCore::Adultery::ParamIndices::ModAttack, "Attack##flt");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Adultery::ParamIndices::ModDecay, "Decay##flt");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Adultery::ParamIndices::ModSustain, "Sustain##flt");
			ImGui::SameLine();
			WSImGuiParamKnob((VstInt32)WaveSabreCore::Adultery::ParamIndices::ModRelease, "Release##flt");
			ImGui::EndTable();
		}
	}

private:
	Adultery *adultery;

	std::vector<std::pair<std::string, int>> mOptions;
	char mFilter[100] = { 0 };
};

#endif
