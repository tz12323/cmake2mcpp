#pragma once
#include "code_model_parser.hpp"
#include "target_parser.hpp"

namespace mcpp {
bool TomlWrite(const std::string& outputFilePath,
               const Project& project,
               const std::vector<Target>& targets);
bool MainTomlWrite(const fs::path& project_path,
                   const Project& project,
                   const std::vector<Target>& targets);
}  // namespace mcpp
