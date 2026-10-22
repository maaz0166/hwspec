#pragma once

#include <iosfwd>
#include <string>
#include <vector>

#include "hwspec/sections.hpp"
#include "admin.hpp"

namespace hwspec::cli {

// The whole command-line program as one function, so the .exe and (later) the Python
// command behave identically and tests can call it without starting a process.
//   args     the arguments without the program name
//   out/err  where normal output and error messages go
//   sections which sections exist (tests pass fake ones)
// Returns the exit code: 0 = success (or elevated launch accepted),
// 1 = cancelled at the admin prompt, 2 = wrong usage.
int run(const std::vector<std::string>& args, std::ostream& out, std::ostream& err,
        const std::vector<Section>& sections);
int run(const std::vector<std::string>& args, std::ostream& out, std::ostream& err,
        const std::vector<Section>& sections, const AdminRuntime& runtime);

}  // namespace hwspec::cli
