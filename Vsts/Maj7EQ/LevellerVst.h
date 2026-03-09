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

	virtual void GenerateDefaults() override
	{
		auto* pDevice = (Leveller*)getDevice();
		auto& p = pDevice->mParams;
		using Params = Leveller::ParamIndices;
		using Band = Leveller::BandParamOffsets;

		p.SetDecibels(Params::OutputVolume, M7::gVolumeCfg12db, 0.0f);
		p.SetBoolValue(Params::EnableDCFilter, false);

		auto setBand = [&](Params base,
		                   bool enabled,
		                   M7::FilterResponse response,
		                   float freqHz,
		                   float gainDb,
		                   float q01)
		{
			M7::ParamAccessor b{pDevice->mParamCache, base};
			b.SetEnumValue(Band::Circuit, M7::FilterCircuit::Biquad);
			b.SetEnumValue(Band::Slope, M7::FilterSlope::Slope24dbOct);
			b.SetEnumValue(Band::Response, response);
			b.SetFrequencyAssumingNoKeytracking(Band::Freq, M7::gFilterFreqConfig, freqHz);
			b.SetRangedValue(Band::Gain, M7::gEqBandGainMin, M7::gEqBandGainMax, gainDb);
			b.Set01Val(Band::Q, q01);
			b.SetBoolValue(Band::Enable, enabled);
		};

		setBand(Params::Band1Circuit, true, M7::FilterResponse::Highpass, 90.0f, 0.0f, 0.335f);
		setBand(Params::Band2Circuit, false, M7::FilterResponse::Peak, 250.0f, 0.0f, 0.5f);
		setBand(Params::Band3Circuit, true, M7::FilterResponse::Peak, 1100.0f, 0.0f, 0.5f);
		setBand(Params::Band4Circuit, false, M7::FilterResponse::Peak, 3000.0f, 0.0f, 0.5f);
		setBand(Params::Band5Circuit, false, M7::FilterResponse::Lowpass, 8500.0f, 0.0f, 0.335f);
	}
};

#endif