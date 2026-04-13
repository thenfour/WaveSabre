
#include "FilterMoog.hpp"

namespace WaveSabreCore::M7
{
#ifdef ENABLE_MOOG_FILTER

void MoogLadderFilter::SetParams(FilterCircuit circuit,
                                 FilterSlope slope,
                                 FilterResponse response,
                                 real cutoffHz,
                                 Param01 reso01,
                                 real gainDb)
{
  if ((mSlope != slope) || (mResponse != response) || (cutoffHz != m_cutoffHz) || (reso01.value != mReso01))
  {
    mSlope = slope;
    mResponse = response;
    m_cutoffHz = cutoffHz;
    mReso01 = reso01.value;
    m_k = mReso01 * real2(3.88);  // this maps dQControl = 0->1 to 0-4 * 0.97 to avoid clippy self oscillation
    Recalc();
  }
}

void MoogLadderFilter::Reset()
{
  for (auto& lpf : m_LPF)
  {
    lpf.Reset();
  }
}

float MoogLadderFilter::ProcessSample(float xn)
{
  real2 dSigma = 0;
  for (auto& lpf : m_LPF)
  {
    dSigma += lpf.getFeedbackOutputL();
  }

  // calculate input to first filter
  real2 dLP[5];
  dLP[0] = (xn - m_k * dSigma) * m_alpha_0;

  static constexpr int8_t letterVals[6][5] = {
      {0, 0, 1, 0, 0},    // lp2
      {0, 0, 0, 0, 1},    // lp4
      {1, -2, 1, 0, 0},   // hp2
      {1, -4, 6, -4, 1},  // hp4
      {0, 2, -2, 0, 0},   // bp2
      {0, 0, 4, -8, 4},   // bp4
  };

  static_assert((size_t)FilterResponse::Lowpass == 0, "filter type enum values must match letterVals table");
  static_assert((size_t)FilterResponse::Highpass == 1, "filter type enum values must match letterVals table");
  static_assert((size_t)FilterResponse::Bandpass == 2, "filter type enum values must match letterVals table");
  size_t filterTypeIndex = (size_t)mResponse * 2 + ((mSlope == FilterSlope::Slope48dbOct) ? 1 : 0);

  // --- cascade of 4 filters
  real2 output = 0;
  for (size_t i = 0; i <= 4; ++i)
  {
    if (i < 4)
      dLP[i + 1] = real2(m_LPF[i].ProcessSample(float(dLP[i])));
    output += dLP[i] * letterVals[filterTypeIndex][i];
  }
  //output += dLP[4] * letterVals[(size_t)m_FilterType][4];

  return float(output);
}

void MoogLadderFilter::Recalc()
{
  // prewarp for BZT
  //real wd = PITimes2 * m_cutoffHz;

  // note: measured input to tan function, it seemed limited to (0.005699, 1.282283).
  // input for fasttan shall be limited to (-pi/2, pi/2) according to documentation
  //real wa = (2 * Helpers::CurrentSampleRateF) * math::tan(wd * Helpers::CurrentSampleRateRecipF * Real(0.5));
  //real g = wa * Helpers::CurrentSampleRateRecipF * Real(0.5);

  //real2 cutoff = math::clamp(m_cutoffHz, 30, 20000);

  real2 g = CalculateFilterG(m_cutoffHz);  // math::tan(real2(cutoff) * math::gPI * Helpers::CurrentSampleRateRecipF);

  // G - the feedforward coeff in the VA One Pole
  //     same for LPF, HPF
  const real2 oneOver1plusg = real2(1) / (g + 1);
  real2 G = g * oneOver1plusg;

  m_gamma = 1;
  for (int i = 3; i >= 0; --i)
  {
    m_LPF[i].m_alpha = G;
    m_LPF[i].m_beta = m_gamma * oneOver1plusg;
    m_gamma *= G;
  }

  m_alpha_0 = real2(1) / (m_k * m_gamma + 1);
}

#endif // ENABLE_MOOG_FILTER

}  // namespace WaveSabreCore::M7
