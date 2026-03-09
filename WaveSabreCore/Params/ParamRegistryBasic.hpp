#pragma once

#include "../Basic/DSPMath.hpp"
#include <stdint.h>


namespace WaveSabreCore::M7
{
enum class ParamType
{
  Float01Param,
  FloatN11Param,
  IntParam,
  BoolParam,
  EnumParam,
  //
};

struct ParamRegistryValue
{
  int16_t mAsDefault;
  float mAsFloat01;

  [[nodiscard]]
  static ParamRegistryValue FromFloat01(float f01)
  {
    auto asdefault = math::Sample32To16(f01);
    return ParamRegistryValue(asdefault, f01);
  }

  [[nodiscard]]
  static ParamRegistryValue FromDefault(int16_t defaultValue)
  {
    auto asFloat01 = math::Sample16To32Bit(defaultValue);
    return ParamRegistryValue(defaultValue, asFloat01);
  }

private:
  ParamRegistryValue(int16_t asDefault, float asFloat01)
      : mAsDefault(asDefault)
      , mAsFloat01(asFloat01)
  {
  }
};

template <typename TparamEnum>
struct ParamRegistryEntry
{
  TparamEnum index;
  ParamType type;
  const char* enumTypeName;
  const char* numericConfigName;  // like for freq, vol params...
  const char* vstParamName;
  const char* displayName;
  ParamRegistryValue defaultValue;
  ParamRegistryValue centerValue;
};

template <typename TparamEnum, size_t TparamCount>
struct ParamRegistry
{
private:
  const ParamRegistryEntry<TparamEnum> entries[TparamCount];

public:
  static constexpr size_t kParamCount = TparamCount;

  constexpr ParamRegistry(const ParamRegistryEntry<TparamEnum> (&entries)[TparamCount])
      : entries(entries)
  {
  }

  const ParamRegistryEntry<TparamEnum>& GetEntry(TparamEnum param) const
  {
    return entries[(size_t)param];
  }
};
}  // namespace WaveSabreCore::M7
