# `pluss::bi::Board::control`

[Back to `Board`](../bi-board.md) · [PlussBI API Reference](../bi.md)

```cpp
#include <pluss/bi/specs.h>

Board& control(const Control& value);
```

`control` adds a control to the board as a tile. The control keeps its key and its placement, and the tile
is appended after the tiles already on the board. A control tile is marked as such when the studio renders,
so a front end can treat it as an input rather than a presentation. Tile keys must be unique across views
and controls on the same board.

## Overloads

This function has one overload.

## Parameters

| Parameter | Meaning |
|---|---|
| `value` | The control to add. Its key must be unique on the board, and its placement must lie inside the grid. |

## Returns

The board itself, so calls chain.

## Errors

A duplicate tile key is refused when the studio renders. A placement outside the grid is refused too.

## Practical Examples

### Example 1

Example 1 adds one control and shows that the board holds one tile, which is a control and not a view.

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
  board.control(bi::category("pick", "region", "Region").place(0, 0));
  std::cout << "tiles: " << board.tiles().size() << '\n';
  std::cout << "tile key: " << board.tiles().front().key() << '\n';
  std::cout << "holds a control: " << board.tiles().front().control().has_value() << '\n';
  std::cout << "holds a view: " << board.tiles().front().view().has_value() << '\n';
  return 0;
}
```

Output

```text
tiles: 1
tile key: pick
holds a control: true
holds a view: false
```

### Example 2

Example 2 renders a control next to a view and shows that the control tile is marked as a control.

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
  const auto rendered = render_with([](bi::Studio& studio) {
    studio.board("overview").grid(1, 2);
    studio.board("overview").control(bi::category("pick", "region", "Region").place(0, 0));
    studio.board("overview").view(bi::table("rows").query(bi::from("sales")).place(0, 1));
  });
  std::cout << "first rendered tile is a control: " << rendered.boards.front().tiles[0].is_control << '\n';
  std::cout << "second rendered tile is a control: " << rendered.boards.front().tiles[1].is_control << '\n';
  return 0;
}
```

Output

```text
first rendered tile is a control: true
second rendered tile is a control: false
```

### Example 3

Example 3 adds a control and a view with the same key. The studio refuses the duplicate when it renders.

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
  // Tile keys must be unique across views and controls on the same board.
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 2);
  studio.board("overview").control(bi::category("same", "region", "Region").place(0, 0));
  studio.board("overview").view(bi::table("same").query(bi::from("sales")).place(0, 1));
  std::cout << "duplicate key refused at render: " << throws([&] { (void)studio.render(); }) << '\n';
  return 0;
}
```

Output

```text
duplicate key refused at render: 1
```

## See Also

- [`view`](view.md) -- the presentation counterpart.

