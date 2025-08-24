#include <WaveSabreVstLib/VstEditor.h>
#include <WaveSabreVstLib/VstPlug.h>
#include <Windows.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cmath>
#include <limits>
#include <numeric>
#include <string>
#include <vector>

using namespace std;
using namespace WaveSabreCore;

namespace WaveSabreVstLib
{

// -------- Perf telemetry (internal) --------------------------------------------------------------

struct PerfSample
{
  double usage;  // wall_time / buffer_time
  double cps;    // cycles per sample
};

// Single-producer single-consumer ring buffer (no allocations, lock-free).
template <typename T, size_t CapacityPow2>
class SpscRing
{
  static_assert((CapacityPow2 & (CapacityPow2 - 1)) == 0, "Capacity must be a power of two.");

public:
  bool Push(const T& v) noexcept
  {
    const size_t h = mHead.load(std::memory_order_relaxed);
    const size_t next = (h + 1) & (CapacityPow2 - 1);
    if (next == mTail.load(std::memory_order_acquire))
      return false;  // full
    mBuf[h] = v;
    mHead.store(next, std::memory_order_release);
    return true;
  }

  bool Pop(T& out) noexcept
  {
    const size_t t = mTail.load(std::memory_order_relaxed);
    if (t == mHead.load(std::memory_order_acquire))
      return false;  // empty
    out = mBuf[t];
    mTail.store((t + 1) & (CapacityPow2 - 1), std::memory_order_release);
    return true;
  }

private:
  alignas(64) std::atomic<size_t> mHead{0};
  alignas(64) std::atomic<size_t> mTail{0};
  std::array<T, CapacityPow2> mBuf{};
};

// Rolling history and stats (UI-thread only).
class PerfAggregator
{
public:
  static constexpr size_t kHistory = 4096;  // size per series

  void Add(const PerfSample& s)
  {
    mUsage[mIdx] = s.usage;
    mCps[mIdx] = s.cps;
    mCount = std::min(mCount + 1, kHistory);
    mIdx = (mIdx + 1) % kHistory;
  }

  PerfStatsSnapshot GetUsageStats() const
  {
    return ComputeSnapshot(mUsage);
  }
  PerfStatsSnapshot GetCpsStats() const
  {
    return ComputeSnapshot(mCps);
  }

  PerfHistogramSnapshot GetUsageHistogram() const
  {
    return ComputeHistogram(mUsage);
  }
  PerfHistogramSnapshot GetCpsHistogram() const
  {
    return ComputeHistogram(mCps);
  }

  PerfSeriesSnapshot GetUsageSeries(size_t maxPoints) const
  {
    return GetSeries(mUsage, maxPoints);
  }
  PerfSeriesSnapshot GetCpsSeries(size_t maxPoints) const
  {
    return GetSeries(mCps, maxPoints);
  }

private:
  PerfStatsSnapshot ComputeSnapshot(const std::array<double, kHistory>& series) const
  {
    PerfStatsSnapshot snap{};
    if (mCount == 0)
      return snap;

    // Gather last mCount samples in chronological order into a temp buffer
    mTemp.resize(mCount);
    size_t start = (mIdx + kHistory - mCount) % kHistory;
    for (size_t i = 0; i < mCount; ++i)
    {
      mTemp[i] = series[(start + i) % kHistory];
    }

    const double sum = std::accumulate(mTemp.begin(), mTemp.end(), 0.0);
    const double mean = sum / static_cast<double>(mCount);
    double sq = 0.0;
    double minv = mTemp[0];
    double maxv = mTemp[0];
    for (double v : mTemp)
    {
      const double d = v - mean;
      sq += d * d;
      minv = std::min(minv, v);
      maxv = std::max(maxv, v);
    }
    const double stddev = std::sqrt(sq / static_cast<double>(mCount));

    // Percentiles and robust stats
    auto tmp = mTemp;  // copy for selection/sort
    const auto nth50 = tmp.begin() + (tmp.size() * 50) / 100;
    const auto nth95 = tmp.begin() + (tmp.size() * 95) / 100;
    std::nth_element(tmp.begin(), nth50, tmp.end());
    const double p50 = *nth50;
    std::nth_element(tmp.begin(), nth95, tmp.end());
    const double p95 = *nth95;

    // Trimmed mean (10%)
    auto tmpSorted = mTemp;
    std::sort(tmpSorted.begin(), tmpSorted.end());
    size_t trim = tmpSorted.size() / 10;  // 10%
    size_t begin = std::min(trim, tmpSorted.size());
    size_t end = (tmpSorted.size() > trim) ? (tmpSorted.size() - trim) : begin;
    double tmean = 0.0;
    if (end > begin)
    {
      double tsum = 0.0;
      for (size_t i = begin; i < end; ++i)
        tsum += tmpSorted[i];
      tmean = tsum / double(end - begin);
    }
    else
    {
      tmean = mean;
    }

    // MAD and MAD-clipped mean
    std::vector<double> dev;
    dev.resize(mTemp.size());
    for (size_t i = 0; i < mTemp.size(); ++i)
      dev[i] = std::abs(mTemp[i] - p50);
    auto devMid = dev.begin() + dev.size() / 2;
    std::nth_element(dev.begin(), devMid, dev.end());
    double mad = *devMid;
    double madClippedMean = 0.0;
    {
      double sumIn = 0.0;
      size_t cntIn = 0;
      double k = 3.0;  // 3*MAD window
      for (double v : mTemp)
      {
        if (std::abs(v - p50) <= k * mad)
        {
          sumIn += v;
          ++cntIn;
        }
      }
      madClippedMean = (cntIn > 0) ? (sumIn / (double)cntIn) : mean;
    }

    snap.count = mCount;
    snap.mean = mean;
    snap.stddev = stddev;
    snap.min = minv;
    snap.max = maxv;
    snap.p50 = p50;
    snap.p95 = p95;
    snap.trimmedMean10 = tmean;
    snap.mad = mad;
    snap.madClippedMean = madClippedMean;
    return snap;
  }

