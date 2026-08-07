#pragma once
#include <string>
#include <vector>
namespace mcpp {
struct Args {
  std::string poject_dir;
  std::string cmake_executable_path;
  std::vector<std::string> cmake_args;
  bool show_cmake_args;
  Args()
      : poject_dir("."),
        cmake_executable_path(""),
        cmake_args({}),
        show_cmake_args(false) {}
};

}  // namespace mcpp