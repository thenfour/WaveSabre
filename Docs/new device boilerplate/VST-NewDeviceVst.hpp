#pragma once

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

extern WaveSabreVstLib::VstEditor* createEditor(AudioEffect* audioEffect);

class Maj7CompVst : public VstPlug
{
public:

	Maj7CompVst(audioMasterCallback audioMaster)
		: VstPlug(audioMaster, (int)Maj7Comp::ParamIndices::NumParams, 2, 2, 'M7cp', new Maj7Comp(), false)
	{
		setEditor(createEditor(this));
	}

	virtual void getParameterName(VstInt32 index, char* text) override
	{
		MAJ7COMP_PARAM_VST_NAMES(paramNames);
		vst_strncpy(text, paramNames[index], kVstMaxParamStrLen);
	}

	virtual bool getEffectName(char* name) override {
		vst_strncpy(name, "WaveSabre - Maj7 Comp", kVstMaxEffectNameLen);
		return true;
	}
	
	virtual bool getProductString(char* text) override {
		vst_strncpy(text, "WaveSabre - Maj7 Comp", kVstMaxProductStrLen);
		return true;
	}

	virtual VstInt32 getChunk(void** data, bool isPreset) override
	{
		MAJ7COMP_PARAM_VST_NAMES(paramNames);
		return GetSimpleJSONVstChunk(GetJSONTagName(), data, GetMaj7Comp()->mParamCache, paramNames);
	}

	virtual VstInt32 setChunk(void* data, VstInt32 byteSize, bool isPreset) override
	{
		MAJ7COMP_PARAM_VST_NAMES(paramNames);
		return SetSimpleJSONVstChunk(GetMaj7Comp(), GetJSONTagName(), data, byteSize, GetMaj7Comp()->mParamCache, paramNames);
	}

	virtual const char* GetJSONTagName() { return "Maj7Comp"; }

	//virtual void OptimizeParams() override
	//{
	//	using Params = Maj7Sat::ParamIndices;
	//	M7::ParamAccessor p{GetMaj7Comp()->mParamCache, 0};
	//	OptimizeBoolParam(p, Params::AEnable);
	//}

	Maj7Comp* GetMaj7Comp() const {
		return (Maj7Comp*)getDevice();
	}
};
