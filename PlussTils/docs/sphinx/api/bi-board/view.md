# `pluss::bi::Board::view`

[Back to `Board`](../bi-board.md) · [PlussBI API Reference](../bi.md)

```cpp
#include <pluss/bi/specs.h>

Board& view(const View& value);
```

`view` adds a presentation to the board as a tile. The view keeps its key, its query, and its placement, and
the tile is appended after the tiles already on the board. Two tiles may cover the same cells: nothing
refuses the overlap, and both tiles are rendered. A tile whose columns or rows run past the grid is refused
when the studio renders.

## Overloads

This function has one overload.

## Parameters

| Parameter | Meaning |
|---|---|
| `value` | The view to add. Its key must be unique on the board, and its placement must lie inside the grid. |

## Returns

The board itself, so calls chain.

## Errors

A duplicate tile key, a tile outside the grid, or a span that is not positive is refused when the studio renders.

## Practical Examples

### Example 1

Example 1 adds one view and shows that the board holds one tile, which is a view.

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
  bi::Board board("overview");
  board.grid(1, 1);
  board.view(bi::table("rows").query(bi::from("sales")).place(0, 0));
  std::cout << "tiles: " << board.tiles().size() << '\n';
  std::cout << "holds a view: " << board.tiles().front().view().has_value() << '\n';
  std::cout << "holds a control: " << board.tiles().front().control().has_value() << '\n';
  return 0;
}
```

Output

```text
tiles: 1
holds a view: true
holds a control: false
```

### Example 2

Example 2 places two views on the same cell. Both are rendered, so the render holds two tiles.

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
  // Two tiles may cover the same cells. Nothing refuses the overlap, so the later tile paints on top.
  const auto rendered = render_with([](bi::Studio& studio) {
    studio.board("overview").grid(1, 1);
    studio.board("overview").view(bi::table("a").query(bi::from("sales")).place(0, 0));
    studio.board("overview").view(bi::table("b").query(bi::from("sales")).place(0, 0));
  });
  std::cout << "tiles rendered over one cell: " << rendered.boards.front().tiles.size() << '\n';
  return 0;
}
```

Output

```text
tiles rendered over one cell: 2
```

### Example 3

Example 3 places a view in column six of a two-column grid. The studio refuses the tile when it renders.

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
  // A tile whose columns run past the grid is refused when the studio renders.
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 2);
  studio.board("overview").view(bi::table("rows").query(bi::from("sales")).place(0, 5));
  std::cout << "out-of-grid tile refused at render: " << throws([&] { (void)studio.render(); }) << '\n';
  return 0;
}
```

Output

```text
out-of-grid tile refused at render: 1
```

## See Also

- [`control`](control.md) -- the input counterpart.
- [`LayoutSpec`](../bi-layout-spec.md) -- the placement a view records.
