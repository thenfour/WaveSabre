#ifndef __CRUSHERVST_H__
#define __CRUSHERVST_H__

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

class CrusherVst : public VstPlug
{
public:
	CrusherVst(audioMasterCallback audioMaster);

	virtual void getParameterName(VstInt32 index, char *text);

	virtual bool getEffectName(char *name);
	virtual bool getProductString(char *text);
	virtual const char* GetJSONTagName() { return "Crusher"; }

	Crusher* GetCrusher() const
	{
		return (Crusher*)getDevice();
	}

	virtual VstInt32 getChunk(void** data, bool isPreset) override
	{
		CRUSHER_PARAM_VST_NAMES(paramNames);
		return GetSimpleJSONVstChunk(GetJSONTagName(), data, GetCrusher()->mParamCache, paramNames, [](clarinoid::JsonVariantWriter&) {});
	}

	virtual VstInt32 setChunk(void* data, VstInt32 byteSize, bool isPreset) override
	{
		CRUSHER_PARAM_VST_NAMES(paramNames);
		return SetSimpleJSONVstChunk(GetCrusher(), GetJSONTagName(), data, byteSize, GetCrusher()->mParamCache, paramNames, [](clarinoid::JsonVariantReader&) {});
	}
};

#endif