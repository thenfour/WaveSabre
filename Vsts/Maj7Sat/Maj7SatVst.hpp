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
		if (audioMaster)
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
		return GetSimpleJSONVstChunk(GetJSONTagName(), data, GetMaj7Sat()->mParamCache, paramNames);
	}

	virtual VstInt32 setChunk(void* data, VstInt32 byteSize, bool isPreset) override
	{
		MAJ7SAT_PARAM_VST_NAMES(paramNames);
		return SetSimpleJSONVstChunk(GetMaj7Sat(), GetJSONTagName(), data, byteSize, GetMaj7Sat()->mParamCache, paramNames);
	}

	virtual const char* GetJSONTagName() { return "Maj7Sat"; }

	virtual void OptimizeParams() override
	{
		using Params = Maj7Sat::ParamIndices;
		M7::ParamAccessor p{GetMaj7Sat()->mParamCache, 0};
		OptimizeEnumParam<M7::LinkwitzRileyFilter::Slope>(p, Params::CrossoverASlope);
		OptimizeBand(Params::AMute);
		OptimizeBand(Params::BMute);
		OptimizeBand(Params::CMute);
	}

	void OptimizeBand(Maj7Sat::ParamIndices baseParam) {
		M7::ParamAccessor defaults{mDefaultParamCache.data(), baseParam};
		M7::ParamAccessor p{ GetMaj7Sat()->mParamCache, baseParam };
		using Param = Maj7Sat::FreqBand::BandParam;
		OptimizeBoolParam(p, Param::EnableEffect);
		if (!p.GetBoolValue(Param::EnableEffect)) {
			// effect not enabled; set defaults to params.
			p.SetRawVal(Param::PanMode, defaults.GetRawVal(Param::PanMode));
			p.SetRawVal(Param::Pan, defaults.GetRawVal(Param::Pan));
			p.SetRawVal(Param::Model, defaults.GetRawVal(Param::Model));
			p.SetRawVal(Param::Drive, defaults.GetRawVal(Param::Drive));
			p.SetRawVal(Param::CompensationGain, defaults.GetRawVal(Param::CompensationGain));
			p.SetRawVal(Param::Threshold, defaults.GetRawVal(Param::Threshold));
			p.SetRawVal(Param::EvenHarmonics, defaults.GetRawVal(Param::EvenHarmonics));
			p.SetRawVal(Param::DryWet, defaults.GetRawVal(Param::DryWet));
		}
		OptimizeBoolParam(p, Param::Mute);
		OptimizeBoolParam(p, Param::Solo);
		OptimizeEnumParam<Maj7Sat::PanMode>(p, Param::PanMode);
		OptimizeEnumParam<Maj7Sat::Model>(p, Param::Model);
	}

	Maj7Sat* GetMaj7Sat() const {
		return (Maj7Sat*)getDevice();
	}
};
