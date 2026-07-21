#include <filesystem>
#include <string_view>
#include <vector>

#include "SpriteWriter.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

namespace fs = std::filesystem;

std::string SpriteWriter::create_def_folder(std::string_view filename) {
  fs::path path(filename);
  std::string folder_name = path.stem().string();

  fs::create_directory(output_dir_ / folder_name);
  return folder_name;
}

void SpriteWriter::write_sprite(FileFormats::Def::DefFile &def_file,
                                std::string group_name,
                                std::vector<std::string> &image_names,
                                std::string folder_name) {
  std::vector<uint8_t> result;

  for (size_t i = 0; i < def_file.header.frame_height; ++i) {
    for (size_t j = 0; j < image_names.size(); ++j) {
      result.insert(result.end(),
                    def_file.images[image_names[j]].bitmap.begin() +
                        def_file.header.frame_width * i * 4,
                    def_file.images[image_names[j]].bitmap.begin() +
                        def_file.header.frame_width * (i + 1) * 4);
    }
  }

  fs::path path{output_dir_ / folder_name / group_name};
  path.replace_extension(".png");

  stbi_write_png(path.c_str(), def_file.header.frame_width * image_names.size(),
                 def_file.header.frame_height, 4, result.data(),
                 def_file.header.frame_width * image_names.size() * 4);
}

void SpriteWriter::write(std::vector<FileFormats::Def::DefFile> &def_files) {
  for (size_t i = 0; i < def_files.size(); i++) {
    std::string folder_name = create_def_folder(def_files[i].name);

    for (auto &[group_name, image_names] : def_files[i].groups) {
      write_sprite(def_files[i], group_name, image_names, folder_name);
    }
  }
}
