#pragma once

#include <cstdint>

// #include "./StrongScalar.hpp"
// #include "LUTs.hpp"
// #include <WaveSabreCore/Helpers.h>
// #include <Windows.h>
// #include <algorithm>
// #include <memory>
// #include <optional>



namespace WaveSabreCore
{
enum class VoiceMode
{
  Polyphonic,
  MonoLegatoTrill,
  Count,
};

namespace M7
{

// too high (1023 for example) results in ugly audible ramping between modulations
// 255 feels actually quite usable
// 127 as well. - maybe this could be a "low cpu" setting.
// 31 feels like a good quality compromise.
// 7 is sharp quality.
// 3 is HD.
enum class QualitySetting  // : uint8_t
{
  Potato,
  Carrot,
  Cauliflower,
  Celery,
  Artichoke,
  Count,
};

static constexpr uint16_t gAudioRecalcSampleMaskValues[] = {
    127,  // Potato,
    63,   // Carrot,
    31,   // Cauliflower,
    7,    // Celery,
    1,    // Artichoke,
};

static constexpr uint16_t gModulationRecalcSampleMaskValues[] = {
    127,  // Potato,
    63,   // Carrot,
    31,   // Cauliflower,
    15,   // Celery,
    7,    // Artichoke,
};


#define QUALITY_SETTING_CAPTIONS(symbolName)                                                                           \
  static constexpr char const* const symbolName[(int)::WaveSabreCore::M7::QualitySetting::Count]{                      \
      "Potato",                                                                                                        \
      "Carrot",                                                                                                        \
      "Cauliflower",                                                                                                   \
      "Celery",                                                                                                        \
      "Artichoke",                                                                                                     \
  };

extern uint16_t GetAudioOscillatorRecalcSampleMask();
extern uint16_t GetModulationRecalcSampleMask();

extern QualitySetting GetQualitySetting();
extern void SetQualitySetting(QualitySetting);


enum class Oversampling  // : uint8_t
{
  Off = 0,
  x2 = 1,  // value can be seen as the # of stages
  x4 = 2,
  x8 = 3,
  Count = 4,
};

#define OVERSAMPLING_CAPTIONS(symbolName)                                                                              \
  static constexpr char const* const symbolName[(int)::WaveSabreCore::M7::Oversampling::Count]{                        \
      "Off",                                                                                                           \
      "x2",                                                                                                            \
      "x4",                                                                                                            \
      "x8",                                                                                                            \
  };


}  // namespace M7


}  // namespace WaveSabreCore
