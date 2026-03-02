// the idea is to verify that the characteristics that make BLEP and BLAMP useful are met.
// the exact polynomial is not tested here.

#include <gtest/gtest.h>

#include <format>

#include <WaveSabreCore/../../Basic/Helpers.h>
#include <WaveSabreCore/../../GigaSynth/Maj7Basic.hpp>
#include <WaveSabreCore/../../Waveshapes/Maj7Oscillator3Shape.hpp>
#include <WaveSabreCore/../../Waveshapes/Maj7Oscillator4WS.hpp>

using namespace WaveSabreCore;
using namespace WaveSabreCore::M7;
using SplitKernels::add_blep;
using SplitKernels::add_blamp;

static constexpr float kTol = 1e-6f; // tolerance

// Edge placement limits
TEST(BLEP, EdgePlacementLimits)
{
  double now = 0, next = 0;
  const double dAmp = 2.0;

  // alpha = 0
  // if the edge is at the start of the sample, all correction goes into "now".
  add_blep(0.0, dAmp, now, next);
  EXPECT_NEAR(now, dAmp * .5, kTol);
  EXPECT_NEAR(next, 0.0, kTol);

  // alpha -> 1 (simulate with 1.0 exactly for this math)
  // if the edge is at the end of the sample, all correction goes into "next".
  now = next = 0;
  add_blep(1.0, dAmp, now, next);  // your engine usually uses [0,1), but formula is well-defined at 1.0
  EXPECT_NEAR(now, 0.0, kTol);
  EXPECT_NEAR(next, -dAmp * .5, kTol);
}

// Midpoint symmetry
TEST(BLEP, MidpointSymmetry)
{
  double now = 0, next = 0;
  const double dAmp = 2;

  // when edge is in the middle of the sample, correction is split evenly between now & next.
  add_blep(0.5, dAmp, now, next);
  EXPECT_NEAR(now, +dAmp * 0.25 * .5, kTol);
  EXPECT_NEAR(next, -dAmp * 0.25 * .5, kTol);
}

