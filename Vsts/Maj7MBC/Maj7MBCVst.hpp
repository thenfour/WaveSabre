#pragma once

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

extern WaveSabreVstLib::VstEditor* createEditor(AudioEffect* audioEffect);

class Maj7MBCVst : public VstPlug
{
public:
	Maj7MBCVst(audioMasterCallback audioMaster)
		: VstPlug(audioMaster, (int)Maj7MBC::ParamIndices::NumParams, 2, 2, 'M7mb', new Maj7MBC(), false)
	{
		setEditor(createEditor(this));
	}

	virtual void getParameterName(VstInt32 index, char* text) override
	{
		MAJ7MBC_PARAM_VST_NAMES(paramNames);
		vst_strncpy(text, paramNames[index], kVstMaxParamStrLen);
	}

	virtual bool getEffectName(char* name) override {
		vst_strncpy(name, "WaveSabre - Maj7 MBC Compressor", kVstMaxEffectNameLen);
		return true;
	}

	virtual bool getProductString(char* text) override {
		vst_strncpy(text, "WaveSabre - Maj7 MBC Compressor", kVstMaxProductStrLen);
		return true;
	}

	virtual const char* GetJSONTagName() { return "Maj7MBC"; }

	Maj7MBC* GetMaj7MBC() const {
		return (Maj7MBC*)getDevice();
	}

	virtual VstInt32 setChunk(void* data, VstInt32 byteSize, bool isPreset) override
	{
		auto p = GetMaj7MBC();
		// import all device params.
		MAJ7MBC_PARAM_VST_NAMES(paramNames);
		return SetSimpleJSONVstChunk(GetMaj7MBC(), GetJSONTagName(), data, byteSize, GetMaj7MBC()->mParamCache, paramNames, [&](clarinoid::JsonVariantReader& elem) {
			if (elem.mKeyName == "BandASolo") {
				p->mBands[0].mVSTConfig.mSolo = elem.mBooleanValue;
			}
			else if (elem.mKeyName == "BandAMute") {
				p->mBands[0].mVSTConfig.mMute = elem.mBooleanValue;
			}
			else if (elem.mKeyName == "BandAOutputStream") {
				p->mBands[0].mVSTConfig.mOutputStream = ToEnum(elem, Maj7MBC::OutputStream::Count__);
			}
			else if (elem.mKeyName == "BandADisplayMode") {
				p->mBands[0].mVSTConfig.mDisplayStyle = ToEnum(elem, Maj7MBC::DisplayStyle::Count__);
			}

			else if (elem.mKeyName == "BandBSolo") {
				p->mBands[1].mVSTConfig.mSolo = elem.mBooleanValue;
			}
			else if (elem.mKeyName == "BandBMute") {
				p->mBands[1].mVSTConfig.mMute = elem.mBooleanValue;
			}
			else if (elem.mKeyName == "BandBOutputStream") {
				p->mBands[1].mVSTConfig.mOutputStream = ToEnum(elem, Maj7MBC::OutputStream::Count__);
			}
			else if (elem.mKeyName == "BandBDisplayMode") {
				p->mBands[1].mVSTConfig.mDisplayStyle = ToEnum(elem, Maj7MBC::DisplayStyle::Count__);
			}

			else if (elem.mKeyName == "BandCSolo") {
				p->mBands[2].mVSTConfig.mSolo = elem.mBooleanValue;
			}
			else if (elem.mKeyName == "BandCMute") {
				p->mBands[2].mVSTConfig.mMute = elem.mBooleanValue;
			}
			else if (elem.mKeyName == "BandCOutputStream") {
				p->mBands[2].mVSTConfig.mOutputStream = ToEnum(elem, Maj7MBC::OutputStream::Count__);
			}
			else if (elem.mKeyName == "BandCDisplayMode") {
				p->mBands[2].mVSTConfig.mDisplayStyle = ToEnum(elem, Maj7MBC::DisplayStyle::Count__);
			}
			});
	}

