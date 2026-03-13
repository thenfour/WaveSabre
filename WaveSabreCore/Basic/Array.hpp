#pragma once

#include <cstdint>
#include <initializer_list>
#include <type_traits>

namespace WaveSabreCore::M7
{

template <typename T, size_t N>
struct PodArray
{
  T mData[N];

  PodArray()
  {
    Zero();
  }

  const T& operator[](size_t i) const
  {
    static_assert(std::is_trivially_copyable_v<T>, "PodArray can only be used with trivially copyable types");
    return mData[i];
  }
  T& operator[](size_t i)
  {
    static_assert(std::is_trivially_copyable_v<T>, "PodArray can only be used with trivially copyable types");
    return mData[i];
  }

  void Zero()
  {
    static_assert(std::is_trivially_copyable_v<T>, "PodArray can only be used with trivially copyable types");
    std::memset(mData, 0, sizeof(mData));
  }

  size_t Size() const
  {
    return N;
  }
};

}  // namespace WaveSabreCore::M7
