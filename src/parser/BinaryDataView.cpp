#include "BinaryDataView.h"

#include <bit>
#include <vector>

void BinaryDataView::ensure_data_available(size_t byte_count) {
  if (byte_count > _bytes.size() - byte_offset)
    throw std::out_of_range("BinaryDataView: not enough data");
}

uint16_t BinaryDataView::read_unaligned_ui16() {
  ensure_data_available(2);

  uint16_t value;
  std::memcpy(&value, _bytes.data() + byte_offset, sizeof(value));
  byte_offset += 2;

  return value;
}

uint32_t BinaryDataView::read_unaligned_ui32() {
  ensure_data_available(4);

  uint32_t value;
  std::memcpy(&value, _bytes.data() + byte_offset, sizeof(value));
  byte_offset += 4;

  return value;
}

uint8_t BinaryDataView::read_ui8() {
  ensure_data_available(1);

  uint8_t value;
  std::memcpy(&value, _bytes.data() + byte_offset, sizeof(value));
  byte_offset += 1;

  return value;
}

uint16_t BinaryDataView::read_le_ui16() {
  uint16_t value = read_unaligned_ui16();

  if (std::endian::native == std::endian::big)
    value = std::byteswap(value);

  return value;
}

uint32_t BinaryDataView::read_le_ui32() {
  uint32_t value = read_unaligned_ui32();

  if (std::endian::native == std::endian::big)
    value = std::byteswap(value);

  return value;
}

void BinaryDataView::skip(size_t byte_count) {
  ensure_data_available(byte_count);
  byte_offset += byte_count;
}

std::string BinaryDataView::read_string(size_t char_count) {
  ensure_data_available(char_count);

  const auto first = _bytes.begin() + byte_offset;
  std::string value{first, first + char_count};

  byte_offset += char_count;
  return value;
}

size_t BinaryDataView::tell() {
  return byte_offset;
}

void BinaryDataView::seek(size_t offset) {
  if (offset > _bytes.size())
    throw std::out_of_range("BinaryDataView: seek out of range");

  byte_offset = offset;
}