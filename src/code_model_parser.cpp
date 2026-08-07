#include "code_model_parser.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
namespace mcpp {
CodeModelParser::CodeModelParser(const fs::path& path)
    : code_model_path(path),
      test_target({"test", "nightly", "example", "experimental"}) {};
CodeModelParser::CodeModelParser(const fs::path& path,
                                 const std::vector<std::string>& test_target)
    : code_model_path(path), test_target(test_target) {};
void CodeModelParser::ReadCodeModelFile() {
  std::cout << "Reading code model file: " << code_model_path << std::endl;
  std::ifstream file(code_model_path);
  if (file.is_open()) {
    file >> code_model_json;
  } else {
    std::cerr << "Failed to open code model file: " << code_model_path.string()
              << std::endl;
    std::exit(EXIT_FAILURE);
  }
};
bool CodeModelParser::isTestTarget(
    const std::string& target,
    const std::vector<std::string>& test_target) {
  auto lower = [&](const std::string& str) {
    std::string lowerStr = str;
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return lowerStr;
  };
  for (const auto& test : test_target) {
    if (lower(target).find(lower(test)) != std::string::npos) {
      return true;
    }
  }
  return false;
};
std::pair<Project, std::vector<fs::path>> CodeModelParser::parse() {
  ReadCodeModelFile();
  Project project;
  auto version_parser = [&]() {
    fs::path cache_path;
    for (const auto& path :
         fs::directory_iterator(code_model_path.parent_path())) {
      if (path.path().filename().string().starts_with("cache-v2")) {
        cache_path = path.path();
        break;
      }
    }
    json cache_json = json::parse(std::ifstream(cache_path));
    std::string version;
    std::string version_major;
    std::string version_minor;
    std::string version_patch;

    for (const auto& entry : cache_json["entries"]) {
      auto name = entry["name"].get<std::string>();
      if (name.find("PROJECT_VERSION") != std::string::npos) {
        if (name == "PROJECT_VERSION_MAJOR") {
          version_major = entry["value"].get<std::string>();
        } else if (name == "PROJECT_VERSION_MINOR") {
          version_minor = entry["value"].get<std::string>();
        } else if (name == "PROJECT_VERSION_PATCH") {
          version_patch = entry["value"].get<std::string>();
        } else if (name == "PROJECT_VERSION") {
          version = entry["value"].get<std::string>();
        }
      }
    }
    if (!version.empty()) {
      project.version = version;
    } else if (!version_major.empty() && !version_minor.empty() &&
               !version_patch.empty()) {
      project.version =
          version_major + "." + version_minor + "." + version_patch;
    } else {
      project.version = "0.1.0";
    }
  };
  version_parser();
  std::vector<fs::path> source_files;
  if (!code_model_json.contains("configurations")) {
    throw std::runtime_error(
        "Code model file does not contain configurations: " +
        code_model_path.string());
  }
  project.name = code_model_json["configurations"][0]["projects"][0]["name"]
                     .get<std::string>();

  for (const auto& config : code_model_json["configurations"]) {
    if (!config.contains("targets")) {
      continue;
    }
    for (const auto& target : config["targets"]) {
      if (target.contains("id")) {
        std::string id = target["id"].get<std::string>();
        if (isTestTarget(id, test_target)) {
          continue;
        }
      } else
        continue;
      if (target.contains("jsonFile")) {
        std::string id = target["jsonFile"].get<std::string>();
        if (isTestTarget(id, test_target)) {
          continue;
        }
      } else
        continue;
      if (target.contains("name")) {
        std::string id = target["name"].get<std::string>();
        if (isTestTarget(id, test_target)) {
          continue;
        }
      } else
        continue;
      source_files.push_back(code_model_path.parent_path() /
                             target["jsonFile"].get<std::string>());
    }
  }
  return std::make_pair(project, source_files);
};
}  // namespace mcpp
