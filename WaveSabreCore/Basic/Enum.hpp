
#pragma once

namespace WaveSabreCore::M7
{
template <typename T>
T CombineFlags(std::initializer_list<T> flags)
{
  T result = static_cast<T>(0);
  for (T flag : flags)
  {
    result = static_cast<T>(static_cast<std::underlying_type_t<T>>(result) |
                            static_cast<std::underlying_type_t<T>>(flag));
  }
  return result;
}

template <typename T, typename... Ts>
constexpr T CombineFlags(T first, Ts... rest)
{
  static_assert(std::is_enum_v<T>, "T must be an enum");
  static_assert((std::is_same_v<T, Ts> && ...), "All flags must be the same enum type");

  using U = std::underlying_type_t<T>;
  U result = static_cast<U>(first);
  ((result |= static_cast<U>(rest)), ...);
  return static_cast<T>(result);
}

template <typename T>
bool HasFlag(T value, T flag)
{
  return (static_cast<std::underlying_type_t<T>>(value) & static_cast<std::underlying_type_t<T>>(flag)) != 0;
}

template <typename T>
T SetFlag(T value, T flag)
{
  return static_cast<T>(static_cast<std::underlying_type_t<T>>(value) | static_cast<std::underlying_type_t<T>>(flag));
}

template <typename T>
T ConditionalFlag(bool enabled, T flag)
{
  return enabled ? flag : static_cast<T>(0);
}

template <typename T>
T ClearFlag(T value, T flag)
{
  return static_cast<T>(static_cast<std::underlying_type_t<T>>(value) & ~static_cast<std::underlying_type_t<T>>(flag));
}

}  // namespace WaveSabreCore::M7