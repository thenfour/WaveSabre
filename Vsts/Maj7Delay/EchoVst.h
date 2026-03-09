#ifndef __ECHOVST_H__
#define __ECHOVST_H__

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

class EchoVst : public VstPlug
{
public:
	EchoVst(audioMasterCallback audioMaster);

	virtual void getParameterName(VstInt32 index, char *text);

	virtual bool getEffectName(char *name);
	virtual bool getProductString(char *text);

	virtual VstInt32 getChunk(void** data, bool isPreset) override
	{
		ECHO_PARAM_VST_NAMES(paramNames);
		auto* p = (WaveSabreCore::Echo*)getDevice();
		return GetSimpleJSONVstChunk(GetJSONTagName(), data, p->mParamCache, paramNames, [](clarinoid::JsonVariantWriter&) {});
	}

	virtual VstInt32 setChunk(void* data, VstInt32 byteSize, bool isPreset) override
	{
		ECHO_PARAM_VST_NAMES(paramNames);
		auto* p = (WaveSabreCore::Echo*)getDevice();
		return SetSimpleJSONVstChunk(p, GetJSONTagName(), data, byteSize, p->mParamCache, paramNames, [](clarinoid::JsonVariantReader&) {});
	}

	virtual void GenerateDefaults() override
	{
		auto& p = ((WaveSabreCore::Echo*)getDevice())->mParams;
		using Params = WaveSabreCore::Echo::ParamIndices;

		p.SetIntValue(Params::LeftDelayCoarse, 8);
		p.SetN11Value(Params::LeftDelayFine, 0.0f);
		p.SetBipolarPowCurvedValue(Params::LeftDelayMS, WaveSabreCore::M7::gEnvTimeCfg, 0.0f);

		p.SetIntValue(Params::RightDelayCoarse, 6);
		p.SetN11Value(Params::RightDelayFine, 0.0f);
		p.SetBipolarPowCurvedValue(Params::RightDelayMS, WaveSabreCore::M7::gEnvTimeCfg, 0.0f);

		p.SetFrequencyAssumingNoKeytracking(Params::LowCutFreq, WaveSabreCore::M7::gFilterFreqConfig, 50.0f);
		p.SetDivCurvedValue(Params::LowCutQ, WaveSabreCore::M7::gBiquadFilterQCfg, 0.75f);
		p.SetFrequencyAssumingNoKeytracking(Params::HighCutFreq, WaveSabreCore::M7::gFilterFreqConfig, 8500.0f);
		p.SetDivCurvedValue(Params::HighCutQ, WaveSabreCore::M7::gBiquadFilterQCfg, 0.75f);

		p.SetDecibels(Params::FeedbackLevel, WaveSabreCore::M7::gVolumeCfg6db, -15.0f);
		p.SetDecibels(Params::FeedbackDriveDB, WaveSabreCore::M7::gVolumeCfg36db, 3.0f);
		p.Set01Val(Params::Cross, 0.25f);

		p.SetDecibels(Params::DryOutput, WaveSabreCore::M7::gVolumeCfg12db, 0.0f);
		p.SetDecibels(Params::WetOutput, WaveSabreCore::M7::gVolumeCfg12db, -12.0f);
	}
	virtual const char* GetJSONTagName() { return "Echo"; }
};

#endif