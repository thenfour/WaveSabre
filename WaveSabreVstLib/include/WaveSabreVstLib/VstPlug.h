#ifndef __WAVESABREVSTLIB_VSTPLUG_H__
#define __WAVESABREVSTLIB_VSTPLUG_H__

#undef _WIN32_WINNT  // todo: WHY?

#define _7ZIP_ST  // single-threaded LZMA lib

#include <WaveSabreCore/Maj7Basic.hpp>

#include <LzmaEnc.h>
#include "audioeffectx.h"

#include <memory>

#include "./Serialization.hpp"
#include "SmoothedValue.h"
#include <WaveSabreCore.h>

// copies 16-bit static array of values to a float buffer. specifically intended to copy a 'defaults' array to a usable float parameter array.
template <size_t N>
inline void Copy16bitDefaults(float* dest, const int16_t (&src)[N])
{
  for (size_t i = 0; i < N; ++i)
  {
    dest[i] = WaveSabreCore::M7::math::Sample16To32Bit(src[i]);
  }
}

namespace WaveSabreVstLib
{

// compresses the given data to test Squishy performance for a given buf.
// just returns the compressed size
inline int TestCompression(int inpSize, void* inpData)
{
  std::vector<uint8_t> compressedData;
  compressedData.resize(inpSize);
  std::vector<uint8_t> encodedProps;
  encodedProps.resize(inpSize);
  SizeT compressedSize = inpSize;
  SizeT encodedPropsSize = inpSize;

  ISzAlloc alloc;
  alloc.Alloc = [](ISzAllocPtr p, SizeT s)
  {
    return malloc(s);
  };
  alloc.Free = [](ISzAllocPtr p, void* addr)
  {
    free(addr);
  };

  CLzmaEncProps props;
  LzmaEncProps_Init(&props);
  props.level = 5;

  int lzresult = LzmaEncode(&compressedData[0],
                            &compressedSize,
                            (const Byte*)inpData,
                            inpSize,
                            &props,
                            encodedProps.data(),
                            &encodedPropsSize,
                            0,
                            nullptr,
                            &alloc,
                            &alloc);
  return (int)compressedSize;
}

template <typename Treal, size_t N>
class MovingAverage
{
public:
  void Update(Treal sample)
  {
    if (num_samples_ < N)
    {
      samples_[num_samples_++] = sample;
      total_ += sample;
    }
    else
    {
      Treal& oldest = samples_[num_samples_++ % N];
      total_ += sample - oldest;
      oldest = sample;
    }
  }

  Treal GetValue() const
  {
    return total_ / GetValidSampleCount();  // std::min(num_samples_, N);
  }

  size_t GetValidSampleCount() const
  {
    return std::min(num_samples_, N);
  }
  size_t GetTotalSamplesTaken() const
  {
    return num_samples_;
  }

  void Clear()
  {
    total_ = 0;
    num_samples_ = 0;
  }

  Treal GetSample(size_t n) const
  {
    return samples_[n % N];
  }

private:
  Treal samples_[N];
  size_t num_samples_ = 0;
  Treal total_ = 0;
};

enum class ChunkStatsResult
{
  Nothing,
  Success,
  NotSupported,
};

struct ChunkStats
{
  ChunkStatsResult result = ChunkStatsResult::Nothing;
  int nonZeroParams = 0;
  int defaultParams = 0;
  int uncompressedSize = 0;
  int compressedSize = 0;
};

// -------- Performance stats public API --------
struct PerfStatsSnapshot
{
  size_t count = 0;
  double mean = 0.0;
  double stddev = 0.0;
  double min = 0.0;
  double max = 0.0;
  double p50 = 0.0;  // median
  double p95 = 0.0;  // 95th percentile
  // Robust centers
  double trimmedMean10 = 0.0;   // mean after trimming 10% tails on both sides
  double mad = 0.0;             // median absolute deviation around median
  double madClippedMean = 0.0;  // mean of samples within 3*MAD of median
};

struct PerfHistogramSnapshot
{
  static constexpr size_t kBins = 50;
  std::array<int, kBins> bins{};
  size_t total = 0;
  double min = 0.0;
  double max = 0.0;
  double binWidth = 0.0;  // nominal width per bin (in value units)
  int modeIndex = -1;     // index of max-count bin
  int modeCount = 0;      // sample count in mode bin
};

struct PerfSeriesSnapshot
{
  std::vector<float> values;  // chronological, oldest -> newest
  float min = 0.0f;
  float max = 0.0f;
};

// Forward declaration of internal perf recorder
struct PerfRecorder;

class VstPlug : public AudioEffectX
{
public:
  VstPlug(audioMasterCallback audioMaster,
          int numParams,
          int numInputs,
          int numOutputs,
          VstInt32 id,
          WaveSabreCore::Device* device,
          bool synth = false);
  virtual ~VstPlug();

  virtual void processReplacing(float** inputs, float** outputs, VstInt32 sampleFrames);
  virtual VstInt32 processEvents(VstEvents* ev);

  virtual void setProgramName(char* name);
  virtual void getProgramName(char* name);

  virtual void setSampleRate(float sampleRate);
  virtual void setTempo(int tempo);

  virtual void setParameter(VstInt32 index, float value);
  virtual float getParameter(VstInt32 index);
  virtual void getParameterLabel(VstInt32 index, char* label);
  virtual void getParameterDisplay(VstInt32 index, char* text);
  virtual void getParameterName(VstInt32 index, char* text);

  virtual VstInt32 getChunk(void** data, bool isPreset) = 0;
  virtual VstInt32 setChunk(void* data, VstInt32 byteSize, bool isPreset) = 0;

