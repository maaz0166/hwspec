#include <cstring>
#include <iostream>

#include "hwspec/version.hpp"

int main() {
  if (std::strcmp(hwspec::version(), EXPECTED_VERSION) != 0) {
    std::cerr << "version mismatch: got '" << hwspec::version() << "', expected '" << EXPECTED_VERSION << "'\n";
    return 1;
  }
  return 0;
}
