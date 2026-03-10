#pragma once

namespace WaveSabreCore::M7
{


struct ISampleSource
{
  virtual ~ISampleSource() {}

  virtual const float* GetSampleData() const = 0;
  virtual size_t GetSampleLength() const = 0; // in samples
  virtual int GetSampleLoopStart() const = 0;
  virtual int GetSampleLoopLength() const = 0;
  virtual int GetSampleRate() const = 0;
};

}  // namespace WaveSabreCore::M7