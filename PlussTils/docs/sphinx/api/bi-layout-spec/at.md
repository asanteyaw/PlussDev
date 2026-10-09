# `pluss::bi::LayoutSpec::at`

[Back to `Board`](../bi-board.md) · [PlussBI API Reference](../bi.md)

```cpp
#include <pluss/bi/specs.h>

static LayoutSpec at(std::int64_t row, std::int64_t col, std::int64_t row_span = 1, std::int64_t col_span = 1);
```

`at` builds a layout from a position and optional spans. Both spans default to one. It is the same layout
that `place` records on a view or a control, and it leaves the minimum height unset.

## Overloads

This function has one overload.

## Parameters

| Parameter | Meaning |
|---|---|
| `row`, `col` | The zero-based cell of the top-left corner. |
| `row_span`, `col_span` | How many rows and columns the tile covers. Default one each. |

## Returns

A `LayoutSpec` with the given position and spans.

## Errors

Zero or negative spans are refused when the studio renders the board that holds the tile.

## Practical Examples

### Example 1

Example 1 builds a layout at row one, column two, with the default spans of one.

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
  // at takes a position and returns a layout. Both spans default to one.
  const bi::LayoutSpec layout = bi::LayoutSpec::at(1, 2);
  std::cout << "row " << layout.row << ", col " << layout.col << '\n';
  std::cout << "span " << layout.row_span << "x" << layout.col_span << '\n';
  return 0;
}
```

Output

```text
row 1, col 2
span 1x1
```

### Example 2

Example 2 builds a layout that spans two rows and three columns.

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
  const bi::LayoutSpec layout = bi::LayoutSpec::at(0, 0, 2, 3);
  std::cout << "span " << layout.row_span << "x" << layout.col_span << '\n';
  return 0;
}
```

Output

```text
span 2x3
```

### Example 3

Example 3 places a view with a zero row span. The studio refuses it when it renders.

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
  // A span must be positive. The refusal happens when the studio renders the board.
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 2);
  studio.board("overview").view(bi::table("rows").query(bi::from("sales")).place(0, 0, 0, 1));
  std::cout << "zero row span refused at render: " << throws([&] { (void)studio.render(); }) << '\n';
  return 0;
}
```

Output

```text
zero row span refused at render: 1
```

## See Also

- [`LayoutSpec`](../bi-layout-spec.md) -- the struct this builds.
- [`Board`](../bi-board.md) -- where these layouts are used.
