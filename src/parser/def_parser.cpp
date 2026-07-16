#include <array>
#include <fstream>
#include <string>
#include <unordered_set>
#include <vector>

#include "BinaryDataView.h"
#include "def_parser.h"

#include "lib/def_types.h"
#include "lib/palette.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace fs = std::filesystem;

namespace {
  static int frame_name_length{13};
  constexpr uint8_t format_1_RLE_first_byte{0xFF};
  constexpr uint8_t format_2_RLE_first_byte{0x7};

  struct DefHeader {
    uint32_t type;
    uint32_t frame_width;
    uint32_t frame_height;
    uint32_t group_count;
  };

  struct ImageData {
    uint32_t frame_full_width;
    uint32_t frame_full_height;

    uint32_t width;
    uint32_t height;

    uint32_t x;
    uint32_t y;

    std::optional<std::vector<uint8_t>> selection;
    std::vector<uint8_t> raw_data;
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

  ImageData parse_image(BinaryDataView &buffer, std::vector<Palette::Color> &palette) {
    struct ImageData image{};

    static bool passed = false;

    uint32_t size = buffer.read_le_ui32();
    uint32_t format = buffer.read_le_ui32(); // format in which pixel data is stores

    image.frame_full_width = buffer.read_le_ui32();  // full width of frame including border
    image.frame_full_height = buffer.read_le_ui32(); // full height of frame including border

    image.width = buffer.read_le_ui32();  // width of pixel data without borders
    image.height = buffer.read_le_ui32(); // height of pixel data without borders
    image.x = buffer.read_le_ui32();      // left margin
    image.y = buffer.read_le_ui32();      // top margin

    const auto base_offset = buffer.tell();
    auto current = base_offset;
    std::vector<uint8_t> palette_indices{};

    // data is not encoded, store as is
    if (format == 0) {
      buffer.loop(image.height * image.width, palette_indices);
    }
    // Run-length encoding rows, used by most creature C*.def files
    // height * uint32 offsets, 0xFF segment_type means raw pixel data,
    // otherwise RLE encoded colors
    else if (format == 1) {
      std::vector<uint32_t> offsets = buffer.loop<uint32_t>(image.height);

      for (size_t i = 0; i < offsets.size(); ++i) {
        buffer.seek(base_offset + offsets[i]);

        uint32_t total_row_length = 0;
        while (total_row_length < image.width) {
          uint8_t segment_type = buffer.read_ui8();
          uint32_t segment_length = buffer.read_ui8() + 1;

          if (segment_type == format_1_RLE_first_byte) {
            // store segment raw data
            buffer.loop<uint8_t>(segment_length, palette_indices);
          } else {
            // in this case segment_type is a color index, store it segment_length times
            palette_indices.insert(palette_indices.end(), segment_length, segment_type);
          }

          total_row_length += segment_length;
        }
      }
    }
    // Used by .def files that using a lot shadow/transparent colors (e.g. rivers)
    // height * uint16 offsets, keeps single byte to store both segment_type and length information
    // 3 bits is for the segment type, 5 bits for it length, so 2^5 (32 pixel) blocks
    else if (format == 2) {
      uint16_t currect_offset = buffer.read_le_ui16();
      buffer.seek(base_offset + currect_offset);

      for (size_t i = 0; i < image.height; ++i) {
        uint32_t total_row_length = 0;

        while (total_row_length < image.width) {
          uint8_t segment = buffer.read_ui8();
          uint8_t segment_type = segment >> 5;         // get 3 first bits
          uint8_t segment_length = (segment & 31) + 1; // get last 5 bits

          if (segment_type == format_2_RLE_first_byte) {
            // store segment raw data
            buffer.loop<uint8_t>(segment_length, palette_indices);
          } else {
            // similar to format 1, segment_type is a color index, store it segment_length times
            palette_indices.insert(palette_indices.end(), segment_length, segment_type);
          }

          total_row_length += segment_length;
        }
      }
    }
    // The packets encoding is identical to compression type 2
    // except this type is block-oriented encoding, with 32 pixel blocks
    else if (format == 3) {
      for (size_t i = 0; i < image.height; ++i) {
        // Jump to the offset address, offset is 16 bits, so i * 2 applied
        buffer.seek(base_offset + i * 2 * (image.width / 32));
        uint16_t current_offset = buffer.read_le_ui16();
        buffer.seek(base_offset + current_offset);

        uint32_t total_row_length = 0;

        while (total_row_length < image.width) {
          uint8_t segment = buffer.read_ui8();
          uint8_t segment_type = segment >> 5;         // get 3 first bits
          uint8_t segment_length = (segment & 31) + 1; // get last 5 bits

          if (segment_type == format_2_RLE_first_byte) {
            // store segment raw data
            buffer.loop<uint8_t>(segment_length, palette_indices);
          } else {
            // similar to format 1, segment_type is a color index, store it segment_length times
            palette_indices.insert(palette_indices.end(), segment_length, segment_type);
          }

          total_row_length += segment_length;
        }
      }
    }

    std::vector<uint8_t> &bitmap = image.raw_data;
    bitmap.resize(image.frame_full_width * image.frame_full_height * 4, 0);

    for (size_t i = 0; i < image.height; i++) {
      uint32_t start = (((i + image.y) * image.frame_full_width) + image.x) * 4;

      for (size_t j = 0; j < image.width; j++) {
        uint32_t index = palette_indices[i * image.width + j];
        Palette::Color rgb_color{palette[index]};

        uint32_t pixel_offset = start + j * 4;
        bitmap[pixel_offset] = rgb_color.r;
        bitmap[pixel_offset + 1] = rgb_color.g;
        bitmap[pixel_offset + 2] = rgb_color.b;
        bitmap[pixel_offset + 3] = rgb_color.a;
      }
    }
    return image;
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

  void combine_images_data(std::unordered_map<std::string, ImageData> &images,
                           std::vector<std::string> &frame_names,
                           DefHeader &def_header,
                           fs::path &folder

  ) {
    std::vector<uint8_t> result;
    for (size_t i = 0; i < def_header.frame_height; ++i) {
      for (size_t j = 0; j < frame_names.size(); ++j) {
        result.insert(
            result.end(), images[frame_names[j]].raw_data.begin() + def_header.frame_width * i * 4,
            images[frame_names[j]].raw_data.begin() + def_header.frame_width * (i + 1) * 4);
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

      std::unordered_map<std::string, ImageData> images{};
      std::vector<std::string> group_frames{};
      for (size_t i = 0; i < frame_count; i++) {
        group_frames.push_back(frame_names[i]);

        if (images.contains(frame_names[i]))
          continue;

        const auto start_offset = buffer.tell();
        buffer.seek(frame_offsets[i]);

        auto image_data = parse_image(buffer, palette);
        images[frame_names[i]] = image_data;

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
