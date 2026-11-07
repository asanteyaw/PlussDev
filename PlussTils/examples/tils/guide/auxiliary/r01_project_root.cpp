// Pinning a project root so every relative path resolves under one directory. Paths printed here are
// shown relative to the root, so the output is the same wherever the project is checked out.
#include <pluss/tils/auxiliary/pathing.h>

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;
namespace loader = pluss::loader;

int main() {
  const fs::path project = fs::temp_directory_path() / "pluss_guide_project_root";
  fs::remove_all(project);
  fs::create_directories(project);

  loader::set_root_path(project);
  const fs::path root = loader::root_path();

  const loader::PathRef dataset = loader::file_path("data/sales.csv");
  std::cout << "dataset relative to root: " << fs::path(dataset).lexically_relative(root).string() << '\n';
  std::cout << "root is the project: " << (root == fs::canonical(project)) << '\n';

  loader::reset_root_path();
  std::cout << "reset root is no longer the project: " << (loader::root_path() != fs::canonical(project)) << '\n';
  fs::remove_all(project);
  return 0;
}
