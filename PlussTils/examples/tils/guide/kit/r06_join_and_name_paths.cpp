// Building several paths at once from a list and from a name map. A run that reads three inputs can
// name them once and look them up by name, and a duplicate name is refused instead of overwritten.
#include <pluss/tils/kit/pathing.h>

#include <exception>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;
namespace loader = pluss::loader;

int main() {
  const fs::path project = fs::temp_directory_path() / "pluss_guide_names";
  fs::remove_all(project);
  fs::create_directories(project);
  loader::set_root_path(project);

  const fs::path joined = loader::join_path("data", {"2024", "sales", "eu"});
  std::cout << "joined parts: " << joined.string() << '\n';

  const auto inputs = loader::file_path_map({{"train", "data/train.csv"}, {"labels", "data/labels.csv"}});
  std::cout << "train is named: " << inputs.count("train") << '\n';
  std::cout << "labels leaf: " << fs::path(inputs.at("labels")).filename().string() << '\n';

  try {
    (void)loader::file_path_map({{"train", "a.csv"}, {"train", "b.csv"}});
    std::cout << "duplicate accepted\n";
  } catch (const std::exception&) {
    std::cout << "duplicate name refused\n";
  }

  loader::reset_root_path();
  fs::remove_all(project);
  return 0;
}
