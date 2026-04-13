#include "FilterOnePole.hpp"

namespace WaveSabreCore::M7
{

//void MoogOnePoleFilter::SetParams(
//    FilterCircuit circuit,
//                                  FilterSlope slope,
//                                  FilterResponse response,
//                                  float cutoffHz,
//                                  Param01 reso01,
//                                  real gainDb)
//{
//#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
//  if (!DoesSupport(circuit, slope, response))
//  {
//    return;
//  }
//#endif  // SELECTABLE_OUTPUT_STREAM_SUPPORT
//  if (mResponse == response && m_cutoffHz == cutoffHz)
//  {
//    return;
//  }
//
//  mResponse = response;
//  m_cutoffHz = cutoffHz;
//  Recalc();
//}
//
//void MoogOnePoleFilter::Reset()
//{
//  m_z_1L = 0;
//  m_feedbackL = 0;
//}
//
//real2 MoogOnePoleFilter::getFeedbackOutputL()
//{
//  return m_beta * (m_z_1L + m_feedbackL * m_delta);
//}
//
//float MoogOnePoleFilter::ProcessSample(float xn__)
//{
//  // for diode filter support
//  real2 xn = xn__ * m_gamma + m_feedbackL + m_epsilon * getFeedbackOutputL();
//  // calculate v(n)
//  real2 vn = (m_a_0 * xn - m_z_1L) * m_alpha;
//  // form LP output
//  real2 lpf = vn + m_z_1L;
//  // update memory
//  m_z_1L = vn + lpf;
//  switch (mResponse)
//  {
//    default:
//    case FilterResponse::Lowpass:
//      return float(lpf);
//    case FilterResponse::Highpass:
//      return float(xn - lpf);
//    case FilterResponse::Allpass:
//      return float(2.0f * lpf - xn);
//  }
//}
//
//void MoogOnePoleFilter::Recalc()
//{
//  // NB: LFOs use this filter so the cutoff should support VERY low frequencies with precision. fortunately single poles are fine with that.
//  // TPT (topology-preserving transform) 1-pole integrator formula
//  // real2 cutoff = math::clamp(m_cutoffHz, 20, 20000);
//  // real2 wd = math::gPI * cutoff;
//  // real2 T = Helpers::CurrentSampleRateRecipF;
//  // real2 g = real2(math::tan(float(wd * T)));
//
//  real2 g = CalculateFilterG(m_cutoffHz);  // math::tan(cutoff * math::gPI * Helpers::CurrentSampleRateRecipF);
//
//  m_alpha = g / (g + 1);
//}

}  // namespace WaveSabreCore::M7
