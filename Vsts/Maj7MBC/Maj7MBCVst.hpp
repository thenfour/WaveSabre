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

  virtual bool getEffectName(char* name) override
  {
    vst_strncpy(name, "Maj7 - MBC Compressor", kVstMaxEffectNameLen);
    return true;
  }

  virtual bool getProductString(char* text) override
  {
    vst_strncpy(text, "Maj7 - MBC Compressor", kVstMaxProductStrLen);
    return true;
  }

  virtual const char* GetJSONTagName()
  {
    return "Maj7MBC";
  }

  Maj7MBC* GetMaj7MBC() const
  {
    return (Maj7MBC*)getDevice();
  }

  virtual VstInt32 setChunk(void* data, VstInt32 byteSize, bool isPreset) override
  {
    auto p = GetMaj7MBC();
    // import all device params.
    MAJ7MBC_PARAM_VST_NAMES(paramNames);
    return SetSimpleJSONVstChunk(
        GetMaj7MBC(),
        GetJSONTagName(),
        data,
        byteSize,
        GetMaj7MBC()->mParamCache,
        paramNames,
        [&](clarinoid::JsonVariantReader& elem)
        {
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
          if (elem.mKeyName == "BandASolo")
          {
            p->mBands[0].mVSTConfig.mSolo = elem.mBooleanValue;
          }
          else if (elem.mKeyName == "BandAMute")
          {
            p->mBands[0].mVSTConfig.mMute = elem.mBooleanValue;
          }
          else if (elem.mKeyName == "BandAOutputStream")
          {
            p->mBands[0].mVSTConfig.mOutputStream = ToEnum(elem, Maj7MBC::OutputStream::Count__);
          }

          else if (elem.mKeyName == "BandBSolo")
          {
            p->mBands[1].mVSTConfig.mSolo = elem.mBooleanValue;
          }
          else if (elem.mKeyName == "BandBMute")
          {
            p->mBands[1].mVSTConfig.mMute = elem.mBooleanValue;
          }
          else if (elem.mKeyName == "BandBOutputStream")
          {
            p->mBands[1].mVSTConfig.mOutputStream = ToEnum(elem, Maj7MBC::OutputStream::Count__);
          }

          else if (elem.mKeyName == "BandCSolo")
          {
            p->mBands[2].mVSTConfig.mSolo = elem.mBooleanValue;
          }
          else if (elem.mKeyName == "BandCMute")
          {
            p->mBands[2].mVSTConfig.mMute = elem.mBooleanValue;
          }
          else if (elem.mKeyName == "BandCOutputStream")
          {
            p->mBands[2].mVSTConfig.mOutputStream = ToEnum(elem, Maj7MBC::OutputStream::Count__);
          }
#endif
          auto& editor = *static_cast<WaveSabreVstLib::VstEditor*>(getEditor());
          auto map = editor.GetVstOnlyParams();
          for (auto& p : map)
          {
            p->TryDeserialize(elem);
          }
        });
  }

  virtual VstInt32 getChunk(void** data, bool isPreset) override
  {
    auto p = GetMaj7MBC();
    MAJ7MBC_PARAM_VST_NAMES(paramNames);
    return GetSimpleJSONVstChunk(
        GetJSONTagName(),
        data,
        GetMaj7MBC()->mParamCache,
        paramNames,
        [&](clarinoid::JsonVariantWriter& elem)
        {
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
          elem.Object_MakeKey("BandASolo").WriteBoolean(p->mBands[0].mVSTConfig.mSolo);
          elem.Object_MakeKey("BandAMute").WriteBoolean(p->mBands[0].mVSTConfig.mMute);
          elem.Object_MakeKey("BandAOutputStream").WriteNumberValue(int(p->mBands[0].mVSTConfig.mOutputStream));

          elem.Object_MakeKey("BandBSolo").WriteBoolean(p->mBands[1].mVSTConfig.mSolo);
          elem.Object_MakeKey("BandBMute").WriteBoolean(p->mBands[1].mVSTConfig.mMute);
          elem.Object_MakeKey("BandBOutputStream").WriteNumberValue(int(p->mBands[1].mVSTConfig.mOutputStream));

          elem.Object_MakeKey("BandCSolo").WriteBoolean(p->mBands[2].mVSTConfig.mSolo);
          elem.Object_MakeKey("BandCMute").WriteBoolean(p->mBands[2].mVSTConfig.mMute);
          elem.Object_MakeKey("BandCOutputStream").WriteNumberValue(int(p->mBands[2].mVSTConfig.mOutputStream));
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
          auto& editor = *static_cast<WaveSabreVstLib::VstEditor*>(getEditor());
          auto map = editor.GetVstOnlyParams();
          for (auto& p : map)
          {
            p->Serialize(elem);
          }
        });
  }


  virtual void OptimizeParams() override
  {
    using Params = Maj7MBC::ParamIndices;
    M7::ParamAccessor defaults{mDefaultParamCache.data(), 0};
    M7::ParamAccessor p{GetMaj7MBC()->mParamCache, 0};
    OptimizeBand(Params::AInputGain);
    OptimizeBand(Params::BInputGain);
    OptimizeBand(Params::CInputGain);
    if (!p.GetBoolValue(Params::SoftClipEnable))
    {
      p.SetRawVal(Params::SoftClipOutput, defaults.GetRawVal(Params::SoftClipOutput));
      p.SetRawVal(Params::SoftClipThresh, defaults.GetRawVal(Params::SoftClipThresh));
    }
  }

  void OptimizeBand(Maj7MBC::ParamIndices baseParam)
  {
    M7::ParamAccessor defaults{mDefaultParamCache.data(), baseParam};
    M7::ParamAccessor p{GetMaj7MBC()->mParamCache, baseParam};
    using Param = Maj7MBC::FreqBand::BandParam;
    OptimizeBoolParam(p, Param::Enable);
    OptimizeBoolParam(p, Param::SidechainFilterEnable);
    if (!p.GetBoolValue(Param::SidechainFilterEnable))
    {
      p.SetRawVal(Param::HighPassFrequency, defaults.GetRawVal(Param::HighPassFrequency));
      p.SetRawVal(Param::HighPassQ, defaults.GetRawVal(Param::HighPassQ));
      p.SetRawVal(Param::LowPassFrequency, defaults.GetRawVal(Param::LowPassFrequency));
      p.SetRawVal(Param::LowPassQ, defaults.GetRawVal(Param::LowPassQ));
    }
    if (!p.GetBoolValue(Param::Enable))
    {
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
      p.SetRawVal(Param::SidechainFilterEnable, defaults.GetRawVal(Param::SidechainFilterEnable));
      p.SetRawVal(Param::LowPassFrequency, defaults.GetRawVal(Param::LowPassFrequency));
      p.SetRawVal(Param::LowPassQ, defaults.GetRawVal(Param::LowPassQ));
      p.SetRawVal(Param::DryWet, defaults.GetRawVal(Param::DryWet));
      p.SetRawVal(Param::Drive, defaults.GetRawVal(Param::Drive));
      p.SetRawVal(Param::SaturationModel, defaults.GetRawVal(Param::SaturationModel));
      p.SetRawVal(Param::SaturationThreshold, defaults.GetRawVal(Param::SaturationThreshold));
      p.SetRawVal(Param::SaturationEvenHarmonics, defaults.GetRawVal(Param::SaturationEvenHarmonics));
      p.SetRawVal(Param::MidSideMix, defaults.GetRawVal(Param::MidSideMix));
      p.SetRawVal(Param::Pan, defaults.GetRawVal(Param::Pan));
    }
  }

  virtual void GenerateDefaults() override
  {
    auto* pDevice = GetMaj7MBC();
    auto& p = pDevice->mParams;
    using Params = Maj7MBC::ParamIndices;
    using BandParam = Maj7MBC::FreqBand::BandParam;

    p.SetDecibels(Params::InputGain, M7::gVolumeCfg24db, 0.0f);
    p.SetEnumValue(Params::ChannelMode, Maj7MBC::ChannelMode::Stereo);
    p.SetBoolValue(Params::MultibandEnable, false);
    p.SetFrequencyAssumingNoKeytracking(Params::CrossoverAFrequency, M7::gFilterFreqConfig, 550.0f);
    p.SetFrequencyAssumingNoKeytracking(Params::CrossoverBFrequency, M7::gFilterFreqConfig, 3000.0f);
    p.SetEnumValue(Params::CrossoverASlope, M7::CrossoverSlope::Slope_24dB);
    p.SetEnumValue(Params::CrossoverBSlope, M7::CrossoverSlope::Slope_24dB);
    p.SetDecibels(Params::OutputGain, M7::gVolumeCfg24db, 0.0f);

    p.SetBoolValue(Params::SoftClipEnable, true);
    p.SetDecibels(Params::SoftClipThresh, M7::gUnityVolumeCfg, -6.0f);
    p.SetDecibels(Params::SoftClipOutput, M7::gUnityVolumeCfg, -0.3f);

    auto setBand = [&](Params base, bool enabled)
    {
      M7::ParamAccessor b{pDevice->mParamCache, base};

      b.SetDecibels(BandParam::InputGain, M7::gVolumeCfg24db, 0.0f);
      b.SetDecibels(BandParam::OutputGain, M7::gVolumeCfg24db, 0.0f);
      b.SetRangedValue(BandParam::Threshold, -60.0f, 0.0f, -20.0f);
      b.SetPowCurvedValue(BandParam::Attack, MonoCompressor::gAttackCfg, 50.0f);
      b.SetPowCurvedValue(BandParam::Release, MonoCompressor::gReleaseCfg, 80.0f);
      b.SetDivCurvedValue(BandParam::Ratio, MonoCompressor::gRatioCfg, 4.0f);
      b.SetRangedValue(BandParam::Knee, 0.0f, 30.0f, 4.0f);
      b.Set01Val(BandParam::ChannelLink, 0.8f);
      b.SetBoolValue(BandParam::Enable, enabled);

      b.SetBoolValue(BandParam::SidechainFilterEnable, false);
      b.SetFrequencyAssumingNoKeytracking(BandParam::HighPassFrequency, M7::gFilterFreqConfig, 110.0f);
      b.SetDivCurvedValue(BandParam::HighPassQ, M7::gBiquadFilterQCfg, 1.0f);
      b.SetFrequencyAssumingNoKeytracking(BandParam::LowPassFrequency, M7::gFilterFreqConfig, 8000.0f);
      b.SetDivCurvedValue(BandParam::LowPassQ, M7::gBiquadFilterQCfg, 1.0f);

      b.SetRangedValue(BandParam::Drive, 0.0f, 30.0f, 0.0f);
      b.SetEnumValue(BandParam::SaturationModel, M7::Maj7SaturationBase::Model::DivClipHard);
      b.SetDecibels(BandParam::SaturationThreshold, M7::gUnityVolumeCfg, -8.0f);
      b.SetRangedValue(BandParam::SaturationEvenHarmonics, 0.0f, 2.0f, 0.0f);

      b.SetN11Value(BandParam::MidSideMix, 0.0f);
      b.SetN11Value(BandParam::Pan, 0.0f);
      b.Set01Val(BandParam::DryWet, 1.0f);
    };

    setBand(Params::AInputGain, false);
    setBand(Params::BInputGain, true);
    setBand(Params::CInputGain, false);
  }
};
