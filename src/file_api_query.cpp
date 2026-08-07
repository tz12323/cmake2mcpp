#include "file_api_query.hpp"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <print>
#include <vector>

#ifdef _WIN32
constexpr auto NullDevice = "nul";
#else
constexpr auto NullDevice = "/dev/null";
#endif

namespace fs = std::filesystem;

namespace mcpp {

bool FileApiQuery::Touch(const fs::path& file) {
  try {
    if (fs::exists(file))
      return true;

    std::ofstream ofs(file);

    return ofs.good();
  } catch (...) {
    return false;
  }
}
void FileApiQuery::SetCmakeExecutablePath(const fs::path& path) {
  CmakeExecutablePath = path;
}
bool FileApiQuery::Create(const fs::path& buildDir,
                          const std::vector<std::string>& cmake_args) {
  /// 检查 CMakeLists.txt 是否存在
  auto tolower = [](const std::string& str) -> std::string {
    std::string result;
    result.reserve(str.size());
    for (char c : str) {
      result.push_back(
          static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }
    return result;
  };
  bool hasCMakeLists = false;
  for (const auto& file : fs::directory_iterator(buildDir.parent_path())) {
    if (file.is_directory()) {
      continue;
    }
    auto filename = tolower(file.path().filename().string());
    if (filename == "cmakelists.txt") {
      hasCMakeLists = true;
    }
  }
  if (!hasCMakeLists) {
    std::cerr << "No CMakeLists.txt found in the project directory."
              << std::endl;
    return false;
  }

  try {
    fs::path queryDir =
        buildDir / ".cmake" / "api" / "v1" / "query" / "client-mcpp";

    fs::create_directories(queryDir);

    bool ok = true;

    ok &= Touch(queryDir / "codemodel-v2");
    ok &= Touch(queryDir / "cache-v2");
    ok &= Touch(queryDir / "cmakeFiles-v1");
    ok &= Touch(queryDir / "toolchains-v1");
    if (!CmakeVersionCompare(GetCMakeVersion(), "3.26.0")) {
      ok &= Touch(queryDir / "configureLog-v1");
      std::cout << "CMake version >= 3.26, configureLog-v1 created\n";
    } else {
      std::cout << "CMake version < 3.26, configureLog-v1 not created\n";
    }
    auto command =
        (CmakeExecutablePath.empty() ? "cmake" : CmakeExecutablePath.string()) +
        " -B " + buildDir.string() + " -S " + buildDir.parent_path().string() +
        " -DCMAKE_EXPORT_COMPILE_COMMANDS=ON";
    for (const auto& arg : cmake_args) {
      command += " -D" + arg;
    }
    std::cout << "Executing command: " << command << std::endl;
    command = command + " > " + std::string(NullDevice) + " 2>&1";

    system(command.c_str());
    /*     client-mcpp/
            ├── codemodel-v2      // 工程、Target、源文件、编译信息（必需）
            ├── cache-v2          // Cache 变量（可选）
            ├── cmakeFiles-v1     // CMakeLists 与包含关系（可选）
            ├── toolchains-v1     // 工具链信息（可选）
            └── configureLog-v1   // CMake 配置日志（可选，CMake 3.26+） */

    return ok;
  } catch (...) {
    return false;
  }
}

std::string FileApiQuery::GetCMakeVersion() {
  const char* cmd;
  if (CmakeExecutablePath.empty()) {
    std::println(
        "CMake executable path not specified, using 'cmake --version' from "
        "PATH.");
    cmd = "cmake --version";  // 未指定 CMake 可执行文件路径
  } else {
    std::string tmp = CmakeExecutablePath.string() + " --version";
    cmd = tmp.c_str();
  }

#ifdef _WIN32
  FILE* pipe = _popen(cmd, "rt");
#else
  FILE* pipe = popen(cmd, "r");
#endif
  if (!pipe) {
    return {};  // 调用失败：cmake 不在 PATH 或无权限
  }
  char buffer[256];
  std::string first_line;
  // cmake 版本号永远在第一行输出
  if (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
    first_line = buffer;
  }
#ifdef _WIN32
  _pclose(pipe);
#else
  pclose(pipe);
#endif
  // 解析固定格式：cmake version x.y.z
  const std::string prefix = "cmake version ";
  auto pos = first_line.find(prefix);
  if (pos == std::string::npos) {
    return {};  // 输出格式不匹配
  }

  // 提取版本号，去除末尾换行/回车符
  std::string version = first_line.substr(pos + prefix.size());
  while (!version.empty() &&
         (version.back() == '\n' || version.back() == '\r')) {
    version.pop_back();
  }
  return version;
}
bool FileApiQuery::CmakeVersionCompare(const std::string& version1,
                                       const std::string& version2) {
  auto parseVersion = [](const std::string& version) -> std::vector<int> {
    std::vector<int> parts;
    // 先截断 '-' 之后的所有后缀（-rc、-beta 等）
    const size_t dashPos = version.find('-');
    const size_t endPos =
        (dashPos == std::string::npos) ? version.size() : dashPos;
    size_t start = 0;
    while (start < endPos) {
      size_t end = version.find('.', start);
      // 超出有效范围就截断到 endPos
      if (end == std::string::npos || end > endPos) {
        end = endPos;
      }
      const std::string seg = version.substr(start, end - start);
      // 空段视为格式非法
      if (seg.empty())
        return {};
      // 校验全为数字
      for (const char c : seg) {
        if (!std::isdigit(static_cast<unsigned char>(c))) {
          return {};
        }
      }
      try {
        parts.push_back(std::stoi(seg));
      } catch (...) {
        return {};  // 溢出等转换失败
      }
      start = end + 1;
    }
    return parts;
  };

  auto v1_parts = parseVersion(version1);
  auto v2_parts = parseVersion(version2);

  for (size_t i = 0; i < std::max(v1_parts.size(), v2_parts.size()); ++i) {
    int v1_part = (i < v1_parts.size()) ? v1_parts[i] : 0;
    int v2_part = (i < v2_parts.size()) ? v2_parts[i] : 0;

    if (v1_part < v2_part) {
      return true;  // version1 < version2
    } else if (v1_part > v2_part) {
      return false;  // version1 > version2
    }
  }
  return false;  // version1 == version2
}
}  // namespace mcpp