# `pluss::bi::Studio::source`

[Back to `Studio`](../bi-studio.md) · [PlussBI API Reference](../bi.md)

```cpp
#include <pluss/bi/specs.h>

Studio& source(const pluss::bundle::Nexus& nexus);
```

`source` gives the studio the named tables its views read. The studio keeps the nexus you pass. Setting a source replaces the earlier one.
A studio with boards but no source cannot render, because no view has any data to show.

## Overloads

This function has one overload.

## Parameters

| Parameter | Meaning |
|---|---|
| `nexus` | The bundle of named tables. Each view refers to a table in it by name. |

## Returns

The studio itself, so calls chain.

## Errors

Rendering without a source throws. A view that names a missing table is refused when the studio renders.

## Practical Examples

### Example 1

Example 1 renders a studio whose view reads the table named sales, and shows three rows.

```cpp
#include <pluss/bi/bi.h>
#include <pluss/bi/serve.h>
#include <pluss/bundle/bundle.h>
#include <pluss/mic.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace bi = pluss::bi;
namespace fs = std::filesystem;

// Three sales rows: regions north, south, north; amounts 10, 20, 30.
pluss::bundle::Nexus make_nexus() {
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame sales({
      {"region", Slate(pluss::make_note({"north", "south", "north"}))},
      {"amount", Slate(pluss::make_series(std::vector<double>{10, 20, 30}, {3}, cfg))},
  });
  return pluss::bundle::Nexus({{"sales", sales}});
}

// A studio with one board holding one table over the sales data.
bi::Studio overview_studio(const pluss::bundle::Nexus& nexus) {
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").title("Sales overview").grid(1, 1);
  studio.board("overview").view(bi::table("rows").query(bi::from("sales")).place(0, 0));
  return studio;
}

// The number of rows the overview table shows on a studio.
std::int64_t rows_shown(const bi::Studio& studio) {
  return studio.render().boards.front().tiles.front().table->height();
}

fs::path private_dir(const std::string& name) {
  const fs::path dir = fs::temp_directory_path() / name;
  fs::remove_all(dir);
  fs::create_directories(dir);
  return dir;
}

std::string read_file(const fs::path& path) {
  std::ifstream in(path, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

template <class Fn>
bool throws(Fn fn) {
  try {
    fn();
  } catch (const std::exception&) {
    return true;
  }
  return false;
}

int main() {
  // Rendering reads tables from the source. The table the view names must exist in it.
  const auto nexus = make_nexus();
  const auto studio = overview_studio(nexus);
  std::cout << "table found in source: " << nexus.has("sales") << '\n';
  std::cout << "rows shown: " << rows_shown(studio) << '\n';
  return 0;
}
```

Output

```text
table found in source: 1
rows shown: 3
```

### Example 2

Example 2 renders a studio that has boards but no source. The render is refused.

```cpp
#include <pluss/bi/bi.h>
#include <pluss/bi/serve.h>
#include <pluss/bundle/bundle.h>
#include <pluss/mic.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace bi = pluss::bi;
namespace fs = std::filesystem;

// Three sales rows: regions north, south, north; amounts 10, 20, 30.
pluss::bundle::Nexus make_nexus() {
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame sales({
      {"region", Slate(pluss::make_note({"north", "south", "north"}))},
      {"amount", Slate(pluss::make_series(std::vector<double>{10, 20, 30}, {3}, cfg))},
  });
  return pluss::bundle::Nexus({{"sales", sales}});
}

// A studio with one board holding one table over the sales data.
bi::Studio overview_studio(const pluss::bundle::Nexus& nexus) {
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").title("Sales overview").grid(1, 1);
  studio.board("overview").view(bi::table("rows").query(bi::from("sales")).place(0, 0));
  return studio;
}

// The number of rows the overview table shows on a studio.
std::int64_t rows_shown(const bi::Studio& studio) {
  return studio.render().boards.front().tiles.front().table->height();
}

fs::path private_dir(const std::string& name) {
  const fs::path dir = fs::temp_directory_path() / name;
  fs::remove_all(dir);
  fs::create_directories(dir);
  return dir;
}

std::string read_file(const fs::path& path) {
  std::ifstream in(path, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

template <class Fn>
bool throws(Fn fn) {
  try {
    fn();
  } catch (const std::exception&) {
    return true;
  }
  return false;
}

int main() {
  // A studio with boards but no source cannot render.
  bi::Studio studio("sales_studio");
  studio.board("overview").grid(1, 1);
  studio.board("overview").view(bi::table("rows").query(bi::from("sales")).place(0, 0));
  std::cout << "render without source refused: " << throws([&] { (void)studio.render(); }) << '\n';
  return 0;
}
```

Output

```text
render without source refused: 1
```

### Example 3

Example 3 replaces the source with one that holds an extra row. The next render shows four rows.

```cpp
#include <pluss/bi/bi.h>
#include <pluss/bi/serve.h>
#include <pluss/bundle/bundle.h>
#include <pluss/mic.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace bi = pluss::bi;
namespace fs = std::filesystem;

// Three sales rows: regions north, south, north; amounts 10, 20, 30.
pluss::bundle::Nexus make_nexus() {
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame sales({
      {"region", Slate(pluss::make_note({"north", "south", "north"}))},
      {"amount", Slate(pluss::make_series(std::vector<double>{10, 20, 30}, {3}, cfg))},
  });
  return pluss::bundle::Nexus({{"sales", sales}});
}

// A studio with one board holding one table over the sales data.
bi::Studio overview_studio(const pluss::bundle::Nexus& nexus) {
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").title("Sales overview").grid(1, 1);
  studio.board("overview").view(bi::table("rows").query(bi::from("sales")).place(0, 0));
  return studio;
}

// The number of rows the overview table shows on a studio.
std::int64_t rows_shown(const bi::Studio& studio) {
  return studio.render().boards.front().tiles.front().table->height();
}

fs::path private_dir(const std::string& name) {
  const fs::path dir = fs::temp_directory_path() / name;
  fs::remove_all(dir);
  fs::create_directories(dir);
  return dir;
}

std::string read_file(const fs::path& path) {
  std::ifstream in(path, std::ios::binary);
  return std::string((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
}

template <class Fn>
bool throws(Fn fn) {
  try {
    fn();
  } catch (const std::exception&) {
    return true;
  }
  return false;
}

int main() {
  // Setting a source replaces the earlier one. The next render reads the new data.
  const auto nexus = make_nexus();
  bi::Studio studio = overview_studio(nexus);
  std::cout << "rows from the first source: " << rows_shown(studio) << '\n';
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame larger({
      {"region", Slate(pluss::make_note({"north", "south", "north", "east"}))},
      {"amount", Slate(pluss::make_series(std::vector<double>{10, 20, 30, 40}, {4}, cfg))},
  });
  const pluss::bundle::Nexus replacement({{"sales", larger}});
  studio.source(replacement);
  std::cout << "rows from the replacement source: " << rows_shown(studio) << '\n';
  return 0;
}
```

Output

```text
rows from the first source: 3
rows from the replacement source: 4
```

## See Also

- [`render`](render.md) -- reads the source.
- [`Studio`](../bi-studio.md) -- the class.
