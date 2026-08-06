#include "index_parser.hpp"
#include <filesystem>
#include <fstream>
#include <iostream>
namespace mcpp {
IndexParser::IndexParser(const fs::path& buildDir) {
  fs::path replyDir = buildDir / ".cmake" / "api" / "v1" / "reply";
  for (const auto& entry : fs::directory_iterator(replyDir)) {
    if (entry.path().filename().string().find("index-") != std::string::npos) {
      indexFilePath = entry.path();
      break;
    }
  }
  if (indexFilePath.empty()) {
    throw std::runtime_error("Cannot find index file in " + replyDir.string());
  }
  std::cout << "Index file found: " << indexFilePath << std::endl;
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
  if (!indexJson.contains("objects"))
    throw std::runtime_error("Missing 'objects'");

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
