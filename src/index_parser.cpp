#include "index_parser.hpp"
#include <spdlog/spdlog.h>
#include <cstdlib>
#include <filesystem>
#include <fstream>
namespace mcpp {
IndexParser::IndexParser(const fs::path& buildDir) {
  fs::path replyDir = buildDir / ".cmake" / "api" / "v1" / "reply";
  for (const auto& entry : fs::directory_iterator(replyDir)) {
    if (entry.path().filename().string().starts_with("index-")) {
      indexFilePath = entry.path();
      break;
    }
  }
  if (indexFilePath.empty()) {
    spdlog::error("Cannot find index file in {}", replyDir.string());
    std::exit(EXIT_FAILURE);
  }
  spdlog::info("Index file found: {}", indexFilePath.string());
};
void IndexParser::ReadIndexFile() {
  std::ifstream indexFile(indexFilePath);
  if (!indexFile.is_open()) {
    throw std::runtime_error("Failed to open index file: " +
                             indexFilePath.string());
  }
  indexFile >> indexJson;
};
fs::path IndexParser::Parse() {
  ReadIndexFile();
  if (!indexJson.contains("objects")) {
    spdlog::error("Missing 'objects' in index file");
    std::exit(EXIT_FAILURE);
  }
  for (const auto& object : indexJson["objects"]) {
    if (!object.contains("kind"))
      continue;

    if (!object.contains("jsonFile"))
      continue;

    if (object["kind"] == "codemodel") {
      return indexFilePath.parent_path() /
             object["jsonFile"].get<std::string>();
    }
  }

  throw std::runtime_error("Cannot find codemodel in " +
                           indexFilePath.string());
};
}  // namespace mcpp
