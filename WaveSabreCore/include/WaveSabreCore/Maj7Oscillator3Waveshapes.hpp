#pragma once

#include <algorithm>
#include <cfloat>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

#include "Maj7Basic.hpp"
#include "Maj7Oscillator3Base.hpp"
#include "Maj7Oscillator3Shape.hpp"

namespace WaveSabreCore
{
namespace M7
{

static inline WVShape MakeSawShape()
{
  return WVShape{.mSegments = {
                     WVSegment{.beginPhase01 = 0.0, .endPhaseIncluding1 = 1, .beginAmp = -1.0f, .slope = +2.0f},
                 }};
}
static inline WVShape MakeTriangleShape()
{
  return WVShape{.mSegments = {
                     WVSegment{.beginPhase01 = 0.0, .endPhaseIncluding1 = 0.5, .beginAmp = -1.0f, .slope = +4.0f},
                     WVSegment{.beginPhase01 = 0.5, .endPhaseIncluding1 = 1.0, .beginAmp = +1.0f, .slope = -4.0f},
                 }};
}
static inline WVShape MakePulseShape(double dutyCycle01)
{
  dutyCycle01 = std::clamp(dutyCycle01, 0.001, 0.999);
  return WVShape{.mSegments = {
                     WVSegment{.beginPhase01 = 0.0, .endPhaseIncluding1 = dutyCycle01, .beginAmp = -1.0f, .slope = 0},
                     WVSegment{.beginPhase01 = dutyCycle01, .endPhaseIncluding1 = 1.0, .beginAmp = 1.0f, .slope = 0},
                 }};
}

// three state pulse: low, high, 0
// segment 0: low
// segment 1: high
// segment 2: 0
static inline WVShape MakeTriStatePulseShape3(double masterDutyCycle01, double subDuty01)
{
  masterDutyCycle01 = std::clamp(masterDutyCycle01, 0.002, 0.998);  // we need slightly more space for the transitions

  // scale low duty to the master duty cycle
  auto lowDutyPhase = std::clamp(subDuty01 * masterDutyCycle01, 0.0, masterDutyCycle01 - 0.001);
  auto highDutyPhase = masterDutyCycle01 - lowDutyPhase;

  return WVShape{.mSegments = {
      WVSegment{.beginPhase01 = 0.0, .endPhaseIncluding1 = lowDutyPhase, .beginAmp = -1.0f, .slope = 0},
                     WVSegment{.beginPhase01 = lowDutyPhase, .endPhaseIncluding1 = masterDutyCycle01, .beginAmp = 1.0f, .slope = 0},
          WVSegment{.beginPhase01 = masterDutyCycle01, .endPhaseIncluding1 = 1.0, .beginAmp = 0.f, .slope = 0},
                 }};
}


// four state pulse: low, mid, high, mid
// segment 0: low
// segment 1: mid (transition up)
// segment 2: high (@ duty cycle)
// segment 3: mid (transition down)
// one way to think of the segment lengths is that this is a pulse wave within a pulse wave.
static inline WVShape MakeTriStatePulseShape4(double masterDutyCycle01, double lowDuty01, double highDuty01)
{
  masterDutyCycle01 = std::clamp(masterDutyCycle01, 0.002, 0.998);  // we need slightly more space for the transitions

  // scale low duty to the master duty cycle
  auto lowDutyPhase = std::clamp(lowDuty01 * masterDutyCycle01, 0.0, masterDutyCycle01 - 0.001);
  // scale high duty to the master duty cycle
  auto mainSeg2 = 1.0 - masterDutyCycle01; // length of the high segment area
  auto highDutyPhase = std::clamp(highDuty01 * mainSeg2, 0.0, mainSeg2 - 0.001);

  return WVShape{.mSegments = {
                     WVSegment{.beginPhase01 = 0.0, .endPhaseIncluding1 = lowDutyPhase, .beginAmp = -1.0f, .slope = 0},
                     WVSegment{.beginPhase01 = lowDutyPhase, .endPhaseIncluding1 = masterDutyCycle01, .beginAmp = 0.f, .slope = 0},
                     WVSegment{.beginPhase01 = masterDutyCycle01, .endPhaseIncluding1 = masterDutyCycle01 + highDutyPhase, .beginAmp = 1.0f, .slope = 0},
                     WVSegment{.beginPhase01 = masterDutyCycle01 + highDutyPhase, .endPhaseIncluding1 = 1.0, .beginAmp = .0f, .slope = 0},
                 }};
}

struct SineCore : public OscillatorCore
{
  SineCore()
      : OscillatorCore(OscillatorWaveform::Sine)
  {
  }
  CoreSample renderSampleAndAdvance(float audioRatePhaseOffset) override
  {
    const auto step = mPhaseAcc.advanceOneSample();
    float y = math::sin(math::gPITimes2 * (float)step.phaseBegin01 + audioRatePhaseOffset);
    return {
        .amplitude = y,
        .naive = y,
        .correction = 0.0f,
        .phaseAdvance = step,
    };
  }
};


}  // namespace M7

}  // namespace WaveSabreCore
