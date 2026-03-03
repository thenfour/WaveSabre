#pragma once

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

extern WaveSabreVstLib::VstEditor* createEditor(AudioEffect* audioEffect);

class Maj7ModulateVst : public VstPlug
{
public:
	Maj7ModulateVst(audioMasterCallback audioMaster)
		: VstPlug(audioMaster, (int)Maj7Modulate::ParamIndices::NumParams, 2, 2, 'M7md', new Maj7Modulate(), false)
	{
		setEditor(createEditor(this));
	}

	virtual void getParameterName(VstInt32 index, char* text) override
	{
		MAJ7MODULATE_PARAM_VST_NAMES(paramNames);
		vst_strncpy(text, paramNames[index], kVstMaxParamStrLen);
	}

	virtual bool getEffectName(char* name) override {
		vst_strncpy(name, "Maj7 - Modulate", kVstMaxEffectNameLen);
		return true;
	}

	virtual bool getProductString(char* text) override {
		vst_strncpy(text, "Maj7 - Modulate", kVstMaxProductStrLen);
		return true;
	}

	virtual const char* GetJSONTagName() { return "Maj7Modulate"; }

	Maj7Modulate* GetMaj7Modulate() const {
		return (Maj7Modulate*)getDevice();
	}

	virtual VstInt32 setChunk(void* data, VstInt32 byteSize, bool isPreset) override
	{
		auto p = GetMaj7Modulate();
		// import all device params.
		MAJ7MODULATE_PARAM_VST_NAMES(paramNames);
		return SetSimpleJSONVstChunk(GetMaj7Modulate(), GetJSONTagName(), data, byteSize, GetMaj7Modulate()->mParamCache, paramNames, [&](clarinoid::JsonVariantReader& elem) {
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
				// editor-only params.
			// if (elem.mKeyName == "BandASolo") {
			// 	p->mBands[0].mVSTConfig.mSolo = elem.mBooleanValue;
			// }
			// else if (elem.mKeyName == "BandAMute") {
			// 	p->mBands[0].mVSTConfig.mMute = elem.mBooleanValue;
			// }
#endif
			auto& editor = *static_cast<WaveSabreVstLib::VstEditor*>(getEditor());
			auto map = editor.GetVstOnlyParams();
			for (auto& p : map) {
				p->TryDeserialize(elem);
			}

			});
	}

	virtual VstInt32 getChunk(void** data, bool isPreset) override
	{
		auto p = GetMaj7Modulate();
		MAJ7MODULATE_PARAM_VST_NAMES(paramNames);
		return GetSimpleJSONVstChunk(GetJSONTagName(), data, GetMaj7Modulate()->mParamCache, paramNames, [&](clarinoid::JsonVariantWriter& elem)
			{
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
				// editor-only params.
				//elem.Object_MakeKey("BandASolo").WriteBoolean(p->mBands[0].mVSTConfig.mSolo);
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
				auto& editor = *static_cast<WaveSabreVstLib::VstEditor*>(getEditor());
				auto map = editor.GetVstOnlyParams();
				for (auto& p : map) {
					p->Serialize(elem);
				}
			}
		);
	}

	virtual void OptimizeParams() override
	{
		using Params = Maj7Modulate::ParamIndices;
		M7::ParamAccessor defaults{ mDefaultParamCache.data(), 0 };
		M7::ParamAccessor p{ GetMaj7Modulate()->mParamCache, 0 };
	}
};
