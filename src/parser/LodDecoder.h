#pragma once

#include <filesystem>
#include <vector>

#include "FileFormats.h"
#include "lib/palette.h"
#include <lib/aligment.h>

namespace fs = std::filesystem;

class LodDecoder {
private:
  static constexpr uint32_t LOD_MAGIC{0x00444F4C};

  const fs::path file_path_{};

  std::ifstream open_fstream();

  FileFormats::Lod::LodHeader extract_header(std::ifstream &stream);
  std::vector<FileFormats::Def::DefFile>
  decode_entries(std::ifstream &stream, const FileFormats::Lod::LodHeader &lod_header);

  FileFormats::Lod::EntryFileHeader extract_entry_header(std::ifstream &stream);
  void decompress_entry(std::ifstream &stream,
                        std::vector<uint8_t> &destination,
                        const FileFormats::Lod::EntryFileHeader &entry_header);

public:
  explicit LodDecoder(const fs::path &path) : file_path_(path) {};
  std::vector<FileFormats::Def::DefFile> decode();
};