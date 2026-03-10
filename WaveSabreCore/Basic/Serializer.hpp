#pragma once

#include "Math.hpp"
#include "Pair.hpp"
#include "DSPMath.hpp"


namespace WaveSabreCore
{
namespace M7
{

struct Serializer
{
  uint8_t* mBuffer = nullptr;
  size_t mAllocatedSize = 0;
  size_t mSize = 0;

  ~Serializer();

  static void FreeBuffer(void* p)
  {
    delete[] p;
  }

  Pair<uint8_t*, size_t> DetachBuffer();
  uint8_t* GrowBy(size_t n);
  void WriteUByte(uint8_t b);

  void WriteFloat(float f);
  void WriteUInt32(uint32_t f);
  void WriteBuffer(const uint8_t* buf, size_t n);

  void WriteInt16NormalizedFloat(float f)
  {
    auto p = (int16_t*)(GrowBy(sizeof(int16_t)));
    *p = math::FloatN11ToDefault16(f);
  }
};

// attaches to some buffer
struct Deserializer
{
  const uint8_t* mpData;
  const uint8_t* mpCursor;
  //const uint8_t* mpEnd;
  //size_t mSize;
  explicit Deserializer(const uint8_t* p);
  //int8_t ReadSByte() {
  //    int8_t ret = *((int8_t*)mpCursor);
  //    mpCursor += sizeof(ret);
  //    return ret;
  //}
  uint8_t ReadUByte();

  //int16_t ReadSWord() {
  //    int16_t ret = *((int16_t*)mpCursor);
  //    mpCursor += sizeof(ret);
  //    return ret;
  //}
  //uint16_t ReadUWord() {
  //    uint16_t ret = *((uint16_t*)mpCursor);
  //    mpCursor += sizeof(ret);
  //    return ret;
  //}

  //  uint32_t ReadUInt32();
  uint32_t ReadUInt32();

  //  uint32_t ReadVarUInt32();
  uint32_t ReadVarUInt32();

  //  float ReadFloat();
  float ReadFloat();
  double ReadDouble();
  // returns a new cursor in the out buffer
  void ReadBuffer(void* out, size_t numbytes);

  float ReadInt16NormalizedFloat();

  //ByteBitfield ReadByteBitfield() {
  //    return ByteBitfield{ ReadUByte() };
  //}
  //WordBitfield ReadWordBitfield() {
  //    return WordBitfield{ ReadUWord() };
  //}
};

}  // namespace M7


}  // namespace WaveSabreCore
