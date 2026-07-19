#pragma once

#include <cstdint>
#include <optional>
#include <vector>

#include "lib/palette.h"
#include "parser/BinaryDataView.h"

using PaletteIndices = std::vector<uint8_t>;

class PcxDecoder {
public:
  struct Image {
    uint32_t frame_width;  // full width of frame including border
    uint32_t frame_height; // full height of frame including border

    uint32_t width;  // width of pixel data without borders
    uint32_t height; // height of pixel data without borders

    uint32_t x; // left margin
    uint32_t y; // top margin

    std::vector<uint8_t> bitmap; // pixel data in a RGBA vector
  };

private:
  enum class COMPRESSION_FORMAT {
    PLAIN = 0,
    RLE = 1,
    RLE_HEIGHT_ORIENTED = 2,
    RLE_BLOCK_ORIENTED = 3
  };

  static constexpr uint8_t RLE_first_byte{0xFF};
  static constexpr uint8_t RLE_HEIGHT_ORIENTED_first_byte{0x7};
  static constexpr uint8_t RLE_BLOCK_ORIENTED_first_byte{0x7};

  std::vector<Palette::Color> palette_{};
  Image image_{};

  void extract_image_meta(BinaryDataView &buffer);
  PaletteIndices get_palette_indices(BinaryDataView &buffer, uint32_t compression_format);

  // data is not encoded, store as is
  PaletteIndices decompress_plain(BinaryDataView &buffer);
  // Run-length encoding rows, used by most creature C*.def files
  // height * uint32 offsets, 0xFF segment_type means raw pixel data,
  // otherwise RLE encoded colors
  PaletteIndices decompress_rle(BinaryDataView &buffer);
  // Used by .def files that using a lot shadow/transparent colors (e.g. rivers)
  // height * uint16 offsets, keeps single byte to store both segment_type and length information
  // 3 bits is for the segment type, 5 bits for it length, so 2^5 (32 pixel) blocks
  PaletteIndices decompress_rle_height_oriented(BinaryDataView &buffer);
  // The encoding is identical to the decompress_rle_height_oriented
  // except this type is block-oriented encoding, with 32 pixel blocks
  // segments are still 3 bits is for type, 5 bits for it length
  PaletteIndices decompress_rle_block_oriented(BinaryDataView &buffer);

  void extract_bitmap(BinaryDataView &buffer, PaletteIndices &palette_indices);

public:
  explicit PcxDecoder(std::vector<Palette::Color> palette) : palette_(palette) {};

  Image decode(BinaryDataView &buffer);
};