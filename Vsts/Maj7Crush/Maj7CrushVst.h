#pragma once

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

extern WaveSabreVstLib::VstEditor* createEditor(AudioEffect* audioEffect);

class Maj7CrushVst : public VstPlug
{
public:

	Maj7CrushVst(audioMasterCallback audioMaster)
		: VstPlug(audioMaster, (int)Maj7Crush::ParamIndices::NumParams, 2, 2, 'M7cr', new Maj7Crush(), false)
	{
		if (audioMaster)
			setEditor(createEditor(this));
	}

	virtual void getParameterName(VstInt32 index, char* text) override
	{
		MAJ7CRUSH_PARAM_VST_NAMES(paramNames);
		vst_strncpy(text, paramNames[index], kVstMaxParamStrLen);
	}

	virtual bool getEffectName(char* name) override {
		vst_strncpy(name, "Maj7 - Crush", kVstMaxEffectNameLen);
		return true;
	}
	
	virtual bool getProductString(char* text) override {
		vst_strncpy(text, "Maj7 - Crush", kVstMaxProductStrLen);
		return true;
	}

	virtual const char* GetJSONTagName() { return "Maj7Crush"; }

	virtual VstInt32 getChunk(void** data, bool isPreset) override
	{
		MAJ7CRUSH_PARAM_VST_NAMES(paramNames);
		return GetSimpleJSONVstChunk(GetJSONTagName(), data, GetMaj7Crush()->mParamCache, paramNames, [](clarinoid::JsonVariantWriter&) {});
	}

	virtual VstInt32 setChunk(void* data, VstInt32 byteSize, bool isPreset) override
	{
		MAJ7CRUSH_PARAM_VST_NAMES(paramNames);
		return SetSimpleJSONVstChunk(GetMaj7Crush(), GetJSONTagName(), data, byteSize, GetMaj7Crush()->mParamCache, paramNames, [](clarinoid::JsonVariantReader&) {});
	}

	Maj7Crush* GetMaj7Crush() const {
		return (Maj7Crush*)getDevice();
	}

	virtual void OptimizeParams() override
	{
		using Params = Maj7Crush::ParamIndices;
		M7::ParamAccessor defaults{ mDefaultParamCache.data(), 0 };
		M7::ParamAccessor p{ GetMaj7Crush()->mParamCache, 0 };
	}

	
	virtual void GenerateDefaults() override
	{
	}

};
