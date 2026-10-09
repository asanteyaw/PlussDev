# PlussTils: Paths and roots

```{raw} html
<meta name="pluss-doc-kind" content="guide">
```

[Back to PlussTils](../index.md) · [PlussTils Core Manual](core.md)

A loader reads and writes files, and every path it uses should land inside the project it belongs to. The paths utilities resolve each relative path under a root, create directories when asked, refuse paths that climb out of the root, and keep URL and folder roots side by side. This chapter covers the global root and how to reset it, the per-call options, a team policy, names and joined paths, URL roots, and independent contexts, with a complete program for each.

## The mental model

There is one global root, and every relative path is resolved under it. Resolving a path computes its location and checks that it stays inside the root, but does not create anything unless you ask. Asking for a directory with `create_dirs` makes it at that moment. A policy holds the team's defaults for creation and canonicalization, and each call may still override them.

A root that a single run needs for itself can be held in a `PathContext`, which keeps its own root and leaves the global one alone. URLs use the same idea with a `UrlContext`, which joins a relative path onto a base address instead of a folder.

## Worked tour

Every example is a complete program, and the output under it is copied from that program's run.

## Example 1: Pinning the project root

Every relative path in a run should resolve under the project. This program pins a project folder as the root, resolves a dataset path under it, and then resets the root to its default.

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

The dataset resolves to data/sales.csv under the project, and the root is the project folder. After the reset, the root is no longer the project.

## Example 2: Resolving a file that does not exist yet

A loader may name an output file before it writes it. Resolution computes the location and creates nothing, so this program checks that neither the file nor its folder exists after the call.

```cpp
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
```

Output

```text
model name: forest.bin
model exists yet: 0
model folder exists yet: 0
```

The name is forest.bin, and both the file and its folder are absent. Resolution is only a computation, so checking for the file later is still meaningful.

## Example 3: Creating an output folder on demand

A run writes its reports into a dated folder that may not exist yet. This program asks for the folder with `create_dirs`, then resolves a file inside it.

```cpp
// Making the output folder that a run needs. Asking for a directory with create_dirs makes it at the
// moment of the call, so the later writes do not have to check for it first.
#include <pluss/tils/aux/pathing.h>

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

The folder exists after the call, and the summary file resolves inside it, so the later write does not need to check for the folder first.

## Example 4: Refusing a path that leaves the root

A configuration value can contain a path that climbs out of the project. This program tries three requests: a normal one, one that climbs with two dots, and one that climbs and then names a system file.

```cpp
// Keeping a run inside its root. A path that climbs out of the root is refused before anything is
// opened, which stops a bad configuration value from reading a file it should never see.
#include <pluss/tils/aux/pathing.h>

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

The normal path is accepted under the root. The two that climb out are refused before anything is opened.

## Example 5: A team policy for path creation

A team wants every run to create the folders it needs. This program reads the default policy, changes one field, applies the change, and creates a folder with the new policy.

```cpp
// A team sets one policy for every path in a run: whether directories are created on demand and
// whether the root is canonicalized. The policy is read back so a run can log what it is using.
#include <pluss/tils/aux/pathing.h>

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

The default does not create folders automatically, and the team policy does. The logs folder exists after the call, because the policy now creates it.

## Example 6: Joining parts and naming inputs

A run reads several inputs by name. This program joins path parts into one path, names two inputs in a map, and tries to name the same key twice.

```cpp
// Building several paths at once from a list and from a name map. A run that reads three inputs can
// name them once and look them up by name, and a duplicate name is refused instead of overwritten.
#include <pluss/tils/aux/pathing.h>

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

The joined path is data/2024/sales/eu. The map holds the train input under its name, and the duplicate key is refused rather than overwritten.

## Example 7: Remote data under a URL root

A loader can read data from a web address the same way it reads a folder. This program builds a URL root, adds a relative path to it, and names two entries in a URL map.

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

The remote file is the base address followed by the relative path, and the root keeps its base. The map holds the two named entries.

## Example 8: Two independent roots at once

Two projects can be open at the same time. This program creates a path context for each and checks each root, then checks that the global root was not changed.

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

Each context holds its own root, and the global root is unchanged, so one project does not disturb another.

## Decisions

| You want | Use | Notes |
|---|---|---|
| Resolve a path under the project | `file_path` after `set_root_path` | Resolution computes the path and does not create it. |
| Create a folder for a run | `file_dir` with `create_dirs` | The folder exists when the call returns. |
| Set the defaults for a whole run | `set_path_policy` | Each call may still override a field. |
| Keep a root that another run should not see | `PathContext` | The global root is left alone. |
| Refer to remote data | `UrlContext` | The root is a base address, not a folder. |

## Pitfalls

**Expecting a resolved path to exist.** r02 shows the file and its folder are absent after resolution.

**Expecting a climbing path to be trusted.** r04 shows paths with dots that leave the root are refused.

**Expecting the global root to change for a local context.** r08 shows a path context leaves the global root alone.

## Where next

- [Loaders](loaders.md) -- the loader guide, which uses these paths to read datasets.
- [Deterministic seeds](core_seeds.md) -- the seed utilities that loaders use for repeatable shuffles.
