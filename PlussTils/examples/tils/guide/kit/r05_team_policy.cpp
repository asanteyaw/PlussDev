// A team sets one policy for every path in a run: whether directories are created on demand and
// whether the root is canonicalized. The policy is read back so a run can log what it is using.
#include <pluss/tils/kit/pathing.h>

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;
namespace loader = pluss::loader;

int main() {
  const fs::path project = fs::temp_directory_path() / "pluss_guide_policy";
  fs::remove_all(project);
  fs::create_directories(project);
  loader::set_root_path(project);

  std::cout << std::boolalpha;
  const auto before = loader::path_policy();
  std::cout << "auto create by default: " << before.auto_create_dirs << '\n';

  loader::PathPolicy team = before;
  team.auto_create_dirs = true;
  loader::set_path_policy(team);
  std::cout << "auto create after the team policy: " << loader::path_policy().auto_create_dirs << '\n';

  const loader::DirRef logs = loader::file_dir("logs/nightly");
  std::cout << "logs folder created by the policy: " << fs::is_directory(fs::path(logs)) << '\n';

  loader::reset_root_path();
  fs::remove_all(project);
  return 0;
}