  virtual bool getEffectName(char* name);
  virtual bool getVendorString(char* text);
  virtual bool getProductString(char* text);
  virtual VstInt32 getVendorVersion();

  virtual VstInt32 canDo(char* text);
  virtual VstInt32 getNumMidiInputChannels();

  // Tell hosts we require keyboard focus (enables effKeysRequired path in some DAWs)
  virtual bool DECLARE_VST_DEPRECATED(keysRequired)() override;

  // UI-presented CPU usage with throttling/hysteresis
  double GetCPUUsage01_UI() const
  {
    return mCPUUsageUI.Get();
  }

  // UI-presented cycles per sample
  double GetCyclesPerSample_UI() const
  {
    return mCyclesPerSampleUI.Get();
  }

  // Performance telemetry: call from UI thread periodically to drain samples
  void Perf_DrainToUiThread();
  PerfStatsSnapshot Perf_GetCPUUsageStats() const;
  PerfStatsSnapshot Perf_GetCyclesPerSampleStats() const;
  PerfHistogramSnapshot Perf_GetCPUUsageHistogram() const;
  PerfHistogramSnapshot Perf_GetCyclesPerSampleHistogram() const;
  PerfSeriesSnapshot Perf_GetCPUUsageSeries(size_t maxPoints) const;
  PerfSeriesSnapshot Perf_GetCyclesPerSampleSeries(size_t maxPoints) const;

  // takes the current patch, returns a binary blob containing the WaveSabre chunk.
  // this is where we serialize "diff" params, and save to 16-bit values.
  // and there's the opportunity to append other things; for example Maj7 Synth sampler devices.
  //
  // default implementation just does this for our param cache.
  virtual int GetMinifiedChunk(void** data, bool deltaFromDefaults);

  // looks at the current params and returns statistics related to size optimization.
  virtual ChunkStats AnalyzeChunkMinification();

  // default impl does nothing.
  virtual void OptimizeParams();

  virtual const char* GetJSONTagName() = 0;

  using ParamAccessor = ::WaveSabreCore::M7::ParamAccessor;

  template <typename Tenum, typename Toffset>
  inline void OptimizeEnumParam(ParamAccessor& params, Toffset offset)
  {
    int paramID = params.GetParamIndex(offset);  // (int)baseParam + (int)paramOffset;
    if ((int)offset < 0 || paramID < 0)
      return;  // invalid IDs exist for example in LFo

    ParamAccessor dp{mDefaultParamCache.data(), params.mBaseParamID};
    if (dp.GetEnumValue<Tenum>(offset) == params.GetEnumValue<Tenum>(offset))
    {
      params.SetRawVal(offset, dp.GetRawVal(offset));
    }
  }

  template <typename Toffset>
  inline void OptimizeIntParam(ParamAccessor& params, const ::WaveSabreCore::M7::IntParamConfig& cfg, Toffset offset)
  {
    int paramID = params.GetParamIndex(offset);  // (int)baseParam + (int)paramOffset;
    if ((int)offset < 0 || paramID < 0)
      return;  // invalid IDs exist for example in LFo

    ParamAccessor dp{mDefaultParamCache.data(), params.mBaseParamID};
    if (dp.GetIntValue(offset, cfg) == params.GetIntValue(offset, cfg))
    {
      params.SetRawVal(offset, dp.GetRawVal(offset));
    }
  }

  template <typename Toffset>
  inline bool OptimizeBoolParam(ParamAccessor& params, Toffset offset)
  {
    int paramID = params.GetParamIndex(offset);  // (int)baseParam + (int)paramOffset;
    if ((int)offset < 0 || paramID < 0)
      return false;  // invalid IDs exist for example in LFo
    ParamAccessor dp{mDefaultParamCache.data(), params.mBaseParamID};
    bool ret = params.GetBoolValue(offset);
    if (ret == dp.GetBoolValue(offset))
    {
      params.SetRawVal(offset, dp.GetRawVal(offset));
    }
    return ret;
  }


  WaveSabreCore::Device* getDevice() const;

  std::vector<float> mDefaultParamCache;
  bool mShowingPerformanceWindow = false;

protected:
  void setEditor(class VstEditor* editor);

private:
  int numParams, numInputs, numOutputs;
  bool synth;

  LARGE_INTEGER perfFreq;
  SmoothedValue<double> mCPUUsageUI;
  SmoothedValue<double> mCyclesPerSampleUI;

  // Internal perf recorder (audio thread producer, UI thread consumer)
  PerfRecorder* mPerf = nullptr;

  char programName[kVstMaxProgNameLen + 1];

  WaveSabreCore::Device* device;
};
}  // namespace WaveSabreVstLib


// accepts the VST chunk (JSON), optimizes & minifies and outputs the wavesabre optimized binary chunk.
template <typename TVST>
inline int WaveSabreDeviceVSTChunkToMinifiedChunk_Impl(const char* deviceName,
                                                       int inpSize,
                                                       void* inpData,
                                                       int* outpSize,
                                                       void** outpData,
                                                       int deltaFromDefaults)
{
  *outpSize = 0;
  auto p = new TVST(nullptr);  // assume too big for stack.
  auto* pVst = (WaveSabreVstLib::VstPlug*)p;
  pVst->setChunk(inpData, inpSize, false);  // apply JSON via the VST.
  p->OptimizeParams();
  *outpSize = p->GetMinifiedChunk(outpData, !!deltaFromDefaults);
  delete p;
  return *outpSize;
}


#endif
