#ifndef __CATHEDRALVST_H__
#define __CATHEDRALVST_H__

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

class CathedralVst : public VstPlug
{
public:
	CathedralVst(audioMasterCallback audioMaster);

	virtual void getParameterName(VstInt32 index, char *text);

	virtual bool getEffectName(char *name);
	virtual bool getProductString(char *text);
	virtual const char* GetJSONTagName() { return "Cathedral"; }

	Cathedral* GetCathedral() const
	{
		return (Cathedral*)getDevice();
	}

	virtual VstInt32 getChunk(void** data, bool isPreset) override
	{
		CATHEDRAL_PARAM_VST_NAMES(paramNames);
		return GetSimpleJSONVstChunk(GetJSONTagName(), data, GetCathedral()->mParamCache, paramNames, [](clarinoid::JsonVariantWriter&) {});
	}

	virtual VstInt32 setChunk(void* data, VstInt32 byteSize, bool isPreset) override
	{
		CATHEDRAL_PARAM_VST_NAMES(paramNames);
		return SetSimpleJSONVstChunk(GetCathedral(), GetJSONTagName(), data, byteSize, GetCathedral()->mParamCache, paramNames, [](clarinoid::JsonVariantReader&) {});
	}
};

#endif