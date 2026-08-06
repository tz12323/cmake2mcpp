#include "target_parser.hpp"
#include <filesystem>
#include <fstream>
#include <optional>
namespace mcpp {

static fs::path GetIncludeDir(const std::vector<std::string>& headers) {
  if (headers.empty())
    return {};
  // 取第一个文件的父目录
  fs::path common = fs::path(headers.front()).parent_path();
  // 求所有路径的公共父目录
  for (size_t i = 1; i < headers.size(); ++i) {
    fs::path p = fs::path(headers[i]).parent_path();
    while (!common.empty()) {
      auto c = common.generic_string();
      auto s = p.generic_string();
      if (s == c || (s.size() > c.size() && s.compare(0, c.size(), c) == 0 &&
                     s[c.size()] == '/')) {
        break;
      }
      common = common.parent_path();
    }
  }
  // 优先返回名为 include 的祖先目录
  for (fs::path p = common; !p.empty(); p = p.parent_path()) {
    if (p.filename() == "include")
      return p;
  }
  return common;
}

TargetParser::TargetParser(const fs::path& path)
    : TargetFile(path),
      test_target({"test", "nightly", "example", "experimental"}) {};
void TargetParser::parseTargetJson() {
  std::ifstream targetFileStream(TargetFile);
  if (!targetFileStream.is_open()) {
    throw std::runtime_error("Failed to open target file: " +
                             TargetFile.string());
  }
  targetFileStream >> TargetJson;
}
bool TargetParser::isTestTarget() {
  auto ifTestinName = [&](const std::string& name) {
    auto lowerName = name;
    std::transform(lowerName.begin(), lowerName.end(), lowerName.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    for (const auto& test : test_target) {
      if (lowerName.find(test) != std::string::npos) {
        return true;
      }
    }
    return false;
  };
  if (TargetJson.contains("backtraceGraph")) {
    if (TargetJson.contains("files")) {
      for (const auto& file : TargetJson["files"]) {
        if (ifTestinName(file.get<std::string>())) {
          return true;
        }
      }
    }
  }
  if (TargetJson.contains("path")) {
    if (ifTestinName(TargetJson["build"].get<std::string>())) {
      return true;
    }
    if (ifTestinName(TargetJson["source"].get<std::string>())) {
      return true;
    }
  }
  if (TargetJson.contains("sources")) {
    for (const auto& source : TargetJson["sources"]) {
      if (ifTestinName(source["path"].get<std::string>())) {
        return true;
      }
    }
  }
  return false;
}
std::optional<Target> TargetParser::parse() {
  parseTargetJson();
  if (isTestTarget()) {
    return std::nullopt;
  }
  if (TargetJson.contains("type")) {
    if (TargetJson["type"].get<std::string>() == "UTILITY") {
      return std::nullopt;
    }
  }
  Target target;
  auto idParser = [](const std::string& id) {
    auto pos = id.find_last_of(':');
    if (pos != std::string::npos) {
      return id.substr(0, pos - 1);
    }
    return id;
  };
  if (TargetJson.contains("compileGroups")) {
    for (const auto& group : TargetJson["compileGroups"]) {
      for (const auto& fragment : group["compileCommandFragments"]) {
        if (fragment.contains("fragment")) {
          target.compileArgs.push_back(fragment["fragment"].get<std::string>());
        }
      }
      for (const auto& include : group["includes"]) {
        target.include_dirs.push_back(include["path"].get<std::string>());
      }
    }
  }
  if (TargetJson.contains("dependencies")) {
    for (const auto& dep : TargetJson["dependencies"]) {
      target.dependencies.push_back(idParser(dep["id"].get<std::string>()));
    }
  }
  if (TargetJson.contains("name")) {
    target.name = TargetJson["name"].get<std::string>();
  }
  auto headers = std::vector<std::string>();
  if (TargetJson.contains("sources")) {
    for (const auto& source : TargetJson["sources"]) {
      auto path = fs::path(source["path"].get<std::string>());
      if (path.extension() == ".cpp" || path.extension() == ".c" ||
          path.extension() == ".cc" || path.extension() == ".cxx" ||
          path.extension() == ".c++" || path.extension() == ".C" ||
          path.extension() == ".CPP" || path.extension() == ".CC" ||
          path.extension() == ".CXX" || path.extension() == ".C++" ||
          path.extension() == ".cppm" || path.extension() == ".ixx" ||
          path.extension() == ".mpp") {
        target.sources.push_back(source["path"].get<std::string>());
      } else {
        headers.push_back(source["path"].get<std::string>());
      }
    }
  }
  auto includeDir = GetIncludeDir(headers);
  if (!includeDir.empty()) {  /// 避免重复添加 include 目录，和INTERFACE_LIBRARY
                              /// 类型的 target 也没有 sources
    target.include_dirs.push_back(includeDir.generic_string());
  }
  if (TargetJson.contains("type")) {
    auto type = TargetJson["type"].get<std::string>();
    if (type == "EXECUTABLE") {
      target.kind = "bin";
    } else if (type == "STATIC_LIBRARY") {
      target.kind = "lib";
    } else if (type == "SHARED_LIBRARY") {
      target.kind = "shared";
    } else if (type == "MODULE_LIBRARY") {
      target.kind = "lib";
    } else if (type == "INTERFACE_LIBRARY") {
      target.kind = "lib";
    }
  }
  return target;
}
}  // namespace mcpp