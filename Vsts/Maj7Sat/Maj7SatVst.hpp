#pragma once

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

extern WaveSabreVstLib::VstEditor* createEditor(AudioEffect* audioEffect);

class Maj7SatVst : public VstPlug
{
public:

	Maj7SatVst(audioMasterCallback audioMaster)
		: VstPlug(audioMaster, (int)Maj7Sat::ParamIndices::NumParams, 2, 2, 'M7st', new Maj7Sat(), false)
	{
		setEditor(createEditor(this));
	}

	virtual void getParameterName(VstInt32 index, char* text) override
	{
		MAJ7SAT_PARAM_VST_NAMES(paramNames);
		vst_strncpy(text, paramNames[index], kVstMaxParamStrLen);
	}

	virtual bool getEffectName(char* name) override {
		vst_strncpy(name, "WaveSabre - Maj7 Sat", kVstMaxEffectNameLen);
		return true;
	}
	
	virtual bool getProductString(char* text) override {
		vst_strncpy(text, "WaveSabre - Maj7 Sat", kVstMaxProductStrLen);
		return true;
	}

	virtual VstInt32 getChunk(void** data, bool isPreset) override
	{
		MAJ7SAT_PARAM_VST_NAMES(paramNames);
		return GetSimpleJSONVstChunk(Echo::gJSONTagName, data, GetMaj7Sat()->mParamCache, paramNames);
	}

	virtual VstInt32 setChunk(void* data, VstInt32 byteSize, bool isPreset) override
	{
		MAJ7SAT_PARAM_VST_NAMES(paramNames);
		return SetSimpleJSONVstChunk(GetMaj7Sat(), Echo::gJSONTagName, data, byteSize, GetMaj7Sat()->mParamCache, paramNames);
	}

	Maj7Sat* GetMaj7Sat() const {
		return (Maj7Sat*)getDevice();
	}
};
