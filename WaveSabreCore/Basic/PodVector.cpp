
#include "PodVector.hpp"

#include <cstring>
#include <stdlib.h>

namespace WaveSabreCore::M7
{

PodBuffer::PodBuffer(size_t elementSize)
    : mElementSize(elementSize)
{
}

PodBuffer::~PodBuffer()
{
  ::free(mData);
}

PodBuffer& PodBuffer::operator=(const PodBuffer& other) noexcept
{
  Clear();
  AppendRange(other.mData, other.mSizeElements);
  return *this;
}

void PodBuffer::allocate(size_t newCapacityElements)
{
  //auto newData = (uint8_t*)::malloc(newCapacityElements * mElementSize);
  auto newData = (uint8_t*)::calloc(newCapacityElements, mElementSize);  // zero-initialize, unconditionally. byte savings.
  //::memset(newData, 0, newCapacityElements * mElementSize);  // zero-initialize, unconditionally. byte savings.
  ::memcpy(newData, mData, mSizeElements * mElementSize);
  ::free(mData);
  mData = newData;
  mCapacityElements = newCapacityElements;
}

void PodBuffer::ReserveElements(size_t n)
{
  if (n > mCapacityElements)
  {
    allocate(n);
  }
}

void PodBuffer::ResizeElements(size_t n)
{
  ReserveElements(n);
  mSizeElements = n;
}

void PodBuffer::AppendRange(const void* values, size_t count)
{
  size_t requiredSize = mSizeElements + count;
  ReserveElements(requiredSize);
  std::memcpy(mData + (mSizeElements * mElementSize), values, count * mElementSize);
  mSizeElements = requiredSize;
}

}  // namespace WaveSabreCore::M7
