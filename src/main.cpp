#include <CLI/CLI.hpp>
#include <filesystem>
#include <iostream>
#include "code_model_parser.hpp"
#include "file_api_query.hpp"
#include "index_parser.hpp"
#include "target_parser.hpp"
#include "toml_writer.hpp"

int main(int argc, char** argv) {
  /// argument parser
  auto app = CLI::App{"CMake File API to TOML Converter"};
  argv = app.ensure_utf8(argv);
  auto build_dir_default = std::string("build");
  app.add_option("-b,--build", build_dir_default, "Build directory");
  auto output_dir_default = std::string(".");
  app.add_option("-o,--output", output_dir_default, "Output TOML file path");
  auto cmake_executable_path_default = std::string("");
  app.add_option("-c,--cmake", cmake_executable_path_default,
                 "CMake executable path (optional)");
  CLI11_PARSE(app, argc, argv);

  mcpp::FileApiQuery query;
  if (!cmake_executable_path_default.empty()) {
    query.SetCmakeExecutablePath(cmake_executable_path_default);
  }
  fs::path build_dir = fs::absolute(build_dir_default);
  fs::path output_file_dir = fs::absolute(output_dir_default);
  std::cout << "Build directory: " << build_dir << std::endl;
  if (!query.Create(build_dir)) {
    std::cerr << "Failed to create File API query." << std::endl;
    throw std::runtime_error("Failed to create File API query.");
  }
  std::cout << "File API query created successfully." << std::endl;

  auto index_parser = mcpp::IndexParser(build_dir);
  auto code_model_path = index_parser.Parse();
  auto code_model_parser = mcpp::CodeModelParser(code_model_path);
  auto [project, target_paths] = code_model_parser.parse();
  auto targets = std::vector<mcpp::Target>();
  for (const auto& target_path : target_paths) {
    auto target_parser = mcpp::TargetParser(target_path);
    auto target = target_parser.parse();
    if (!target.has_value()) {
      continue;
    }
    targets.push_back(target.value());
  }
  if (!mcpp::TomlWrite((output_file_dir / "mcpp.toml").string(), project, targets)) {
    std::cerr << "Failed to write TOML file." << std::endl;
    return -1;
  }
  std::cout << "TOML file written successfully." << std::endl;

  return 0;
}