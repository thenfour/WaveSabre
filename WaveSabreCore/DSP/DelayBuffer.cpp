
#include "DelayBuffer.h"

namespace WaveSabreCore
{

void AudioBuffer::SetLengthSamples(size_t sampleCount)
{
  if (sampleCount == mLength)
    return;
  if (sampleCount < 1)
    sampleCount = 1;
  auto oldBuf = mBuffer;
  auto newBuffer = new float[sampleCount];
  memset(newBuffer, 0, sizeof(float) * sampleCount);
  mCursor = 0;
  mLength = (int)sampleCount;
  mBuffer = newBuffer;
  delete[] oldBuf;
}

void AudioBuffer::WriteAndAdvance(float sample)
{
  mBuffer[mCursor] = sample;
  mCursor = (mCursor + 1) % mLength;
}

void DelayBuffer::SetLength(float lengthMs)
{
  int newLength = (int)M7::math::MillisecondsToSamples(lengthMs);
  mBuffer.SetLengthSamples(newLength);
}

void DelayBuffer::WriteSample(float sample)
{
  mBuffer.WriteAndAdvance(sample);
}

float DelayBuffer::ReadSample() const
{
  return mBuffer.PeekAtCursor();
}
}  // namespace WaveSabreCore