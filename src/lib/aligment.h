#pragma once

#include <bit>
#include <concepts>

#if defined(__clang__) || defined(__GNUC__) || defined(_MSC_VER)

#if defined(_MSC_VER)
#define PACKED_STRUCT_START __pragma(pack(push, 1))
#define PACKED_STRUCT_END __pragma(pack(pop))

#else
#define PACKED_STRUCT_START
#define PACKED_STRUCT_END __attribute__((packed))

#endif

#endif

template <std::integral T> inline T little_to_native(T value) {
  if constexpr (std::endian::native == std::endian::big)
    value = std::byteswap(value);

  return value;
}
