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
		return GetSimpleJSONVstChunk("WSLeveller", data, p->mParamCache, paramNames);
	}

	virtual VstInt32 setChunk(void* data, VstInt32 byteSize, bool isPreset) override
	{
		LEVELLER_PARAM_VST_NAMES(paramNames);
		auto* p = (Leveller*)getDevice();
		return SetSimpleJSONVstChunk(p, "WSLeveller", data, byteSize, p->mParamCache, paramNames);
	}

	virtual const char* GetJSONTagName() { return "Leveller"; }


	virtual void OptimizeParams() override
	{
		using Params = Leveller::ParamIndices;
		OptimizeBand(Params::Band1Type);
		OptimizeBand(Params::Band2Type);
		OptimizeBand(Params::Band3Type);
		OptimizeBand(Params::Band4Type);
		OptimizeBand(Params::Band5Type);
	}

	void OptimizeBand(Leveller::ParamIndices baseParam) {
		M7::ParamAccessor defaults{ mDefaultParamCache.data(), baseParam };
		M7::ParamAccessor p{ ((Leveller*)getDevice())->mParamCache, baseParam };
		using Param = Leveller::BandParamOffsets;
		OptimizeBoolParam(p, Param::Enable);
		OptimizeEnumParam<BiquadFilterType>(p, Param::Type);

		if (!p.GetBoolValue(Param::Enable)) {
			// effect not enabled; set defaults to params.
			p.SetRawVal(Param::Type, defaults.GetRawVal(Param::Type));
			p.SetRawVal(Param::Freq, defaults.GetRawVal(Param::Freq));
			p.SetRawVal(Param::Gain, defaults.GetRawVal(Param::Gain));
			p.SetRawVal(Param::Q, defaults.GetRawVal(Param::Q));
		}
	}
};

#endif