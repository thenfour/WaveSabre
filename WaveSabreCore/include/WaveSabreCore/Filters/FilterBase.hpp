

#pragma once

#include "../Maj7Basic.hpp"

namespace WaveSabreCore
{
namespace M7
{
using real = real_t;
template <typename T>
constexpr real Real(const T x)
{
  return static_cast<real>(x);
  //return {x};
};
constexpr real RealPI = real{3.14159265358979323846264338327950288f};
constexpr real PITimes2 = real{2.0f} * RealPI;
constexpr real Real0 = real{0.0f};
constexpr real Real1 = real{1.0f};
constexpr real Real2 = real{2.0f};

// because many filters use certain static multipliers (coefs) based on filter type,
// these should be indexable. that's why LP and LP2 are the same value, so there's no "empty" items here. there is no
// scenario where LP and LP2 are required to be distinctly separate underlying values.
// enum class FilterType
// {
//     LP = 0, // interpreted as "any lp"
//     LP2 = 0,
//     LP4 = 1,
//     LP8 = 2,

//     BP = 3, // interpreted as "any bp"
//     BP2 =3,
//     BP4 = 4,
//     BP8 = 5,

//     HP = 6, // interpreted as "any hp"
//     HP2 = 6,
//     HP4 = 7,
// };

enum class FilterCircuit
{
  Disabled = 0,
  OnePole,
  Biquad,
  Butterworth,
  Moog,
  K35,
  Diode,
};

// 0 = bypass,
// 1 stage = 12db/oct.
// 2 stage = 24
// ...
// 4 stage = 48db/oct.
// ...
// 8 stage = 96db/oct.
enum class FilterSlope
{
  Flat = 0,
  Slope6dbOct = 1,
  Slope12dbOct = 2, // 1 stage of biquad
  Slope24dbOct = 3,
  Slope36dbOct = 4,
  Slope48dbOct = 5,
  Slope60dbOct = 6,
  Slope72dbOct = 7,
  Slope84dbOct = 8,
  Slope96dbOct = 9, // 8 stages of biquad
};

enum class FilterResponse
{
  Lowpass,
  Highpass,
  Bandpass,
  Notch,
  Allpass,
  Peak,
  HighShelf,
  LowShelf,
};

struct IFilter
{
  virtual void SetParams(FilterCircuit circuit, FilterSlope slope, FilterResponse response, real cutoffHz, real reso) = 0;
  virtual bool DoesSupport(FilterCircuit circuit, FilterSlope slope, FilterResponse response) = 0;
  virtual real ProcessSample(real x) = 0;
  virtual real GetMagnitudeAtFrequency(real freqHz) const = 0;
  virtual void Reset() = 0;
};

struct NullFilter : IFilter
{
  virtual void SetParams(FilterCircuit circuit, FilterSlope slope, FilterResponse response, real cutoffHz, real reso) override {}
  virtual bool DoesSupport(FilterCircuit circuit, FilterSlope slope, FilterResponse response) override { return true; }
  virtual real ProcessSample(real x) override
  {
    return x;
  }
  virtual real GetMagnitudeAtFrequency(real freqHz) const override
  {
    return 1.0f;
  }
  virtual void Reset() override {}
};

}  // namespace M7
}  // namespace WaveSabreCore
