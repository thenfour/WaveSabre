#pragma once

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;


class Maj7WidthVst : public VstPlug
{
public:
	Maj7WidthVst(audioMasterCallback audioMaster);
	
	virtual void getParameterName(VstInt32 index, char *text);

	virtual bool getEffectName(char *name);
	virtual bool getProductString(char *text);

	Maj7Width* GetMaj7Width() const;

	virtual const char* GetJSONTagName() { return "Maj7Width"; }

	virtual VstInt32 getChunk(void** data, bool isPreset) override
	{
		MAJ7WIDTH_PARAM_VST_NAMES(paramNames);
		return GetSimpleJSONVstChunk(GetJSONTagName(), data, GetMaj7Width()->mParamCache, paramNames, [](clarinoid::JsonVariantWriter&) {});
	}

	virtual VstInt32 setChunk(void* data, VstInt32 byteSize, bool isPreset) override
	{
		MAJ7WIDTH_PARAM_VST_NAMES(paramNames);
		return SetSimpleJSONVstChunk(GetMaj7Width(), GetJSONTagName(), data, byteSize, GetMaj7Width()->mParamCache, paramNames, [](clarinoid::JsonVariantReader&) {});
	}
};
