#include "toml_writer.hpp"
#include <toml++/toml.h>
#include <iostream>
#include <set>
#include <string>
#include <vector>
#include "toml++/impl/table.hpp"
namespace mcpp {
static toml::array to_array(const std::vector<std::string>& vec) {
  toml::array arr;
  for (const auto& s : vec)
    arr.push_back(s);
  return arr;
}
static toml::array to_array(const std::set<std::string>& set) {
  toml::array arr;
  for (const auto& s : set)
    arr.push_back(s);
  return arr;
}
bool TomlWrite(const std::string& outputFilePath,
               const Project& project,
               const std::vector<Target>& targets) {
  try {
    toml::table root;

    root.insert("package", toml::table{
                               {"name", project.name},
                               {"version", project.version},
                           });
    toml::array targetArray;
    toml::table targetsTable;
    toml::table buildTable;
    auto incudeDirsArray = std::vector<std::string>();
    auto sourcesArray = std::vector<std::string>();
    for (const auto& target : targets) {
      toml::table targetTable;
      targetTable.insert("kind", target.kind);
      targetTable.insert("cxxflags", to_array(target.compileArgs));
      targetsTable.insert(target.name, targetTable);

      incudeDirsArray.insert(incudeDirsArray.end(), target.include_dirs.begin(),
                             target.include_dirs.end());
      sourcesArray.insert(sourcesArray.end(), target.sources.begin(),
                          target.sources.end());
    }
    auto uniqueIncludeDirs =
        std::set<std::string>(incudeDirsArray.begin(), incudeDirsArray.end());
    auto uniqueSources =
        std::set<std::string>(sourcesArray.begin(), sourcesArray.end());
    buildTable.insert("sources", to_array(uniqueSources));
    buildTable.insert("include_dirs", to_array(uniqueIncludeDirs));
    root.insert("targets", targetsTable);
    root.insert("build", buildTable);
    std::ofstream outputFile(outputFilePath);
    if (!outputFile.is_open()) {
      throw std::runtime_error("Failed to open output file: " + outputFilePath);
    }
    outputFile << root;
  } catch (const std::exception& e) {
    std::cerr << "Error writing TOML file: " << e.what() << std::endl;
    return false;
  }

  return true;
}

}  // namespace mcpp