	virtual VstInt32 getChunk(void** data, bool isPreset) override
	{
		auto p = GetMaj7MBC();
		MAJ7MBC_PARAM_VST_NAMES(paramNames);
		return GetSimpleJSONVstChunk(GetJSONTagName(), data, GetMaj7MBC()->mParamCache, paramNames, [&](clarinoid::JsonVariantWriter& elem)
			{
				elem.Object_MakeKey("BandASolo").WriteBoolean(p->mBands[0].mVSTConfig.mSolo);
				elem.Object_MakeKey("BandAMute").WriteBoolean(p->mBands[0].mVSTConfig.mMute);
				elem.Object_MakeKey("BandAOutputStream").WriteNumberValue(int(p->mBands[0].mVSTConfig.mOutputStream));
				elem.Object_MakeKey("BandADisplayMode").WriteNumberValue(int(p->mBands[0].mVSTConfig.mDisplayStyle));

				elem.Object_MakeKey("BandBSolo").WriteBoolean(p->mBands[1].mVSTConfig.mSolo);
				elem.Object_MakeKey("BandBMute").WriteBoolean(p->mBands[1].mVSTConfig.mMute);
				elem.Object_MakeKey("BandBOutputStream").WriteNumberValue(int(p->mBands[1].mVSTConfig.mOutputStream));
				elem.Object_MakeKey("BandBDisplayMode").WriteNumberValue(int(p->mBands[1].mVSTConfig.mDisplayStyle));

				elem.Object_MakeKey("BandCSolo").WriteBoolean(p->mBands[2].mVSTConfig.mSolo);
				elem.Object_MakeKey("BandCMute").WriteBoolean(p->mBands[2].mVSTConfig.mMute);
				elem.Object_MakeKey("BandCOutputStream").WriteNumberValue(int(p->mBands[2].mVSTConfig.mOutputStream));
				elem.Object_MakeKey("BandCDisplayMode").WriteNumberValue(int(p->mBands[2].mVSTConfig.mDisplayStyle));
			}
		);
	}


	virtual void OptimizeParams() override
	{
		using Params = Maj7MBC::ParamIndices;
		M7::ParamAccessor defaults{ mDefaultParamCache.data(), 0 };
		M7::ParamAccessor p{ GetMaj7MBC()->mParamCache, 0 };
		OptimizeBand(Params::AInputGain);
		OptimizeBand(Params::BInputGain);
		OptimizeBand(Params::CInputGain);
		if (!p.GetBoolValue(Params::SoftClipEnable)) {
			p.SetRawVal(Params::SoftClipOutput, defaults.GetRawVal(Params::SoftClipOutput));
			p.SetRawVal(Params::SoftClipThresh, defaults.GetRawVal(Params::SoftClipThresh));
		}
	}

	void OptimizeBand(Maj7MBC::ParamIndices baseParam) {
		M7::ParamAccessor defaults{ mDefaultParamCache.data(), baseParam };
		M7::ParamAccessor p{ GetMaj7MBC()->mParamCache, baseParam };
		using Param = Maj7MBC::FreqBand::BandParam;
		OptimizeBoolParam(p, Param::Enable);
		if (!p.GetBoolValue(Param::Enable)) {
			// effect not enabled; set defaults to params.
			p.SetRawVal(Param::InputGain, defaults.GetRawVal(Param::InputGain));
			p.SetRawVal(Param::OutputGain, defaults.GetRawVal(Param::OutputGain));
			p.SetRawVal(Param::Threshold, defaults.GetRawVal(Param::Threshold));
			p.SetRawVal(Param::Attack, defaults.GetRawVal(Param::Attack));
			p.SetRawVal(Param::Release, defaults.GetRawVal(Param::Release));
			p.SetRawVal(Param::Ratio, defaults.GetRawVal(Param::Ratio));
			p.SetRawVal(Param::Knee, defaults.GetRawVal(Param::Knee));
			p.SetRawVal(Param::ChannelLink, defaults.GetRawVal(Param::ChannelLink));
			p.SetRawVal(Param::HighPassFrequency, defaults.GetRawVal(Param::HighPassFrequency));
			p.SetRawVal(Param::HighPassQ, defaults.GetRawVal(Param::HighPassQ));
		}
	}
};
