#pragma once

#include <iostream>
#include <span>
#include <vector>

class BinaryReader {
public:
  explicit BinaryReader(std::span<const uint8_t> data) : _data{data} {}

  uint8_t read_uint8();
  uint16_t read_uint16();
  uint32_t read_uint32();
  template <typename T> T read();

  template <typename T> std::vector<T> loop(uint32_t times);
  template <typename T> void loop(uint32_t times, std::vector<T> &buffer);

  void skip(size_t bytes);
  std::string read_string(size_t char_count);

  size_t tell();
  void seek(size_t pos);

private:
  std::span<const uint8_t> _data;
  size_t position{0};
};

template <typename T> std::vector<T> BinaryReader::loop(uint32_t times) {
  std::vector<T> result;
  result.reserve(times);

  for (uint32_t i = 0; i < times; ++i) {
    result.emplace_back(read<T>());
  }

  return result;
}

template <typename T> void BinaryReader::loop(uint32_t times, std::vector<T> &buffer) {
  for (uint32_t i = 0; i < times; ++i) {
    buffer.push_back(read<T>());
  }
}