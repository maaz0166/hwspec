#include <iostream>
#include <string_view>

#include "hwspec/version.hpp"

namespace {

void print_help() {
  std::cout << "hwspec " << hwspec::version() << " - PC hardware specs in one command (work in progress)\n"
            << "\n"
            << "Usage:\n"
            << "  hwspec --version    Show the version\n"
            << "  hwspec --help       Show this help\n"
            << "\n"
            << "Section flags (--cpu, --mem, --all, ...) arrive in milestone M1.\n";
}

}  // namespace

int main(int argc, char** argv) {
  if (argc < 2) {
    print_help();
    return 0;
  }

  const std::string_view arg = argv[1];
  if (arg == "--version" || arg == "-v") {
    std::cout << "hwspec " << hwspec::version() << "\n";
    return 0;
  }
  if (arg == "--help" || arg == "-h") {
    print_help();
    return 0;
  }

  std::cerr << "hwspec: unknown option '" << arg << "'. Try 'hwspec --help'.\n";
  return 2;
}
