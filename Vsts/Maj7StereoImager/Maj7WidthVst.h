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
		p.SetRangedValue(Params::RotationAngle, -M7::math::gPIHalf, M7::math::gPIHalf, 0.0f);
		p.SetRangedValue(Params::MSShear, -Maj7Width::gShearAngleLimit, Maj7Width::gShearAngleLimit, 0.0f);
		p.SetRawVal(Params::SideHPFrequency, 0);
		p.SetN11Value(Params::MidSideBalance, 0.0f);
		p.SetN11Value(Params::Pan, 0.0f);
		p.SetDecibels(Params::OutputGain, Maj7Width::gVolumeCfg, 0.0f);
		p.SetFrequencyAssumingNoKeytracking(Params::CrossoverAFrequency, M7::gFilterFreqConfig, 650.0f);
		p.SetFrequencyAssumingNoKeytracking(Params::CrossoverBFrequency, M7::gFilterFreqConfig, 3500.0f);
		p.SetEnumValue(Params::CrossoverASlope, M7::CrossoverSlope::Slope_24dB);
		p.SetEnumValue(Params::CrossoverBSlope, M7::CrossoverSlope::Slope_24dB);
		p.SetDecibels(Params::Band1Gain, Maj7Width::gVolumeCfg, 0.0f);
		p.SetDecibels(Params::Band2Gain, Maj7Width::gVolumeCfg, 0.0f);
		p.SetDecibels(Params::Band3Gain, Maj7Width::gVolumeCfg, 0.0f);
		p.SetRangedValue(Params::Band1Rotation, -M7::math::gPIHalf, M7::math::gPIHalf, 0.0f);
		p.SetRangedValue(Params::Band2Rotation, -M7::math::gPIHalf, M7::math::gPIHalf, 0.0f);
		p.SetRangedValue(Params::Band3Rotation, -M7::math::gPIHalf, M7::math::gPIHalf, 0.0f);
		p.SetRangedValue(Params::Band1Shear, -Maj7Width::gShearAngleLimit, Maj7Width::gShearAngleLimit, 0.0f);
		p.SetRangedValue(Params::Band2Shear, -Maj7Width::gShearAngleLimit, Maj7Width::gShearAngleLimit, 0.0f);
		p.SetRangedValue(Params::Band3Shear, -Maj7Width::gShearAngleLimit, Maj7Width::gShearAngleLimit, 0.0f);
	}
};
