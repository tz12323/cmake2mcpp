#include "toml_writer.hpp"
#include <spdlog/spdlog.h>
#include <toml++/toml.h>
#include <filesystem>
#include <set>
#include <string>
#include <vector>
#include "target_parser.hpp"
#include "toml++/impl/table.hpp"
namespace mcpp {
static toml::array to_array(const std::vector<std::string>& vec,
                            const fs::path& project_path) {
  toml::array arr;
  for (const auto& s : vec)
    arr.push_back("../../" + s);
  return arr;
}
static toml::array to_array(const std::set<std::string>& set,
                            const fs::path& project_path) {
  toml::array arr;
  for (const auto& s : set)
    arr.push_back("../../" + s);
  return arr;
}

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
    auto definesArray = std::vector<std::string>();
    for (const auto& target : targets) {
      toml::table targetTable;
      targetTable.insert("kind", target.kind);
      targetTable.insert("cxxflags", to_array(target.compileArgs));
      targetsTable.insert(target.name, targetTable);

      incudeDirsArray.insert(incudeDirsArray.end(), target.include_dirs.begin(),
                             target.include_dirs.end());
      sourcesArray.insert(sourcesArray.end(), target.sources.begin(),
                          target.sources.end());
      definesArray.insert(definesArray.end(), target.defines.begin(),
                          target.defines.end());
    }
    auto uniqueIncludeDirs =
        std::set<std::string>(incudeDirsArray.begin(), incudeDirsArray.end());
    auto uniqueSources =
        std::set<std::string>(sourcesArray.begin(), sourcesArray.end());
    auto uniqueDefines =
        std::set<std::string>(definesArray.begin(), definesArray.end());

    buildTable.insert("sources", to_array(uniqueSources));
    buildTable.insert("include_dirs", to_array(uniqueIncludeDirs));
    buildTable.insert("defines", to_array(uniqueDefines));
    root.insert("targets", targetsTable);
    root.insert("build", buildTable);
    std::ofstream outputFile(outputFilePath);
    if (!outputFile.is_open()) {
      throw std::runtime_error("Failed to open output file: " + outputFilePath);
    }
    outputFile << root;
  } catch (const std::exception& e) {
    spdlog::error("Error writing TOML file: {}", e.what());
    return false;
  }

  return true;
}
bool SubTomlWrite(const fs::path& project_path, const Target& target) {
  try {
    auto sub_dir_path =
        project_path / "cmake2mcpp_generated" / (target.name + "-member");
    toml::table root;
    root.insert("package", toml::table{
                               {"name", target.name + "-member"},
                               {"version", "0.1.0"},
                           });
    toml::table buildTable;
    toml::table targetTable;
    auto relativeOutputFilePath =
        [&](const std::vector<std::string>& uniquePath) {
          auto paths = std::vector<std::string>();
          for (auto& path : uniquePath) {
            fs::path p(path);
            if (p.is_absolute()) {
              paths.push_back("../../" +
                              fs::relative(p, project_path).string());
            } else {
              paths.push_back("../../" + path);
            }
          }
          return paths;
        };
    buildTable.insert("sources", to_array(target.sources, project_path));
    buildTable.insert("include_dirs",
                      to_array(relativeOutputFilePath(target.include_dirs)));
    buildTable.insert("defines", to_array(target.defines));
    buildTable.insert("cxxflags", to_array(target.compileArgs));
    targetTable.insert("kind", target.kind);
    root.insert("build", buildTable);
    root.insert("targets", toml::table{{target.name, targetTable}});
    if (!fs::exists(sub_dir_path)) {
      fs::create_directories(sub_dir_path);
    }
    auto sub_toml_path = sub_dir_path / "mcpp.toml";
    std::ofstream outputFile(sub_toml_path);
    if (!outputFile.is_open()) {
      spdlog::error("Failed to open output file: {}", sub_toml_path.string());
      return false;
    }
    outputFile << root;
    spdlog::info("Sub TOML file written successfully for target: {}",
                 target.name);
    return true;
  } catch (const std::exception& e) {
    spdlog::error("Error writing sub TOML file: {}", e.what());
    return false;
  }
}

bool MainTomlWrite(const fs::path& project_path,
                   const Project& project,
                   const std::vector<Target>& targets) {
  try {
    /// Write sub TOML files for each target
    std::vector<std::string> target_names;
    for (const auto& target : targets) {
      if (!SubTomlWrite(project_path, target)) {
        spdlog::error("Failed to write sub TOML for target: {}", target.name);
        continue;
      }
      target_names.push_back(target.name);
    }
    toml::table root;
    root.insert("package", toml::table{
                               {"name", project.name},
                               {"version", project.version},
                           });
    toml::table dependencesTable;
    for (const auto& target_name : target_names) {
      dependencesTable.insert(
          target_name + "-member",
          toml::table{
              {"path", (project_path.filename() / "cmake2mcpp_generated" /
                        (target_name + "-member"))
                           .string()},
          });
    }
    /// 创建临时的 build 表和 target 表，指向生成的源文件和库
    fs::path generated_source_path =
        project_path / "cmake2mcpp_generated" / "cmake2mcpp_generated.cpp";
    if (!fs::exists(generated_source_path)) {
      std::ofstream generated_source_file(generated_source_path);
      if (!generated_source_file.is_open()) {
        spdlog::error("Failed to create generated source file: {}",
                      generated_source_path.string());
      }
      generated_source_file
          << "// This is a generated source file for cmake2mcpp.\n"
          << "void cmake2mcpp_generate() {}\n";
      generated_source_file.close();
    }

    toml::table buildTable;
    buildTable.insert(
        "sources",
        toml::array{"cmake2mcpp_generated/cmake2mcpp_generated.cpp"});
    toml::table targetTable;
    targetTable.insert("kind", "lib");
    root.insert("targets", toml::table{{"cmake2mcpp_generated", targetTable}});
    root.insert("build", buildTable);
    root.insert("dependencies", dependencesTable);
    /*     toml::table dependenciesTable;
        for (const auto& target_name : target_names) {
          dependenciesTable.insert(
              target_name, toml::table{
                               {"path", (project_path.filename() /
                                         "cmake2mcpp_generated" / target_name)
                                            .string()},
                           });
        }
        root.insert("dependencies", dependenciesTable); */

    auto main_toml_path = project_path / "mcpp.toml";
    std::ofstream outputFile(main_toml_path);
    if (!outputFile.is_open()) {
      spdlog::error("Failed to open output file: {}", main_toml_path.string());
      return false;
    }
    outputFile << root;
    return true;
  } catch (const std::exception& e) {
    spdlog::error("Error writing main TOML file: {}", e.what());
    return false;
  }
}

}  // namespace mcpp