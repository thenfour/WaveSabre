#ifndef __CHAMBERVST_H__
#define __CHAMBERVST_H__

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

class ChamberVst : public VstPlug
{
public:
	ChamberVst(audioMasterCallback audioMaster);

	virtual void getParameterName(VstInt32 index, char *text);

	virtual bool getEffectName(char *name);
	virtual bool getProductString(char *text);
	virtual const char* GetJSONTagName() { return "Chamber"; }

	Chamber* GetChamber() const {
		return (Chamber*)getDevice();
	}

	virtual VstInt32 getChunk(void** data, bool isPreset) override
	{
		CHAMBER_PARAM_VST_NAMES(paramNames);
		return GetSimpleJSONVstChunk(GetJSONTagName(), data, GetChamber()->mParamCache, paramNames);
	}

	virtual VstInt32 setChunk(void* data, VstInt32 byteSize, bool isPreset) override
	{
		CHAMBER_PARAM_VST_NAMES(paramNames);
		return SetSimpleJSONVstChunk(GetChamber(), GetJSONTagName(), data, byteSize, GetChamber()->mParamCache, paramNames);
	}
};

#endif