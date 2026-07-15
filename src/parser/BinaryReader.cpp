#include "BinaryReader.h"
#include <vector>

uint32_t BinaryReader::read_uint32() {
  uint32_t result;

  std::memcpy(&result, _data.data() + position, sizeof(result));
  position += 4;
  return result;
}

uint16_t BinaryReader::read_uint16() {
  uint16_t result;

  std::memcpy(&result, _data.data() + position, sizeof(result));
  position += 2;
  return result;
}

uint8_t BinaryReader::read_uint8() {
  uint8_t result;

  std::memcpy(&result, _data.data() + position, sizeof(result));
  position += 1;
  return result;
}

template <> uint8_t BinaryReader::read<uint8_t>() {
  return read_uint8();
}

template <> uint16_t BinaryReader::read<uint16_t>() {
  return read_uint16();
}

template <> uint32_t BinaryReader::read<uint32_t>() {
  return read_uint32();
}

void BinaryReader::skip(size_t bytes) {
  position += bytes;
}

std::string BinaryReader::read_string(size_t char_count) {
  if (position + char_count > _data.size())
    throw std::out_of_range("Failed to extract string!");

  const auto first = _data.begin() + position;
  std::string result{first, first + char_count};

  position += char_count;
  return result;
}

size_t BinaryReader::tell() {
  return position;
}

void BinaryReader::seek(size_t pos) {
  if (pos > _data.size())
    throw std::out_of_range("Failed to seek!");

  position = pos;
}