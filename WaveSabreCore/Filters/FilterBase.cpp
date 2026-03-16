#include "FilterBase.hpp"
#include "../Basic/Helpers.h"


namespace WaveSabreCore::M7
{
float CalculateFilterG(float cutoffHz)
{
    cutoffHz = math::ClampFrequencyHz(cutoffHz);
    return math::tan(M7::math::gPI * cutoffHz * Helpers::CurrentSampleRateRecipF);
}
}  // namespace WaveSabreCore::M7
