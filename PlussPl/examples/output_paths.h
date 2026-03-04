#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>

namespace plusspl::examples {

inline std::filesystem::path outputs_dir(const char* file_path) {
  const auto source = std::filesystem::path(file_path);
  const auto root = source.parent_path().parent_path().parent_path();
  const auto outputs = root / "outputs";
  std::filesystem::create_directories(outputs);
  return outputs;
}

inline std::filesystem::path output_format_dir(const char* file_path, const std::string& filename) {
  const auto extension = std::filesystem::path(filename).extension().string();
  if (extension.empty()) {
    throw std::invalid_argument("plusspl::examples::output_path: filename must include an extension");
  }
  std::string bucket = extension;
  if (!bucket.empty() && bucket.front() == '.') {
    bucket.erase(bucket.begin());
  }
  const auto dir = outputs_dir(file_path) / bucket;
  std::filesystem::create_directories(dir);
  return dir;
}

inline std::string output_path(const char* file_path, const std::string& filename) {
  return (output_format_dir(file_path, filename) / filename).string();
}

} // namespace plusspl::examples
