#ifndef __TWISTEREDITOR_H__
#define __TWISTEREDITOR_H__

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

class TwisterEditor : public VstEditor
{
public:

	TwisterEditor(AudioEffect* audioEffect)
		: VstEditor(audioEffect, 900, 300)
	{
	}

	virtual void renderImgui() override
	{

		ImGui::Text("not usable at the moment. possibly we remove this device.");

	}
};

#endif
