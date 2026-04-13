#pragma once

//#include "./Maj7Oscillator3Shape.hpp"
#include "../Basic/Pair.hpp"

namespace WaveSabreCore
{
namespace M7
{

// alpha [0,1): time of the edge within the current sample.
// u = 1 - alpha = remaining fraction of the sample after the edge.
// dAmp = postAmp - preAmp, dSlope = postSlope - preSlope (slope is dy/dphase).
namespace SplitKernels
{
// * 0.5 for canonical polyBLEP normalization (dA * .5), to return correction for 1 unit.
// since delta Y is max 2 (-1 to +1), poly_blep has to halve it for the function to work.
static inline void add_blep(double alpha, double dAmp, double& now, double& next)
{
  const double u = 1.0 - alpha;
  const double halfDAmp = 0.5 * dAmp;
  now += halfDAmp * (u * u);
  next += halfDAmp * (-(alpha * alpha));
}

static inline void add_blamp(double alpha, double dSlope, double dt, double& now, double& next)
{
  static constexpr double OneThird = 1.0 / 3.0;
  const double u = 1.0 - alpha;
  const double outputScale = dSlope * dt * 0.5;
  now += outputScale * u * u * u * OneThird;
  const double um1 = u - 1.0;
  next += outputScale * -OneThird * um1 * um1 * um1;
}
};  // namespace SplitKernels

struct CorrectionSpill
{
  double now = 0.0;
  double next = 0.0;
  inline void open_sample()
  {
    now = next;
    next = 0.0;
  }
  inline void add_edge(double alpha, const ::WaveSabreCore::M7::DoublePair& dAmpSlope, double dt)
  {
    auto dAmp = dAmpSlope[0];
    if (dAmp != 0.0)
      SplitKernels::add_blep(alpha, dAmp, now, next);
    auto dSlope = dAmpSlope[1];
    if (dSlope != 0.0)
      SplitKernels::add_blamp(alpha, dSlope, dt, now, next);
  }
  void reset()
  {
    now = 0.0;
    next = 0.0;
  }
};

}  // namespace M7

}  // namespace WaveSabreCore
