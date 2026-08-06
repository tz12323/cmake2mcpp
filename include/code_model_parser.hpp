#pragma once
#include <filesystem>
#include <nlohmann/json.hpp>
#include <string>
#include <utility>
#include <vector>
namespace fs = std::filesystem;
using json = nlohmann::json;

namespace mcpp {
struct Project {
  std::string name;
  std::string version;
};
class CodeModelParser {
 public:
  CodeModelParser(const fs::path& path);
  CodeModelParser(const fs::path& path,
                  const std::vector<std::string>& test_target);
  std::pair<Project, std::vector<fs::path>> parse();
  static bool isTestTarget(const std::string& target,
                           const std::vector<std::string>& test_target);

 private:
  void ReadCodeModelFile();
  fs::path code_model_path;
  json code_model_json;
  std::vector<std::string> test_target;
};
}  // namespace mcpp
