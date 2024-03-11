#ifndef __CHAMBEREDITOR_H__
#define __CHAMBEREDITOR_H__

#include <WaveSabreVstLib.h>
#include <WaveSabreCore.h>

using namespace WaveSabreVstLib;

class ChamberEditor : public VstEditor
{
public:

	ChamberEditor(AudioEffect* audioEffect)
		: VstEditor(audioEffect, 800, 600)
	{
	}

	~ChamberEditor()
	{
	}

	virtual void renderImgui() override
	{
		ImGui::Text("not usable at the moment. possibly we remove this device.");
	}
};

#endif
