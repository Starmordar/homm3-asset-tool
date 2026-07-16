#include "parse-lod.h"
#include "file_types.h"
#include "parse-def.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>

#include <zlib.h>

namespace {
  const int LOD_MAGIC{0x00444F4C};

  struct LodHeader {
    uint32_t magic;
    uint32_t type;
    uint32_t files_count;

    char archive_name[80];
  };

  struct EntryFileHeader {
    char name[16];

    uint32_t offset;
    uint32_t uncompressed_size;
    uint32_t file_type;
    uint32_t compressed_size;
  };
} // namespace

namespace fs = std::filesystem;

void decompress_entry_file(std::ifstream &file_stream,
                           std::vector<uint8_t> &uncompressed_data,
                           const EntryFileHeader &entry_header) {
  std::vector<uint8_t> compressed_content(entry_header.compressed_size);
  file_stream.read(reinterpret_cast<char *>(compressed_content.data()), compressed_content.size());

  unsigned long uncompressed_size = entry_header.uncompressed_size;
  uncompress(uncompressed_data.data(), &uncompressed_size, compressed_content.data(),
             compressed_content.size());
}

void parse_entry_files(std::ifstream &file_stream, const LodHeader &lod_header) {
  std::unordered_map<std::string, std::vector<uint8_t>> file_contents{};
  file_contents.reserve(lod_header.files_count);

  for (int i = 0; i < lod_header.files_count; ++i) {
    EntryFileHeader entry_header{};
    file_stream.read(reinterpret_cast<char *>(&entry_header), sizeof(entry_header));

    // Process only .def files with a known type
    if (!FileType::def_types.contains(entry_header.file_type))
      continue;

    if (std::strcmp(entry_header.name, "CDDRAG.def"))
      continue;

    const auto curr_pos = file_stream.tellg();

    auto &content = file_contents[entry_header.name];
    content.resize(entry_header.uncompressed_size);

    file_stream.seekg(entry_header.offset);

    if (entry_header.compressed_size == 0)
      file_stream.read(reinterpret_cast<char *>(content.data()), content.size());
    else
      decompress_entry_file(file_stream, content, entry_header);

    file_stream.seekg(curr_pos);

    parse_def_file(content, entry_header.file_type, entry_header.name);
  }
}

void parse_lod_file(std::string &input_path) {
  fs::path lod_path{fs::current_path() / input_path};
  std::ifstream file_stream{lod_path, std::ios::in | std::ios::binary};

  if (!file_stream)
    throw std::runtime_error("Error: Cannot open file: '" + lod_path.string() + "' for reading!");

  LodHeader file_header{};
  file_stream.read(reinterpret_cast<char *>(&file_header), sizeof(file_header));

  if (file_header.magic != LOD_MAGIC)
    throw std::runtime_error("Error: '" + lod_path.string() + "' is not a LOD file!");

  file_stream.exceptions(std::ios::failbit | std::ios::badbit);
  parse_entry_files(file_stream, file_header);
}