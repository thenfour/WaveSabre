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
		if (audioMaster)
			setEditor(createEditor(this));
	}

	virtual void getParameterName(VstInt32 index, char* text) override
	{
		MAJ7COMP_PARAM_VST_NAMES(paramNames);
		vst_strncpy(text, paramNames[index], kVstMaxParamStrLen);
	}

	virtual bool getEffectName(char* name) override {
		vst_strncpy(name, "Maj7 - Comp", kVstMaxEffectNameLen);
		return true;
	}
	
	virtual bool getProductString(char* text) override {
		vst_strncpy(text, "Maj7 - Comp", kVstMaxProductStrLen);
		return true;
	}

	virtual const char* GetJSONTagName() { return "Maj7Comp"; }

	virtual VstInt32 getChunk(void** data, bool isPreset) override
	{
		MAJ7COMP_PARAM_VST_NAMES(paramNames);
		return GetSimpleJSONVstChunk(GetJSONTagName(), data, GetMaj7Comp()->mParamCache, paramNames, [](clarinoid::JsonVariantWriter&) {});
	}

	virtual VstInt32 setChunk(void* data, VstInt32 byteSize, bool isPreset) override
	{
		MAJ7COMP_PARAM_VST_NAMES(paramNames);
		return SetSimpleJSONVstChunk(GetMaj7Comp(), GetJSONTagName(), data, byteSize, GetMaj7Comp()->mParamCache, paramNames, [](clarinoid::JsonVariantReader&) {});
	}

	Maj7Comp* GetMaj7Comp() const {
		return (Maj7Comp*)getDevice();
	}


	virtual void OptimizeParams() override
	{
		using Params = Maj7Comp::ParamIndices;
		M7::ParamAccessor defaults{ mDefaultParamCache.data(), 0 };
		M7::ParamAccessor p{ GetMaj7Comp()->mParamCache, 0 };
		if (!p.GetBoolValue(Params::EnableSidechainFilter)) {
			p.SetRawVal(Params::HighPassFrequency, defaults.GetRawVal(Params::HighPassFrequency));
			p.SetRawVal(Params::HighPassQ, defaults.GetRawVal(Params::HighPassQ));
			p.SetRawVal(Params::LowPassFrequency, defaults.GetRawVal(Params::LowPassFrequency));
			p.SetRawVal(Params::LowPassQ, defaults.GetRawVal(Params::LowPassQ));
		}
	}

	virtual void GenerateDefaults() override {
		auto& p = GetMaj7Comp()->mParams;
		using Params = Maj7Comp::ParamIndices;

		p.SetDecibels(Params::InputGain, M7::gVolumeCfg24db, 0.0f);
		p.SetRangedValue(Params::Threshold, -60.0f, 0.0f, -20.0f);
		p.SetPowCurvedValue(Params::Attack, MonoCompressor::gAttackCfg, 50.0f);
		p.SetPowCurvedValue(Params::Release, MonoCompressor::gReleaseCfg, 80.0f);
		p.SetDivCurvedValue(Params::Ratio, MonoCompressor::gRatioCfg, 4.0f);
		p.SetRangedValue(Params::Knee, 0.0f, 30.0f, 4.0f);
		p.Set01Val(Params::ChannelLink, 0.8f);

		p.SetDecibels(Params::CompensationGain, M7::gVolumeCfg24db, 0.0f);
		p.Set01Val(Params::DryWet, 1.0f);

		p.SetBoolValue(Params::EnableSidechainFilter, false);
		p.SetFrequencyAssumingNoKeytracking(Params::HighPassFrequency, M7::gFilterFreqConfig, 110.0f);
		p.Set01Val(Params::HighPassQ, 0.2f);
		p.SetFrequencyAssumingNoKeytracking(Params::LowPassFrequency, M7::gFilterFreqConfig, 8000.0f);
		p.Set01Val(Params::LowPassQ, 0.2f);

		p.SetEnumValue(Params::OutputSignal, Maj7Comp::OutputSignal::Normal);
		p.SetDecibels(Params::OutputGain, M7::gVolumeCfg24db, 0.0f);
	}

};

