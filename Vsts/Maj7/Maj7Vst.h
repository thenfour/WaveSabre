
#pragma once

#include <WaveSabreVstLib.h>
//using namespace WaveSabreVstLib;
//
//#include <WaveSabreCore.h>
//using namespace WaveSabreCore;

class Maj7Vst : public WaveSabreVstLib::VstPlug
{
public:
	Maj7Vst(audioMasterCallback audioMaster);
	
	virtual void getParameterName(VstInt32 index, char *text);

	virtual bool getEffectName(char *name);
	virtual bool getProductString(char *text);

	WaveSabreCore::M7::Maj7 *GetMaj7() const;
};
