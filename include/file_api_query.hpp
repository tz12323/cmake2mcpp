#pragma once

#include <filesystem>
#include <string>

namespace mcpp {

class FileApiQuery {
 public:
  /**
   * @brief 创建 File API Query
   *
   * @param buildDir CMake Build 目录
   * @return true 成功
   * @return false 失败
   */
  bool Create(const std::filesystem::path& buildDir);
  FileApiQuery() = default;
  FileApiQuery(const std::filesystem::path& cmakeExecutablePath)
      : CmakeExecutablePath(cmakeExecutablePath) {}
  void SetCmakeExecutablePath(const std::filesystem::path& path);
 private:
  std::filesystem::path CmakeExecutablePath;
  static bool Touch(const std::filesystem::path& file);
  std::string GetCMakeVersion();
  static bool CmakeVersionCompare(const std::string& version1,
                                  const std::string& version2);
};

}  // namespace mcpp