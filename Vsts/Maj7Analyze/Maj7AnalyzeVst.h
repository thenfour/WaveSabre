#pragma once

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

extern WaveSabreVstLib::VstEditor* createEditor(AudioEffect* audioEffect);

class Maj7AnalyzeVst : public VstPlug
{
public:

	Maj7AnalyzeVst(audioMasterCallback audioMaster)
		: VstPlug(audioMaster, (int)Maj7Analyze::ParamIndices::NumParams, 2, 2, 'M7an', new Maj7Analyze(), false)
	{
		if (audioMaster)
			setEditor(createEditor(this));
	}

	virtual void getParameterName(VstInt32 index, char* text) override
	{
		MAJ7ANALYZE_PARAM_VST_NAMES(paramNames);
		vst_strncpy(text, paramNames[index], kVstMaxParamStrLen);
	}

	virtual bool getEffectName(char* name) override {
		vst_strncpy(name, "Maj7 - Analyze", kVstMaxEffectNameLen);
		return true;
	}
	
	virtual bool getProductString(char* text) override {
		vst_strncpy(text, "Maj7 - Analyze", kVstMaxProductStrLen);
		return true;
	}

	virtual const char* GetJSONTagName() { return "Maj7Analyze"; }

	virtual VstInt32 getChunk(void** data, bool isPreset) override
	{
		MAJ7ANALYZE_PARAM_VST_NAMES(paramNames);
		return GetSimpleJSONVstChunk(GetJSONTagName(), data, GetMaj7Analyze()->mParamCache, paramNames, [](clarinoid::JsonVariantWriter&) {});
	}

	virtual VstInt32 setChunk(void* data, VstInt32 byteSize, bool isPreset) override
	{
		MAJ7ANALYZE_PARAM_VST_NAMES(paramNames);
		return SetSimpleJSONVstChunk(GetMaj7Analyze(), GetJSONTagName(), data, byteSize, GetMaj7Analyze()->mParamCache, paramNames, [](clarinoid::JsonVariantReader&) {});
	}

	Maj7Analyze* GetMaj7Analyze() const {
		return (Maj7Analyze*)getDevice();
	}

	virtual void OptimizeParams() override
	{
		using Params = Maj7Comp::ParamIndices;
		M7::ParamAccessor defaults{ mDefaultParamCache.data(), 0 };
		M7::ParamAccessor p{ GetMaj7Analyze()->mParamCache, 0 };
	}


};
