#ifndef __LEVELLERVST_H__
#define __LEVELLERVST_H__

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

class LevellerVst : public VstPlug
{
public:
	LevellerVst(audioMasterCallback audioMaster);

	virtual void getParameterName(VstInt32 index, char *text);

	virtual bool getEffectName(char *name);
	virtual bool getProductString(char *text);

	virtual VstInt32 getChunk(void** data, bool isPreset) override
	{
		LEVELLER_PARAM_VST_NAMES(paramNames);
		auto* p = (WaveSabreCore::Leveller*)getDevice();
		return GetSimpleJSONVstChunk("WSLeveller", data, p->mParamCache, paramNames);
	}

	virtual VstInt32 setChunk(void* data, VstInt32 byteSize, bool isPreset) override
	{
		LEVELLER_PARAM_VST_NAMES(paramNames);
		auto* p = (WaveSabreCore::Leveller*)getDevice();
		return SetSimpleJSONVstChunk("WSLeveller", data, byteSize, p->mParamCache, paramNames);
	}

};

#endif