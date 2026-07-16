#include "lod_parser.h"
#include "def_parser.h"

#include "lib/aligment.h"
#include "lib/def_types.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <unordered_map>

#include <zlib.h>

namespace fs = std::filesystem;

namespace {
  constexpr uint32_t LOD_MAGIC{0x00444F4C};

  PACKED_STRUCT_START
  struct LodHeader {
    uint32_t magic;
    uint32_t type;
    uint32_t files_count;

    char archive_name[80];
  } PACKED_STRUCT_END;

  PACKED_STRUCT_START
  struct EntryFileHeader {
    char name[16];

    uint32_t offset;
    uint32_t uncompressed_size;
    uint32_t file_type;
    uint32_t compressed_size;
  } PACKED_STRUCT_END;

  void decompress_entry(std::ifstream &stream,
                        std::vector<uint8_t> &destination,
                        const EntryFileHeader &entry_header) {
    std::vector<uint8_t> compressed_content(entry_header.compressed_size);
    stream.read(reinterpret_cast<char *>(compressed_content.data()), compressed_content.size());

    unsigned long uncompressed_size = entry_header.uncompressed_size;
    uncompress(destination.data(), &uncompressed_size, compressed_content.data(),
               compressed_content.size());
  }

  EntryFileHeader extract_entry_header(std::ifstream &stream) {
    EntryFileHeader header;
    stream.read(reinterpret_cast<char *>(&header), sizeof(header));

    header.offset = little_to_native(header.offset);
    header.uncompressed_size = little_to_native(header.uncompressed_size);
    header.file_type = little_to_native(header.file_type);
    header.compressed_size = little_to_native(header.compressed_size);

    return header;
  }

  void parse_entries(std::ifstream &stream, const LodHeader &lod_header) {
    for (uint32_t i = 0; i < lod_header.files_count; ++i) {
      EntryFileHeader entry_header = extract_entry_header(stream);

      // Process only .def files with a known type
      if (!DEF::allowed_types.contains(entry_header.file_type))
        continue;

      // TODO: Remove, temporary for debug
      if (std::strcmp(entry_header.name, "CDDRAG.def"))
        continue;

      const auto start_offset = stream.tellg();
      stream.seekg(entry_header.offset);

      std::vector<uint8_t> content(entry_header.uncompressed_size);
      if (entry_header.compressed_size == 0)
        stream.read(reinterpret_cast<char *>(content.data()), content.size());
      else
        decompress_entry(stream, content, entry_header);

      stream.seekg(start_offset);

      std::string filename(entry_header.name);
      parse_def(content, entry_header.file_type, filename);
    }
  }

  LodHeader extract_lod_header(std::ifstream &stream) {
    LodHeader header;
    stream.read(reinterpret_cast<char *>(&header), sizeof(header));

    header.magic = little_to_native(header.magic);
    header.type = little_to_native(header.type);
    header.files_count = little_to_native(header.files_count);

    return header;
  }
} // namespace

void parse_lod(const std::string &path) {
  const fs::path lod_path{fs::current_path() / path};
  std::ifstream stream{lod_path, std::ios::in | std::ios::binary};

  if (!stream)
    throw std::runtime_error("Error: Cannot open file: '" + lod_path.string() + "' for reading!");

  stream.exceptions(std::ios::failbit | std::ios::badbit);
  LodHeader lod_header = extract_lod_header(stream);

  if (lod_header.magic != LOD_MAGIC)
    throw std::runtime_error("Error: '" + lod_path.string() + "' is not a LOD file!");

  parse_entries(stream, lod_header);
}