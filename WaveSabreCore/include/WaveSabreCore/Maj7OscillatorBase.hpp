#pragma once

#include <WaveSabreCore/Filters/FilterOnePole.hpp>
#include <WaveSabreCore/Maj7Basic.hpp>
#include <WaveSabreCore/Maj7Envelope.hpp>
#include <WaveSabreCore/Maj7ModMatrix.hpp>

namespace WaveSabreCore
{
namespace M7
{
static constexpr float gOscillatorHeadroomScalar =
    0.75f;  // scale oscillator outputs down to make room for blepping etc.

// sampler, oscillator, LFO @ device level
struct ISoundSourceDevice
{
  ParamAccessor mParams;
  bool mEnabledCache;

  const ModSource mAmpEnvModSourceID;
  const ModDestination mModDestBaseID;

  ModulationSpec* mAmpEnvModulation;

  virtual bool IsEnabled() const = 0;
  virtual bool MatchesKeyRange(int midiNote) const = 0;

  virtual ~ISoundSourceDevice() {}

  ISoundSourceDevice(float* paramCache,
                     ModulationSpec* ampEnvModulation,
                     ParamIndices baseParamID,
                     ModSource ampEnvModSourceID,
                     ModDestination modDestBaseID)
      : mParams(paramCache, baseParamID)
      , mAmpEnvModulation(ampEnvModulation)
      , mAmpEnvModSourceID(ampEnvModSourceID)
      , mModDestBaseID(modDestBaseID)
  {
  }

  void InitDevice()
  {
    mAmpEnvModulation->SetSourceAmp(mAmpEnvModSourceID,
                                    AddEnum(mModDestBaseID, SourceModParamIndexOffsets::HiddenVolume),
                                    &mEnabledCache);
  }

  virtual void BeginBlock()
  {
    mEnabledCache = this->IsEnabled();
  }
  virtual void EndBlock() = 0;

  struct Voice
  {
    ISoundSourceDevice* mpSrcDevice;
    ModMatrixNode* mpModMatrix;
    EnvelopeNode*
        mpAmpEnv;  // voices are responsible for knowing when they are playing or not; this is needed for that.

    Voice(ISoundSourceDevice* psrcDevice, ModMatrixNode* pModMatrix, EnvelopeNode* pAmpEnv)
        : mpSrcDevice(psrcDevice)
        , mpModMatrix(pModMatrix)
        , mpAmpEnv(pAmpEnv)
    {
    }

    virtual ~Voice() {}

    virtual void NoteOn(bool legato) = 0;
    virtual void NoteOff() = 0;
    virtual void BeginBlock() = 0;

    void SetModMatrix(ModMatrixNode* pModMatrix)
    {
      mpModMatrix = pModMatrix;
    }
  };
};

// KEEP SYNCHRONIZED:
// - enum class OscillatorWaveform
// - InstantiateWaveformCore(OscillatorWaveform
// - #define OSCILLATOR_WAVEFORM_CAPTIONS
enum class OscillatorWaveform : uint8_t
{
  SineDCClip,
  SineClipSqueeze,
  SineHarmDCClip,
  SineHarmClipSqueeze,

  ShapeCoreSawTri,
  ShapeCoreSawPulse2,
  ShapeCoreSawPulse3,  // 3-state pulse
  ShapeCoreSawPulse4,  // 4-state pulse
  ShapeCoreSawTriSquare,

  ShapeCoreTrap1,
  ShapeCoreTrap2,

  FoldedSine,
  FoldedTriangle,

  EvolvingGrainNoise,

  Noise_SaH_LP4,
  Noise_SaH_HP4,

  Noise_White_ProbDuty,
  Noise_White_ProbLP,
  Noise_White_ProbBP,
  Noise_White_DutyLP,
  Noise_White_DutyBP,

  Count,

  DefaultForAudio = SineDCClip,
  DefaultForLFO = SineDCClip,
};


// KEEP SYNCHRONIZED:
// - enum class OscillatorWaveform
// - InstantiateWaveformCore(OscillatorWaveform
// - #define OSCILLATOR_WAVEFORM_CAPTIONS

// size-optimize using macro
// clang-format off
#define OSCILLATOR_WAVEFORM_CAPTIONS(symbolName)                                                                       \
  static constexpr char const* const symbolName[(int)::WaveSabreCore::M7::OscillatorWaveform::Count]                   \
  {                                                                                                                    \
  "Sine DC Clip",\
  "Sine Clip Squeez",\
  "Sine Harm DC Clip",\
  "Sine Harm Clip Squeez",\
  "SS Saw-Tri",\
  "SS pulse 2 stage",\
  "SS pulse 3 stage",\
  "SS pulse 4 stage",\
  "SS Tri-Square",\
  "SS trap 1",\
  "SS trap 2",\
  "Folded Sine",\
  "Folded Tri",\
  "Noise Grain Evolve",\
  "Noise_SaH_LP4",\
  "Noise_SaH_HP4",\
  "Noise White Prob+Duty",\
  "Noise White Prob+LP",\
  "Noise White Prob+BP",\
  "Noise White Duty+LP",\
  "Noise White Duty+BP",\
  }
// clang-format on

/////////////////////////////////////////////////////////////////////////////
enum class OscillatorIntention : uint8_t
{
  LFO,
  Audio,
};

}  // namespace M7


}  // namespace WaveSabreCore
