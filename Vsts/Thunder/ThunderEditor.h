#ifndef __THUNDEREDITOR_H__
#define __THUNDEREDITOR_H__

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;


class ThunderEditor : public VstEditor
{
public:
	ThunderEditor(AudioEffect *audioEffect) : VstEditor(audioEffect, 400, 200) {}
	virtual void renderImgui() {
		ImGui::Text("obsolete dont use");
	}
};

#endif
