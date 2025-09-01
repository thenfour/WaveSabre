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

enum class OscillatorWaveform : uint8_t
{
  Sine,

  PulseNaive,
  PulseBlep1,
  //PulseBlep2,
  //PulseBlep3,

  SawNaive,
  SawBlep1,
  //SawBlep2,
  //SawBlep3,

  TriNaive,
  TriBlep1,
  //TriBlep2,
  //TriBlep3,

  //Pulse,
  //PulseTristate,
  //SawClip,
  //SineClip,
  //SineHarmTrunc,
  //TriSquare,
  //TriTrunc,
  //VarTrapezoid,
  //WhiteNoiseSH,
  //SineRectified,
  //SinePhaseDist,
  //StaircaseSaw,
  //TriFold,
  //DoublePulse,
  Count,

  DefaultForAudio = SawNaive,
  DefaultForLFO = TriNaive,
};
// size-optimize using macro
// clang-format off
#define OSCILLATOR_WAVEFORM_CAPTIONS(symbolName)                                                                       \
  static constexpr char const* const symbolName[(int)::WaveSabreCore::M7::OscillatorWaveform::Count]                   \
  {                                                                                                                    \
  "Sine",\
  "PulseNaive",\
  "PulseBlep1",\
  "SawNaive",\
  "SawBlep1",\
  "TriNaive",\
  "TriBlep1",\
  }
// clang-format on

//"Pulse", "PulseTristate", "Saw", "Sine", "SineHarmTrunc", "TriSquare", "TriTrunc", "VarTrapezoid", "WhiteNoiseSH", \
    //    "SineRectified", "SinePhaseDist", "StaircaseSaw", "TriFold", "DoublePulse",                                    \

/////////////////////////////////////////////////////////////////////////////
enum class OscillatorIntention : uint8_t
{
  LFO,
  Audio,
};

