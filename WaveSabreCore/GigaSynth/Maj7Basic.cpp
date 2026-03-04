#include "Maj7Basic.hpp"

namespace WaveSabreCore
{
namespace M7
{

uint16_t gModulationRecalcSampleMask = gModulationRecalcSampleMaskValues[(size_t)QualitySetting::Celery];
QualitySetting gQualitySetting = QualitySetting::Celery;


#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
uint16_t gAudioOscRecalcSampleMask = gAudioRecalcSampleMaskValues[(size_t)QualitySetting::Celery];
uint16_t GetAudioOscillatorRecalcSampleMask()
{
  return gAudioOscRecalcSampleMask;
}
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT

uint16_t GetModulationRecalcSampleMask()
{
  return gModulationRecalcSampleMask;
}

void SetQualitySetting(QualitySetting n)
{
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  gAudioOscRecalcSampleMask = gAudioRecalcSampleMaskValues[(size_t)n];
#endif // SELECTABLE_OUTPUT_STREAM_SUPPORT
  gModulationRecalcSampleMask = gModulationRecalcSampleMaskValues[(size_t)n];
  gQualitySetting = n;
}
QualitySetting GetQualitySetting()
{
  return gQualitySetting;
}

}  // namespace M7


}  // namespace WaveSabreCore
