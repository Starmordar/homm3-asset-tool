#pragma once

#include <cstdint>
#include <unordered_map>
#include <vector>

#include "lib/aligment.h"

namespace FileFormats {
  namespace Lod {
    PACKED_STRUCT_START
    struct LodHeader {       // Header of a .lod archive.
      uint32_t magic;        // magic number identifying a .lod file
      uint32_t type;         // .lod file type identifier
      uint32_t files_count;  // number of files stored in .lod file
      char archive_name[80]; // reserved 80-byte field for the name (null-terminated)
    } PACKED_STRUCT_END;

    PACKED_STRUCT_START
    struct EntryFileHeader {      // Header describing an entry stored in a .lod archive.
      char name[16];              // null-terminated file name
      uint32_t offset;            // offset to the file data within the .lod
      uint32_t uncompressed_size; // file size after decompression
      uint32_t file_type;         // file type identifier
      uint32_t compressed_size;   // zlib compressed size (0 if uncompressed)
    } PACKED_STRUCT_END;

    struct EntryFile { // Decoded .lod archive entry containing its header and file data.
      EntryFileHeader header;
      std::vector<uint8_t> data;
    };
  } // namespace Lod

  namespace Def {
    struct DefHeader { // Main .def file header describing the contained image data.
      std::string type;
      uint32_t frame_width;
      uint32_t frame_height;
      uint32_t group_count;
    };

    struct PcxImage {        // PCX image data stored within the .def file format.
      uint32_t frame_width;  // full width of frame including border
      uint32_t frame_height; // full height of frame including border

      uint32_t width;  // width of pixel data without borders
      uint32_t height; // height of pixel data without borders

      uint32_t x; // left margin
      uint32_t y; // top margin

      std::vector<uint8_t> bitmap; // pixel data in a RGBA vector
    };

    struct DefFile {
      DefHeader header;
      std::string name;
      std::unordered_map<std::string, FileFormats::Def::PcxImage> images;
      std::unordered_map<std::string, std::vector<std::string>> groups;
    };
  } // namespace Def
} // namespace FileFormats
