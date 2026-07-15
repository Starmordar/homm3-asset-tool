#include "./parser/parse-lod.h"
#include <iostream>

static std::optional<std::string> critical_error;
static std::string input_lod_path{"input/H3sprite.lod"};

int init() {
  try {
    parse_lod_file(input_lod_path);
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
