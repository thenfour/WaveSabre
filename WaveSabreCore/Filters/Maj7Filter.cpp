
#include "Maj7Filter.hpp"

namespace WaveSabreCore::M7
{

void FilterNode::SetParams(FilterResponse response,
                           float cutoffHz,
                           Param01 reso01,
                           float gainDb)
{
  const auto q = Decibels {gBiquadFilterQCfg.Param01ToValue(reso01.value)};
  mFilter.SetBiquadParams(response, cutoffHz, q, gainDb);
}

// void FilterNode::ResetState()
// {
//   mFilter.Reset();
// }

// float FilterNode::ProcessSample(float inputSample)
// {
//   return mFilter.ProcessSample(inputSample);
// }


}  // namespace WaveSabreCore::M7
