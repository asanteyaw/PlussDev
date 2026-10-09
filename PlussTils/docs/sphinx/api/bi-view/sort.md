# `pluss::bi::View::sort`

[Back to `View`](../bi-view.md) · [PlussBI API Reference](../bi.md)

```cpp
#include <pluss/bi/specs.h>

View& sort(bool enable = true);
```

`sort` decides the order of a pivot's columns. With it on, the columns are ordered by name. With it off, they keep the order in which each key first appears in the source. The flag is used only by pivot tables. A bar or a line does not read it, and its page is the same either way.

## Parameters

| Parameter | Meaning |
|---|---|
| `enable` | True orders the pivot columns by name. False keeps first-seen order. Default true. |

## Returns

The view, so the call chains.

## Errors

None. The flag is recorded and used when the pivot renders.

## Practical Examples

### Example 1

Example 1 reads the default value of the flag on a bar.

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
  bi::View view = bi::bar("by").x("region").y("amount");
  std::cout << "sort by default: " << view.bind().sort << '\n';
  return 0;
}
```

Output

```text
sort by default: true
```

The default is true, as the binding stores it.

### Example 2

Example 2 sets the flag to false and reads it back.

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
  bi::View view = bi::bar("by").x("region").y("amount");
  view.sort(false);
  std::cout << "sort after setting false: " << view.bind().sort << '\n';
  return 0;
}
```

Output

```text
sort after setting false: false
```

The stored value is false.

### Example 3

Example 3 renders a pivot with sort on. Its columns are east, north, and south, in name order.

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
// Four months of sales. The region of the third row is missing, so a pivot on region has a missing column.
pluss::bundle::Nexus make_nexus() {
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  // Regions in first-seen order: south, north, missing, east.
  const DataFrame sales({
      {"month", Slate(pluss::make_note({"jan", "jan", "feb", "feb"}))},
      {"region", Slate(pluss::make_note(std::vector<std::string>{"south", "north", "NULL", "east"}),
                        pluss::make_series(std::vector<bool>{true, true, false, true}))},
      {"amount", Slate(pluss::make_series(std::vector<double>{10, 20, 30, 40}, {4}, cfg))},
  });
  return pluss::bundle::Nexus({{"sales", sales}});
}

// The column names of the pivot the studio renders, in the order the studio keeps them.
std::string pivot_columns(bool dropna, bool sort) {
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 1);
  studio.board("overview").view(bi::pivot("pv").pivot_index({"month"}).pivot_columns({"region"}).pivot_values({"amount"})
      .op(pluss::table::GroupOp::Sum).dropna(dropna).sort(sort).query(bi::from("sales")).place(0, 0));
  const auto tile = studio.render().boards.front().tiles.front();
  std::string out;
  for (const auto& name : tile.table->features()) out += name + " ";
  return out;
}

// The text of the page a studio exports, for checks on what is shown.
std::string page_text(const bi::Studio& studio) {
  const fs::path dir = fs::temp_directory_path() / "pluss_view_flag_page";
  fs::remove_all(dir);
  fs::create_directories(dir);
  studio.export_html((dir / "page.html").string(), "T", 900, 600, bi::DashboardChrome::None);
  std::ifstream in((dir / "page.html").string());
  std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  fs::remove_all(dir);
  return text;
}

// The page of a bar with the given flags. A bar does not use dropna or sort, so its page should not depend on them.
std::string bar_page(bool dropna, bool sort) {
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 1);
  studio.board("overview").view(bi::bar("by").x("region").y("amount").dropna(dropna).sort(sort).query(bi::from("sales")).place(0, 0));
  return page_text(studio);
}

