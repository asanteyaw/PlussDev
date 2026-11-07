# `pluss::loader path and root functions`

[Back to Loaders](loaders.md) · [Loader API Reference](../api_reference.md)

## Signature

```cpp
#include <pluss/tils/auxiliary/pathing.h>

std::filesystem::path root_path();
void set_root_path(std::filesystem::path root, RootResolve resolve_mode = RootResolve::WorkingDirectory);
void reset_root_path();
PathPolicy path_policy();
void set_path_policy(PathPolicy policy);

PathRef file_path(std::filesystem::path subpath, PathOptions options = {});
std::vector<PathRef> file_paths(std::initializer_list<std::filesystem::path> subpaths, PathOptions options = {});
std::unordered_map<std::string, PathRef> file_path_map(
    std::initializer_list<std::pair<std::string_view, std::filesystem::path>> entries, PathOptions options = {});
DirRef file_dir(std::filesystem::path subpath, PathOptions options = {});
PathRef absolute_path(std::filesystem::path path, PathOptions options = {});
std::filesystem::path join_path(std::filesystem::path base, std::initializer_list<std::string_view> parts);
```

## What It Does

There is one process-wide root, set with `set_root_path` and read back with `root_path`; `reset_root_path` returns it to the auto-detected project root. `file_path` resolves a path under that root and checks that it does not climb outside it; it computes the location and does not create anything, unless `PathOptions::create_dirs` is set. `file_dir` does the same for a directory, and returns a `DirRef`, whose own `file` member resolves paths under that directory. `file_paths` and `file_path_map` resolve several paths at once, the map form keyed by a name you choose, and refuse a duplicate name. `absolute_path` resolves a path that is meant to be outside the root. `join_path` concatenates path parts without resolving or checking anything.

`path_policy` reads the process-wide policy, and `set_path_policy` replaces it. `auto_create_dirs`, when set, makes `file_path` and `file_dir` create what they resolve, the same as passing `PathOptions{.create_dirs = true}` on every call. `force_replace_dirs` only affects `file_dir`: when it and directory creation are both in effect, an existing directory at the resolved location is removed and recreated empty, instead of being left as-is; it has no effect on `file_path`, which never creates a directory outright.

## Parameters

| Parameter | Meaning |
|---|---|
| `subpath` / `path` | The path to resolve, relative to the root unless noted. |
| `options` | Per-call overrides of the create-directory policy. |
| `root` | The new process-wide root, for `set_root_path`. |
| `policy` | The new process-wide policy, for `set_path_policy`. |
| `base`, `parts` | The path and the parts to append, for `join_path`. |

## `PathPolicy` Fields

| Field | Meaning | Default |
|---|---|---|
| `canonicalize_root` | Whether the root is resolved to its canonical (symlink-free) form. | `true` |
| `auto_create_dirs` | Whether `file_path` and `file_dir` create what they resolve, unless overridden per call by `PathOptions::create_dirs`. | `false` |
| `force_replace_dirs` | Whether `file_dir` removes and recreates an existing directory instead of leaving it, when directory creation is in effect. Unless overridden per call by `PathOptions::force_replace_dirs`. | `false` |

## `PathOptions` Fields

| Field | Meaning | Default |
|---|---|---|
| `create_dirs` | Per-call override of `PathPolicy::auto_create_dirs`. | unset (falls back to the policy) |
| `force_replace_dirs` | Per-call override of `PathPolicy::force_replace_dirs`. | unset (falls back to the policy) |

## Returns

`file_path`/`absolute_path` return a `PathRef`. `file_dir` returns a `DirRef`. `file_paths`/`file_path_map` return a collection of `PathRef`. `join_path` returns a plain `std::filesystem::path`.

## Errors

A path that resolves outside the root is refused. A duplicate name in `file_path_map` is refused.

## Practical Examples

### Example 1

Example 1 resolves a path that does not exist yet, without creating anything.

```cpp
// Resolving the path to a file that does not exist yet. Resolution only computes the location; it does
// not create anything. A loader that writes the file later can name it now and check for it later.
#include <pluss/tils/auxiliary/pathing.h>

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
```

Output

```text
model name: forest.bin
model exists yet: 0
model folder exists yet: 0
```

The resolved name is correct, and neither the file nor its folder exists after the call.

### Example 2

