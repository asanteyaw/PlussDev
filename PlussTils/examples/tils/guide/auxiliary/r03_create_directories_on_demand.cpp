// Making the output folder that a run needs. Asking for a directory with create_dirs makes it at the
// moment of the call, so the later writes do not have to check for it first.
#include <pluss/tils/auxiliary/pathing.h>

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;
namespace loader = pluss::loader;

int main() {
  const fs::path project = fs::temp_directory_path() / "pluss_guide_dirs";
  fs::remove_all(project);
  fs::create_directories(project);
  loader::set_root_path(project);

  loader::PathOptions create{};
  create.create_dirs = true;
  const loader::DirRef reports = loader::file_dir("runs/2024/reports", create);
  std::cout << "reports folder created: " << fs::is_directory(fs::path(reports)) << '\n';

  // A file inside the directory resolves under it.
  const loader::PathRef summary = reports.file("summary.txt");
  std::cout << "summary inside reports: " << (fs::path(summary).parent_path() == fs::path(reports)) << '\n';

  loader::reset_root_path();
  fs::remove_all(project);
  return 0;
}
