#include <CLI/CLI.hpp>
#include <filesystem>
#include <iostream>
#include "args.hpp"
#include "code_model_parser.hpp"
#include "file_api_query.hpp"
#include "index_parser.hpp"
#include "target_parser.hpp"
#include "toml_writer.hpp"
int main(int argc, char** argv) {
  /// argument parser
  mcpp::Args args;
  auto app = CLI::App{"CMake File API to TOML Converter"};
  argv = app.ensure_utf8(argv);
  app.add_option("-p,--project_dir", args.poject_dir, "Project directory")
      ->check(CLI::ExistingDirectory);
  app.add_option("-c,--cmake_executable_path", args.cmake_executable_path,
                 "CMake executable path")
      ->check(CLI::ExistingFile);
  app.add_option("-D", args.cmake_args, "CMake build arguments");
  app.add_flag("-s,--show_cmake_args", args.show_cmake_args,
               "Show CMake build arguments");
  CLI11_PARSE(app, argc, argv);

  /// 创建 cmake2mcpp 的 File API 查询
  mcpp::FileApiQuery query;
  if (!args.cmake_executable_path.empty()) {
    query.SetCmakeExecutablePath(args.cmake_executable_path);
  }
  fs::path build_dir = fs::absolute(fs::path(args.poject_dir) / "build");
  fs::path output_file_dir = fs::absolute(fs::path(args.poject_dir));
  std::cout << "Build directory: " << build_dir << std::endl;
  if (!query.Create(build_dir)) {
    std::cerr << "Failed to create File API query." << std::endl;
    return -1;
  }
  std::cout << "File API query created successfully." << std::endl;
  /// 解析 CMake File API 输出
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
  if (!mcpp::TomlWrite((output_file_dir / "mcpp.toml").string(), project,
                       targets)) {
    std::cerr << "Failed to write TOML file." << std::endl;
    return -1;
  }
  std::cout << "TOML file written successfully." << std::endl;

  return 0;
}