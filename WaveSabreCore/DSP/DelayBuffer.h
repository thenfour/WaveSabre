#pragma once

#include "../Basic/DSPMath.hpp"
#include "../Basic/PodVector.hpp"

namespace WaveSabreCore
{
// not a template to avoid code size bloat.
// circular buffer with a cursor.
struct AudioBuffer
{
private:
  M7::PodVector<float> mBuffer;
  size_t mCursor = 0;

  // from comb
  float mFeedback = 0;
  float mDamp1 = 0;  // one pole lowpass filter coefficient, 0..1. 0 = no damping, 1 = full damping.
  float mFilterStore = 0;

public:
  // seems that this gets called on a separate thread than audio processing; make it naively thread-safe.
  void SetLengthSamples(size_t sampleCount);
  void SetLengthMilliseconds(float lengthMs);

  void WriteAndAdvance(float sample);

  float PeekAtCursor() const;

  void SetCombParams(float damp, float feedback);

  float ProcessComb(float input);
  float ProcessAllPass(float input);

  float& operator[](size_t i)
  {
    CCASSERT(i > 0);
    CCASSERT(i < mBuffer.size());
    return mBuffer[i];
  }
};

}  // namespace WaveSabreCore
