#include <gtest/gtest.h>

#include <format>

#include <WaveSabreCore/Helpers.h>
#include <WaveSabreCore/Maj7Basic.hpp>
#include <WaveSabreCore/Maj7Oscillator3Shape.hpp>
#include <WaveSabreCore/Maj7Oscillator4WS.hpp>

using namespace WaveSabreCore;
using namespace WaveSabreCore::M7;
using M7Osc4::SplitKernels::add_blep;

static constexpr float kTol = 1e-6f; // tolerance

// 1) Edge placement limits
TEST(BLEP, EdgePlacementLimits)
{
  double now = 0, next = 0;
  const double dAmp = 2.0;

  // alpha = 0
  // if the edge is at the start of the sample, all correction goes into "now".
  add_blep(0.0, dAmp, now, next);
  EXPECT_NEAR(now, dAmp, kTol);
  EXPECT_NEAR(next, 0.0, kTol);

  // alpha -> 1 (simulate with 1.0 exactly for this math)
  // if the edge is at the end of the sample, all correction goes into "next".
  now = next = 0;
  add_blep(1.0, dAmp, now, next);  // your engine usually uses [0,1), but formula is well-defined at 1.0
  EXPECT_NEAR(now, 0.0, kTol);
  EXPECT_NEAR(next, -dAmp, kTol);
}

// 2) Midpoint symmetry
TEST(BLEP, MidpointSymmetry)
{
  double now = 0, next = 0;
  const double dAmp = 3.5;

  // when edge is in the middle of the sample, correction is split evenly between now & next.
  add_blep(0.5, dAmp, now, next);
  EXPECT_NEAR(now, +dAmp * 0.25, kTol);
  EXPECT_NEAR(next, -dAmp * 0.25, kTol);
}

// 3) Sign & monotonicity for dAmp > 0
TEST(BLEP, SignAndMonotonicity_PositiveStep)
{
  const double dAmp = 1.0;
  const std::vector<double> alphas = {0.0, 0.1, 0.25, 0.5, 0.75, 0.9};

  double prevNow = 1e9;
  double prevNext = -1e9;

  for (double a : alphas)
  {
    double now = 0, next = 0;
    add_blep(a, dAmp, now, next);

    EXPECT_GE(now, 0.0);   // now >= 0
    EXPECT_LE(next, 0.0);  // next <= 0

    // monotone: now decreases with alpha; next decreases (more negative) with alpha
    EXPECT_LE(now, prevNow);
    EXPECT_LE(next, prevNext);  // e.g., -0.01 <= -0.005 is false; we want more negative, so compare value
    prevNow = now;
    prevNext = next;
  }
}

// 4) Linearity in dAmp
TEST(BLEP, Linearity)
{
  const double a = 0.37;
  const double d1 = -0.7, d2 = 1.9;

  double n12 = 0, x12 = 0;
  add_blep(a, d1 + d2, n12, x12);

  double n1 = 0, x1 = 0;
  add_blep(a, d1, n1, x1);
  double n2 = 0, x2 = 0;
  add_blep(a, d2, n2, x2);

  EXPECT_NEAR(n12, n1 + n2, kTol);
  EXPECT_NEAR(x12, x1 + x2, kTol);
}

// 5) Zero step gives zero correction
TEST(BLEP, ZeroStepZeroCorrection)
{
  double now = 0, next = 0;
  add_blep(0.42, 0.0, now, next);
  EXPECT_NEAR(now, 0.0, kTol);
  EXPECT_NEAR(next, 0.0, kTol);
}

// 6) Discrete jump reduction across boundary
TEST(BLEP, JumpReductionAcrossBoundary)
{
  const double pre = 0.4;
  const double dAmp = -1.6;  // arbitrary sign
  const double post = pre + dAmp;

  const std::vector<double> alphas = {0.0, 0.1, 0.25, 0.5, 0.75, 0.9, 1.0};

  for (double a : alphas)
  {
    double now = 0, next = 0;
    add_blep(a, dAmp, now, next);

    const double y0 = pre + now;
    const double y1 = post + next;

    const double u = 1.0 - a;
    const double predicted = 2.0 * dAmp * u * (1.0 - u);  // closed-form from your split

    EXPECT_NEAR((y1 - y0), predicted, 1e-12);
    EXPECT_LE(std::abs(y1 - y0), 0.5 * std::abs(dAmp) + 1e-12);
  }
}

// 7) Time-reversal symmetry: now(a) == -next(1-a)
TEST(BLEP, TimeReversalSymmetry)
{
  const double dAmp = 0.8;

  const std::vector<double> alphas = {0.0, 0.1, 0.25, 0.5, 0.75, 0.9};

  for (double a : alphas)
  {
    double nowA = 0, nextA = 0;
    add_blep(a, dAmp, nowA, nextA);
    double nowB = 0, nextB = 0;
    add_blep(1.0 - a, dAmp, nowB, nextB);

    EXPECT_NEAR(nowA, -nextB, 1e-12);
  }
}