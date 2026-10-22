#include <pybind11/pybind11.h>
#include <pybind11/iostream.h>
#include <pybind11/stl.h>
#include <algorithm>
#include <iostream>
#include <sstream>
#include "admin.hpp"
#include "run.hpp"
#include "hwspec/sections.hpp"
#include "hwspec/version.hpp"

namespace py = pybind11;

PYBIND11_MODULE(_native, module) {
  module.doc() = "Windows hardware collection through the shared hwspec C++ core";
  module.attr("__version__") = hwspec::version();
  module.def("collect", [](const std::vector<std::string>& names, bool detail, bool skip_admin) {
    const auto& sections = hwspec::default_sections();
    std::vector<std::string> args = {"--json"};
    for (const auto& name : names) {
      if (name != "all" && std::none_of(sections.begin(), sections.end(),
          [&name](const hwspec::Section& section) { return section.flag == name; }))
        throw py::value_error("Unknown section: " + name);
      args.push_back("--" + name);
    }
    if (detail) args.push_back("--detail");
    if (skip_admin) args.push_back("--skip-admin");
    std::ostringstream out, err;
    int code;
    {
      py::gil_scoped_release release;
      auto runtime = hwspec::cli::native_admin_runtime(err);
      runtime.interactive = false; // Library calls never prompt, pause or elevate.
      code = hwspec::cli::run(args, out, err, sections, runtime);
    }
    if (code != 0) throw std::runtime_error(err.str());
    return py::module_::import("json").attr("loads")(out.str()).cast<py::dict>();
  }, py::arg("sections"), py::arg("detail") = false, py::arg("skip_admin") = false);

  module.def("run_cli", [](const std::vector<std::string>& args, const std::wstring& interpreter) {
    if (interpreter.empty()) throw py::value_error("A Python interpreter path is required");
    // Keep the GIL while the CLI redirects output through Python streams.
    py::scoped_ostream_redirect output(std::cout, py::module_::import("sys").attr("stdout"));
    py::scoped_estream_redirect errors(std::cerr, py::module_::import("sys").attr("stderr"));
    const hwspec::cli::RelaunchCommand command{interpreter, {L"-m", L"hwspec"}};
    auto runtime = hwspec::cli::native_admin_runtime(std::cerr, command);
    return hwspec::cli::run(args, std::cout, std::cerr, hwspec::default_sections(), runtime);
  }, py::arg("args"), py::arg("interpreter"));
}
