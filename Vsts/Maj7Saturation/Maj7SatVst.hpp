#pragma once

#include <WaveSabreVstLib.h>
using namespace WaveSabreVstLib;

#include <WaveSabreCore.h>
using namespace WaveSabreCore;

extern WaveSabreVstLib::VstEditor *createEditor(AudioEffect *audioEffect);

class Maj7SatVst : public VstPlug {
public:
  Maj7SatVst(audioMasterCallback audioMaster)
      : VstPlug(audioMaster, (int)Maj7Sat::ParamIndices::NumParams, 2, 2,
                'M7st', new Maj7Sat(), false) {
    if (audioMaster)
      setEditor(createEditor(this));
  }

  virtual void getParameterName(VstInt32 index, char *text) override {
    MAJ7SAT_PARAM_VST_NAMES(paramNames);
    vst_strncpy(text, paramNames[index], kVstMaxParamStrLen);
  }

  virtual bool getEffectName(char *name) override {
    vst_strncpy(name, "Maj7 - Saturation", kVstMaxEffectNameLen);
    return true;
  }

  virtual bool getProductString(char *text) override {
    vst_strncpy(text, "Maj7 - Saturation", kVstMaxProductStrLen);
    return true;
  }

  virtual VstInt32 getChunk(void **data, bool isPreset) override {
    auto p = GetMaj7Sat();
    MAJ7SAT_PARAM_VST_NAMES(paramNames);
    return GetSimpleJSONVstChunk(
        GetJSONTagName(), data, GetMaj7Sat()->mParamCache, paramNames,
        [&](clarinoid::JsonVariantWriter &elem) {
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
          elem.Object_MakeKey("BandASolo")
              .WriteBoolean(p->mBands[0].mVSTConfig.mSolo);
          elem.Object_MakeKey("BandAMute")
              .WriteBoolean(p->mBands[0].mVSTConfig.mMute);
          elem.Object_MakeKey("BandBSolo")
              .WriteBoolean(p->mBands[1].mVSTConfig.mSolo);
          elem.Object_MakeKey("BandBMute")
              .WriteBoolean(p->mBands[1].mVSTConfig.mMute);
          elem.Object_MakeKey("BandCSolo")
              .WriteBoolean(p->mBands[2].mVSTConfig.mSolo);
          elem.Object_MakeKey("BandCMute")
              .WriteBoolean(p->mBands[2].mVSTConfig.mMute);
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
        });
  }

  virtual VstInt32 setChunk(void *data, VstInt32 byteSize,
                            bool isPreset) override {
    auto p = GetMaj7Sat();
    MAJ7SAT_PARAM_VST_NAMES(paramNames);
    return SetSimpleJSONVstChunk(
        GetMaj7Sat(), GetJSONTagName(), data, byteSize,
        GetMaj7Sat()->mParamCache, paramNames,
        [&](clarinoid::JsonVariantReader &elem) {
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
          if (elem.mKeyName == "BandASolo") {
            p->mBands[0].mVSTConfig.mSolo = elem.mBooleanValue;
          } else if (elem.mKeyName == "BandAMute") {
            p->mBands[0].mVSTConfig.mMute = elem.mBooleanValue;
          }
          else if (elem.mKeyName == "BandBSolo") {
            p->mBands[1].mVSTConfig.mSolo = elem.mBooleanValue;
          } else if (elem.mKeyName == "BandBMute") {
            p->mBands[1].mVSTConfig.mMute = elem.mBooleanValue;
          }
          else if (elem.mKeyName == "BandCSolo") {
            p->mBands[2].mVSTConfig.mSolo = elem.mBooleanValue;
          }
          else if (elem.mKeyName == "BandCMute") {
            p->mBands[2].mVSTConfig.mMute = elem.mBooleanValue;
          }
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT

        });
  }

  virtual const char *GetJSONTagName() { return "Maj7Sat"; }

