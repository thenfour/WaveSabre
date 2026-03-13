
#include "DelayBuffer.h"

namespace WaveSabreCore
{

void AudioBuffer::SetLengthSamples(size_t sampleCount)
{
  sampleCount = std::max(sampleCount, (size_t)1);  // avoid zero-length buffer, which would cause modulo by zero in WriteAndAdvance.
  if (sampleCount == mBuffer.size())
    return;

  // this internally zeroes new elements.
  mBuffer.resize(sampleCount);
  mCursor = 0;  // necessary in case of shrinking buffer.
}

void AudioBuffer::SetLengthMilliseconds(float lengthMs)
{
  auto sampleCount = (size_t)M7::math::MillisecondsToSamples(lengthMs);
  SetLengthSamples(sampleCount);
}

void AudioBuffer::WriteAndAdvance(float sample)
{
  mBuffer[mCursor] = sample;
  mCursor = (mCursor + 1) % mBuffer.size();
}

float AudioBuffer::PeekAtCursor() const
{
  if (mBuffer.empty())
    return 0.0f;
  return mBuffer[mCursor];
}

void AudioBuffer::SetCombParams(float damp, float feedback)
{
  mDamp1 = damp;
  mFeedback = feedback;
}

float AudioBuffer::ProcessComb(float input)
{
  float output = PeekAtCursor();
  mFilterStore = M7::math::lerp(output, mFilterStore, mDamp1);
  WriteAndAdvance(input + (mFilterStore * mFeedback));
  return output;
}

float AudioBuffer::ProcessAllPass(float input)
{
  float bufferOut = PeekAtCursor();
  float output = -input + bufferOut;
  WriteAndAdvance(input + (bufferOut * mFeedback));
  return output;
}

}  // namespace WaveSabreCore