#ifndef __SMASHERVST_H__
#define __SMASHERVST_H__

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

class SmasherVst : public VstPlug
{
public:
	class Smasher* mpSmasher;
	SmasherVst(audioMasterCallback audioMaster);

	virtual void getParameterName(VstInt32 index, char *text);

	virtual bool getEffectName(char *name);
	virtual bool getProductString(char *text);
	virtual const char* GetJSONTagName() { return "Smasher"; }
};

#endif