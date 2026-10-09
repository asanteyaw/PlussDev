# `pluss::bi::table`

[Back to PlussBI API Reference](bi.md)

```cpp
#include <pluss/bi/specs.h>

View table(std::string_view key);
```

`table` builds a detail table tile. It shows the rows of its query. It is the default kind, so a view that sets no kind is a table.

## Parameters

| Parameter | Meaning |
|---|---|
| `key` | The key of the view. It must be unique on its board. |

## Returns

A `View` of kind `DetailTable`. Add the columns and the query with setters, then place it.

## Errors

A table with no query is refused, as every view is.

## Practical Examples

### Example 1

Example 1 renders a table and reads the number of rows it shows.

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
  const auto tile = one_tile([] { return bi::table("rows").query(bi::from("sales")).place(0, 0); });
  std::cout << "table rows: " << (tile.table->ncases()) << '\n';
  return 0;
}
```

Output

```text
table rows: 6
```

The tile shows six rows, the same as the source.

### Example 2

Example 2 renders a table and checks that it has no plot.

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
  const auto tile = one_tile([] { return bi::table("rows").query(bi::from("sales")).place(0, 0); });
  std::cout << "table has a plot: " << (tile.plot != nullptr) << '\n';
  return 0;
}
```

Output

```text
table has a plot: 0
```

A value of 0 means a table holds rows, not a plot.

### Example 3

Example 3 checks the kind of a table built by the constructor.

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
  std::cout << std::boolalpha;
  bi::View view = bi::table("rows");
  std::cout << "table kind: " << (view.bind().kind == bi::ViewKind::DetailTable) << '\n';
  return 0;
}
```

Output

```text
table kind: true
```

The check is true, so the constructor sets the detail table kind.

## See Also

- [`View::bind`](bi-view/bind.md)