  PerfHistogramSnapshot ComputeHistogram(const std::array<double, kHistory>& series) const
  {
    PerfHistogramSnapshot h{};
    if (mCount == 0)
      return h;

    // Collect values and min/max
    mTemp.resize(mCount);
    size_t start = (mIdx + kHistory - mCount) % kHistory;
    double minv = std::numeric_limits<double>::infinity();
    double maxv = -std::numeric_limits<double>::infinity();
    for (size_t i = 0; i < mCount; ++i)
    {
      double v = series[(start + i) % kHistory];
      mTemp[i] = v;
      minv = std::min(minv, v);
      maxv = std::max(maxv, v);
    }
    if (minv == maxv)
    {
      // Put all in the middle bin
      size_t mid = PerfHistogramSnapshot::kBins / 2;
      h.bins[mid] = (int)mCount;
      h.total = mCount;
      h.min = minv;
      h.max = maxv;
      h.binWidth = 0.0;
      h.modeIndex = (int)mid;
      h.modeCount = (int)mCount;
      return h;
    }

    const double range = maxv - minv;
    int modeIdx = 0;
    int modeCnt = 0;
    for (double v : mTemp)
    {
      int bin = (int)std::floor((v - minv) / range * (PerfHistogramSnapshot::kBins - 1));
      bin = std::max(0, std::min((int)PerfHistogramSnapshot::kBins - 1, bin));
      int c = ++h.bins[(size_t)bin];
      if (c > modeCnt)
      {
        modeCnt = c;
        modeIdx = bin;
      }
    }
    h.total = mCount;
    h.min = minv;
    h.max = maxv;
    h.binWidth = range / (double)PerfHistogramSnapshot::kBins;
    h.modeIndex = modeIdx;
    h.modeCount = modeCnt;
    return h;
  }

  // when maxPoints==0, return all available (up to kHistory)
  PerfSeriesSnapshot GetSeries(const std::array<double, kHistory>& series, size_t maxPoints = 0) const
  {
    PerfSeriesSnapshot ss{};

    if (maxPoints == 0)
      maxPoints = kHistory;

    if (mCount == 0 || maxPoints == 0)
      return ss;
    const size_t n = std::min(mCount, maxPoints);
    ss.values.resize(n);
    double minv = std::numeric_limits<double>::infinity();
    double maxv = -std::numeric_limits<double>::infinity();
    size_t start = (mIdx + kHistory - mCount) % kHistory;
    // Skip older samples if we need to downsample window
    size_t first = (mCount > n) ? (mCount - n) : 0;
    for (size_t i = 0; i < n; ++i)
    {
      double v = series[(start + first + i) % kHistory];
      ss.values[i] = (float)v;
      minv = std::min(minv, v);
      maxv = std::max(maxv, v);
    }
    ss.min = (float)minv;
    ss.max = (float)maxv;
    return ss;
  }

