#ifndef __TWISTERVST_H__
#define __TWISTERVST_H__

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

class TwisterVst : public VstPlug
{
public:
	TwisterVst(audioMasterCallback audioMaster);

	virtual void getParameterName(VstInt32 index, char *text);

	virtual bool getEffectName(char *name);
	virtual bool getProductString(char *text);
	virtual const char* GetJSONTagName() { return "Twister"; }

	Twister* GetTwister() const
	{
		return (Twister*)getDevice();
	}

	virtual VstInt32 getChunk(void** data, bool isPreset) override
	{
		TWISTER_PARAM_VST_NAMES(paramNames);
		return GetSimpleJSONVstChunk(GetJSONTagName(), data, GetTwister()->mParamCache, paramNames);
	}

	virtual VstInt32 setChunk(void* data, VstInt32 byteSize, bool isPreset) override
	{
		TWISTER_PARAM_VST_NAMES(paramNames);
		return SetSimpleJSONVstChunk(GetTwister(), GetJSONTagName(), data, byteSize, GetTwister()->mParamCache, paramNames);
	}
};

#endif