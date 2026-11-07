// Keeping a run inside its root. A path that climbs out of the root is refused before anything is
// opened, which stops a bad configuration value from reading a file it should never see.
#include <pluss/tils/auxiliary/pathing.h>

#include <exception>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;
namespace loader = pluss::loader;

int main() {
  const fs::path project = fs::temp_directory_path() / "pluss_guide_escape";
  fs::remove_all(project);
  fs::create_directories(project);
  loader::set_root_path(project);

  const char* requests[] = {"data/train.csv", "../secrets.txt", "data/../../etc/hosts"};
  for (const char* request : requests) {
    try {
      const loader::PathRef path = loader::file_path(request);
      std::cout << request << " accepted under the root: "
                << (fs::path(path).lexically_relative(loader::root_path()).string().rfind("..", 0) != 0) << '\n';
    } catch (const std::exception&) {
      std::cout << request << " refused\n";
    }
  }

  loader::reset_root_path();
  fs::remove_all(project);
  return 0;
}