// Sign & monotonicity for dAmp > 0
// now(alpha) is non-negative and decreases along [0,1).
// next(alpha) is non-positive and goes more negative along [0,1).
TEST(BLEP, SignAndMonotonicity_PositiveStep)
{
  const double dAmp = 1.0;
  const std::vector<double> alphas = {0.0, 0.1, 0.25, 0.5, 0.75, 0.9};

  double prevNow = 1e9;
  double prevNext = 1e9;

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

// Linearity in dAmp
// add_blep(alpha, d1 + d2) == add_blep(alpha, d1) + add_blep(alpha, d2) (for both now & next).
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

// Zero step gives zero correction
TEST(BLEP, ZeroStepZeroCorrection)
{
  double now = 0, next = 0;
  add_blep(0.42, 0.0, now, next);
  EXPECT_NEAR(now, 0.0, kTol);
  EXPECT_NEAR(next, 0.0, kTol);
}

// Time-reversal symmetry: now(a) == -next(1-a)
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


static constexpr double KS   = 0.5;     // BLAMP halving factor

static inline std::pair<double,double> EvalBLAMP(double alpha, double dSlope, double dt)
{
    double now = 0.0, next = 0.0;
    add_blamp(alpha, dSlope, dt, now, next);
    return {now, next};
}




// 1) Edge placement limits
TEST(BLAMP, EdgePlacementLimits)
{
    const double dS = 2.0;
    const double dt = 0.3;

    // alpha = 0 → all in "now"
    {
        auto [now, next] = EvalBLAMP(0.0, dS, dt);
        EXPECT_NEAR(now,  KS * dS * dt * (1.0/3.0), kTol);
        EXPECT_NEAR(next, 0.0,                      kTol);
    }

    // alpha = 1 → all in "next" (kernel-wise; scheduler may defer this)
    {
        auto [now, next] = EvalBLAMP(1.0, dS, dt);
        EXPECT_NEAR(now,  0.0,                      kTol);
        EXPECT_NEAR(next, KS * dS * dt * (1.0/3.0), kTol);
    }
}

// 2) Midpoint symmetry
TEST(BLAMP, MidpointSymmetry)
{
    const double dS = 3.5;
    const double dt = 0.2;
    auto [now, next] = EvalBLAMP(0.5, dS, dt);
    EXPECT_NEAR(now,  KS * dS * dt * (1.0/24.0), kTol);
    EXPECT_NEAR(next, KS * dS * dt * (1.0/24.0), kTol);
}

// 3) Sign & monotonicity for positive dSlope
TEST(BLAMP, SignAndMonotonicity_PositiveSlope)
{
    const double dS = 1.0;
    const double dt = 0.4;

    const std::vector<double> alphas = {0.0, 0.1, 0.25, 0.5, 0.75, 0.9};

    double prevNow  = 1e9;  // start high; should decrease
    double prevNext = -1e9; // start low; should increase

    for (double a : alphas)
    {
        auto [now, next] = EvalBLAMP(a, dS, dt);

        // non-negativity
        EXPECT_GE(now,  0.0);
        EXPECT_GE(next, 0.0);

        // monotonic: now decreases with alpha, next increases
        EXPECT_LE(now,  prevNow);
        EXPECT_GE(next, prevNext);

        prevNow  = now;
        prevNext = next;
    }
}

// 4) Linearity in dSlope and dt
TEST(BLAMP, LinearityInSlopeAndDt)
{
    const double a  = 0.37;
    const double d1 = -0.7, d2 = 1.9;
    const double dt = 0.15;

    // linear in slope
    auto [n12, x12] = EvalBLAMP(a, d1 + d2, dt);
    auto [n1,  x1 ] = EvalBLAMP(a, d1,       dt);
    auto [n2,  x2 ] = EvalBLAMP(a, d2,       dt);
    EXPECT_NEAR(n12, n1 + n2, kTol);
    EXPECT_NEAR(x12, x1 + x2, kTol);

    // linear in dt
    auto [nA, xA] = EvalBLAMP(a, d2, dt * 2.0);
    auto [nB, xB] = EvalBLAMP(a, d2, dt);
    EXPECT_NEAR(nA, 2.0 * nB, 1e-12);
    EXPECT_NEAR(xA, 2.0 * xB, 1e-12);
}

// 5) Zero cases
TEST(BLAMP, ZeroCases)
{
    // zero slope
    {
        auto [now, next] = EvalBLAMP(0.42, 0.0, 0.25);
        EXPECT_NEAR(now,  0.0, kTol);
        EXPECT_NEAR(next, 0.0, kTol);
    }
    // zero dt
    {
        auto [now, next] = EvalBLAMP(0.42, 1.0, 0.0);
        EXPECT_NEAR(now,  0.0, kTol);
        EXPECT_NEAR(next, 0.0, kTol);
    }
}

// 6) Time-reversal symmetry: now(a) == next(1-a)
TEST(BLAMP, TimeReversalSymmetry)
{
    const double dS = -0.8;
    const double dt = 0.12;
    const std::vector<double> alphas = {0.0, 0.1, 0.25, 0.5, 0.75, 0.9};

    for (double a : alphas)
    {
        auto [nowA, nextA] = EvalBLAMP(a,       dS, dt);
        auto [nowB, nextB] = EvalBLAMP(1.0 - a, dS, dt);
        EXPECT_NEAR(nowA, nextB, 1e-12);
    }
}

// 7) Bounds for positive slope
TEST(BLAMP, Bounds_PositiveSlope)
{
    const double dS = 2.5;
    const double dt = 0.33;
    const double maxTap = KS * dS * dt * (1.0/3.0);

    const std::vector<double> alphas = {0.0, 0.2, 0.5, 0.8, 0.99};

    for (double a : alphas)
    {
        auto [now, next] = EvalBLAMP(a, dS, dt);
        EXPECT_GE(now,  0.0);
        EXPECT_GE(next, 0.0);
        EXPECT_LE(now,  maxTap + 1e-12);
        EXPECT_LE(next, maxTap + 1e-12);
    }
}


