// Remote data lives under a URL instead of a folder. A URL context keeps the base address and adds a
// relative path to it, so a loader can name a remote file the same way it names a local one.
#include <pluss/tils/kit/pathing.h>

#include <iostream>

namespace loader = pluss::loader;

int main() {
  const auto remote = loader::UrlContext::from_root("https://data.example.org/exports");
  const loader::UrlRef monthly = remote.file_path("2024/march.csv");
  std::cout << "remote file: " << monthly.string() << '\n';
  std::cout << "root kept: " << remote.root() << '\n';

  const auto named = remote.file_path_map({{"march", "2024/march.csv"}, {"april", "2024/april.csv"}});
  std::cout << "named entries: " << named.size() << '\n';
  return 0;
}