  mutable std::vector<double> mTemp;  // scratch
  std::array<double, kHistory> mUsage{};
  std::array<double, kHistory> mCps{};
  size_t mIdx = 0;
  size_t mCount = 0;
};

// Owns the SPSC and UI-side aggregator.
struct PerfRecorder
{
  // Large enough to absorb bursts without dropping (power-of-two).
  static constexpr size_t kSpscCapacity = 16384;

  bool SubmitFromAudio(const PerfSample& s) noexcept
  {
    return ring.Push(s);
  }

  // UI thread: drain what’s available and update rolling stats.
  void DrainToUi()
  {
    PerfSample s;
    while (ring.Pop(s))
      agg.Add(s);
  }

  PerfStatsSnapshot GetUsageStats() const
  {
    return agg.GetUsageStats();
  }
  PerfStatsSnapshot GetCpsStats() const
  {
    return agg.GetCpsStats();
  }
  PerfHistogramSnapshot GetUsageHistogram() const
  {
    return agg.GetUsageHistogram();
  }
  PerfHistogramSnapshot GetCpsHistogram() const
  {
    return agg.GetCpsHistogram();
  }
  PerfSeriesSnapshot GetUsageSeries(size_t maxPoints) const
  {
    return agg.GetUsageSeries(maxPoints);
  }
  PerfSeriesSnapshot GetCpsSeries(size_t maxPoints) const
  {
    return agg.GetCpsSeries(maxPoints);
  }

private:
  SpscRing<PerfSample, kSpscCapacity> ring;
  PerfAggregator agg;
};

VstPlug::VstPlug(audioMasterCallback audioMaster,
                 int numParams,
                 int numInputs,
                 int numOutputs,
                 VstInt32 id,
                 Device* device,
                 bool synth)
    : AudioEffectX(audioMaster, 1, numParams)
{
  // assume the device has default values populated; grab a copy.
  mDefaultParamCache.reserve(numParams);
  for (int i = 0; i < numParams; ++i)
  {
    mDefaultParamCache.push_back(device->GetParam(i));
  }

  QueryPerformanceFrequency(&this->perfFreq);

  this->numParams = numParams;
  this->numInputs = numInputs;
  this->numOutputs = numOutputs;
  this->device = device;
  this->synth = synth;

  setNumInputs(numInputs);
  setNumOutputs(numOutputs);
  setUniqueID(id);
  canProcessReplacing();
  canDoubleReplacing(false);
  programsAreChunks();
  if (synth)
    isSynth();

  vst_strncpy(programName, "Default", kVstMaxProgNameLen);

  // Configure UI smoother defaults
  mCPUUsageUI.SetPublishPeriodMs(200);
  mCPUUsageUI.SetHysteresisAbs(0.01);
  mCPUUsageUI.SetRoundingStep(0.01);

  // Initialize perf recorder
  mPerf = new PerfRecorder();
}

VstPlug::~VstPlug()
{
  if (device)
    delete device;
  delete mPerf;
  mPerf = nullptr;
}

void VstPlug::processReplacing(float** inputs, float** outputs, VstInt32 sampleFrames)
{
  if (device && sampleFrames)
  {
    VstTimeInfo* ti = getTimeInfo(0);
    if (ti)
    {
      if ((ti->flags & kVstTempoValid) > 0)
        setTempo((int)ti->tempo);
    }

    MxcsrFlagGuard mxcsrFlagGuard;

    LARGE_INTEGER timeStart, timeEnd;

    // Measure CPU cycles used by this thread during processing
    ULONG64 cyclesStart = 0, cyclesEnd = 0;
    QueryThreadCycleTime(GetCurrentThread(), &cyclesStart);
    QueryPerformanceCounter(&timeStart);
    device->Run(inputs, outputs, sampleFrames);

    QueryPerformanceCounter(&timeEnd);
    // to calculate CPU, assume that 100% CPU usage would mean it runs at the same as realtime speed. a 3ms buffer took 3ms to process.
    QueryThreadCycleTime(GetCurrentThread(), &cyclesEnd);

    // Real-time load (wall-clock). Guard against invalid sampleRate at startup.
    if (this->sampleRate > 0.0f)
    {
      double elapsedSeconds = (double)(timeEnd.QuadPart - timeStart.QuadPart) / (double)this->perfFreq.QuadPart;
      double frameSeconds = (double)sampleFrames / this->sampleRate;
      double usage = elapsedSeconds / frameSeconds;
      // Drive UI smoothed value (throttled publish)
      mCPUUsageUI.Submit(usage);

      // Submit to perf recorder (drop if ring is full; never block)
      if (mPerf)
      {
        PerfSample s{};
        s.usage = usage;
        if (cyclesEnd >= cyclesStart && sampleFrames > 0)
        {
          s.cps = (double)(cyclesEnd - cyclesStart) / (double)sampleFrames;
        }
        else
        {
          s.cps = 0.0;
        }
        (void)mPerf->SubmitFromAudio(s);
      }
    }

    // Cycles per sample as a compute cost proxy
    if (cyclesEnd >= cyclesStart && sampleFrames > 0)
    {
      const double cyclesPerSample = (double)(cyclesEnd - cyclesStart) / (double)sampleFrames;
      mCyclesPerSampleUI.Submit(cyclesPerSample);
    }
  }
  else
  {
    // keep last UI value; no submit to avoid flicker on stop
  }
}

VstInt32 VstPlug::processEvents(VstEvents* ev)
{
  if (device)
  {
    for (VstInt32 i = 0; i < ev->numEvents; i++)
    {
      if (ev->events[i]->type == kVstMidiType)
      {
        VstMidiEvent* midiEvent = (VstMidiEvent*)ev->events[i];
        char* midiData = midiEvent->midiData;
        int status = midiData[0] & 0xf0;

        //cc::log("Vst midi event; status=%02x, flags=%x, type=d", status, midiEvent->flags, midiEvent->type);

        if (status == 0xb0)
        {
          if (midiData[1] == 0x7e || midiData[1] == 0x7b)
          {
            device->AllNotesOff();
          }
          else
          {
            device->MidiCC(midiData[1], midiData[2], midiEvent->deltaFrames);
          }
        }
        else if (status == 0x90 || status == 0x80)
        {
          int note = midiData[1] & 0x7f;
          int vel = midiData[2] & 0x7f;
          if (vel == 0 || status == 0x80)
            device->NoteOff(note, midiEvent->deltaFrames);
          else
            device->NoteOn(note, vel, midiEvent->deltaFrames);
        }
        else if (status == 0xe0)
        {
          int msb = midiData[2];
          int lsb = midiData[1];
          device->PitchBend(lsb, msb, midiEvent->deltaFrames);
        }
      }
    }
  }
  return 1;
}

void VstPlug::setProgramName(char* name)
{
  vst_strncpy(programName, name, kVstMaxProgNameLen);
}

void VstPlug::getProgramName(char* name)
{
  vst_strncpy(name, programName, kVstMaxProgNameLen);
}

void VstPlug::setSampleRate(float sampleRate)
{
  AudioEffect::setSampleRate(sampleRate);
#ifdef SELECTABLE_OUTPUT_STREAM_SUPPORT
  // minsizerel hardcodes the samplerate so changes are unsupported.
  if (device)
    device->SetSampleRate(sampleRate);
#endif
  // no sample-rate dependency for UI smoother anymore
}

void VstPlug::setTempo(int tempo)
{
  if (device)
    device->SetTempo(tempo);
}

void VstPlug::setParameter(VstInt32 index, float value)
{
  if (device)
    device->SetParam(index, value);
  //if (editor) ((AEffGUIEditor *)editor)->setParameter(index, value);
}

float VstPlug::getParameter(VstInt32 index)
{
  return device ? device->GetParam(index) : 0.0f;
}

void VstPlug::getParameterLabel(VstInt32 index, char* label)
{
  vst_strncpy(label, "%", kVstMaxParamStrLen);
}

void VstPlug::getParameterDisplay(VstInt32 index, char* text)
{
  vst_strncpy(text, to_string(device->GetParam(index) * 100.0f).c_str(), kVstMaxParamStrLen);
}

void VstPlug::getParameterName(VstInt32 index, char* text)
{
  vst_strncpy(text, "Name", kVstMaxParamStrLen);
}

//VstInt32 VstPlug::getChunk(void **data, bool isPreset)
//{
//	//return device ? device->GetChunk(data) : 0;
//}

//VstInt32 VstPlug::setChunk(void *data, VstInt32 byteSize, bool isPreset)
//{
//	if (device) device->SetChunk(data, byteSize);
//	return byteSize;
//}

bool VstPlug::getEffectName(char* name)
{
  vst_strncpy(name, "I AM GOD, BITCH", kVstMaxEffectNameLen);
  return true;
}

bool VstPlug::getVendorString(char* text)
{
  vst_strncpy(text, "tenfour", kVstMaxVendorStrLen);
  return true;
}

bool VstPlug::getProductString(char* text)
{
  vst_strncpy(text, "I AM GOD, BITCH", kVstMaxProductStrLen);
  return true;
}

VstInt32 VstPlug::getVendorVersion()
{
  return 1337;
}

VstInt32 VstPlug::canDo(char* text)
{
  if (synth && (!strcmp(text, "receiveVstEvents") || !strcmp(text, "receiveVstMidiEvents")))
    return 1;
  return -1;
}

VstInt32 VstPlug::getNumMidiInputChannels()
{
  return synth ? 1 : 0;
}

// Indicate to the host that this editor requires keys (enables routing in some DAWs)
bool VstPlug::DECLARE_VST_DEPRECATED(keysRequired)()
{
  return true;
}

void VstPlug::setEditor(VstEditor* editor)
{
  this->editor = editor;
}

Device* VstPlug::getDevice() const
{
  return device;
}

void VstPlug::OptimizeParams()
{
  // override this to optimize params.
}

// assumes that p has had its default param cache filled.
// takes the current patch, returns a binary blob containing the WaveSabre chunk.
// this is where we serialize "diff" params, and save to 16-bit values.
// and there's the opportunity to append other things; for example Maj7 Synth sampler devices.
//
// default implementation just does this for our param cache.
int VstPlug::GetMinifiedChunk(void** data, bool deltaFromDefaults)
{
  M7::Serializer s;

  //auto defaultParamCache = GetDefaultParamCache();
  CCASSERT(mDefaultParamCache.size());

  for (int i = 0; i < numParams; ++i)
  {
    float f = getParameter(i);
    if (deltaFromDefaults)
    {
      f -= mDefaultParamCache[i];
    }
    //static constexpr double eps = 0.000001; // NB: 1/65536 = 0.0000152587890625
    //double af = f < 0 ? -f : f;
    //if (af < eps) {
    //	f = 0;
    //}
    s.WriteInt16NormalizedFloat(f);
  }

  auto ret = s.DetachBuffer();
  *data = ret.first;
  return (int)ret.second;
}

// looks at the current params and returns statistics related to size optimization.
ChunkStats VstPlug::AnalyzeChunkMinification()
{
  ChunkStats ret;
  void* data;
  int size = GetMinifiedChunk(&data, true);
  ret.uncompressedSize = size;

  for (size_t i = 0; i < (size_t)numParams; ++i)
  {
    float d = getParameter((VstInt32)i) - mDefaultParamCache[i];
    if (fabsf(d) > 0.00001f)
      ret.nonZeroParams++;
    else
      ret.defaultParams++;
  }

  std::vector<uint8_t> compressedData;
  compressedData.resize(size);
  std::vector<uint8_t> encodedProps;
  encodedProps.resize(size);
  SizeT compressedSize = size;
  SizeT encodedPropsSize = size;

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
                            (const Byte*)data,
                            size,
                            &props,
                            encodedProps.data(),
                            &encodedPropsSize,
                            0,
                            nullptr,
                            &alloc,
                            &alloc);
  ret.compressedSize = (int)compressedSize;

