#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "FileFormats.h"

class SpriteWriter {
private:
  std::filesystem::path output_dir_;

  std::string create_def_folder(std::string_view filename);
  void write_sprite(FileFormats::Def::DefFile &def_file,
                    std::string group_name,
                    std::vector<std::string> &image_names,
                    std::string folder_name);

public:
  explicit SpriteWriter(std::string_view output_dir) : output_dir_(output_dir) {};
  void write(std::vector<FileFormats::Def::DefFile> &def_files);
};