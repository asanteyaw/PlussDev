# `pluss::bi::Control::key`

[Back to `Control`](../bi-control.md) · [PlussBI API Reference](../bi.md)

```cpp
#include <pluss/bi/specs.h>

const std::string& key() const noexcept;
Control& key(std::string_view value);
```

`key` gives the control its name. The name must be unique on its board, and the control's tile carries the same key.

## Overloads

This member has two overloads: a getter with no arguments, and a setter that takes the key.

## Parameters

| Parameter | Meaning |
|---|---|
| `value` | The key to store. |

## Returns

The getter returns the stored key. The setter returns the control, so it chains.

## Errors

A duplicate key on one board is refused when the studio renders.

## Practical Examples

### Example 1

Example 1 builds a control under a key and reads it back.

```cpp
#include <pluss/bi/bi.h>
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

// Six sales rows. Regions: north, south, north, east, south, north. Months: jan, jan, feb, feb, mar, mar.
// Amounts 10 to 60, units 1 to 6, and one date per row from January to June 2024.
pluss::bundle::Nexus make_nexus() {
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame sales({
      {"region", Slate(pluss::make_note({"north", "south", "north", "east", "south", "north"}))},
      {"month", Slate(pluss::make_note({"jan", "jan", "feb", "feb", "mar", "mar"}))},
      {"amount", Slate(pluss::make_series(std::vector<double>{10, 20, 30, 40, 50, 60}, {6}, cfg))},
      {"units", Slate(pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6}, {6}, cfg))},
      {"placed", Slate(pluss::make_timeline({"2024-01-05", "2024-02-10", "2024-03-15", "2024-04-20", "2024-05-25", "2024-06-30"}))},
  });
  return pluss::bundle::Nexus({{"sales", sales}});
}

// Renders a studio that holds one board with one view, and returns the first rendered tile.
template <class Build>
bi::TileRender one_tile(Build build) {
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 1);
  studio.board("overview").view(build());
  return studio.render().boards.front().tiles.front();
}

// The text of the page a studio exports, for checks on what is shown.
std::string page_text(const bi::Studio& studio) {
  const fs::path dir = fs::temp_directory_path() / "pluss_view_page_example";
  fs::remove_all(dir);
  fs::create_directories(dir);
  studio.export_html((dir / "page.html").string(), "T", 900, 600, bi::DashboardChrome::None);
  std::ifstream in((dir / "page.html").string());
  std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  fs::remove_all(dir);
  return text;
}

// Renders a studio that holds one board with one control, and returns the first rendered tile.
bi::TileRender control_tile(bi::Control control) {
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 1);
  studio.board("overview").control(control);
  return studio.render().boards.front().tiles.front();
}

// True when the exported page contains the given text.
bool page_has(const bi::Studio& studio, const std::string& needle) {
  return page_text(studio).find(needle) != std::string::npos;
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
  std::cout << std::boolalpha;
  bi::Control control = bi::category("pick", "region", "Region");
  std::cout << "control key: " << (control.key()) << '\n';
  return 0;
}
```

Output

```text
control key: pick
```

The printed key is the one given to the constructor.

### Example 2

Example 2 reads the key from the tile the studio rendered.

