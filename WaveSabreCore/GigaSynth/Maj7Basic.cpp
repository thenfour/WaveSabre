#include "Maj7Basic.hpp"

namespace WaveSabreCore
{
namespace M7
{

uint16_t gAudioOscRecalcSampleMask = gAudioRecalcSampleMaskValues[(size_t)QualitySetting::Celery];
uint16_t gModulationRecalcSampleMask = gModulationRecalcSampleMaskValues[(size_t)QualitySetting::Celery];
QualitySetting gQualitySetting = QualitySetting::Celery;

uint16_t GetAudioOscillatorRecalcSampleMask()
{
  return gAudioOscRecalcSampleMask;
}
uint16_t GetModulationRecalcSampleMask()
{
  return gModulationRecalcSampleMask;
}

void SetQualitySetting(QualitySetting n)
{
  gAudioOscRecalcSampleMask = gAudioRecalcSampleMaskValues[(size_t)n];
  gModulationRecalcSampleMask = gModulationRecalcSampleMaskValues[(size_t)n];
  gQualitySetting = n;
}
QualitySetting GetQualitySetting()
{
  return gQualitySetting;
}

}  // namespace M7


}  // namespace WaveSabreCore
