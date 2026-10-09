# `pluss::bi::LayoutSpec`

[Back to PlussBI API Reference](bi.md)

```cpp
#include <pluss/bi/specs.h>

struct LayoutSpec {
  std::int64_t row{0};
  std::int64_t col{0};
  std::int64_t row_span{1};
  std::int64_t col_span{1};
  std::optional<double> min_height{};
};
```

`LayoutSpec` records where a tile sits on a board's grid: its row and column, how many rows and columns it
spans, and an optional minimum height. Rows and columns are zero-based. Views and controls fill in the
layout when you place them, so you normally read a layout rather than write one.

The minimum height is optional and unset by default. Set it with `min_height` on a view or a control. The tile is then never
shorter than that many pixels when the studio renders it. Placing a view or a control leaves it unset, so `place` and
`min_height` are separate steps.

## Parameters

| Field | Meaning |
|---|---|
| `row`, `col` | The zero-based cell of the tile's top-left corner. |
| `row_span`, `col_span` | How many rows and columns the tile covers. Must be positive when the board renders. |
| `min_height` | An optional minimum height in pixels. Unset by default. |

## Returns

Not applicable. `LayoutSpec` is a struct. Use `at` to build one from a position.

## Errors

Non-negative coordinates and positive spans are checked when the studio renders. A minimum height must be positive, and `min_height` refuses zero or a negative value when it is called.

## Practical Examples

### Example 1

Example 1 shows the default layout: the first cell, one row, one column, and no minimum height.

```cpp
#include <pluss/bi/bi.h>
#include <pluss/bundle/bundle.h>
#include <pluss/mic.h>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace bi = pluss::bi;

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

// Renders a studio that holds one board built by the caller, and returns the render.
template <class Build>
bi::StudioRender render_with(Build build) {
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  build(studio);
  return studio.render();
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
  // A default LayoutSpec places a tile at the first cell, one row and one column, with no minimum height.
  const bi::LayoutSpec layout{};
  std::cout << "row " << layout.row << ", col " << layout.col << '\n';
  std::cout << "span " << layout.row_span << "x" << layout.col_span << '\n';
  std::cout << "minimum height set: " << layout.min_height.has_value() << '\n';
  return 0;
}
```

Output

```text
row 0, col 0
span 1x1
minimum height set: 0
```

### Example 2

Example 2 reads the layout that `place` records on a view, with row and column and spans.

```cpp
#include <pluss/bi/bi.h>
#include <pluss/bundle/bundle.h>
#include <pluss/mic.h>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace bi = pluss::bi;

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

// Renders a studio that holds one board built by the caller, and returns the render.
template <class Build>
bi::StudioRender render_with(Build build) {
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  build(studio);
  return studio.render();
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
  bi::View view = bi::table("rows").query(bi::from("sales")).place(2, 1, 1, 3);
  const bi::LayoutSpec& layout = view.layout();
  std::cout << "row " << layout.row << ", col " << layout.col << '\n';
  std::cout << "span " << layout.row_span << "x" << layout.col_span << '\n';
  return 0;
}
```

Output

```text
row 2, col 1
span 1x3
```

### Example 3

Example 3 shows that `place` leaves the minimum height unset.

```cpp
#include <pluss/bi/bi.h>
#include <pluss/bundle/bundle.h>
#include <pluss/mic.h>

#include <cstdint>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace bi = pluss::bi;

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

// Renders a studio that holds one board built by the caller, and returns the render.
template <class Build>
bi::StudioRender render_with(Build build) {
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  build(studio);
  return studio.render();
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
  // Placing a view or a control fills in the position and spans. It leaves the minimum height unset.
  const bi::View view = bi::table("rows").query(bi::from("sales")).place(0, 0, 2, 2);
  std::cout << "minimum height set by place: " << view.layout().min_height.has_value() << '\n';
  return 0;
}
```

Output

```text
minimum height set by place: false
```

### Example 4

A view sets a minimum height of 320 pixels, and a control sets 180. Both values are stored in the layout.

```cpp
#include <pluss/bi/bi.h>
#include <pluss/bundle/bundle.h>
#include <pluss/mic.h>

#include <iostream>
#include <vector>

namespace bi = pluss::bi;

int main() {
  // A view or a control can set a minimum height in pixels. The tile records it in its layout.
  const bi::View tall = bi::table("rows").query(bi::from("sales")).place(0, 0).min_height(320.0);
  const bi::Control picker = bi::category("pick", "region", "Region").place(0, 1).min_height(180.0);
  std::cout << "view minimum height: " << *tall.layout().min_height << '\n';
  std::cout << "control minimum height: " << *picker.layout().min_height << '\n';
  return 0;
}
```

Output

```text
view minimum height: 320
control minimum height: 180
```

### Example 5

The minimum height reaches the rendered page. The rendered tile reports 320, and the exported document carries that height.

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
  // The minimum height reaches the rendered page as the tile's minimum height.
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 1);
  studio.board("overview").view(bi::table("rows").query(bi::from("sales")).place(0, 0).min_height(320.0));
  const auto rendered = studio.render();
  std::cout << "rendered tile minimum height: " << *rendered.boards.front().tiles.front().layout.min_height << '\n';
  const fs::path dir = private_dir("pluss_layout_min_height_example");
  studio.export_html((dir / "page.html").string(), "T", 900, 600, bi::DashboardChrome::None);
  std::cout << "page carries the tile height: " << (read_file(dir / "page.html").find("--tile-min-h:320px") != std::string::npos) << '\n';
  fs::remove_all(dir);
  return 0;
}
```

Output

```text
rendered tile minimum height: 320
page carries the tile height: 1
```

### Example 6

A zero minimum height is refused when it is set.

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
  // A minimum height must be positive. The setter refuses zero when it is called.
  std::cout << "zero minimum height refused: " << throws([] {
    (void)bi::table("rows").query(bi::from("sales")).place(0, 0).min_height(0.0);
  }) << '\n';
  return 0;
}
```

Output

```text
zero minimum height refused: 1
```

## See Also

- [`at`](bi-layout-spec/at.md) -- the factory that builds a layout from a position.
- [`Tile`](bi-tile.md) -- the type that records a layout.
