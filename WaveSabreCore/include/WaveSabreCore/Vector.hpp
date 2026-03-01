#pragma once

#include <memory>

namespace WaveSabreCore
{
namespace M7
{
template <typename T>
struct Vector
{
  std::unique_ptr<T[]> mData;
  size_t mSize = 0;
  size_t mCapacity = 0;

  Vector() = default;
  Vector(std::initializer_list<T> list)
  {
    mSize = list.size();
    mCapacity = mSize;
    mData = std::make_unique<T[]>(mCapacity);
    size_t i = 0;
    for (const auto& item : list)
    {
      mData[i++] = item;
    }
  }
  size_t size() const
  {
    return mSize;
  }
  const T& operator[](size_t i) const
  {
    return mData[i];
  }
  void reserve(size_t n)
  {
    if (n > mCapacity)
    {
      auto newData = std::make_unique<T[]>(n);
      for (size_t i = 0; i < mSize; ++i)
      {
        newData[i] = std::move(mData[i]);
      }
      mData = std::move(newData);
      mCapacity = n;
    }
  }
  void clear()
  {
    mSize = 0;
  }
  bool empty() const
  {
    return mSize == 0;
  }
  void push_back(const T& value)
  {
    if (mSize >= mCapacity)
    {
      reserve(mCapacity > 0 ? mCapacity * 2 : 4);
    }
    mData[mSize++] = value;
  }

  // begin / end impl
  using iterator = T*;
  using const_iterator = const T*;
  T* begin()
  {
    return mData.get();
  }
  T* end()
  {
    return mData.get() + mSize;
  }
  const T* begin() const
  {
    return mData.get();
  }
  const T* end() const
  {
    return mData.get() + mSize;
  }

  // void erase(const_iterator pos)
  // {
  //   if (pos >= begin() && pos < end())
  //   {
  //     size_t index = pos - begin();
  //     for (size_t i = index; i + 1 < mSize; ++i)
  //     {
  //       mData[i] = std::move(mData[i + 1]);
  //     }
  //     --mSize;
  //   }
  // }

  void erase(const_iterator first, const_iterator last)
  {
    if (first >= begin() && last <= end() && first <= last)
    {
      size_t indexFirst = first - begin();
      size_t indexLast = last - begin();
      size_t count = indexLast - indexFirst;
      for (size_t i = indexFirst; i + count < mSize; ++i)
      {
        mData[i] = std::move(mData[i + count]);
      }
      mSize -= count;
    }
  }
  
};
}  // namespace M7

}  // namespace WaveSabreCore
