#include "parser/LodDecoder.h"
#include "parser/SpriteWriter.h"
#include <iostream>

static std::optional<std::string> critical_error;
static std::string input_lod_path{"input/H3sprite.lod"};

int init() {
  try {
    LodDecoder lod_decoder{input_lod_path};
    std::vector<FileFormats::Def::DefFile> def_files = lod_decoder.decode();

    SpriteWriter sprite_writer;
    sprite_writer.write(def_files);
  } catch (const std::exception &e) {
    critical_error = e.what();
  }

  return 0;
}

void handle_critical_errors(std::string &message) {
  std::string message_to_show = "Fatal error! " + message;
  std::cerr << message_to_show << '\n';

  ::exit(1);
}

int main() {
  init();

  if (critical_error.has_value())
    handle_critical_errors(critical_error.value());
}
