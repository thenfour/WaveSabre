#ifndef __CATHEDRALEDITOR_H__
#define __CATHEDRALEDITOR_H__

#include <WaveSabreVstLib.h>
#include <WaveSabreCore.h>
using namespace WaveSabreVstLib;

#include "CathedralVst.h"

class CathedralEditor : public VstEditor
{
	Cathedral* mpCathedral;
	CathedralVst* mpCathedralVST;
public:
	CathedralEditor(AudioEffect* audioEffect)
		: VstEditor(audioEffect, 800, 600),
		mpCathedralVST((CathedralVst*)audioEffect)//,
	{
		mpCathedral = (Cathedral*)mpCathedralVST->getDevice();
	}

	virtual void PopulateMenuBar() override
	{
		CATHEDRAL_PARAM_VST_NAMES(paramNames);
		PopulateStandardMenuBar(mCurrentWindow, "Cathedral", mpCathedral, mpCathedralVST, "gParamDefaults", "Cathedral::ParamIndices::NumParams", mpCathedral->mParamCache, paramNames);
	}

	virtual void renderImgui() override
	{
		ImGui::BeginGroup();

		//WSImGuiParamCheckbox((VstInt32)WaveSabreCore::Cathedral::ParamIndices::Freeze, "Freeze");
		//Maj7ImGuiBoolParamToggleButton(WaveSabreCore::Cathedral::ParamIndices::Freeze, "Freeze");

		using ParamIndices = WaveSabreCore::Cathedral::ParamIndices;

		Maj7ImGuiParamFloat01((VstInt32)ParamIndices::PreDelay, "PreDelay", 0, 0);
		ImGui::SameLine(); Maj7ImGuiParamFloat01((VstInt32)ParamIndices::RoomSize, "RoomSize", 0.5f, 0);
		ImGui::SameLine(); Maj7ImGuiParamFloat01((VstInt32)ParamIndices::Damp, "Damp", 0.15f, 0);
		//ImGui::SameLine(); Maj7ImGuiParamFloat01((VstInt32)ParamIndices::Width, "Width", 0.9f, 0);

		//float f1 = 0, f2 = 0;
		//M7::FrequencyParam fp{ f1, f2, M7::gFilterFreqConfig };
		M7::QuickParam fp{ M7::gFilterFreqConfig };

		fp.SetFrequencyAssumingNoKeytracking(145);
		Maj7ImGuiParamFrequency((int)ParamIndices::LowCutFreq, -1, "HP Freq(Hz)", M7::gFilterFreqConfig, fp.GetRawValue(), {});
		ImGui::SameLine();

		fp.SetFrequencyAssumingNoKeytracking(5500);
		Maj7ImGuiParamFrequency((int)ParamIndices::HighCutFreq, -1, "LP Freq(Hz)", M7::gFilterFreqConfig, fp.GetRawValue(), {});

		Maj7ImGuiParamVolume((VstInt32)ParamIndices::DryOut, "Dry output", M7::gVolumeCfg12db, 0, {});
		ImGui::SameLine();
		Maj7ImGuiParamVolume((VstInt32)ParamIndices::WetOut, "Wet output", M7::gVolumeCfg12db, -6, {});


		ImGui::EndGroup();

		ImGui::SameLine(); VUMeter("vu_inp", mpCathedral->mInputAnalysis[0], mpCathedral->mInputAnalysis[1]);

		ImGui::SameLine(); VUMeter("vu_outp", mpCathedral->mOutputAnalysis[0], mpCathedral->mOutputAnalysis[1]);



		//Maj7ImGuiParamScaledFloat((VstInt32)ParamIndices::X, "x", 0.1f, 3, 1,1, 0, {});

	}

};

#endif
