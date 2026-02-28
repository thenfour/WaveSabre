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
// - #define OSCILLATOR_WAVEFORM_UI_STYLES

struct OscillatorWaveformUiStyle
{
  bool startNewRow;
  const char* label;
  const char* shapeALabel;
  const char* shapeBLabel;
  float defaultWaveshapeA;
  float defaultWaveshapeB;
  const char* foregroundColorHtml;
  const char* backgroundColorHtml;
};

#define SINE_COLOR_FG "#66ccff"
#define SINE_COLOR_BG "#0a1a24"
#define SHAPE_COLOR_FG "#ffaa33"
#define SHAPE_COLOR_BG "#261a0a"
#define FOLDED_COLOR_FG "#ff9966"
#define FOLDED_COLOR_BG "#2a140d"
#define NOISE_GRAIN_FG "#b388ff"
#define NOISE_GRAIN_BG "#1a1226"
#define NOISE_SAH_FG "#7d7dff"
#define NOISE_SAH_BG "#111633"
#define NOISE_WHITE_FG "#55dd88"
#define NOISE_WHITE_BG "#102417"

#define OSCILLATOR_WAVEFORM_UI_STYLES(symbolName)                                                                      \
  static constexpr ::WaveSabreCore::M7::OscillatorWaveformUiStyle symbolName[(int)::WaveSabreCore::M7::OscillatorWaveform::Count] \
  {                                                                                                                    \
  {true,  "Sine DC Clip",            "Clip",   "DC",         0.0f,    0.0f,    SINE_COLOR_FG,        SINE_COLOR_BG},\
  {false, "Sine Clip Squeez",        "Clip",   "Duty",       0.0f,    0.0f,    SINE_COLOR_FG,        SINE_COLOR_BG},\
  {false, "Sine Harm Clip",          "Clip",   "Harm",       0.0f,    0.50f,   SINE_COLOR_FG,        SINE_COLOR_BG},\
  {false, "Sine Harm Clip Squeez",   "Duty",   "Harm",       0.0f,    0.50f,   SINE_COLOR_FG,        SINE_COLOR_BG},\
  {true,  "SS Saw-Tri",              "Duty",   "Tri-Saw",    0.50f,   0.50f,   SHAPE_COLOR_FG,       SHAPE_COLOR_BG},\
  {false, "SS pulse 2 stage",        "Duty",   "--",         0.50f,   0.50f,   SHAPE_COLOR_FG,       SHAPE_COLOR_BG},\
  {false, "SS pulse 3 stage",        "Duty",   "Sym",        2.f/3.f, 0.50f,   SHAPE_COLOR_FG,       SHAPE_COLOR_BG},\
  {false, "SS pulse 4 stage",        "Duty",   "Sym",        0.50f,   0.50f,   SHAPE_COLOR_FG,       SHAPE_COLOR_BG},\
  {false, "SS Tri-Square",           "Duty",   "Tri-Square", 0.50f,   0.50f,   SHAPE_COLOR_FG,       SHAPE_COLOR_BG},\
  {true,  "Folded Sine",             "Fold",   "Bias",       0.30f,   0,       FOLDED_COLOR_FG,      FOLDED_COLOR_BG},\
  {false, "Folded Tri",              "Fold",   "Bias",       0.30f,   0,       FOLDED_COLOR_FG,      FOLDED_COLOR_BG},\
  {true,  "Noise Grain Evolve",      "Grain",  "Mutate",     0.35f,   0.15f,   NOISE_GRAIN_FG,       NOISE_GRAIN_BG},\
  {false, "Noise_SaH_LP4",           "LP Cut", "Jitter",     0.80f,   0.0f,    NOISE_SAH_FG,         NOISE_SAH_BG},\
  {false, "Noise_SaH_HP4",           "HP Cut", "Jitter",     0.20f,   0.0f,    NOISE_SAH_FG,         NOISE_SAH_BG},\
  {true,  "Noise White Prob+Duty",   "Prob",   "Duty",       0.35f,   0.25f,   NOISE_WHITE_FG,       NOISE_WHITE_BG},\
  {false, "Noise White Prob+LP",     "Prob",   "LP Q",       0.35f,   0.60f,   NOISE_WHITE_FG,       NOISE_WHITE_BG},\
  {false, "Noise White Prob+BP",     "Prob",   "BP Q",       0.35f,   0.35f,   NOISE_WHITE_FG,       NOISE_WHITE_BG},\
  {false, "Noise White Duty+LP",     "Duty",   "LP Q",       0.50f,   0.60f,   NOISE_WHITE_FG,       NOISE_WHITE_BG},\
  {false, "Noise White Duty+BP",     "Duty",   "BP Q",       0.50f,   0.35f,   NOISE_WHITE_FG,       NOISE_WHITE_BG},\
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