Example 2 creates a directory on demand with file_dir, then resolves a file inside it.

```cpp
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
```

Output

```text
reports folder created: 1
summary inside reports: 1
```

The directory exists as soon as the call returns, and the file resolves inside it.

### Example 3

Example 3 tries three requests, one normal and two that climb out of the root.

```cpp
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
```

Output

```text
data/train.csv accepted under the root: 1
../secrets.txt refused
data/../../etc/hosts refused
```

The normal path is accepted, and both paths that climb out are refused.

### Example 4

Example 4 reads the default policy, changes auto_create_dirs, and applies it.

```cpp
// A team sets one policy for every path in a run: whether directories are created on demand and
// whether the root is canonicalized. The policy is read back so a run can log what it is using.
#include <pluss/tils/auxiliary/pathing.h>

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
```

Output

```text
auto create by default: false
auto create after the team policy: true
logs folder created by the policy: true
```

The default does not create directories automatically, and the changed policy does.

### Example 5

Example 5 joins path parts, and resolves a named map of two inputs, including a refused duplicate name.

```cpp
// Building several paths at once from a list and from a name map. A run that reads three inputs can
// name them once and look them up by name, and a duplicate name is refused instead of overwritten.
#include <pluss/tils/auxiliary/pathing.h>

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
```

Output

```text
joined parts: data/2024/sales/eu
train is named: 1
labels leaf: labels.csv
duplicate name refused
```

The joined path concatenates the parts, the map holds the named entries, and the duplicate name is refused.

### Example 6

Example 6 resolves a file path and a directory with create_dirs from a loader example.

```cpp
#include <pluss/tils/auxiliary/data_loader.h>
#include <pluss/pluss.h>
#include <pluss/tils/auxiliary/pathing.h>

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
```

Output

```text
train under root: true
train exists yet: false
cache directory created: true
cache is under root: true
escape refused: true
```

The file resolves under the root without creating it, and the directory is created and found under the root.

### Example 7

Example 7 shows force_replace_dirs clearing an existing directory, and that it has no bearing on file_path.

```cpp
// force_replace_dirs removes and recreates an existing directory instead of leaving it, when
// directory creation is in effect. It only affects file_dir; file_path never creates a directory
// outright, so the flag has nothing to act on there.
#include <pluss/tils/auxiliary/pathing.h>

#include <filesystem>
#include <fstream>
#include <iostream>

namespace fs = std::filesystem;
namespace loader = pluss::loader;

int main() {
  std::cout << std::boolalpha;
  const fs::path project = fs::temp_directory_path() / "pluss_guide_force_replace";
  fs::remove_all(project);
  fs::create_directories(project);
  loader::set_root_path(project);

  // Create the directory once, and leave a marker file inside it.
  loader::PathOptions create{};
  create.create_dirs = true;
  const loader::DirRef cache = loader::file_dir("cache/features", create);
  std::ofstream(fs::path(cache) / "marker.txt") << "first run";
  std::cout << "marker exists before: " << fs::exists(fs::path(cache) / "marker.txt") << '\n';

  // Without force_replace_dirs, resolving the same directory again leaves it as-is.
  const loader::DirRef kept = loader::file_dir("cache/features", create);
  std::cout << "marker survives without force_replace_dirs: "
            << fs::exists(fs::path(kept) / "marker.txt") << '\n';

  // With force_replace_dirs, the existing directory is removed and recreated empty.
  loader::PathOptions replace{};
  replace.create_dirs = true;
  replace.force_replace_dirs = true;
  const loader::DirRef replaced = loader::file_dir("cache/features", replace);
  std::cout << "marker gone after force_replace_dirs: "
            << !fs::exists(fs::path(replaced) / "marker.txt") << '\n';
  std::cout << "directory still exists after replacement: " << fs::is_directory(fs::path(replaced)) << '\n';

  loader::reset_root_path();
  fs::remove_all(project);
  return 0;
}
```

Output

```text
marker exists before: true
marker survives without force_replace_dirs: true
marker gone after force_replace_dirs: true
directory still exists after replacement: true
```

Resolving the directory again without the flag leaves the marker in place; setting it removes and recreates the directory, and the directory itself still exists afterward.

## See Also

- [`PathContext` and `UrlContext`](loader-path-context.md) -- a root held independently of the process-wide one.
