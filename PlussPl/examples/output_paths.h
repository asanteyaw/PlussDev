#pragma once

#include <filesystem>
#include <string>

namespace plusspl::examples {

inline std::filesystem::path outputs_dir(const char* file_path) {
  const auto source = std::filesystem::path(file_path);
  const auto root = source.parent_path().parent_path().parent_path();
  const auto outputs = root / "outputs";
  std::filesystem::create_directories(outputs);
  return outputs;
}

inline std::string output_path(const char* file_path, const std::string& filename) {
  return (outputs_dir(file_path) / filename).string();
}

} // namespace plusspl::examples