```cpp
#include <pluss/bi/bi.h>
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

// Six sales rows. Regions: north, south, north, east, south, north. Months: jan, jan, feb, feb, mar, mar.
// Amounts 10 to 60, units 1 to 6, and one date per row from January to June 2024.
pluss::bundle::Nexus make_nexus() {
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame sales({
      {"region", Slate(pluss::make_note({"north", "south", "north", "east", "south", "north"}))},
      {"month", Slate(pluss::make_note({"jan", "jan", "feb", "feb", "mar", "mar"}))},
      {"amount", Slate(pluss::make_series(std::vector<double>{10, 20, 30, 40, 50, 60}, {6}, cfg))},
      {"units", Slate(pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6}, {6}, cfg))},
      {"placed", Slate(pluss::make_timeline({"2024-01-05", "2024-02-10", "2024-03-15", "2024-04-20", "2024-05-25", "2024-06-30"}))},
  });
  return pluss::bundle::Nexus({{"sales", sales}});
}

// Renders a studio that holds one board with one view, and returns the first rendered tile.
template <class Build>
bi::TileRender one_tile(Build build) {
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 1);
  studio.board("overview").view(build());
  return studio.render().boards.front().tiles.front();
}

// The text of the page a studio exports, for checks on what is shown.
std::string page_text(const bi::Studio& studio) {
  const fs::path dir = fs::temp_directory_path() / "pluss_view_page_example";
  fs::remove_all(dir);
  fs::create_directories(dir);
  studio.export_html((dir / "page.html").string(), "T", 900, 600, bi::DashboardChrome::None);
  std::ifstream in((dir / "page.html").string());
  std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  fs::remove_all(dir);
  return text;
}

// Renders a studio that holds one board with one control, and returns the first rendered tile.
bi::TileRender control_tile(bi::Control control) {
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 1);
  studio.board("overview").control(control);
  return studio.render().boards.front().tiles.front();
}

// True when the exported page contains the given text.
bool page_has(const bi::Studio& studio, const std::string& needle) {
  return page_text(studio).find(needle) != std::string::npos;
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
  const auto tile = control_tile(bi::category("pick", "region", "Region").place(0, 0));
  std::cout << "tile key: " << tile.key << '\n';
  return 0;
}
```

Output

```text
tile key: pick
```

The tile key is the control key.

### Example 3

Example 3 places two controls with the same key on one board. The studio refuses the second.

```cpp
#include <pluss/bi/bi.h>
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

// Six sales rows. Regions: north, south, north, east, south, north. Months: jan, jan, feb, feb, mar, mar.
// Amounts 10 to 60, units 1 to 6, and one date per row from January to June 2024.
pluss::bundle::Nexus make_nexus() {
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame sales({
      {"region", Slate(pluss::make_note({"north", "south", "north", "east", "south", "north"}))},
      {"month", Slate(pluss::make_note({"jan", "jan", "feb", "feb", "mar", "mar"}))},
      {"amount", Slate(pluss::make_series(std::vector<double>{10, 20, 30, 40, 50, 60}, {6}, cfg))},
      {"units", Slate(pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6}, {6}, cfg))},
      {"placed", Slate(pluss::make_timeline({"2024-01-05", "2024-02-10", "2024-03-15", "2024-04-20", "2024-05-25", "2024-06-30"}))},
  });
  return pluss::bundle::Nexus({{"sales", sales}});
}

// Renders a studio that holds one board with one view, and returns the first rendered tile.
template <class Build>
bi::TileRender one_tile(Build build) {
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 1);
  studio.board("overview").view(build());
  return studio.render().boards.front().tiles.front();
}

// The text of the page a studio exports, for checks on what is shown.
std::string page_text(const bi::Studio& studio) {
  const fs::path dir = fs::temp_directory_path() / "pluss_view_page_example";
  fs::remove_all(dir);
  fs::create_directories(dir);
  studio.export_html((dir / "page.html").string(), "T", 900, 600, bi::DashboardChrome::None);
  std::ifstream in((dir / "page.html").string());
  std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  fs::remove_all(dir);
  return text;
}

// Renders a studio that holds one board with one control, and returns the first rendered tile.
bi::TileRender control_tile(bi::Control control) {
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 1);
  studio.board("overview").control(control);
  return studio.render().boards.front().tiles.front();
}

// True when the exported page contains the given text.
bool page_has(const bi::Studio& studio, const std::string& needle) {
  return page_text(studio).find(needle) != std::string::npos;
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
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 1);
  studio.board("overview").control(bi::category("pick", "region", "Region").place(0, 0));
  studio.board("overview").control(bi::category("pick", "month", "Month").place(0, 1));
  std::cout << "duplicate control key refused: " << throws([&] { (void)studio.render(); }) << '\n';
  return 0;
}
```

Output

```text
duplicate control key refused: 1
```

A value of 1 means the duplicate key was refused.

## See Also

- - [`Control`](../bi-control.md) -- the class that owns this member.
- - [`Board`](../bi-board.md) -- where controls are placed.
