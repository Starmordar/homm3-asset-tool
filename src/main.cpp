#include <iostream>
#include <string>
#include <tuple>

#include "parser/LodDecoder.h"
#include "parser/SpriteWriter.h"

static std::optional<std::string> critical_error;
constexpr std::string_view input_file_name{"--input_file="};
constexpr std::string_view output_dir_arg_name{"--output_dir="};

int init(std::string_view input_file, std::string_view output_dir) {
  try {
    LodDecoder lod_decoder{input_file};
    std::vector<FileFormats::Def::DefFile> def_files = lod_decoder.decode();

    SpriteWriter sprite_writer{output_dir};
    sprite_writer.write(def_files);
  } catch (const std::exception &e) {
    critical_error = e.what();
  }

  return 0;
}

std::pair<std::string_view, std::string_view> parse_args(int argc, char *argv[]) {
  std::optional<std::string_view> input_file;
  std::optional<std::string_view> output_dir;

  for (size_t i = 0; i < argc; ++i) {
    std::string_view arg{argv[i]};

    if (arg.starts_with(input_file_name))
      input_file = arg.substr(input_file_name.length(), arg.length());
    else if (arg.starts_with(output_dir_arg_name))
      output_dir = arg.substr(output_dir_arg_name.length(), arg.length());
  }

  if (!input_file.has_value())
    throw std::runtime_error("missing required argument '--input_file=<path>'");
  else if (!output_dir.has_value())
    throw std::runtime_error("missing required argument '--output_dir=<path>'");

  return {input_file.value(), output_dir.value()};
}

void handle_critical_errors(std::string &message) {
  std::string message_to_show = "Fatal error! " + message;
  std::cerr << message_to_show << '\n';

  ::exit(1);
}

int main(int argc, char *argv[]) {
  std::string_view input_file;
  std::string_view output_dir;

  try {
    std::tie(input_file, output_dir) = parse_args(argc, argv);
  } catch (const std::exception &e) {
    std::cerr << e.what() << '\n';
    ::exit(1);
  }

  init(input_file, output_dir);

  if (critical_error.has_value())
    handle_critical_errors(critical_error.value());
}
