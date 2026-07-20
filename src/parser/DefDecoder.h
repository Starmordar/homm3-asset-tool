#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "BinaryDataView.h"
#include "FileFormats.h"

#include "lib/palette.h"

class DefDecoder {
private:
  static constexpr int image_name_length{13};

  FileFormats::Def::DefHeader def_header_;
  std::array<Palette::Color, 256> palette_;

  void extract_header_data(BinaryDataView &buffer);
  void extract_palette(BinaryDataView &buffer);
  // Legacy DEF frames don't have `width`, `height`, `x`, `y` in header
  bool is_legacy_def(BinaryDataView &buffer,
                     std::vector<uint32_t> &image_sizes,
                     std::vector<uint32_t> &image_offsets);

  std::string get_animation_group_name(uint32_t group_type);
  std::vector<std::string> get_image_names(BinaryDataView &buffer, uint32_t frame_count);
  std::vector<uint32_t> get_image_offsets(BinaryDataView &buffer, uint32_t frame_count);
  std::vector<uint32_t> get_image_sizes(BinaryDataView &buffer, std::vector<uint32_t> &offsets);
  FileFormats::Def::DefFile decode_animation_groups(BinaryDataView &buffer);

public:
  DefDecoder() = default;
  FileFormats::Def::DefFile decode(FileFormats::Lod::EntryFile &entry_file);
};