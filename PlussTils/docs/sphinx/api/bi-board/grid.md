# `pluss::bi::Board::grid`

[Back to `Board`](../bi-board.md) · [PlussBI API Reference](../bi.md)

```cpp
#include <pluss/bi/specs.h>

Board& grid(std::int64_t row_count, std::int64_t col_count);
```

`grid` sets the number of rows and columns the board lays its tiles out in. It replaces any earlier grid
and bumps the version. Tile positions are checked against the grid when the studio renders, so set the grid
before you place tiles in it, or set it any time before the render. A zero or negative count is accepted
here and refused when the studio renders.

## Overloads

This function has one overload.

## Parameters

| Parameter | Meaning |
|---|---|
| `row_count` | The number of rows. Must be at least one when the board renders. |
| `col_count` | The number of columns. Must be at least one when the board renders. |

## Returns

The board itself, so calls chain.

## Errors

Zero or negative counts are refused when the studio renders. Tiles that lie outside the grid are refused too.

## Practical Examples

### Example 1

Example 1 sets a two-by-three grid, then replaces it with a one-by-one grid. The second call wins.

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
  board.grid(2, 3);
  std::cout << "shape: " << board.nrows() << " by " << board.ncols() << '\n';
  board.grid(1, 1);
  std::cout << "shape after a second grid call: " << board.nrows() << " by " << board.ncols() << '\n';
  return 0;
}
```

Output

```text
shape: 2 by 3
shape after a second grid call: 1 by 1
```

### Example 2

Example 2 sets zero rows. The studio refuses the board when it renders.

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
  // A grid needs at least one row and one column. Zero rows is refused when the studio renders.
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(0, 2);
  std::cout << "zero rows refused at render: " << throws([&] { (void)studio.render(); }) << '\n';
  return 0;
}
```

Output

```text
zero rows refused at render: 1
```

### Example 3

Example 3 sets a negative row count. The studio refuses the board when it renders.

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
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(-1, 2);
  std::cout << "negative rows refused at render: " << throws([&] { (void)studio.render(); }) << '\n';
  return 0;
}
```

Output

```text
negative rows refused at render: 1
```

## See Also

- [`nrows`](nrows.md) and [`ncols`](ncols.md) -- the values this call sets.
- [`view`](view.md) -- tiles placed inside the grid.
