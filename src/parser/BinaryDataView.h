#pragma once

#include <iostream>
#include <span>
#include <vector>

class BinaryDataView {
private:
  std::span<const uint8_t> _bytes;
  size_t byte_offset{0};

  uint16_t read_unaligned_ui16();
  uint32_t read_unaligned_ui32();
  void ensure_data_available(size_t byte_count);
  template <typename T> T read_impl();

public:
  explicit BinaryDataView(std::span<const uint8_t> data) : _bytes{data} {}

  uint8_t read_ui8();
  uint16_t read_le_ui16();
  uint32_t read_le_ui32();

  void skip(size_t byte_count);
  std::string read_string(size_t char_count);
  size_t tell();
  size_t size();
  void seek(size_t offset);
  template <typename T> std::vector<T> loop(size_t times);
  template <typename T> void loop(size_t times, std::vector<T> &buffer);
};

template <typename T> T BinaryDataView::read_impl() {
  if constexpr (std::is_same<T, uint8_t>::value)
    return read_ui8();
  else if constexpr (std::is_same<T, uint16_t>::value)
    return read_le_ui16();
  else if constexpr (std::is_same<T, uint32_t>::value)
    return read_le_ui32();
}

template <typename T> std::vector<T> BinaryDataView::loop(size_t times) {
  std::vector<T> value;
  value.reserve(times);

  for (size_t i = 0; i < times; ++i) {
    value.emplace_back(read_impl<T>());
  }

  return value;
}

template <typename T> void BinaryDataView::loop(size_t times, std::vector<T> &buffer) {
  for (size_t i = 0; i < times; ++i) {
    buffer.push_back(read_impl<T>());
  }
}