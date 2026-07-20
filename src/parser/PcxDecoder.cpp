#include <cassert>

#include "BinaryDataView.h"
#include "FileFormats.h"
#include "PcxDecoder.h"
#include "lib/palette.h"

using Image = FileFormats::Def::PcxImage;

void PcxDecoder::extract_image_meta(BinaryDataView &buffer) {
  image_.frame_width = buffer.read_le_ui32();
  image_.frame_height = buffer.read_le_ui32();

  if (is_legacy_format_) {
    image_.width = image_.frame_width;
    image_.height = image_.frame_height;

    image_.x = 0;
    image_.y = 0;
  } else {
    image_.width = buffer.read_le_ui32();
    image_.height = buffer.read_le_ui32();

    image_.x = buffer.read_le_ui32();
    image_.y = buffer.read_le_ui32();
  }
}

PaletteIndices PcxDecoder::get_palette_indices(BinaryDataView &buffer,
                                               uint32_t compression_format) {
  switch (static_cast<PcxDecoder::COMPRESSION_FORMAT>(compression_format)) {
  case PcxDecoder::COMPRESSION_FORMAT::PLAIN:
    return decompress_plain(buffer);
  case PcxDecoder::COMPRESSION_FORMAT::RLE:
    return decompress_rle(buffer);
  case PcxDecoder::COMPRESSION_FORMAT::RLE_HEIGHT_ORIENTED:
    return decompress_rle_height_oriented(buffer);
  case PcxDecoder::COMPRESSION_FORMAT::RLE_BLOCK_ORIENTED:
    return decompress_rle_block_oriented(buffer);
  default:
    throw std::runtime_error("PcxDecoder: unknown compression format");
  }
}

PaletteIndices PcxDecoder::decompress_plain(BinaryDataView &buffer) {
  PaletteIndices palette_indices{};
  palette_indices.reserve(image_.height * image_.width);

  buffer.loop(image_.height * image_.width, palette_indices);

  return palette_indices;
}

PaletteIndices PcxDecoder::decompress_rle(BinaryDataView &buffer) {
  PaletteIndices palette_indices{};
  palette_indices.reserve(image_.width * image_.height);

  const auto base_offset = buffer.tell();

  std::vector<uint32_t> offsets = buffer.loop<uint32_t>(image_.height);

  for (size_t i = 0; i < offsets.size(); ++i) {
    buffer.seek(base_offset + offsets[i]);

    uint32_t total_row_length = 0;
    while (total_row_length < image_.width) {
      uint8_t segment_type = buffer.read_ui8();
      uint32_t segment_length = buffer.read_ui8() + 1;

      if (segment_type == RLE_first_byte) { // store segment raw data
        buffer.loop<uint8_t>(segment_length, palette_indices);
      } else { // in this case segment_type is a color index, store it segment_length times
        palette_indices.insert(palette_indices.end(), segment_length, segment_type);
      }

      total_row_length += segment_length;
    }
  }

  return palette_indices;
}

PaletteIndices PcxDecoder::decompress_rle_height_oriented(BinaryDataView &buffer) {
  PaletteIndices palette_indices{};
  palette_indices.reserve(image_.width * image_.height);

  const auto base_offset = buffer.tell();
  uint16_t currect_offset = buffer.read_le_ui16();

  buffer.seek(base_offset + currect_offset);

  for (size_t i = 0; i < image_.height; ++i) {
    uint32_t total_row_length = 0;

    while (total_row_length < image_.width) {
      uint8_t segment = buffer.read_ui8();
      uint8_t segment_type = segment >> 5;         // get 3 first bits
      uint8_t segment_length = (segment & 31) + 1; // get last 5 bits

      if (segment_type == RLE_HEIGHT_ORIENTED_first_byte) { // store segment raw data
        buffer.loop<uint8_t>(segment_length, palette_indices);
      } else { // similar to RLE, segment_type is a color index, store it segment_length times
        palette_indices.insert(palette_indices.end(), segment_length, segment_type);
      }

      total_row_length += segment_length;
    }
  }

  return palette_indices;
}

PaletteIndices PcxDecoder::decompress_rle_block_oriented(BinaryDataView &buffer) {
  PaletteIndices palette_indices{};
  palette_indices.reserve(image_.width * image_.height);

  const auto base_offset = buffer.tell();

  for (size_t i = 0; i < image_.height; ++i) {
    // Jump to the offset address, offset is 16 bits, so i * 2 applied
    buffer.seek(base_offset + i * 2 * (image_.width / 32));
    uint16_t current_offset = buffer.read_le_ui16();
    buffer.seek(base_offset + current_offset);

    uint32_t total_row_length = 0;

    while (total_row_length < image_.width) {
      uint8_t segment = buffer.read_ui8();
      uint8_t segment_type = segment >> 5;         // get 3 first bits
      uint8_t segment_length = (segment & 31) + 1; // get last 5 bits

      if (segment_type == RLE_BLOCK_ORIENTED_first_byte) { // store segment raw data
        buffer.loop<uint8_t>(segment_length, palette_indices);
      } else { // similar to RLE, segment_type is a color index, store it segment_length times
        palette_indices.insert(palette_indices.end(), segment_length, segment_type);
      }

      total_row_length += segment_length;
    }
  }

  return palette_indices;
}

void PcxDecoder::extract_bitmap(BinaryDataView &buffer, PaletteIndices &palette_indices) {
  std::vector<uint8_t> &bitmap = image_.bitmap;
  bitmap.resize(image_.frame_width * image_.frame_height * 4, 0);

  // Offset the image within the frame by (x, y). Pixels not covered by the
  // image are left untouched and remain transparent, effectively padding the
  // image to the full frame size.
  for (size_t i = 0; i < image_.height; i++) {
    uint32_t start = (((i + image_.y) * image_.frame_width) + image_.x) * 4;

    for (size_t j = 0; j < image_.width; j++) {
      uint32_t index = palette_indices[i * image_.width + j];
      const auto &color{palette_[index]};

      uint32_t pixel_offset = start + j * 4;
      bitmap[pixel_offset] = color.r;
      bitmap[pixel_offset + 1] = color.g;
      bitmap[pixel_offset + 2] = color.b;
      bitmap[pixel_offset + 3] = color.a;
    }
  }
}

Image PcxDecoder::decode(BinaryDataView &buffer) {
  uint32_t image_size = buffer.read_le_ui32();
  uint32_t compression_format = buffer.read_le_ui32();
  extract_image_meta(buffer);

  const auto base_offset = buffer.tell();
  PaletteIndices palette_indices = get_palette_indices(buffer, compression_format);
  extract_bitmap(buffer, palette_indices);

  return image_;
}