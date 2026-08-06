#pragma once
#include <filesystem>
#include <nlohmann/json.hpp>
#include <string>
#include <vector>
namespace fs = std::filesystem;
using json = nlohmann::json;
namespace mcpp {
struct Target {
  std::string kind;
  std::string name;
  std::vector<std::string> sources;
  std::vector<std::string> compileArgs;
  std::vector<std::string> dependencies;
  std::vector<std::string> include_dirs;
};
class TargetParser {
 public:
  TargetParser(const fs::path& path);
  std::optional<Target> parse();

 private:
  json TargetJson;
  fs::path TargetFile;
  std::vector<std::string> test_target;
  void parseTargetJson();
  bool isTestTarget();
};
}  // namespace mcpp
