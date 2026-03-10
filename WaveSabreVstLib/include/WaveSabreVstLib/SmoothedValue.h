#ifndef __WAVESABREVSTLIB_SMOOTHEDVALUE_H__
#define __WAVESABREVSTLIB_SMOOTHEDVALUE_H__

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cmath>
#include <type_traits>

namespace WaveSabreVstLib
{
// A reusable wrapper to present a stable UI value from a fast-changing measurement.
// Features:
// - Throttled publish cadence (time-based or call-count-based)
// - Hysteresis (deadband) so tiny changes do not update
// - Rounding to step to reduce flicker
// - Optional clamp to [min,max]
// - Optional attack/release smoothing per publish step
// Submit the latest value each update; the wrapper decides when to publish.

template <typename T>
class SmoothedValue
{
  static_assert(std::is_arithmetic<T>::value, "SmoothedValue requires arithmetic type");
  using Clock = std::chrono::steady_clock;
  using Duration = std::chrono::nanoseconds;

public:
  // Per-update submit. Will publish to UI at the configured cadence with hysteresis & rounding.
  // The second parameter is ignored and kept only for call-site compatibility.
  void Submit(T currentValue)
  {
    const auto now = Clock::now();

    bool timeReady = (mPublishPeriod.count() <= 0) || (now >= mNextPublishAt);
    bool callsReady = (mCallsPerPublish > 0 && mCallsToNextPublish == 0);

    if (!timeReady && !callsReady)
    {
      // decrement call-based countdown if enabled
      if (mCallsPerPublish > 0 && mCallsToNextPublish > 0)
        --mCallsToNextPublish;
      return;
    }

    // schedule next publish points
    if (timeReady && mPublishPeriod.count() > 0)
      mNextPublishAt = now + mPublishPeriod;
    if (mCallsPerPublish > 0)
      mCallsToNextPublish = mCallsPerPublish - 1;  // publish now, then wait N-1 calls

    // Derive target value
    T v = currentValue;
    if (mRoundingStep > T(0))
      v = RoundToStep(v, mRoundingStep);

    // Hysteresis: skip if change is small
    if (!mHasPublished || std::abs(v - mLastPublished) >= mHysteresisAbs)
    {
      mPublished.store(v, std::memory_order_relaxed);
      mDisplayValue = v;
      mLastPublished = v;
      mHasPublished = true;
    }
  }

  // UI thread reads this (lock-free)
  T Get() const
  {
    return mPublished.load(std::memory_order_relaxed);
  }

  // Configuration
  // Time-based cadence. Set to <= 0 to publish every call.
  void SetPublishPeriodMs(int ms)
  {
    if (ms <= 0)
    {
      mPublishPeriod = Duration::zero();
    }
    else
    {
      mPublishPeriod = std::chrono::duration_cast<Duration>(std::chrono::milliseconds(ms));
    }
    mNextPublishAt = Clock::now() + mPublishPeriod;
  }

  // Call-based cadence: publish every N Submit calls. Set to 0 to disable.
  void SetPublishEveryCalls(size_t n)
  {
    mCallsPerPublish = n;
    mCallsToNextPublish = (n == 0 ? 0 : n);
  }

  void SetHysteresisAbs(T absDelta)
  {
    mHysteresisAbs = absDelta;
  }
  void SetRoundingStep(T step)
  {
    mRoundingStep = step;
  }

  void Reset()
  {
    mHasPublished = false;
    mLastPublished = T(0);
    mDisplayValue = T(0);
    mPublished.store(T(0), std::memory_order_relaxed);
    mNextPublishAt = Clock::now() + mPublishPeriod;
    mCallsToNextPublish = (mCallsPerPublish == 0 ? 0 : mCallsPerPublish);
  }

private:
  static T RoundToStep(T v, T step)
  {
    if constexpr (std::is_floating_point<T>::value)
      return std::round(v / step) * step;
    else
      return (T)((v + step / 2) / step) * step;  // crude for integers
  }

  // config
  Duration mPublishPeriod = std::chrono::milliseconds(700);  // default ~5 Hz
  size_t mCallsPerPublish = 0;                               // 0 disables call-based cadence
  T mHysteresisAbs = (T)0.01;                                // default 1% for 0..1 metrics
  T mRoundingStep = (T)0.01;                                 // default 0.01

  // state
  std::atomic<T> mPublished{(T)0};
  T mLastPublished = (T)0;
  T mDisplayValue = (T)0;
  bool mHasPublished = false;

  Clock::time_point mNextPublishAt = Clock::now() + mPublishPeriod;
  size_t mCallsToNextPublish = 0;
};

}  // namespace WaveSabreVstLib

#endif  // __WAVESABREVSTLIB_SMOOTHEDVALUE_H__
