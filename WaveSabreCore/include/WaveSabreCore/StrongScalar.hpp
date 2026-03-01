#pragma once


namespace WaveSabreCore
{
namespace M7
{

template <typename T, typename Tag>
struct StrongScalar
{
  T value;
  explicit constexpr StrongScalar(T val_)
      : value(val_)
  {
  }

  // default copy/move ctor and assignment are fine.
  StrongScalar() = delete;
  StrongScalar(const StrongScalar&) = default;
  StrongScalar(StrongScalar&&) = default;
  StrongScalar& operator=(const StrongScalar&) = default;
  StrongScalar& operator=(StrongScalar&&) = default;

  bool operator==(const StrongScalar<T, Tag>& other) const
  {
    return this->value == other.value;
  }

  bool operator!=(const StrongScalar<T, Tag>& other) const
  {
    return !(*this == other);
  }

  // multiply by scalar
  template<typename TotherScalar, std::enable_if_t<std::is_arithmetic_v<TotherScalar>, int> = 0>
  StrongScalar<T, Tag> operator*(TotherScalar scalar) const
  {
    return StrongScalar<T, Tag>(this->value * scalar);
  }
};

}  // namespace M7
}  // namespace WaveSabreCore