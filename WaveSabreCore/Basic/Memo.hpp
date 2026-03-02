#pragma once

#include "LUTs.hpp"

namespace WaveSabreCore
{
namespace M7
{

template <typename T, typename... Deps>
class Memo
{
public:
  // usage: get([&]{ return compute(); }, dep1, dep2, ...)
  template <typename F>
  const T& get(F&& compute, const Deps&... deps)
  {
    std::tuple<Deps...> newDeps{deps...};
    if (!value_ || !depsEqual(newDeps))
    {
      value_.emplace(std::invoke(std::forward<F>(compute)));
      deps_ = std::move(newDeps);
    }
    return *value_;
  }

  // Force recompute on next call.
  void invalidate()
  {
    value_.reset();
  }

  bool hasValue() const noexcept
  {
    return value_.has_value();
  }

private:
  // equality helpers
  //static bool eq(const float& a, const float& b, double eps) {
  //  const double da = a, db = b;
  //  const double scale = 1.0 + std::max(std::abs(da), std::abs(db));
  //  return std::abs(da - db) <= eps * scale;
  //}
  //static bool eq(const double& a, const double& b, double eps) {
  //  const double scale = 1.0 + std::max(std::abs(a), std::abs(b));
  //  return std::abs(a - b) <= eps * scale;
  //}
  //static bool eq(const long double& a, const long double& b, double eps) {
  //  const long double scale = 1.0L + std::max(std::abs(a), std::abs(b));
  //  return std::abs(a - b) <= static_cast<long double>(eps) * scale;
  //}
  template <class U>
  static bool eq(const U& a, const U& b)
  {
    return a == b;
  }

  template <std::size_t... I>
  bool depsEqualImpl(const std::tuple<Deps...>& nd, std::index_sequence<I...>) const
  {
    return (eq(std::get<I>(deps_), std::get<I>(nd)) && ...);
  }
  bool depsEqual(const std::tuple<Deps...>& nd) const
  {
    return depsEqualImpl(nd, std::index_sequence_for<Deps...>{});
  }

  std::optional<T> value_;
  std::tuple<Deps...> deps_{};
};


}  // namespace M7


}  // namespace WaveSabreCore
