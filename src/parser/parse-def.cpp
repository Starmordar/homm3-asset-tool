#include <array>
#include <fstream>
#include <string>
#include <unordered_set>
#include <vector>

#include "BinaryReader.h"
#include "file_types.h"
#include "parse-def.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

namespace {
  static int frame_name_length{13};
  constexpr int format_1_RLE_first_byte{0xFF};
  constexpr int format_2_RLE_first_byte{0x7};

  struct DefHeader {
    uint32_t type;
    uint32_t width;
    uint32_t height;
    uint32_t group_count;
  };

  struct Color {
    uint8_t r;
    uint8_t g;
    uint8_t b;
    uint8_t a;
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
} // namespace

std::vector<Color> get_palette(BinaryReader &reader) {
  std::vector<Color> palette{};

  for (size_t i = 0; i < 256; i++) {
    palette.push_back(
        {reader.read<uint8_t>(), reader.read<uint8_t>(), reader.read<uint8_t>(), 255});
  }

  palette[0] = {0, 0, 0, 0};   // Transparency                 (0,255,255)
  palette[1] = {0, 0, 0, 64};  // Shadow border                (255,150,255)
  palette[4] = {0, 0, 0, 128}; // Shadow body                  (255,0,255)
  palette[5] = {0, 0, 0, 0};   // Selection                    (255,255,0)
  palette[6] = {0, 0, 0, 128}; // Selection over shadow body   (180,0,255)
  palette[7] = {0, 0, 0, 64};  // Selection over shadow border (0,255,0)

  return palette;
}

std::string get_def_group(uint32_t def_type, uint32_t group_type) {
  if (!FileType::def_groups.contains(def_type)) {
    if (group_type == 0)
      return "all";

    std::cerr << "Unknown animation type";
    return " ";
  }

  if (FileType::def_groups.at(def_type).size() > group_type)
    return FileType::def_groups.at(def_type).at(group_type);
  else
    std::cerr << "Unknown animation type";

  return " ";
}

std::vector<std::string> get_frame_names(BinaryReader &reader, int frame_count) {
  std::vector<std::string> frame_names{};

  for (size_t i = 0; i < frame_count; i++) {
    std::string name = reader.read_string(frame_name_length);

    if (name.find('\0') != std::string::npos) // Clear null terminated strings
      name.resize(frame_name_length - 1);

    frame_names.push_back(name);
  }

  return frame_names;
}

std::vector<uint32_t> get_frame_offsets(BinaryReader &reader, int frame_count) {
  std::vector<uint32_t> frame_offsets{};

  for (size_t i = 0; i < frame_count; i++) {
    frame_offsets.push_back(reader.read<uint32_t>());
  }

  return frame_offsets;
}

std::vector<uint32_t> get_image_sizes(BinaryReader &reader, std::vector<uint32_t> &offsets) {
  std::vector<uint32_t> sizes{};

  for (size_t i = 0; i < offsets.size(); ++i) {
    reader.seek(offsets[i]);
    sizes.push_back(reader.read<uint32_t>() + 32);
  }

  return sizes;
}

ImageData parse_image(BinaryReader &reader, std::vector<Color> &palette) {
  struct ImageData image{};

  static bool passed = false;

  uint32_t size = reader.read<uint32_t>();
  uint32_t format = reader.read<uint32_t>(); // format in which pixel data is stores

  image.frame_full_width = reader.read<uint32_t>();  // full width of frame including border
  image.frame_full_height = reader.read<uint32_t>(); // full height of frame including border

  image.width = reader.read<uint32_t>();  // width of pixel data without borders
  image.height = reader.read<uint32_t>(); // height of pixel data without borders
  image.x = reader.read<uint32_t>();      // left margin
  image.y = reader.read<uint32_t>();      // top margin

  const auto base_offset = reader.tell();
  auto current = base_offset;
  std::vector<uint8_t> palette_indices{};

  // data is not encoded, store as is
  if (format == 0) {
    reader.loop(image.height * image.width, palette_indices);
  }
  // Run-length encoding rows, used by most creature C*.def files
  // height * uint32 offsets, 0xFF segment_type means raw pixel data,
  // otherwise RLE encoded colors
  else if (format == 1) {
    std::vector<uint32_t> offsets = reader.loop<uint32_t>(image.height);

    for (size_t i = 0; i < offsets.size(); ++i) {
      reader.seek(base_offset + offsets[i]);

      uint32_t total_row_length = 0;
      while (total_row_length < image.width) {
        uint8_t segment_type = reader.read<uint8_t>();
        uint32_t segment_length = reader.read<uint8_t>() + 1;

        if (segment_type == format_1_RLE_first_byte) {
          // store segment raw data
          reader.loop<uint8_t>(segment_length, palette_indices);
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
    uint16_t currect_offset = reader.read<uint16_t>();
    reader.seek(base_offset + currect_offset);

    for (size_t i = 0; i < image.height; ++i) {
      uint32_t total_row_length = 0;

      while (total_row_length < image.width) {
        uint8_t segment = reader.read<uint8_t>();
        uint8_t segment_type = segment >> 5;         // get 3 first bits
        uint8_t segment_length = (segment & 31) + 1; // get last 5 bits

        if (segment_type == format_2_RLE_first_byte) {
          // store segment raw data
          reader.loop<uint8_t>(segment_length, palette_indices);
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
      reader.seek(base_offset + i * 2 * (image.width / 32));
      uint16_t current_offset = reader.read<uint16_t>();
      reader.seek(base_offset + current_offset);

      uint32_t total_row_length = 0;

      while (total_row_length < image.width) {
        uint8_t segment = reader.read<uint8_t>();
        uint8_t segment_type = segment >> 5;         // get 3 first bits
        uint8_t segment_length = (segment & 31) + 1; // get last 5 bits

        if (segment_type == format_2_RLE_first_byte) {
          // store segment raw data
          reader.loop<uint8_t>(segment_length, palette_indices);
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
      Color rgb_color{palette[index]};

      uint32_t pixel_offset = start + j * 4;
      bitmap[pixel_offset] = rgb_color.r;
      bitmap[pixel_offset + 1] = rgb_color.g;
      bitmap[pixel_offset + 2] = rgb_color.b;
      bitmap[pixel_offset + 3] = rgb_color.a;
    }
  }

  // std::vector<uint8_t> &bitmap = image.raw_data;
  // for (size_t i = 0; i < palette_indices.size(); i++) {
  //   Color rgb_color{palette[palette_indices[i]]};
  //   bitmap.insert(bitmap.end(), {rgb_color.r, rgb_color.g, rgb_color.b, rgb_color.a});
  // }

  if (!passed) {
    std::cout << format << '\n';
    // std::cout << "palette_indices: " << palette_indices.size() << '\n';
    // std::cout << "widt * height: " << image.width * image.height << '\n';

    // std::cout << "bitmap: " << image.raw_data.size() << '\n';
    // std::cout << "width * height * 4: " << image.width * image.height * 4 << '\n';

    // for (size_t i = 0; i < palette.size(); i++) {
    //   std::cout << static_cast<int>(palette[i].r) << ',' << static_cast<int>(palette[i].g) << ','
    //             << static_cast<int>(palette[i].b) << ',' << static_cast<int>(palette[i].a) <<
    //             '\n';
    // }

    passed = true;
  }
  return image;
}

void parse_def_file(std::vector<uint8_t> &content, uint32_t file_type, char *file_name) {
  BinaryReader reader{content};
  uint32_t def_type = reader.read<uint32_t>();
  uint32_t width = reader.read<uint32_t>();
  uint32_t height = reader.read<uint32_t>();

  uint32_t group_count = reader.read<uint32_t>();

  std::vector<Color> palette{get_palette(reader)};
  std::unordered_map<std::string, std::vector<std::string>> groups{};
  std::unordered_map<std::string, ImageData> images{};

  std::filesystem::path path(file_name);
  std::string folder_name = path.stem().string();

  std::filesystem::create_directory("input/" + folder_name);

  for (size_t i = 0; i < group_count; i++) {
    std::string group_name{get_def_group(file_type, reader.read<uint32_t>())};
    uint32_t frame_count{reader.read<uint32_t>()};
    reader.skip(8); // unknown

    std::vector<std::string> frame_names{get_frame_names(reader, frame_count)};
    std::vector<uint32_t> frame_offsets{get_frame_offsets(reader, frame_count)};

    const auto current = reader.tell();
    std::vector<uint32_t> image_sizes{get_image_sizes(reader, frame_offsets)};
    reader.seek(current);

    std::vector<std::string> &group_frames = groups[group_name];

    for (size_t i = 0; i < frame_count; i++) {
      group_frames.push_back(frame_names[i]);

      if (images.contains(frame_names[i]))
        continue;

      const auto current = reader.tell();
      reader.seek(frame_offsets[i]);

      auto image_data = parse_image(reader, palette);
      images[frame_names[i]] = image_data;

      std::filesystem::path path{"input/" + folder_name + '/' + frame_names[i]};
      path.replace_extension(".png");

      stbi_write_png(path.c_str(), image_data.frame_full_width, image_data.frame_full_height, 4,
                     image_data.raw_data.data(), image_data.frame_full_width * 4);

      reader.seek(current);
    }
  }
}
