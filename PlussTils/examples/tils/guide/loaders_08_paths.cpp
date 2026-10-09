#include <pluss/tils/aux/data_loader.h>
#include <pluss/pluss.h>
#include <pluss/tils/aux/pathing.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

int main() {
  std::cout << std::boolalpha;
  namespace fs = std::filesystem;
  namespace loader = pluss::loader;

  // A private root for this example, so nothing outside it is touched.
  const fs::path root = fs::temp_directory_path() / "pluss_loader_paths_example";
  fs::remove_all(root);
  fs::create_directories(root);
  loader::set_root_path(root);

  // A file path resolves under the root. It does not create the file.
  const loader::PathRef train = loader::file_path("data/train.csv");
  // The root is canonicalized, so compare against the loader's own root.
  const fs::path canonical_root = loader::root_path();
  std::cout << "train under root: " << (fs::path(train).lexically_relative(canonical_root).string() == "data/train.csv") << '\n';
  std::cout << "train exists yet: " << fs::exists(train) << '\n';

  // A directory with create_dirs makes the directory now.
  loader::PathOptions create{};
  create.create_dirs = true;
  const loader::DirRef cache = loader::file_dir("cache/features", create);
  std::cout << "cache directory created: " << fs::is_directory(fs::path(cache)) << '\n';
  std::cout << "cache is under root: " << (fs::path(cache).lexically_relative(canonical_root).string() == "cache/features") << '\n';

  // A path that climbs out of the root is refused.
  bool refused = false;
  try {
    (void)loader::file_path("../escape.csv");
  } catch (const std::exception&) {
    refused = true;
  }
  std::cout << "escape refused: " << refused << '\n';

  loader::reset_root_path();
  fs::remove_all(root);
}
