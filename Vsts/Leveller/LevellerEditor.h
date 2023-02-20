#ifndef __LEVELLEREDITOR_H__
#define __LEVELLEREDITOR_H__

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;
#include <WaveSabreCore.h>
using namespace WaveSabreCore;

class LevellerEditor : public VstEditor
{
public:
	LevellerEditor(AudioEffect* audioEffect)
		: VstEditor(audioEffect, 800, 500)
	{
	}

	virtual void renderImgui() override
	{
		static constexpr int columnHeight = 320;
		if (ImGui::BeginTable("main", 5)){//, 0, {0,400})) {
			ImGui::TableNextRow();

			ImGui::TableNextColumn();
			ImGui::BeginChild("lc", ImVec2(0, columnHeight), true, ImGuiWindowFlags_MenuBar);

			if (ImGui::BeginMenuBar())
			{
				ImGui::Text("Lowcut");
				ImGui::EndMenuBar();
			}
			ImGui::Dummy({0, 80});
			WSImGuiParamKnob((VstInt32)Leveller::ParamIndices::LowCutFreq, "Freq##lc", ParamBehavior::Frequency);
			WSImGuiParamKnob((VstInt32)Leveller::ParamIndices::LowCutQ, "Q##lc", ParamBehavior::FilterQ);
			ImGui::EndChild();


			ImGui::TableNextColumn();
			ImGui::BeginChild("p1", ImVec2(0, columnHeight), true, ImGuiWindowFlags_MenuBar);

			if (ImGui::BeginMenuBar())
			{
				ImGui::Text("Band 1");
				ImGui::EndMenuBar();
			}

			WSImGuiParamKnob((VstInt32)Leveller::ParamIndices::Peak1Gain, "Gain##1", ParamBehavior::Db);
			WSImGuiParamKnob((VstInt32)Leveller::ParamIndices::Peak1Freq, "Freq##1", ParamBehavior::Frequency);
			WSImGuiParamKnob((VstInt32)Leveller::ParamIndices::Peak1Q, "Q##1", ParamBehavior::FilterQ);

			ImGui::EndChild();




			ImGui::TableNextColumn();
			ImGui::BeginChild("p2", ImVec2(0, columnHeight), true, ImGuiWindowFlags_MenuBar);

			if (ImGui::BeginMenuBar())
			{
				ImGui::Text("Band 2");
				ImGui::EndMenuBar();
			}

			WSImGuiParamKnob((VstInt32)Leveller::ParamIndices::Peak2Gain, "Gain##2", ParamBehavior::Db);
			WSImGuiParamKnob((VstInt32)Leveller::ParamIndices::Peak2Freq, "Freq##2", ParamBehavior::Frequency);
			WSImGuiParamKnob((VstInt32)Leveller::ParamIndices::Peak2Q, "Q##2", ParamBehavior::FilterQ);

			ImGui::EndChild();




			ImGui::TableNextColumn();
			ImGui::BeginChild("p3", ImVec2(0, columnHeight), true, ImGuiWindowFlags_MenuBar);

			if (ImGui::BeginMenuBar())
			{
				ImGui::Text("Band 3");
				ImGui::EndMenuBar();
			}

			WSImGuiParamKnob((VstInt32)Leveller::ParamIndices::Peak3Gain, "Gain##3", ParamBehavior::Db);
			WSImGuiParamKnob((VstInt32)Leveller::ParamIndices::Peak3Freq, "Freq##3", ParamBehavior::Frequency);
			WSImGuiParamKnob((VstInt32)Leveller::ParamIndices::Peak3Q, "Q##3", ParamBehavior::FilterQ);

			ImGui::EndChild();




			ImGui::TableNextColumn();
			ImGui::BeginChild("hc", ImVec2(0, columnHeight), true, ImGuiWindowFlags_MenuBar);

			if (ImGui::BeginMenuBar())
			{
				ImGui::Text("Highcut");
				ImGui::EndMenuBar();
			}

			ImGui::Dummy({ 0, 80 });
			WSImGuiParamKnob((VstInt32)Leveller::ParamIndices::HighCutFreq, "Freq##hc", ParamBehavior::Frequency);
			WSImGuiParamKnob((VstInt32)Leveller::ParamIndices::HighCutQ, "Q##hc", ParamBehavior::FilterQ);
			ImGui::EndChild();


			ImGui::EndTable();
		}

		WSImGuiParamKnob((VstInt32)Leveller::ParamIndices::Master, "Output volume##hc");
	}


};

#endif
