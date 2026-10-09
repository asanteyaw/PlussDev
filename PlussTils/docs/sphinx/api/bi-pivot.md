# `pluss::bi::pivot`

[Back to PlussBI API Reference](bi.md)

```cpp
#include <pluss/bi/specs.h>

View pivot(std::string_view key);
```

`pivot` builds a pivot table tile. It takes an index, columns, and values, and shows a table rather than a plot.

## Parameters

| Parameter | Meaning |
|---|---|
| `key` | The key of the view. It must be unique on its board. |

## Returns

A `View` of kind `PivotTable`. Add the columns and the query with setters, then place it.

## Errors

A pivot with no index still renders, and its table is the one the studio builds.

## Practical Examples

### Example 1

Example 1 renders a pivot with all three role lists and checks its kind.

```cpp
#include <pluss/bi/bi.h>
#include <pluss/bundle/bundle.h>
#include <pluss/mic.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
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

// The text of the first kpi value on the page.
std::string kpi_value(const bi::Studio& studio) {
  const std::string page = page_text(studio);
  const std::string key = "class=\"bi-kpi-value\">";
  const auto at = page.find(key);
  if (at == std::string::npos) return "none";
  const auto end = page.find('<', at + key.size());
  return page.substr(at + key.size(), end - at - key.size());
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
  const auto tile = one_tile([] { return bi::pivot("pv").pivot_index({"region"}).pivot_columns({"month"}).pivot_values({"amount"}).query(bi::from("sales")).place(0, 0); });
  std::cout << "pivot kind: " << (tile.kind == bi::ViewKind::PivotTable) << '\n';
  return 0;
}
```

Output

```text
pivot kind: 1
```

A value of 1 means the tile kind is the pivot table.

### Example 2

Example 2 renders a pivot with no index. The tile still holds a table.

```cpp
#include <pluss/bi/bi.h>
#include <pluss/bundle/bundle.h>
#include <pluss/mic.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
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

// The text of the first kpi value on the page.
std::string kpi_value(const bi::Studio& studio) {
  const std::string page = page_text(studio);
  const std::string key = "class=\"bi-kpi-value\">";
  const auto at = page.find(key);
  if (at == std::string::npos) return "none";
  const auto end = page.find('<', at + key.size());
  return page.substr(at + key.size(), end - at - key.size());
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
  const auto tile = one_tile([] { return bi::pivot("pv").pivot_columns({"month"}).pivot_values({"amount"}).query(bi::from("sales")).place(0, 0); });
  std::cout << "pivot without an index renders a table: " << (tile.table != nullptr) << '\n';
  return 0;
}
```

Output

```text
pivot without an index renders a table: 1
```

A value of 1 means the pivot rendered a table without an index.

### Example 3

Example 3 renders a pivot and checks that it has no plot.

```cpp
#include <pluss/bi/bi.h>
#include <pluss/bundle/bundle.h>
#include <pluss/mic.h>

#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>
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

// The text of the first kpi value on the page.
std::string kpi_value(const bi::Studio& studio) {
  const std::string page = page_text(studio);
  const std::string key = "class=\"bi-kpi-value\">";
  const auto at = page.find(key);
  if (at == std::string::npos) return "none";
  const auto end = page.find('<', at + key.size());
  return page.substr(at + key.size(), end - at - key.size());
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
  const auto tile = one_tile([] { return bi::pivot("pv").pivot_index({"region"}).pivot_columns({"month"}).pivot_values({"amount"}).query(bi::from("sales")).place(0, 0); });
  std::cout << "pivot has a plot: " << (tile.plot != nullptr) << '\n';
  return 0;
}
```

Output

```text
pivot has a plot: 0
```

A value of 0 means the pivot holds a table, not a plot.

## See Also

- [`View::pivot_index`](bi-view/pivot_index.md)
- [`View::pivot_values`](bi-view/pivot_values.md)
