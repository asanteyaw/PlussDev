// Resolving the path to a file that does not exist yet. Resolution only computes the location; it does
// not create anything. A loader that writes the file later can name it now and check for it later.
#include <pluss/tils/aux/pathing.h>

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;
namespace loader = pluss::loader;

int main() {
  const fs::path project = fs::temp_directory_path() / "pluss_guide_resolve";
  fs::remove_all(project);
  fs::create_directories(project);
  loader::set_root_path(project);

  const loader::PathRef model = loader::file_path("models/forest.bin");
  std::cout << "model name: " << fs::path(model).filename().string() << '\n';
  std::cout << "model exists yet: " << fs::exists(model) << '\n';
  std::cout << "model folder exists yet: " << fs::exists(fs::path(model).parent_path()) << '\n';

  loader::reset_root_path();
  fs::remove_all(project);
  return 0;
}
