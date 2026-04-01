#pragma once

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;


class Maj7WidthVst : public VstPlug
{
public:
	Maj7WidthVst(audioMasterCallback audioMaster);

	virtual void getParameterName(VstInt32 index, char* text);

	virtual bool getEffectName(char* name);
	virtual bool getProductString(char* text);

	Maj7Width* GetMaj7Width() const;

	virtual const char* GetJSONTagName() { return "Maj7Width"; }

	virtual VstInt32 getChunk(void** data, bool isPreset) override
	{
		MAJ7WIDTH_PARAM_VST_NAMES(paramNames);
		return GetSimpleJSONVstChunk(GetJSONTagName(), data, GetMaj7Width()->mParamCache, paramNames, [&](clarinoid::JsonVariantWriter& elem) {
			auto& editor = *static_cast<WaveSabreVstLib::VstEditor*>(getEditor());
			auto map = editor.GetVstOnlyParams();
			for (auto& p : map) {
				p->Serialize(elem);
			}
		});
	}

	virtual VstInt32 setChunk(void* data, VstInt32 byteSize, bool isPreset) override
	{
		MAJ7WIDTH_PARAM_VST_NAMES(paramNames);
		return SetSimpleJSONVstChunk(GetMaj7Width(), GetJSONTagName(), data, byteSize, GetMaj7Width()->mParamCache, paramNames, [&](clarinoid::JsonVariantReader& elem) {
			auto& editor = *static_cast<WaveSabreVstLib::VstEditor*>(getEditor());
			auto map = editor.GetVstOnlyParams();
			for (auto& p : map) {
				p->TryDeserialize(elem);
			}
		});
	}

	virtual void GenerateDefaults() override
	{
		auto& p = GetMaj7Width()->mParams;
		using Params = Maj7Width::ParamIndices;

		p.SetN11Value(Params::LeftSource, -1.0f);
		p.SetN11Value(Params::RightSource, 1.0f);
		p.SetBoolValue(Params::LInvert, false);
		p.SetBoolValue(Params::RInvert, false);
		p.SetRangedValue(Params::RotationAngle, -M7::math::gPIQuarter, M7::math::gPIQuarter, 0.0f);
		//p.SetFrequencyAssumingNoKeytracking(Params::SideHPFrequency, M7::gFilterFreqConfig, 20.0f);
		p.SetRawVal(Params::SideHPFrequency, 0);
		p.SetN11Value(Params::MidSideBalance, 0.0f);
		p.SetN11Value(Params::Pan, 0.0f);
		p.SetDecibels(Params::OutputGain, Maj7Width::gVolumeCfg, 0.0f);
	}
};