//
///////////////////////////////////////////////////////////////////////////////
//struct IOscillatorWaveform
//{
//  float mShapeA = 0.5;  // wave shape
//  float mShapeB = 0.5;  // wave shape
//  float mPhaseOffset = 0;
//
//  float mDCOffset =
//      0;  // while we could try to bake our wave generator with correct DC, it's very inconvenient. This is just way easier.
//  float mScale =
//      1;  // same here; way easier to just scale in post than to try and get everything scaled correctly during generation.
//
//  float mFrequency = 0;
//  double mPhase = 0;           // phase cursor 0-1
//  double mPhaseIncrement = 0;  // dt
//
//  float mBlepBefore;
//  float mBlepAfter;
//
//  OscillatorIntention mIntention = OscillatorIntention::LFO;
//
//  //IOscillatorWaveform() = default;
//  //virtual ~IOscillatorWaveform() = default;
//
//  virtual float NaiveSample(float phase01) = 0;       // return amplitude at phase
//  virtual float NaiveSampleSlope(float phase01) = 0;  // return slope at phase
//  virtual void AfterSetParams() = 0;
//  // offers waveforms the opportunity to accumulate bleps along the advancement.
//  virtual void Visit(double newPhase, float samples, float samplesTillNextSample) {}
//
//  // override if you need to adjust things
//  virtual void SetParams(float freq,
//                         float phaseOffsetN11,
//                         float waveshapeA,
//                         float waveshapeB,
//                         double sampleRate /*required for VST to display a graphic*/,
//                         OscillatorIntention intention)
//  {
//    mIntention = intention;
//    mFrequency = freq;
//    mPhaseOffset = math::fract(phaseOffsetN11);
//    mShapeA = waveshapeA;
//    mShapeB = waveshapeB;
//
//    double newPhaseInc = (double)mFrequency / sampleRate;
//
//    mPhaseIncrement = newPhaseInc;
//
//    AfterSetParams();
//  }
//
//  // process discontinuity due to restarting phase right now.
//  // updates blep before and blep after discontinuity.
//  void OSC_RESTART(float samplesBeforeNext)
//  {
//    float sampleBefore = this->NaiveSample((float)this->mPhase);
//    double newPhase = this->mPhaseOffset;
//
//    // fix bleps
//    float sampleAfter = NaiveSample((float)newPhase);
//    float blepScale = (sampleAfter - sampleBefore) * .5f;  // full sample scale is 2; *.5 to bring 0-1
//
//    mBlepBefore += blepScale * BlepBefore(samplesBeforeNext);  // blep the phase restart.
//    mBlepAfter += blepScale * BlepAfter(samplesBeforeNext);
//
//    // fix blamps.
//    float slopeBefore = NaiveSampleSlope((float)mPhase);
//    float slopeAfter = NaiveSampleSlope((float)newPhase);
//    float blampScale = float(this->mPhaseIncrement * (slopeAfter - slopeBefore));
//    mBlepBefore += blampScale * BlampBefore(samplesBeforeNext);
//    mBlepAfter += blampScale * BlampAfter(samplesBeforeNext);
//
//    mPhase = newPhase;
//
//    //return { blepBefore, blepAfter };
//  }
//
//  float samplesSinceEdge;
//  float samplesFromEdgeToNextSample;
//
//  //void WeirdPreBlepBlampCode(double newPhase,
//  //                           float edge,
//  //                           float blepScale,
//  //                           float samplesFromNewPositionUntilNextSample,
//  //                           decltype(BlepBefore) pProcBefore,
//  //                           decltype(BlepBefore) pProcAfter
//  //)
//  //{
//  //  if (!math::DoesEncounter(mPhase, newPhase, edge))
//  //    return;
//  //  samplesSinceEdge = float(math::fract(newPhase - edge) / this->mPhaseIncrement);
//  //  samplesFromEdgeToNextSample = math::fract(samplesSinceEdge + samplesFromNewPositionUntilNextSample);
//  //  mBlepBefore += blepScale * pProcBefore(samplesFromEdgeToNextSample);
//  //  mBlepAfter += blepScale * pProcAfter(samplesFromEdgeToNextSample);
//  //}
//
//  inline void OSC_ACCUMULATE_BLEP(double newPhase,
//                                  float edge,
//                                  float blepScale,
//                                  float samples,
//                                  float samplesFromNewPositionUntilNextSample)
//  {
//    //WeirdPreBlepBlampCode(newPhase, edge, blepScale, samplesFromNewPositionUntilNextSample, BlepBefore, BlepAfter);
//  }
//
//  inline void OSC_ACCUMULATE_BLAMP(double newPhase,
//                                   float edge,
//                                   float blampScale,
//                                   float samples,
//                                   float samplesFromNewPositionUntilNextSample)
//  {
//    //WeirdPreBlepBlampCode(newPhase, edge, blampScale, samplesFromNewPositionUntilNextSample, BlampBefore, BlampAfter);
//  }
//
//
//  // samples is 0<samples<1
//  // assume this.phase is currently 0<t<1
//  // this.phase may not be on a sample boundary.
//  // accumulates blep before and blep after discontinuity.
//  virtual void OSC_ADVANCE(float samples, float samplesTillNextSample)
//  {
//    //mPhaseIncrement += mDTDT * samples;
//    double phaseToAdvance = samples * mPhaseIncrement;
//    //double newPhase = math::fract(mPhase + phaseToAdvance); // advance slave; doing it here helps us calculate discontinuity.
//    double newPhase = mPhase + phaseToAdvance;
//    if (newPhase > 1)
//      newPhase -= 1;  // slightly faster t han fract()
//    //FloatPair bleps{ 0.0f,0.0f };
//    if (mIntention == OscillatorIntention::Audio)
//    {
//      Visit(newPhase, samples, samplesTillNextSample);
//    }
//    this->mPhase = newPhase;
//    //return bleps;
//  }
//};

}  // namespace M7


}  // namespace WaveSabreCore
