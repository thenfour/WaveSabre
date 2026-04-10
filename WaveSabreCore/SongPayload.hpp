#include "WSCore/Device.h"

#include "./include/Devices.h"

namespace WaveSabreCore
{

// NB: keep in sync with WaveSabreConvert/Song.cs
enum class DeviceId
{
  Maj7Analyze,
  Maj7Comp,
  Maj7CReverb,
  Maj7Crush,
  Maj7Delay,
  Maj7EQ,
  Maj7GigaSynth,
  Maj7MBC,
  Maj7Saturation,
  Maj7Space,
  Maj7StereoImager,
  Maj7Modulate,
};

typedef Device* (*DeviceFactory)(DeviceId);

typedef struct
{
  DeviceFactory factory;
  const unsigned char* blob;
} Song;

}  // namespace WaveSabreCore