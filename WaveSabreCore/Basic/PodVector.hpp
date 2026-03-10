#pragma once

#include <cstdint>
#include <initializer_list>
#include <type_traits>

namespace WaveSabreCore::M7
{

// represents the underlying memory buffer, used by PodVector.
// main motivation for this is that it's not templated, so ANY non-trivial vector
// methods should be implemented here, and not in PodVector.
struct PodBuffer
{
private:
  const size_t mElementSize = 0;
  uint8_t* mData = nullptr;
  size_t mSizeElements = 0;
  size_t mCapacityElements = 0;

  void allocate(size_t newCapacityElements);

public:
  ~PodBuffer();
  explicit PodBuffer(size_t elementSize);
  PodBuffer(const PodBuffer& other) = delete;
  PodBuffer(PodBuffer&& other) = delete;
  PodBuffer& operator=(PodBuffer&& other) = delete;

  PodBuffer& operator=(const PodBuffer& other) noexcept;

  void ReserveElements(size_t n);
  void ResizeElements(size_t n);
  void PushBack(const void* value)
  {
    AppendRange(value, 1);
  }
  void AppendRange(const void* values, size_t count);
  void Clear()
  {
    mSizeElements = 0;
  }

  bool Empty() const
  {
    return mSizeElements == 0;
  }
  size_t SizeElements() const
  {
    return mSizeElements;
  }
  size_t SizeBytes() const
  {
    return mSizeElements * mElementSize;
  }
  size_t CapacityElements() const
  {
    return mCapacityElements;
  }

  template <typename T>
  const T* Data() const
  {
    static_assert(std::is_trivially_copyable_v<T>, "PodBuffer can only be used with trivially copyable types");
    return reinterpret_cast<const T*>(mData);
  }

  template <typename T>
  T* Data()
  {
    static_assert(std::is_trivially_copyable_v<T>, "PodBuffer can only be used with trivially copyable types");
    return reinterpret_cast<T*>(mData);
  }
};


// vector implementation for trivially-copyable types.
// - favor small compiled binary size.
template <typename T>
struct PodVector
{
  static_assert(std::is_trivially_copyable_v<T>, "PodVector can only be used with trivially copyable types");
  PodBuffer mBuffer;

  PodVector()
      : mBuffer(sizeof(T))
  {
  }
  PodVector(std::initializer_list<T> list)
      : mBuffer(sizeof(T))
  {
    mBuffer.AppendRange(list.begin(), list.size());
  }
  size_t size() const
  {
    return mBuffer.SizeElements();
  }
  const T& operator[](size_t i) const
  {
    return mBuffer.Data<T>()[i];
  }
  T& operator[](size_t i)
  {
    return mBuffer.Data<T>()[i];
  }
  void reserve(size_t n)
  {
    mBuffer.ReserveElements(n);
  }
  void resize(size_t n)
  {
    mBuffer.ResizeElements(n);
  }
  void clear()
  {
    mBuffer.Clear();
  }
  bool empty() const
  {
    return mBuffer.Empty();
  }
  T* data()
  {
    return mBuffer.Data<T>();
  }
  const T* data() const
  {
    return mBuffer.Data<T>();
  }
  void push_back(const T& value)
  {
    mBuffer.PushBack(&value);
  }

  // begin / end impl
  using iterator = T*;
  using const_iterator = const T*;
  T* begin()
  {
    return mBuffer.Data<T>();
  }
  T* end()
  {
    return mBuffer.Data<T>() + mBuffer.SizeElements();
  }
  const T* begin() const
  {
    return mBuffer.Data<T>();
  }
  const T* end() const
  {
    return mBuffer.Data<T>() + mBuffer.SizeElements();
  }

};  // struct PodVector

template <typename T>
struct PodSpan
{
private:
  const T* mData = nullptr;
  size_t mSizeElements = 0;

public:

  explicit PodSpan(const T* data, size_t sizeElements)
      : mData(data)
      , mSizeElements(sizeElements)
  {
  }
  const T* data() const
  {
    static_assert(std::is_trivially_copyable_v<T>, "Span can only be used with trivially copyable types");
    return mData;
  }
  size_t size() const
  {
    return mSizeElements;
  }
};

}  // namespace WaveSabreCore::M7
