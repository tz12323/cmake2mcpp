#include <spdlog/spdlog.h>
#include <CLI/CLI.hpp>
#include <cstdlib>
#include <filesystem>
#include <string>
#include "args.hpp"
#include "code_model_parser.hpp"
#include "file_api_query.hpp"
#include "index_parser.hpp"
#include "target_parser.hpp"
#include "toml_writer.hpp"

void showCmakeArgs(const std::string& project_dir) {
  auto lower = [](const std::string& str) -> std::string {
    std::string result;
    result.reserve(str.size());
    for (char c : str) {
      result.push_back(
          static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return result;
  };
  bool hasCMakeLists = false;
  for (const auto& file : std::filesystem::directory_iterator(
           std::filesystem::absolute(std::filesystem::path(project_dir)))) {
    if (file.is_directory()) {
      continue;
    }
    auto filename = lower(file.path().filename().string());
    if (filename == "cmakelists.txt") {
      hasCMakeLists = true;
      break;
    }
  }
  if (!hasCMakeLists) {
    spdlog::error("No CMakeLists.txt found in the project directory.");
    return;
  }
  auto sourceDir =
      std::filesystem::absolute(std::filesystem::path(project_dir));
  auto buildDir = sourceDir / "build";
#ifdef _WIN32
  std::string cmd = "cmake -B \"" + buildDir.string() + "\" -S \"" +
                    sourceDir.string() + "\" -LH 2>NUL";
  FILE* pipe = _popen(cmd.c_str(), "rt");
#else
  std::string cmd = "cmake -B \"" + buildDir.string() + "\" -S \"" +
                    sourceDir.string() + "\" -LH 2>/dev/null";
  FILE* pipe = popen(cmd.c_str(), "r");
#endif

  if (!pipe)
    spdlog::error("Failed to execute cmake.");

  std::string output;
  char buffer[4096];

  while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    output += buffer;
  }

#ifdef _WIN32
  _pclose(pipe);
#else
  pclose(pipe);
#endif
  spdlog::info("CMake build arguments: {}", output);
}
void clean_build_dir(const mcpp::Args& args) {
  if (!std::filesystem::exists(args.poject_dir)) {
    spdlog::error("Project directory does not exist: {}", args.poject_dir);
    std::exit(EXIT_FAILURE);
  }
  auto build_dir = {
      std::filesystem::absolute(std::filesystem::path(args.poject_dir) /
                                "build"),
      std::filesystem::absolute(std::filesystem::path(args.poject_dir) /
                                "target"),
      std::filesystem::absolute(std::filesystem::path(args.poject_dir) /
                                "cmake2mcpp_generated"),
      std::filesystem::absolute(std::filesystem::path(args.poject_dir) /
                                "mcpp.toml"),
      std::filesystem::absolute(std::filesystem::path(args.poject_dir) /
                                "compile_commands.json")};
  for (auto& dir : build_dir) {
    if (std::filesystem::exists(dir)) {
      spdlog::info("Cleaning build directory: {}", dir.string());
      std::error_code ec;
      std::filesystem::remove_all(dir, ec);
      if (ec) {
        spdlog::error("Failed to clean build directory: {}", ec.message());
        std::exit(EXIT_FAILURE);
      }
    }
  }
}
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
  auto clean_cmd = app.add_subcommand("clean", "Clean build directory");
  CLI11_PARSE(app, argc, argv);
  if (args.show_cmake_args) {
    showCmakeArgs(args.poject_dir);
    return 0;
  }
  if (clean_cmd->parsed()) {
    clean_build_dir(args);
    return 0;
  }
  /// 创建 cmake2mcpp 的 File API 查询
  mcpp::FileApiQuery query;
  if (!args.cmake_executable_path.empty()) {
    query.SetCmakeExecutablePath(args.cmake_executable_path);
  }
  fs::path build_dir = fs::absolute(fs::path(args.poject_dir) / "build");
  fs::path output_file_dir = fs::absolute(fs::path(args.poject_dir));
  spdlog::info("Build directory: {}", build_dir.string());
  if (!query.Create(build_dir)) {
    spdlog::error("Failed to create File API query.");
    return -1;
  }
  spdlog::info("File API query created successfully.");
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
  if (!mcpp::MainTomlWrite(fs::absolute(fs::path(args.poject_dir)), project,
                           targets)) {
    spdlog::error("Failed to write TOML file.");
    return -1;
  }
  spdlog::info("TOML file written successfully.");

  return 0;
}