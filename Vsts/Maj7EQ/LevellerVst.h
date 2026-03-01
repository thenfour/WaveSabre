#ifndef __LEVELLERVST_H__
#define __LEVELLERVST_H__

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

class LevellerVst : public VstPlug
{
	using Leveller = WaveSabreCore::Leveller;
public:
	LevellerVst(audioMasterCallback audioMaster);

	virtual void getParameterName(VstInt32 index, char *text);

	virtual bool getEffectName(char *name);
	virtual bool getProductString(char *text);

	virtual VstInt32 getChunk(void** data, bool isPreset) override
	{
		LEVELLER_PARAM_VST_NAMES(paramNames);
		auto* p = (Leveller*)getDevice();
		return GetSimpleJSONVstChunk("WSLeveller", data, p->mParamCache, paramNames, [](clarinoid::JsonVariantWriter&) {});
	}

	virtual VstInt32 setChunk(void* data, VstInt32 byteSize, bool isPreset) override
	{
		LEVELLER_PARAM_VST_NAMES(paramNames);
		auto* p = (Leveller*)getDevice();
		return SetSimpleJSONVstChunk(p, "WSLeveller", data, byteSize, p->mParamCache, paramNames, [](clarinoid::JsonVariantReader&) {});
	}

	virtual const char* GetJSONTagName() { return "Leveller"; }


	virtual void OptimizeParams() override
	{
		using Params = Leveller::ParamIndices;
		OptimizeBand(Params::Band1Circuit);
		OptimizeBand(Params::Band2Circuit);
		OptimizeBand(Params::Band3Circuit);
		OptimizeBand(Params::Band4Circuit);
		OptimizeBand(Params::Band5Circuit);
	}

	void OptimizeBand(Leveller::ParamIndices baseParam) {
		M7::ParamAccessor defaults{ mDefaultParamCache.data(), baseParam };
		M7::ParamAccessor p{ ((Leveller*)getDevice())->mParamCache, baseParam };
		using Param = Leveller::BandParamOffsets;
		OptimizeBoolParam(p, Param::Enable);
		OptimizeEnumParam<M7::FilterCircuit>(p, Param::Circuit);
		OptimizeEnumParam<M7::FilterSlope>(p, Param::Slope);
		OptimizeEnumParam<M7::FilterResponse>(p, Param::Response);

		if (!p.GetBoolValue(Param::Enable)) {
			// effect not enabled; set defaults to params.
			p.SetRawVal(Param::Circuit, defaults.GetRawVal(Param::Circuit));
			p.SetRawVal(Param::Slope, defaults.GetRawVal(Param::Slope));
			p.SetRawVal(Param::Response, defaults.GetRawVal(Param::Response));
			p.SetRawVal(Param::Freq, defaults.GetRawVal(Param::Freq));
			p.SetRawVal(Param::Gain, defaults.GetRawVal(Param::Gain));
			p.SetRawVal(Param::Q, defaults.GetRawVal(Param::Q));
		}
	}
};

#endif