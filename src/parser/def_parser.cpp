#include <array>
#include <fstream>
#include <string>
#include <unordered_set>
#include <vector>

#include "BinaryDataView.h"
#include "PcxDecoder.h"
#include "def_parser.h"

#include "lib/def_types.h"
#include "lib/palette.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace fs = std::filesystem;

namespace {
  static int frame_name_length{13};

  struct DefHeader {
    uint32_t type;
    uint32_t frame_width;
    uint32_t frame_height;
    uint32_t group_count;
  };

  std::vector<Palette::Color> extract_palette(BinaryDataView &buffer) {
    std::vector<Palette::Color> palette{};

    for (size_t i = 0; i < 256; i++) {
      palette.push_back({buffer.read_ui8(), buffer.read_ui8(), buffer.read_ui8(), 255});

      if (Palette::predefined_palette_indices.contains(i))
        palette[i] = Palette::predefined_palette_indices.at(i);
    }

    return palette;
  }

  std::string get_animation_group_name(uint32_t def_type, uint32_t group_type) {
    if (!DEF::animation_groups.contains(def_type)) {
      if (group_type == 0)
        return "all";

      std::cerr << "Unknown animation type";
      return " ";
    }

    if (DEF::animation_groups.at(def_type).size() > group_type)
      return DEF::animation_groups.at(def_type).at(group_type);
    else
      std::cerr << "Unknown animation type";

    return " ";
  }

  std::vector<std::string> get_frame_names(BinaryDataView &buffer, uint32_t frame_count) {
    std::vector<std::string> frame_names{};

    for (size_t i = 0; i < frame_count; ++i) {
      std::string name = buffer.read_string(frame_name_length);

      if (auto pos = name.find('\0'); pos != std::string::npos)
        name.resize(pos);

      frame_names.push_back(name);
    }

    return frame_names;
  }

  std::vector<uint32_t> get_frame_offsets(BinaryDataView &buffer, uint32_t frame_count) {
    std::vector<uint32_t> frame_offsets{};

    for (size_t i = 0; i < frame_count; ++i) {
      frame_offsets.push_back(buffer.read_le_ui32());
    }

    return frame_offsets;
  }

  std::vector<uint32_t> get_image_sizes(BinaryDataView &buffer, std::vector<uint32_t> &offsets) {
    std::vector<uint32_t> sizes{};

    for (size_t i = 0; i < offsets.size(); ++i) {
      buffer.seek(offsets[i]);
      sizes.push_back(buffer.read_le_ui32() + 32);
    }

    return sizes;
  }

  std::string create_def_folder(std::string_view filename) {
    fs::path path(filename);
    std::string folder_name = path.stem().string();

    fs::create_directory("output/" + folder_name);

    return folder_name;
  }

  fs::path create_animation_group_folder(std::string_view def_folder_name,
                                         std::string_view group_folder_name) {
    fs::path group_path = fs::path("output") / def_folder_name / group_folder_name;
    fs::create_directories(group_path);

    return group_path;
  }

  void combine_images_data(std::unordered_map<std::string, PcxDecoder::Image> &images,
                           std::vector<std::string> &frame_names,
                           DefHeader &def_header,
                           fs::path &folder

  ) {
    std::vector<uint8_t> result;
    for (size_t i = 0; i < def_header.frame_height; ++i) {
      for (size_t j = 0; j < frame_names.size(); ++j) {
        result.insert(result.end(),
                      images[frame_names[j]].bitmap.begin() + def_header.frame_width * i * 4,
                      images[frame_names[j]].bitmap.begin() + def_header.frame_width * (i + 1) * 4);
      }
    }

    fs::path path{folder / "sprite.png"};

    stbi_write_png(path.c_str(), def_header.frame_width * frame_names.size(),
                   def_header.frame_height, 4, result.data(),
                   def_header.frame_width * frame_names.size() * 4);
  }

  void parse_def_groups(BinaryDataView &buffer,
                        DefHeader &def_header,
                        uint32_t file_type,
                        std::string_view def_folder_name) {
    std::vector<Palette::Color> palette{extract_palette(buffer)};

    for (size_t i = 0; i < def_header.group_count; ++i) {
      std::string animation_group_name{get_animation_group_name(file_type, buffer.read_le_ui32())};
      uint32_t frame_count = buffer.read_le_ui32();
      buffer.skip(8); // unknown

      std::vector<std::string> frame_names{get_frame_names(buffer, frame_count)};
      std::vector<uint32_t> frame_offsets{get_frame_offsets(buffer, frame_count)};

      const auto start_offset = buffer.tell();
      std::vector<uint32_t> image_sizes{get_image_sizes(buffer, frame_offsets)};
      buffer.seek(start_offset);

      fs::path group_path = create_animation_group_folder(def_folder_name, animation_group_name);

      std::unordered_map<std::string, PcxDecoder::Image> images{};
      std::vector<std::string> group_frames{};
      for (size_t i = 0; i < frame_count; i++) {
        group_frames.push_back(frame_names[i]);

        if (images.contains(frame_names[i]))
          continue;

        const auto start_offset = buffer.tell();
        buffer.seek(frame_offsets[i]);

        PcxDecoder pcx_decoder{palette};

        const PcxDecoder::Image image = pcx_decoder.decode(buffer);
        images[frame_names[i]] = image;

        // fs::path path = {group_path / frame_names[i]};
        // path.replace_extension(".png");

        // stbi_write_png(path.c_str(), def_header.frame_width, def_header.frame_height, 4,
        //                image_data.raw_data.data(), def_header.frame_width * 4);

        buffer.seek(start_offset);
      }

      combine_images_data(images, frame_names, def_header, group_path);
    }
  }
} // namespace

void parse_def(std::vector<uint8_t> &content, uint32_t file_type, std::string_view filename) {
  BinaryDataView buffer{content};

  DefHeader def_header;
  def_header.type = buffer.read_le_ui32();
  def_header.frame_width = buffer.read_le_ui32();
  def_header.frame_height = buffer.read_le_ui32();
  def_header.group_count = buffer.read_le_ui32();

  std::string def_folder_name = create_def_folder(filename);
  parse_def_groups(buffer, def_header, file_type, def_folder_name);
}
