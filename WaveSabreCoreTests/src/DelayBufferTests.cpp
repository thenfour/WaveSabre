#include <gtest/gtest.h>

#include <WaveSabreCore/../../DSP/DelayBuffer.h>
#include <WaveSabreCore/../../GigaSynth/Maj7Basic.hpp>
#include <WaveSabreCore/../../Basic/Helpers.h>

using namespace WaveSabreCore;

// Small epsilon for floating comparisons
static constexpr float kEps = 1e-6f;

TEST(DelayBufferTests, WriteThenReadZeroDelay)
{
    // Ensure a known sample rate for deterministic conversion
    Helpers::SetSampleRate(48000);

    DelayBuffer db;
    db.SetLength(0.0f); // ms -> at least 1 sample internally

    // When length is effectively 0ms (>=1 sample), Write advances cursor, Read peeks at cursor
    // So the value at the current cursor is the last written sample.
    db.WriteSample(0.25f);
    EXPECT_NEAR(db.ReadSample(), 0.25f, kEps);

    db.WriteSample(-0.75f);
    EXPECT_NEAR(db.ReadSample(), -0.75f, kEps);
}

TEST(DelayBufferTests, FixedDelayOneMillisecond)
{
    Helpers::SetSampleRate(48000);

    DelayBuffer db;
    db.SetLength(1.0f); // 1 ms

    // Feed an impulse and then zeros, read should output zeros until the delay wraps
    db.WriteSample(1.0f);
    for (int i = 0; i < 47; ++i)
    {
        EXPECT_NEAR(db.ReadSample(), 0.0f, kEps);
        db.WriteSample(0.0f);
    }

    // After 48 writes total, the cursor wraps to the written 1.0f
    EXPECT_NEAR(db.ReadSample(), 1.0f, kEps);
}