  delete[] data;
  return ret;
}

void VstPlug::Perf_DrainToUiThread()
{
  if (mPerf)
    mPerf->DrainToUi();
}

PerfStatsSnapshot VstPlug::Perf_GetCPUUsageStats() const
{
  return mPerf ? mPerf->GetUsageStats() : PerfStatsSnapshot{};
}

PerfStatsSnapshot VstPlug::Perf_GetCyclesPerSampleStats() const
{
  return mPerf ? mPerf->GetCpsStats() : PerfStatsSnapshot{};
}

PerfHistogramSnapshot VstPlug::Perf_GetCPUUsageHistogram() const
{
  return mPerf ? mPerf->GetUsageHistogram() : PerfHistogramSnapshot{};
}

PerfHistogramSnapshot VstPlug::Perf_GetCyclesPerSampleHistogram() const
{
  return mPerf ? mPerf->GetCpsHistogram() : PerfHistogramSnapshot{};
}

PerfSeriesSnapshot VstPlug::Perf_GetCPUUsageSeries(size_t maxPoints) const
{
  return mPerf ? mPerf->GetUsageSeries(maxPoints) : PerfSeriesSnapshot{};
}

PerfSeriesSnapshot VstPlug::Perf_GetCyclesPerSampleSeries(size_t maxPoints) const
{
  return mPerf ? mPerf->GetCpsSeries(maxPoints) : PerfSeriesSnapshot{};
}


}  // namespace WaveSabreVstLib