  virtual void OptimizeParams() override {
    using Params = Maj7Sat::ParamIndices;
    M7::ParamAccessor p{GetMaj7Sat()->mParamCache, 0};
    // OptimizeEnumParam<M7::LinkwitzRileyFilter::Slope>(p,
    // Params::CrossoverASlope); p.SetRawVal(Params::CrossoverASlope, 0);
    OptimizeBand(Params::AModel);
    OptimizeBand(Params::BModel);
    OptimizeBand(Params::CModel);
  }

  virtual void GenerateDefaults() override {
    auto *pDevice = GetMaj7Sat();
    auto &p = pDevice->mParams;
    using Params = Maj7Sat::ParamIndices;
    using BandParam = Maj7Sat::FreqBand::BandParam;

    p.SetDecibels(Params::InputGain, M7::gVolumeCfg24db, 0.0f);
    p.SetFrequencyAssumingNoKeytracking(Params::CrossoverAFrequency, M7::gFilterFreqConfig, 550.0f);
    p.SetFrequencyAssumingNoKeytracking(Params::CrossoverBFrequency, M7::gFilterFreqConfig, 3000.0f);
    p.Set01Val(Params::OverallDryWet, 1.0f);
    p.SetDecibels(Params::OutputGain, M7::gVolumeCfg24db, 0.0f);

    auto setBand = [&](Params base)
    {
      M7::ParamAccessor b{pDevice->mParamCache, base};
      b.SetEnumValue(BandParam::Model, Maj7Sat::Model::TanhClip);
      b.SetDecibels(BandParam::Drive, M7::gVolumeCfg36db, 0.0f);
      b.SetDecibels(BandParam::CompensationGain, M7::gVolumeCfg12db, 0.0f);
      b.SetDecibels(BandParam::OutputGain, M7::gVolumeCfg12db, 0.0f);
      b.SetDecibels(BandParam::Threshold, M7::gUnityVolumeCfg, -8.0f);
#ifdef MAJ7SAT_ENABLE_ANALOG
      b.SetRangedValue(BandParam::EvenHarmonics, 0.0f, Maj7Sat::gAnalogMaxLin, 0.12f);
#else
      b.SetRangedValue(BandParam::EvenHarmonics, 0.0f, Maj7Sat::gAnalogMaxLin, 0.0f);
#endif
      b.Set01Val(BandParam::DryWet, 1.0f);
      b.SetBoolValue(BandParam::EnableEffect, false);
    };

    setBand(Params::AModel);
    setBand(Params::BModel);
    setBand(Params::CModel);
  }

  void OptimizeBand(Maj7Sat::ParamIndices baseParam) {
    M7::ParamAccessor defaults{mDefaultParamCache.data(), baseParam};
    M7::ParamAccessor p{GetMaj7Sat()->mParamCache, baseParam};
    using Param = Maj7Sat::FreqBand::BandParam;
    OptimizeBoolParam(p, Param::EnableEffect);
    if (!p.GetBoolValue(Param::EnableEffect)) {
      // effect not enabled; set defaults to params.
      // p.SetRawVal(Param::PanMode, defaults.GetRawVal(Param::PanMode));
      // p.SetRawVal(Param::Pan, defaults.GetRawVal(Param::Pan));
      p.SetRawVal(Param::Model, defaults.GetRawVal(Param::Model));
      p.SetRawVal(Param::Drive, defaults.GetRawVal(Param::Drive));
      p.SetRawVal(Param::CompensationGain,
                  defaults.GetRawVal(Param::CompensationGain));
      p.SetRawVal(Param::Threshold, defaults.GetRawVal(Param::Threshold));
      // p.SetRawVal(Param::EvenHarmonics,
      // defaults.GetRawVal(Param::EvenHarmonics));
      p.SetRawVal(Param::DryWet, defaults.GetRawVal(Param::DryWet));
    }
    // OptimizeBoolParam(p, Param::Mute);
    // OptimizeBoolParam(p, Param::Solo);
    // OptimizeEnumParam<Maj7Sat::PanMode>(p, Param::PanMode);
    OptimizeEnumParam<Maj7Sat::Model>(p, Param::Model);
  }

  Maj7Sat *GetMaj7Sat() const { return (Maj7Sat *)getDevice(); }
};
