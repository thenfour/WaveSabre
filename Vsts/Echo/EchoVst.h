#ifndef __ECHOVST_H__
#define __ECHOVST_H__

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

class EchoVst : public VstPlug
{
public:
	EchoVst(audioMasterCallback audioMaster);

	virtual void getParameterName(VstInt32 index, char *text);

	virtual bool getEffectName(char *name);
	virtual bool getProductString(char *text);

	virtual VstInt32 getChunk(void** data, bool isPreset) override
	{
		ECHO_PARAM_VST_NAMES(paramNames);
		auto* p = (WaveSabreCore::Echo*)getDevice();
		return GetSimpleJSONVstChunk(GetJSONTagName(), data, p->mParamCache, paramNames);
	}

	virtual VstInt32 setChunk(void* data, VstInt32 byteSize, bool isPreset) override
	{
		ECHO_PARAM_VST_NAMES(paramNames);
		auto* p = (WaveSabreCore::Echo*)getDevice();
		return SetSimpleJSONVstChunk(p, GetJSONTagName(), data, byteSize, p->mParamCache, paramNames);
	}
	virtual const char* GetJSONTagName() { return "Echo"; }
};

#endif