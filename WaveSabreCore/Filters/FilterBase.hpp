

#pragma once


#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  #include <memory>
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT

#include "../Basic/DSPMath.hpp"

namespace WaveSabreCore::M7
{

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

enum class FilterCircuit  // : uint8_t
{
  Disabled = 0,
  OnePole,
  Biquad,
  Butterworth,
  Moog,
  K35,
  Diode,

  Count,
};

// 0 = bypass,
// 1 stage = 12db/oct.
// 2 stage = 24
// ...
// 4 stage = 48db/oct.
// ...
// 8 stage = 96db/oct.
enum class FilterSlope  // : uint8_t
{
  Flat = 0,
  Slope6dbOct = 1,
  Slope12dbOct = 2,  // 1 stage of biquad
  Slope24dbOct = 3,
  Slope36dbOct = 4,
  Slope48dbOct = 5,
  Slope60dbOct = 6,
  Slope72dbOct = 7,
  Slope84dbOct = 8,
  Slope96dbOct = 9,  // 8 stages of biquad

  Count,
};

enum class FilterResponse  // : uint8_t
{
  Lowpass,
  Highpass,
  Bandpass,
  Notch,
  Allpass,
  Peak,
  HighShelf,
  LowShelf,

  Count,
};

extern float CalculateFilterG(float cutoffHz);


#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
// NB: This duplicated matrix is acceptable; the class-specific instances should actually be removed.
// we need to be able to check capabilities without an instance.
INLINE bool DoesFilterSupport(M7::FilterCircuit circuit, M7::FilterSlope slope, M7::FilterResponse response)
{
  switch (circuit)
  {
    case M7::FilterCircuit::Disabled:
      return slope == M7::FilterSlope::Slope6dbOct && response == M7::FilterResponse::Lowpass;
    case M7::FilterCircuit::OnePole:
      return slope == M7::FilterSlope::Slope6dbOct &&
             (response == M7::FilterResponse::Lowpass || response == M7::FilterResponse::Highpass ||
              response == M7::FilterResponse::Allpass);
    case M7::FilterCircuit::Biquad:
      return slope >= M7::FilterSlope::Slope12dbOct && slope <= M7::FilterSlope::Slope96dbOct;
    case M7::FilterCircuit::Butterworth:
      return slope >= M7::FilterSlope::Slope12dbOct && slope <= M7::FilterSlope::Slope96dbOct &&
             (response == M7::FilterResponse::Lowpass || response == M7::FilterResponse::Highpass);
    case M7::FilterCircuit::Moog:
      return (slope == M7::FilterSlope::Slope24dbOct || slope == M7::FilterSlope::Slope48dbOct) &&
             (response == M7::FilterResponse::Lowpass || response == M7::FilterResponse::Highpass ||
              response == M7::FilterResponse::Bandpass);
    case M7::FilterCircuit::K35:
      return (slope == M7::FilterSlope::Slope12dbOct || slope == M7::FilterSlope::Slope24dbOct) &&
             (response == M7::FilterResponse::Lowpass || response == M7::FilterResponse::Highpass);
    case M7::FilterCircuit::Diode:
      return (slope == M7::FilterSlope::Slope24dbOct || slope == M7::FilterSlope::Slope48dbOct) &&
             response == M7::FilterResponse::Lowpass;
    default:
      return false;
  }
};
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

struct FilterParams
{
  FilterCircuit circuit;
  FilterSlope slope;
  FilterResponse response;
  real cutoffHz;
  Param01 reso01;
  real gainDb;
};

struct IFilter
{
  virtual void SetParams(
      FilterCircuit circuit,
      FilterSlope slope,
      FilterResponse response,
      real cutoffHz,
      // NB: this must be a 0-1 param value, because different filters interpret "resonance" differently.
      // it's NOT the same as Q.
      Param01 reso01,
      real gainDb) = 0;
  virtual real ProcessSample(real x) = 0;

#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  virtual bool DoesSupport(FilterCircuit circuit, FilterSlope slope, FilterResponse response) = 0;
  virtual real GetMagnitudeAtFrequency(real freqHz) const = 0;
  // needed in order for VSTs to be able to clone filters for the response graph without interrupting realtime audio processing.
  virtual std::unique_ptr<IFilter> Clone() const = 0;
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT

  virtual void Reset() = 0;
};

struct NullFilter : IFilter
{
  virtual void SetParams(FilterCircuit circuit,
                         FilterSlope slope,
                         FilterResponse response,
                         real cutoffHz,
                         Param01 reso01,
                         real gainDb) override
  {
  }
  virtual real ProcessSample(real x) override
  {
    return x;
  }
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  virtual bool DoesSupport(FilterCircuit circuit, FilterSlope slope, FilterResponse response) override
  {
    return true;
  }
  virtual std::unique_ptr<IFilter> Clone() const override
  {
    return std::make_unique<NullFilter>();
  }
  virtual real GetMagnitudeAtFrequency(real freqHz) const override
  {
    return 1.0f;
  }
#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
  virtual void Reset() override {}
};

}  // namespace WaveSabreCore::M7
