
#include "Serializer.hpp"

namespace WaveSabreCore
{
namespace M7
{



uint8_t* Serializer::GrowBy(size_t n)
{
  size_t oldSize = mBuffer.SizeElements();
  size_t newSize = oldSize + n;
  mBuffer.ResizeElements(newSize);
  return mBuffer.Data<uint8_t>() + oldSize;
}

void Serializer::WriteUByte(uint8_t b)
{
  *GrowBy(sizeof(b)) = b;
}
void Serializer::WriteFloat(float f)
{
  auto p = (float*)(GrowBy(sizeof(f)));
  *p = f;
}
void Serializer::WriteUInt32(uint32_t f)
{
  auto p = (uint32_t*)(GrowBy(sizeof(f)));
  *p = f;
}
void Serializer::WriteBuffer(const uint8_t* buf, size_t n)
{
  auto p = GrowBy(n);
  memcpy(p, buf, n);
}


Deserializer::Deserializer(const uint8_t* p)
    : mpData(p)
    , mpCursor(p)  //,
                   //mSize(n),
                   //mpEnd(p + n)
{
}

uint8_t Deserializer::ReadUByte()
{
  uint8_t ret = *((uint8_t*)mpCursor);
  mpCursor += sizeof(ret);
  return ret;
}

uint32_t Deserializer::ReadUInt32()
{
  uint32_t ret = *((uint32_t*)mpCursor);
  mpCursor += sizeof(ret);
  return ret;
}

float Deserializer::ReadFloat()
{
  float ret = *((float*)mpCursor);
  mpCursor += sizeof(ret);
  return ret;
}

double Deserializer::ReadDouble()
{
  double ret = *((double*)mpCursor);
  mpCursor += sizeof(ret);
  return ret;
}

// returns a new cursor in the out buffer
void Deserializer::ReadBuffer(void* out, size_t numbytes)
{
  memcpy(out, mpCursor, numbytes);
  mpCursor += numbytes;
}

uint32_t Deserializer::ReadVarUInt32()
{
  uint32_t value = 0;
  uint32_t shift = 0;
  // byte stream: [c-------]...
  uint8_t b = 0;
  //do {
  //	b = ReadUByte();
  //	value |= static_cast<uint32_t>(b >> 1) << shift;
  //	shift += 7;
  //} while (!!(b & 1));
  do
  {
    b = ReadUByte();
    value |= static_cast<uint32_t>(b & 0x7f) << shift;
    shift += 7;
  } while ((b & 0x80) != 0);
  return value;
}

float Deserializer::ReadInt16NormalizedFloat()
{
  int16_t ret = *((int16_t*)mpCursor);
  mpCursor += sizeof(ret);
  return math::Default16ToFloatN11(ret);
  //return math::Sample16To32Bit(ret);
  //return math::clampN11(float(ret) / 32768);
}


}  // namespace M7
}  // namespace WaveSabreCore
