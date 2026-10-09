# `pluss::bi::Board`

[Back to PlussBI API Reference](bi.md)

```cpp
#include <pluss/bi/specs.h>

class Board {
public:
  explicit Board(std::string_view key);
  // ... key, title, grid, control, view, tiles, nrows, ncols, version
};
```

A `Board` is one page of a dashboard. It has a key that identifies it inside a studio, an optional
title, a grid of rows and columns, and an ordered list of tiles. Each tile is either a view that presents
data or a control that changes the session. A studio renders each board with its own grid, so a board
decides the layout of its tiles and the studio decides what the tiles show.

A board is built step by step. You set its grid, then add controls and views, and each change bumps the
board's version. Nothing is checked at the moment you add a tile: a tile placed outside the grid, a
duplicate key, or a zero-sized grid is refused only when the studio renders. This keeps building cheap and
puts every refusal in one place.

Use `Studio::board` to obtain a board by key. It creates the board the first time and returns the same one
after that.

## Parameters

| Method | Purpose |
|---|---|
| [`key`](bi-board/key.md) | The board's key |
| [`title`](bi-board/title.md) | Read or set the title |
| [`grid`](bi-board/grid.md) | Set the rows and columns |
| [`nrows`](bi-board/nrows.md) and [`ncols`](bi-board/ncols.md) | Read the grid shape |
| [`control`](bi-board/control.md) | Add a control tile |
| [`view`](bi-board/view.md) | Add a view tile |
| [`tiles`](bi-board/tiles.md) | Read the tiles in order |
| [`version`](bi-board/version.md) | Read the change counter |

## Returns

Not applicable. `Board` is constructed directly from a key, or obtained from a studio.

## Errors

Board checks run when the studio renders, not when a call is made. A refused render names the problem.
The board must have a non-empty key, and its grid must have at least one row and one column.
Each tile key must be non-empty and unique on the board, and each tile's coordinates must be non-negative.
Each tile's spans must be positive, and the tile must lie inside the grid. Each tile holds exactly one view or one control.

## Practical Examples

### Example 1

Example 1 builds a board and reads its defaults. A new board has an empty title, a one-by-one grid, no tiles, and version zero.

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
  // A new board has its key, an empty title, a 1 by 1 grid, no tiles, and version zero.
  bi::Board board("overview");
  std::cout << "key: " << board.key() << '\n';
  std::cout << "title empty: " << board.title().empty() << '\n';
  std::cout << "grid: " << board.nrows() << " by " << board.ncols() << '\n';
  std::cout << "tiles: " << board.tiles().size() << '\n';
  std::cout << "version: " << board.version() << '\n';
  return 0;
}
```

Output

```text
key: overview
title empty: true
grid: 1 by 1
tiles: 0
version: 0
```

### Example 2

Example 2 adds a control and a view to a two-column grid. Tiles keep the order in which they were added, and each one reports what it holds.

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
  // Tiles keep the order in which they were added, whether they are controls or views.
  bi::Board board("overview");
  board.grid(1, 2);
  board.control(bi::category("pick", "region", "Region").place(0, 0));
  board.view(bi::table("rows").query(bi::from("sales")).place(0, 1));
  std::cout << "first tile: " << board.tiles()[0].key() << ", control: " << board.tiles()[0].control().has_value() << '\n';
  std::cout << "second tile: " << board.tiles()[1].key() << ", view: " << board.tiles()[1].view().has_value() << '\n';
  return 0;
}
```

Output

```text
first tile: pick, control: true
second tile: rows, view: true
```

### Example 3

Example 3 renders a studio whose board was built by these calls. The rendered board carries the same key, title, grid, and tiles.

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
    studio.board("overview").title("Sales overview").grid(1, 2);
    studio.board("overview").control(bi::category("pick", "region", "Region").place(0, 0));
    studio.board("overview").view(bi::table("rows").query(bi::from("sales")).place(0, 1));
  });
  const auto& board = rendered.boards.front();
  std::cout << "key: " << board.key << ", title: " << board.title << '\n';
  std::cout << "grid: " << board.rows << " by " << board.cols << ", tiles: " << board.tiles.size() << '\n';
  return 0;
}
```

Output

```text
key: overview, title: Sales overview
grid: 1 by 2, tiles: 2
```

## See Also

- [`Studio`](bi.md) -- renders boards and owns them by key.
- [`Tile`](bi-tile.md) -- the entry in a board's tile list.
- [`LayoutSpec`](bi-layout-spec.md) -- the placement a tile records.
