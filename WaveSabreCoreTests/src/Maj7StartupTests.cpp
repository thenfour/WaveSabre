#include <gtest/gtest.h>

#include <WaveSabreCore/../../Basic/Helpers.h>
#include <WaveSabreCore/../../GigaSynth/Maj7.hpp>

using namespace WaveSabreCore;
using namespace WaveSabreCore::M7;

namespace
{
void InitMaj7TestRuntime()
{
  Helpers::SetSampleRate(44100);
  Helpers::CurrentTempo = 125;
}
}

TEST(Maj7Startup, LoadDefaultsKeepsVoiceSettingsSane)
{
  InitMaj7TestRuntime();

  Maj7 synth;

  EXPECT_EQ(synth.mParams.GetIntValue(GigaSynthParamIndices::MaxVoices), 24);
  EXPECT_EQ(synth.mMaxVoices, 24);
  EXPECT_EQ(synth.mParams.GetIntValue(GigaSynthParamIndices::Unisono), 1);
  EXPECT_EQ(synth.mVoicesUnisono, 1);
  EXPECT_EQ(synth.mVoiceMode, VoiceMode::Polyphonic);
}

TEST(Maj7Startup, InvalidVoiceCountsAreClamped)
{
  InitMaj7TestRuntime();

  Maj7 synth;
  constexpr float kClearlyInvalidSerializedInt = IntParamConfig::serializeToFloatN11(2311);

  synth.SetParam((int)GigaSynthParamIndices::MaxVoices, kClearlyInvalidSerializedInt);
  EXPECT_EQ(synth.mMaxVoices, gMaxMaxVoices);

  synth.SetParam((int)GigaSynthParamIndices::Unisono, kClearlyInvalidSerializedInt);
  EXPECT_EQ(synth.mVoicesUnisono, gUnisonoVoiceMax);

  synth.SetParam((int)GigaSynthParamIndices::MaxVoices, IntParamConfig::serializeToFloatN11(-2311));
  EXPECT_EQ(synth.mMaxVoices, 1);

  synth.SetParam((int)GigaSynthParamIndices::Unisono, IntParamConfig::serializeToFloatN11(-2311));
  EXPECT_EQ(synth.mVoicesUnisono, 1);
}