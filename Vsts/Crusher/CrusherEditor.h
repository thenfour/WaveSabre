#ifndef __CRUSHEREDITOR_H__
#define __CRUSHEREDITOR_H__

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;


#include <WaveSabreCore.h>
using namespace WaveSabreCore;



class CrusherEditor : public VstEditor
{
public:
	CrusherEditor(AudioEffect* audioEffect)
		: VstEditor(audioEffect, 600, 400)
	{
	}


	virtual void renderImgui() override
	{
		ImGui::Text("not usable at the moment. possibly we remove this device.");
		//WSImGuiParamKnob((VstInt32)WaveSabreCore::Crusher::ParamIndices::Vertical, "Bitcrush");
		//ImGui::SameLine();
		//WSImGuiParamKnob((VstInt32)WaveSabreCore::Crusher::ParamIndices::Horizontal, "Samplecrush");
		//ImGui::SameLine();
		//WSImGuiParamKnob((VstInt32)WaveSabreCore::Crusher::ParamIndices::DryWet, "DRY/WET");
	}

};

#endif
