// Two projects open at once, each with its own root. A PathContext holds its own root, so it does not
// disturb the global one, and each context resolves paths under its own directory.
#include <pluss/tils/kit/pathing.h>

#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;
namespace loader = pluss::loader;

int main() {
  const fs::path alpha = fs::temp_directory_path() / "pluss_guide_alpha";
  const fs::path beta = fs::temp_directory_path() / "pluss_guide_beta";
  fs::remove_all(alpha);
  fs::remove_all(beta);
  fs::create_directories(alpha);
  fs::create_directories(beta);

  const auto first = loader::PathContext::from_root(alpha);
  const auto second = loader::PathContext::from_root(beta);
  std::cout << "first root is alpha: " << (first.root_path() == fs::canonical(alpha)) << '\n';
  std::cout << "second root is beta: " << (second.root_path() == fs::canonical(beta)) << '\n';
  std::cout << "global root untouched: " << (loader::root_path() != fs::canonical(alpha)) << '\n';

  fs::remove_all(alpha);
  fs::remove_all(beta);
  return 0;
}