int main() {
  std::cout << "pivot columns with sort on: " << pivot_columns(true, true) << '\n';
  return 0;
}
```

Output

```text
pivot columns with sort on: east north south 
```

The columns are in alphabetical order, whatever order the source had.

### Example 4

Example 4 renders the same pivot with sort off. Its columns are south, north, and east, in the order each first appears in the source.

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
// Four months of sales. The region of the third row is missing, so a pivot on region has a missing column.
pluss::bundle::Nexus make_nexus() {
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  // Regions in first-seen order: south, north, missing, east.
  const DataFrame sales({
      {"month", Slate(pluss::make_note({"jan", "jan", "feb", "feb"}))},
      {"region", Slate(pluss::make_note(std::vector<std::string>{"south", "north", "NULL", "east"}),
                        pluss::make_series(std::vector<bool>{true, true, false, true}))},
      {"amount", Slate(pluss::make_series(std::vector<double>{10, 20, 30, 40}, {4}, cfg))},
  });
  return pluss::bundle::Nexus({{"sales", sales}});
}

// The column names of the pivot the studio renders, in the order the studio keeps them.
std::string pivot_columns(bool dropna, bool sort) {
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 1);
  studio.board("overview").view(bi::pivot("pv").pivot_index({"month"}).pivot_columns({"region"}).pivot_values({"amount"})
      .op(pluss::table::GroupOp::Sum).dropna(dropna).sort(sort).query(bi::from("sales")).place(0, 0));
  const auto tile = studio.render().boards.front().tiles.front();
  std::string out;
  for (const auto& name : tile.table->features()) out += name + " ";
  return out;
}

// The text of the page a studio exports, for checks on what is shown.
std::string page_text(const bi::Studio& studio) {
  const fs::path dir = fs::temp_directory_path() / "pluss_view_flag_page";
  fs::remove_all(dir);
  fs::create_directories(dir);
  studio.export_html((dir / "page.html").string(), "T", 900, 600, bi::DashboardChrome::None);
  std::ifstream in((dir / "page.html").string());
  std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  fs::remove_all(dir);
  return text;
}

// The page of a bar with the given flags. A bar does not use dropna or sort, so its page should not depend on them.
std::string bar_page(bool dropna, bool sort) {
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 1);
  studio.board("overview").view(bi::bar("by").x("region").y("amount").dropna(dropna).sort(sort).query(bi::from("sales")).place(0, 0));
  return page_text(studio);
}

int main() {
  std::cout << "pivot columns with sort off: " << pivot_columns(true, false) << '\n';
  return 0;
}
```

Output

```text
pivot columns with sort off: south north east 
```

The columns follow the source, so the order depends on the data rather than on the names.

### Example 5

Example 5 renders a bar with the flag on and with it off. The two pages are the same, so a bar does not use the flag.

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
// Four months of sales. The region of the third row is missing, so a pivot on region has a missing column.
pluss::bundle::Nexus make_nexus() {
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  // Regions in first-seen order: south, north, missing, east.
  const DataFrame sales({
      {"month", Slate(pluss::make_note({"jan", "jan", "feb", "feb"}))},
      {"region", Slate(pluss::make_note(std::vector<std::string>{"south", "north", "NULL", "east"}),
                        pluss::make_series(std::vector<bool>{true, true, false, true}))},
      {"amount", Slate(pluss::make_series(std::vector<double>{10, 20, 30, 40}, {4}, cfg))},
  });
  return pluss::bundle::Nexus({{"sales", sales}});
}

// The column names of the pivot the studio renders, in the order the studio keeps them.
std::string pivot_columns(bool dropna, bool sort) {
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 1);
  studio.board("overview").view(bi::pivot("pv").pivot_index({"month"}).pivot_columns({"region"}).pivot_values({"amount"})
      .op(pluss::table::GroupOp::Sum).dropna(dropna).sort(sort).query(bi::from("sales")).place(0, 0));
  const auto tile = studio.render().boards.front().tiles.front();
  std::string out;
  for (const auto& name : tile.table->features()) out += name + " ";
  return out;
}

// The text of the page a studio exports, for checks on what is shown.
std::string page_text(const bi::Studio& studio) {
  const fs::path dir = fs::temp_directory_path() / "pluss_view_flag_page";
  fs::remove_all(dir);
  fs::create_directories(dir);
  studio.export_html((dir / "page.html").string(), "T", 900, 600, bi::DashboardChrome::None);
  std::ifstream in((dir / "page.html").string());
  std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  fs::remove_all(dir);
  return text;
}

// The page of a bar with the given flags. A bar does not use dropna or sort, so its page should not depend on them.
std::string bar_page(bool dropna, bool sort) {
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 1);
  studio.board("overview").view(bi::bar("by").x("region").y("amount").dropna(dropna).sort(sort).query(bi::from("sales")).place(0, 0));
  return page_text(studio);
}

int main() {
  std::cout << std::boolalpha;
  std::cout << "bar page same with sort on and off: " << (bar_page(true, true) == bar_page(true, false)) << '\n';
  return 0;
}
```

Output

```text
bar page same with sort on and off: true
```

A value of true means the two pages match.

### Example 6

Example 6 turns the flag off and renders a bar. The bar still renders.

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
  const auto tile = one_tile([] { return bi::bar("by").x("region").y("amount").sort(false).query(bi::from("sales")).place(0, 0); });
  std::cout << "bar with sort off renders a plot: " << (tile.plot != nullptr) << '\n';
  return 0;
}
```

Output

```text
bar with sort off renders a plot: 1
```

The render succeeds, so turning the flag off never refuses a render.

## See Also

- - [`View`](../bi-view.md) -- the class that owns this member.
- - [`pivot_index`](pivot_index.md) -- with the pivot columns and values.
- - [`Studio::render`](../bi-studio/render.md) -- where the flag takes effect.
