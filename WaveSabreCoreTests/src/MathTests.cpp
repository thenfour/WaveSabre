#include <gtest/gtest.h>

#include <WaveSabreCore/Maj7Basic.hpp>
#include <WaveSabreCore/Helpers.h>

using namespace WaveSabreCore;

static constexpr float kEps = 1e-6f;

TEST(MathTests, MillisecondsToSamples)
{
    Helpers::SetSampleRate(44100);

    float samples = M7::math::MillisecondsToSamples(10.0f);
    // Allow small float error from 1/1000 factor in single-precision
    EXPECT_NEAR(samples, 441.0f, 1e-3f);
}

TEST(MathTests, FloatEqualsBasic)
{
    using M7::math::FloatEquals;
    EXPECT_TRUE(FloatEquals(1.0f, 1.0f + 5e-8f, 1e-6f));
    EXPECT_FALSE(FloatEquals(1.0f, 1.001f, 1e-4f));
}
