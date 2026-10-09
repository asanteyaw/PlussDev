# `pluss::bi::Board::tiles`

[Back to `Board`](../bi-board.md) · [PlussBI API Reference](../bi.md)

```cpp
#include <pluss/bi/specs.h>

const std::vector<Tile>& tiles() const noexcept;
```

`tiles` returns the board's tiles in the order they were added. Each tile is either a view or a control,
never both and never neither, because the board only adds tiles through `view` and `control`. The returned
reference refers to the board's own storage, so read it while the board is in use.

## Parameters

Not applicable. The method takes no parameters.

## Returns

A const reference to the vector of tiles.

## Errors

This call does not throw.

## Practical Examples

### Example 1

Example 1 adds three tiles and prints their keys in order: first, second, then third.

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
  bi::Board board("overview");
  board.grid(1, 3);
  board.view(bi::table("first").query(bi::from("sales")).place(0, 0));
  board.control(bi::category("second", "region", "Region").place(0, 1));
  board.view(bi::table("third").query(bi::from("sales")).place(0, 2));
  for (const auto& tile : board.tiles()) std::cout << tile.key() << '\n';
  return 0;
}
```

Output

```text
first
second
third
```

### Example 2

Example 2 shows that a new board has no tiles, and that a studio renders such a board with zero tiles.

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
  bi::Board board("overview");
  std::cout << "tiles on a new board: " << board.tiles().size() << '\n';
  std::cout << "empty board renders with zero tiles: " << render_with([](bi::Studio& studio) {
    studio.board("overview").grid(1, 1);
  }).boards.front().tiles.size() << '\n';
  return 0;
}
```

Output

```text
tiles on a new board: 0
empty board renders with zero tiles: 0
```

### Example 3

Example 3 shows that each tile reports exactly one of a view or a control.

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
  // Each tile holds exactly one kind of content, a view or a control.
  bi::Board board("overview");
  board.grid(1, 2);
  board.control(bi::category("pick", "region", "Region").place(0, 0));
  board.view(bi::table("rows").query(bi::from("sales")).place(0, 1));
  for (const auto& tile : board.tiles()) {
    std::cout << tile.key() << ": view " << tile.view().has_value() << ", control " << tile.control().has_value() << '\n';
  }
  return 0;
}
```

Output

```text
pick: view false, control true
rows: view true, control false
```

## See Also

- [`Tile`](../bi-tile.md) -- the entry type.
- [`view`](view.md) and [`control`](control.md) -- the only ways to add tiles.
