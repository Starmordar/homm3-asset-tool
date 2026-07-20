#pragma once

#include <string>
#include <vector>

#include "FileFormats.h"

class SpriteWriter {
private:
  static constexpr std::string ouput_dir{"output/"};

  std::string create_def_folder(std::string_view filename);
  void write_sprite(FileFormats::Def::DefFile &def_file,
                    std::string group_name,
                    std::vector<std::string> &image_names,
                    std::string folder_name);

public:
  SpriteWriter() = default;
  void write(std::vector<FileFormats::Def::DefFile> &def_files);
};