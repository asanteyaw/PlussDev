# `pluss::loader::PathContext and UrlContext`

[Back to Loaders](loaders.md) · [Loader API Reference](../api_reference.md)

```cpp
#include <pluss/tils/aux/pathing.h>

enum class RootResolve { WorkingDirectory, CallerFile };

class PathContext {
public:
  explicit PathContext(std::filesystem::path root, RootResolve resolve_mode = RootResolve::WorkingDirectory);
  static PathContext from_root(std::filesystem::path root, RootResolve resolve_mode = RootResolve::WorkingDirectory);
  std::filesystem::path root_path() const;
  void set_root_path(std::filesystem::path root, RootResolve resolve_mode = RootResolve::WorkingDirectory);
};

class UrlContext {
public:
  explicit UrlContext(std::string root_url);
  static UrlContext from_root(std::string root_url);
  const std::string& root() const noexcept;
  UrlRef file_path(std::string_view subpath) const;
  std::vector<UrlRef> file_paths(std::initializer_list<std::string_view> subpaths) const;
  std::unordered_map<std::string, UrlRef> file_path_map(
      std::initializer_list<std::pair<std::string_view, std::string_view>> entries) const;
};
```

`PathContext` holds its own root, independent of the process-wide root the free functions (`root_path`, `file_path`, and the rest) read. Use it when two pieces of code need different roots at the same time, such as two datasets loaded from different projects in one program; building or changing a `PathContext` never touches the global root.

`UrlContext` is the same idea for a remote address instead of a local folder: it holds a base URL, and `file_path` joins a relative path onto it, returning a `UrlRef` rather than resolving anything on disk. `file_paths` and `file_path_map` build several URLs at once, the map form keyed by a name you choose, the same way the free `file_paths`/`file_path_map` functions do for local paths.

## Parameters

| Parameter | Meaning |
|---|---|
| `root` / `root_url` | The root to hold: a local path for `PathContext`, a base URL for `UrlContext`. |
| `resolve_mode` | Whether a relative root is resolved against the working directory or the caller's own source file. |
| `subpath` / `subpaths` / `entries` | The relative path, paths, or named paths to join onto the context's root. |

## Returns

`root_path()`/`root()` return the context's own root. `file_path` and its plural forms return `UrlRef` values built from the context's root.

## Errors

None beyond what the underlying path or URL resolution refuses.

## Practical Examples

### Example 1

Example 1 pins a project root in the process-wide, global sense, for contrast with a PathContext's own root.

```cpp
// Pinning a project root so every relative path resolves under one directory. Paths printed here are
// shown relative to the root, so the output is the same wherever the project is checked out.
#include <pluss/tils/aux/pathing.h>

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
```

Output

```text
dataset relative to root: data/sales.csv
root is the project: 1
reset root is no longer the project: 1
```

A path resolves under the pinned root, and resetting the root changes what later resolutions use.

### Example 2

Example 2 builds two PathContext values for two projects open at once, and checks the global root is untouched.

```cpp
// Two projects open at once, each with its own root. A PathContext holds its own root, so it does not
// disturb the global one, and each context resolves paths under its own directory.
#include <pluss/tils/aux/pathing.h>

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
```

Output

```text
first root is alpha: 1
second root is beta: 1
global root untouched: 1
```

Each context holds its own root, and building or using them never changes the global root.

### Example 3

Example 3 builds a UrlContext and joins paths onto it, including a named map.

```cpp
// Remote data lives under a URL instead of a folder. A URL context keeps the base address and adds a
// relative path to it, so a loader can name a remote file the same way it names a local one.
#include <pluss/tils/aux/pathing.h>

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
```

Output

```text
remote file: https://data.example.org/exports/2024/march.csv
root kept: https://data.example.org/exports
named entries: 2
```

The joined URL is the base address followed by the relative path, the context keeps its own base, and the map holds the named entries.

## See Also

- [Paths and roots](loader-paths.md) -- the free functions that read and write the process-wide root.
