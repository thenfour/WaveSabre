#pragma once

#include "Base.hpp"
#include "../Basic/Math.hpp"

namespace WaveSabreCore
{
namespace M7
{

template <typename Tfirst, typename Tsecond>
struct Pair
{
  Tfirst first;
  Tsecond second;
};

template <typename T, std::enable_if_t<std::is_floating_point_v<T>, int> = 0>
struct TFloatPair
{
  TFloatPair() = default;
  TFloatPair(T x0, T x1)
      : x{x0, x1}
  {
  }

  T x[2] = {0};
  T& operator[](size_t i)
  {
    return x[i];
  }
  const T& operator[](size_t i) const
  {
    return x[i];
  }
  [[nodiscard]]
  TFloatPair<T> add(const TFloatPair<T>& m) const
  {
    return {x[0] + m.x[0], x[1] + m.x[1]};
  }
  [[nodiscard]]
  TFloatPair<T> sub(const TFloatPair<T>& m) const
  {
    return {x[0] - m.x[0], x[1] - m.x[1]};
  }
  [[nodiscard]]
  TFloatPair<T> mul(T m) const
  {
    return {x[0] * m, x[1] * m};
  }
  [[nodiscard]]
  TFloatPair<T> mul(const TFloatPair<T>& m) const
  {
    return {x[0] * m.x[0], x[1] * m.x[1]};
  }

  [[nodiscard]]
  TFloatPair<T> operator+(const TFloatPair<T>& m) const
  {
    return add(m);
  }

  [[nodiscard]]
  TFloatPair<T> operator-(const TFloatPair<T>& m) const
  {
    return sub(m);
  }

  [[nodiscard]]
  TFloatPair<T> operator*(T m) const
  {
    return mul(m);
  }

  TFloatPair<T>& operator*=(T m)
  {
    x[0] *= m;
    x[1] *= m;
    return *this;
  }

  TFloatPair<T>& operator+=(const TFloatPair<T>& m)
  {
    x[0] += m.x[0];
    x[1] += m.x[1];
    return *this;
  }

  TFloatPair<T>& Accumulate(const T m)
  {
    x[0] += m;
    x[1] += m;
    return *this;
  }
  TFloatPair<T>& Accumulate(const TFloatPair<T>& m)
  {
    x[0] += m.x[0];
    x[1] += m.x[1];
    return *this;
  }

  [[nodiscard]]
  TFloatPair<T> yx() const
  {
    return {x[1], x[0]};
  }

  [[nodiscard]]
  T Left() const
  {
    return x[0];
  }
  [[nodiscard]]
  T Right() const
  {
    return x[1];
  }
  [[nodiscard]]
  T Mid() const
  {
    return x[0];
  }
  [[nodiscard]]
  T Side() const
  {
    return x[1];
  }
  [[nodiscard]]
  T Amp() const
  {
    return x[0];
  }
  [[nodiscard]]
  T Slope() const
  {
    return x[1];
  }

  void Clear()
  {
    x[0] = 0;
    x[1] = 0;
  }
  // hm this is a weird function actually... not sure it should be expressed this way.
  static TFloatPair<T> Mix(const TFloatPair<T>& a, const TFloatPair<T>& b, T aLin, T bLin);

  [[nodiscard]]
  TFloatPair<T> MSEncode() const
  {
    return {(x[0] + x[1]) * math::gSqrt2Recip, (x[0] - x[1]) * math::gSqrt2Recip};
  }

  [[nodiscard]]
  TFloatPair<T> MSDecode() const
  {
    return MSEncode();
  }

  //
  [[nodiscard]]
  TFloatPair<T> MidSideMixOnStereo(T midSideN11) const
  {
    auto ms = MSEncode();
    if (midSideN11 < 0)
    {
      ms.x[1] *= (midSideN11 + 1);  // reduce side when negative
    }
    else if (midSideN11 > 0)
    {
      ms.x[0] *= (1 - midSideN11);  // reduce mid when positive
    }
    //M7::MSDecode(mid, side, &output.x[0], &output.x[1]);
    return ms.MSDecode();
  }

  //template<typename T2, std::enable_if_t<std::is_floating_point_v<T2, int> = 0>
  template <typename T2>
  [[nodiscard]]
  TFloatPair<T2> CastTo() const
  {
    return {static_cast<T2>(x[0]), static_cast<T2>(x[1])};
  }
};

using FloatPair = TFloatPair<float>;
using DoublePair = TFloatPair<double>;

INLINE FloatPair SinCosF(float angle)
{
  return {math::sin(angle), math::cos(angle)};
}

INLINE DoublePair SinCosD(double angle)
{
  return {math::sin(angle), math::cos(angle)};
}

}  // namespace M7


}  // namespace WaveSabreCore
