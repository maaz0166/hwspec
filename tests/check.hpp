#pragma once

// A tiny test helper, so the project needs no test library yet.
// CHECK(condition) and CHECK_EQ(a, b) record a failure and keep going;
// return finish() from main() so the program exits with 1 if anything failed.

#include <iostream>

inline int& check_failures() {
  static int count = 0;
  return count;
}

#define CHECK(condition)                                                                 \
  do {                                                                                   \
    if (!(condition)) {                                                                  \
      std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK failed: " #condition "\n";    \
      ++check_failures();                                                                \
    }                                                                                    \
  } while (0)

#define CHECK_EQ(actual, expected)                                                       \
  do {                                                                                   \
    const auto actual_value = (actual);                                                  \
    const auto expected_value = (expected);                                              \
    if (!(actual_value == expected_value)) {                                             \
      std::cerr << __FILE__ << ":" << __LINE__ << ": CHECK_EQ failed: " #actual          \
                << " == " #expected "\n  actual:   " << actual_value                     \
                << "\n  expected: " << expected_value << "\n";                           \
      ++check_failures();                                                                \
    }                                                                                    \
  } while (0)

inline int finish() {
  if (check_failures() > 0) {
    std::cerr << check_failures() << " check(s) failed\n";
    return 1;
  }
  return 0;
}
