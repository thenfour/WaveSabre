
#include "Maj7Filter.hpp"

namespace WaveSabreCore::M7
{

void FilterNode::SetParams(FilterCircuit circuit,
                           FilterSlope slope,
                           FilterResponse response,
                           float cutoffHz,
                           Param01 reso01,
                           float gainDb)
{
  // select filter & set type
  IFilter* nextFilter = nullptr;
  switch (circuit)
  {
    default:
    case FilterCircuit::Disabled:
      nextFilter = &mNullFilter;
      break;
    case FilterCircuit::OnePole:
      nextFilter = &mOnePole;
      break;
    case FilterCircuit::Biquad:
      nextFilter = &mBiquad;
      break;
    case FilterCircuit::Butterworth:
      nextFilter = &mButterworth;
      break;
    case FilterCircuit::Diode:
      nextFilter = &mDiode;
      break;
    case FilterCircuit::K35:
      nextFilter = &mK35;
      break;
    case FilterCircuit::Moog:
      nextFilter = &mMoog;
      break;
  }
  if (mSelectedFilter != nextFilter)
  {
    nextFilter->Reset();
    mSelectedFilter = nextFilter;
  }
  mSelectedFilter->SetParams(circuit, slope, response, cutoffHz, reso01, gainDb);
}

void FilterNode::ResetState()
{
  mSelectedFilter->Reset();
}

float FilterNode::ProcessSample(float inputSample)
{
  return mSelectedFilter->ProcessSample(inputSample);
}


}  // namespace WaveSabreCore::M7
