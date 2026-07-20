#include <cassert>
#include <filesystem>
#include <unordered_map>
#include <vector>

#include "BinaryDataView.h"
#include "DefDecoder.h"
#include "FileFormats.h"
#include "PcxDecoder.h"

#include "lib/file_types.h"
#include "lib/palette.h"

std::string DefDecoder::get_animation_group_name(uint32_t group_type) {
  if (!FileTypes::animation_groups.contains(def_header_.type)) {
    if (group_type == 0)
      return "all";

    std::cerr << "DefDecoder: Unknown animation type";
    return "";
  }

  if (FileTypes::animation_groups.at(def_header_.type).size() > group_type)
    return FileTypes::animation_groups.at(def_header_.type).at(group_type);
  else
    std::cerr << "DefDecoder: Unknown animation type";

  return "";
}

std::vector<std::string> DefDecoder::get_image_names(BinaryDataView &buffer, uint32_t frame_count) {
  std::vector<std::string> image_names{};

  for (size_t i = 0; i < frame_count; ++i) {
    std::string name = buffer.read_string(image_name_length);

    if (auto pos = name.find('\0'); pos != std::string::npos)
      name.resize(pos);

    image_names.push_back(name);
  }

  return image_names;
}

std::vector<uint32_t> DefDecoder::get_image_offsets(BinaryDataView &buffer, uint32_t frame_count) {
  std::vector<uint32_t> image_offsets = buffer.loop<uint32_t>(frame_count);
  return image_offsets;
}

std::vector<uint32_t> DefDecoder::get_image_sizes(BinaryDataView &buffer,
                                                  std::vector<uint32_t> &offsets) {
  std::vector<uint32_t> sizes{};

  for (size_t i = 0; i < offsets.size(); ++i) {
    buffer.seek(offsets[i]);
    sizes.push_back(buffer.read_le_ui32() + 32);
  }

  return sizes;
}

bool DefDecoder::is_legacy_def(BinaryDataView &buffer,
                               std::vector<uint32_t> &image_sizes,
                               std::vector<uint32_t> &image_offsets) {
  for (size_t i = 0; i < image_sizes.size(); i++) {
    if (image_sizes[i] + image_offsets[i] > buffer.size())
      return true;
  }

  return false;
}

FileFormats::Def::DefFile DefDecoder::decode_animation_groups(BinaryDataView &buffer) {
  FileFormats::Def::DefFile def_file;

  for (size_t i = 0; i < def_header_.group_count; ++i) { // Decode specific animation group
    std::string animation_group_name{get_animation_group_name(buffer.read_le_ui32())};
    if (animation_group_name.empty()) // In case of unknown animation show the error and continue
      continue;

    uint32_t frame_count = buffer.read_le_ui32();
    buffer.skip(8); // unknown

    std::vector<std::string> image_names{get_image_names(buffer, frame_count)};
    std::vector<uint32_t> image_offsets{get_image_offsets(buffer, frame_count)};

    const auto start_offset = buffer.tell();
    std::vector<uint32_t> image_sizes{get_image_sizes(buffer, image_offsets)};
    buffer.seek(start_offset);

    bool is_legacy_format = is_legacy_def(buffer, image_sizes, image_offsets);

    for (size_t i = 0; i < frame_count; i++) { // Decode each animation group
      def_file.groups[animation_group_name].push_back(image_names[i]);

      if (def_file.images.contains(image_names[i]))
        continue;

      const auto start_offset = buffer.tell();
      buffer.seek(image_offsets[i]);

      PcxDecoder pcx_decoder{palette_, is_legacy_format};

      const FileFormats::Def::PcxImage image = pcx_decoder.decode(buffer);
      def_file.images[image_names[i]] = image;

      buffer.seek(start_offset);
    }
  }

  return def_file;
}

void DefDecoder::extract_header_data(BinaryDataView &buffer) {
  uint32_t type = buffer.read_le_ui32();
  assert(FileTypes::type_name_map.contains(type) &&
         "DefDecoder: unsupported file type (expected a .def file)");

  def_header_.type = FileTypes::type_name_map.at(type);
  def_header_.frame_width = buffer.read_le_ui32();
  def_header_.frame_height = buffer.read_le_ui32();
  def_header_.group_count = buffer.read_le_ui32();
}

void DefDecoder::extract_palette(BinaryDataView &buffer) {
  for (size_t i = 0; i < 256; i++) {
    palette_.push_back({buffer.read_ui8(), buffer.read_ui8(), buffer.read_ui8(), 255});

    if (Palette::predefined_palette_indices.contains(i))
      palette_[i] = Palette::predefined_palette_indices.at(i);
  }
}

FileFormats::Def::DefFile DefDecoder::decode(FileFormats::Lod::EntryFile &entry_file) {
  std::cout << "Start decoding for: " << entry_file.header.name << '\n';
  BinaryDataView buffer{entry_file.data};

  extract_header_data(buffer);
  extract_palette(buffer);

  FileFormats::Def::DefFile def_file = decode_animation_groups(buffer);
  def_file.name = std::string(entry_file.header.name);
  def_file.header = def_header_;

  std::cout << "Finish decoding for: " << entry_file.header.name << '\n';

  return def_file;
}
