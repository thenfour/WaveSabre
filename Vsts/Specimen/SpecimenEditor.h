#ifndef __SPECIMENEDITOR_H__
#define __SPECIMENEDITOR_H__

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

#include "SpecimenVst.h"

class SpecimenEditor : public VstEditor
{
public:
	SpecimenEditor(AudioEffect* audioEffect) : VstEditor(audioEffect, 1000, 800)
	{
		mFileDialog.SetTitle("WaveSabre: Load sample");
		mFileDialog.SetTypeFilters({ ".wav" });

		specimen = ((SpecimenVst *)audioEffect)->GetSpecimen();
	}

	virtual void renderImgui() override
	{
		static constexpr char const* const interpModeNames[] = { "Nearest", "Linear" };
		static constexpr char const* const loopModeNames[] = { "Disabled", "Repeat", "Pingpong" };

		if (ImGui::Button("load sample")) {
			mFileDialog.Open();
		}

		if (!this->specimen->sample) {
			ImGui::Text("No sample loaded");
		}
		else {
			auto* p = this->specimen->sample;

			ImGui::Text("Uncompressed size: %d, compressed to %d (%d%%)\r\n%d Samples", p->UncompressedSize, p->CompressedSize, (p->CompressedSize * 100) / p->UncompressedSize, p->SampleLength);
		}

		switch (mStatusStyle)
		{
		case StatusStyle::NoStyle:
			ImGui::Text("%s", mStatus.c_str());
			break;
		case StatusStyle::Error:
			ImGui::TextColored(ImColor::HSV(0 / 7.0f, 0.6f, 0.6f), "%s", mStatus.c_str());
			break;
		case StatusStyle::Warning:
			ImGui::TextColored(ImColor::HSV(1 / 7.0f, 0.6f, 0.6f), "%s", mStatus.c_str());
			break;
		case StatusStyle::Green:
			ImGui::TextColored(ImColor::HSV(2 / 7.0f, 0.6f, 0.6f), "%s", mStatus.c_str());
			break;
		}



		ImGui::BeginChild("main", ImVec2(0, 140), true, ImGuiWindowFlags_MenuBar);

		if (ImGui::BeginMenuBar())
		{
			ImGui::Text("Master");
			ImGui::EndMenuBar();
		}

		WSImGuiParamKnob((VstInt32)Specimen::ParamIndices::VoicesUnisono, "UNISONO", ParamBehavior::Unisono);
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)Specimen::ParamIndices::VoicesDetune, "DETUNE");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)Specimen::ParamIndices::VoicesPan, "PAN");

		ImGui::SameLine(0, 70);
		WSImGuiParamKnob((VstInt32)Specimen::ParamIndices::CoarseTune, "COARSE TUNE");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)Specimen::ParamIndices::FineTune, "FINE TUNE");

		ImGui::SameLine(0, 70);
		WSImGuiParamVoiceMode((VstInt32)Slaughter::ParamIndices::VoiceMode, "MODE");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)Slaughter::ParamIndices::SlideTime, "SLIDE");

		ImGui::SameLine(0, 70);
		WSImGuiParamEnumList((VstInt32)Specimen::ParamIndices::InterpolationMode, "INTERPOLATION", (int)InterpolationMode::NumInterpolationModes, interpModeNames);
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)Specimen::ParamIndices::Master, "MASTER");

		ImGui::EndChild();


		ImGui::BeginChild("footprint", ImVec2(0, 140), true, ImGuiWindowFlags_MenuBar);

		if (ImGui::BeginMenuBar())
		{
			ImGui::Text("Sample footprint");
			ImGui::EndMenuBar();
		}

		WSImGuiParamCheckbox((VstInt32)Specimen::ParamIndices::Reverse, "REVERSE");
		ImGui::SameLine(0, 70);
		WSImGuiParamKnob((VstInt32)Specimen::ParamIndices::SampleStart, "SAMPLE START");
		ImGui::SameLine();
		WSImGuiParamEnumList((VstInt32)Specimen::ParamIndices::LoopMode, "LOOP MODE", (int)LoopMode::NumLoopModes, loopModeNames);
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)Specimen::ParamIndices::LoopStart, "LOOP START");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)Specimen::ParamIndices::LoopLength, "LOOP LENGTH");

		ImGui::EndChild();


		ImGui::BeginChild("env", ImVec2(0, 140), true, ImGuiWindowFlags_MenuBar);

		if (ImGui::BeginMenuBar())
		{
			ImGui::Text("Amplitude envelope");
			ImGui::EndMenuBar();
		}

		WSImGuiParamKnob((VstInt32)Specimen::ParamIndices::AmpAttack, "ATTACK");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)Specimen::ParamIndices::AmpDecay, "DECAY");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)Specimen::ParamIndices::AmpSustain, "SUSTAIN");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)Specimen::ParamIndices::AmpRelease, "RELEASE");

		ImGui::EndChild();


		ImGui::BeginChild("flt", ImVec2(0, 140), true, ImGuiWindowFlags_MenuBar);

		if (ImGui::BeginMenuBar())
		{
			ImGui::Text("Filter");
			ImGui::EndMenuBar();
		}

		WSImGuiParamFilterType((VstInt32)Specimen::ParamIndices::FilterType, "TYPE");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)Specimen::ParamIndices::FilterFreq, "FREQ", ParamBehavior::Frequency);
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)Specimen::ParamIndices::FilterResonance, "Q");

		ImGui::SameLine(0, 80);
		WSImGuiParamKnob((VstInt32)Specimen::ParamIndices::ModAttack, "Mod attack");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)Specimen::ParamIndices::ModDecay, "DECAY");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)Specimen::ParamIndices::ModSustain, "SUSTAIN");
		ImGui::SameLine();
		WSImGuiParamKnob((VstInt32)Specimen::ParamIndices::ModRelease, "RELEASE");
		
		ImGui::SameLine(0, 80);
		WSImGuiParamKnob((VstInt32)Specimen::ParamIndices::FilterModAmt, "Mod amount");

		ImGui::EndChild();


		mFileDialog.Display();

		if (mFileDialog.HasSelected())
		{
			mStatus.clear();
			LoadSample(mFileDialog.GetSelected().string().c_str());
			mFileDialog.ClearSelected();
		}
	}

	enum class StatusStyle
	{
		NoStyle,
		Green,
		Warning,
		Error,
	};

private:
	static BOOL __stdcall driverEnumCallback(HACMDRIVERID driverId, DWORD_PTR dwInstance, DWORD fdwSupport);
	static BOOL __stdcall formatEnumCallback(HACMDRIVERID driverId, LPACMFORMATDETAILS formatDetails, DWORD_PTR dwInstance, DWORD fdwSupport);

	static HACMDRIVERID driverId;
	static WAVEFORMATEX *foundWaveFormat;

	void OnLoadSample();
	bool LoadSample(const char *path); // returning bool just so we can 1-line return errors. it's meaningless.

	CFileSelector *fileSelector = nullptr;

	Specimen *specimen;

	ImGui::FileBrowser mFileDialog;
	std::string mStatus = "Ready.";
	StatusStyle mStatusStyle = StatusStyle::NoStyle;

	bool SetStatus(StatusStyle style, const std::string& str) {
		mStatusStyle = style;
		mStatus = str;
		return false;
	}

};

#endif
