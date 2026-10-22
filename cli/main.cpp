#include <iostream>
#include <string>
#include <vector>

#include "hwspec/sections.hpp"
#include "run.hpp"

int main(int argc, char** argv) {
  std::vector<std::string> args;
  for (int i = 1; i < argc; ++i) {  // skip argv[0], the program name
    args.emplace_back(argv[i]);
  }
  return hwspec::cli::run(args, std::cout, std::cerr, hwspec::default_sections());
}
