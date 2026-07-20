#include <filesystem>
#include <fstream>
#include <vector>

#include "DefDecoder.h"
#include "FileFormats.h"
#include "LodDecoder.h"
#include "lib/file_types.h"

#include <zlib.h>

namespace fs = std::filesystem;

std::ifstream LodDecoder::open_fstream() {
  std::ifstream stream{file_path_, std::ios::in | std::ios::binary};

  if (!stream)
    throw std::runtime_error("LodDecoder: Cannot open file: '" + file_path_.string() +
                             "' for reading!");

  stream.exceptions(std::ios::failbit | std::ios::badbit);

  return stream;
}

FileFormats::Lod::LodHeader LodDecoder::extract_header(std::ifstream &stream) {
  FileFormats::Lod::LodHeader header;
  stream.read(reinterpret_cast<char *>(&header), sizeof(header));

  header.magic = little_to_native(header.magic);
  header.type = little_to_native(header.type);
  header.files_count = little_to_native(header.files_count);

  if (header.magic != LOD_MAGIC)
    throw std::runtime_error("LodDecoder: '" + file_path_.string() + "' is not a LOD file!");

  return header;
}

FileFormats::Lod::EntryFileHeader LodDecoder::extract_entry_header(std::ifstream &stream) {
  FileFormats::Lod::EntryFileHeader header;
  stream.read(reinterpret_cast<char *>(&header), sizeof(header));

  header.offset = little_to_native(header.offset);
  header.uncompressed_size = little_to_native(header.uncompressed_size);
  header.file_type = little_to_native(header.file_type);
  header.compressed_size = little_to_native(header.compressed_size);

  return header;
}

void LodDecoder::decompress_entry(std::ifstream &stream,
                                  std::vector<uint8_t> &destination,
                                  const FileFormats::Lod::EntryFileHeader &entry_header) {
  std::vector<uint8_t> compressed(entry_header.compressed_size);
  stream.read(reinterpret_cast<char *>(compressed.data()), compressed.size());

  unsigned long uncompressed_size = entry_header.uncompressed_size;
  uncompress(destination.data(), &uncompressed_size, compressed.data(), compressed.size());
}

std::vector<FileFormats::Def::DefFile>
LodDecoder::decode_entries(std::ifstream &stream, const FileFormats::Lod::LodHeader &lod_header) {
  std::vector<FileFormats::Def::DefFile> def_files{};

  for (uint32_t i = 0; i < lod_header.files_count; ++i) {
    FileFormats::Lod::EntryFile entry_file{};
    entry_file.header = extract_entry_header(stream);
    entry_file.data.resize(entry_file.header.uncompressed_size);

    // Process only .def files with a known type
    if (!FileTypes::type_name_map.contains(entry_file.header.file_type))
      continue;

    // TODO: Remove, temporary for debug
    if (std::strcmp(entry_file.header.name, "CDDRAG.def"))
      continue;

    const auto base_offset = stream.tellg();
    stream.seekg(entry_file.header.offset);

    if (entry_file.header.compressed_size == 0) // get uncompressed data
      stream.read(reinterpret_cast<char *>(entry_file.data.data()), entry_file.data.size());
    else
      decompress_entry(stream, entry_file.data, entry_file.header);

    stream.seekg(base_offset);

    DefDecoder def_decoder{};
    def_files.push_back(def_decoder.decode(entry_file));
  }

  return def_files;
}

std::vector<FileFormats::Def::DefFile> LodDecoder::decode() {
  std::ifstream stream{open_fstream()};
  FileFormats::Lod::LodHeader lod_header = extract_header(stream);

  return decode_entries(stream, lod_header);
}