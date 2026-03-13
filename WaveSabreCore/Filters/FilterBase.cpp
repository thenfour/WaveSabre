#include "FilterBase.hpp"
#include "../Basic/Helpers.h"


namespace WaveSabreCore::M7
{
float CalculateFilterG(float cutoffHz)
{
    cutoffHz = math::clamp(cutoffHz, 10.0f, 0.5f * Helpers::CurrentSampleRateF);
    return math::tan(M7::math::gPI * cutoffHz * Helpers::CurrentSampleRateRecipF);
}
}  // namespace WaveSabreCore::M7
