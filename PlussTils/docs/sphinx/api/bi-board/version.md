# `pluss::bi::Board::version`

[Back to `Board`](../bi-board.md) · [PlussBI API Reference](../bi.md)

```cpp
#include <pluss/bi/specs.h>

std::uint64_t version() const noexcept;
```

`version` returns the board's change counter. It starts at zero, and each call that changes the board
moves it by one. The changing calls are `title`, `grid`, `control`, and `view`. A refused render does not
change the version, because rendering does not change the board. Reading the board never moves it.

## Parameters

Not applicable. The method takes no parameters.

## Returns

The number of changes made to the board since it was created.

## Errors

This call does not throw.

## Practical Examples

### Example 1

Example 1 shows that a new board is at version zero.

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
  std::cout << "new board version: " << board.version() << '\n';
  return 0;
}
```

Output

```text
new board version: 0
```

### Example 2

Example 2 makes four changes, one each by title, grid, control, and view, and shows the counter after each.

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
  board.title("Sales");
  std::cout << "after title: " << board.version() << '\n';
  board.grid(1, 2);
  std::cout << "after grid: " << board.version() << '\n';
  board.control(bi::category("pick", "region", "Region").place(0, 0));
  std::cout << "after control: " << board.version() << '\n';
  board.view(bi::table("rows").query(bi::from("sales")).place(0, 1));
  std::cout << "after view: " << board.version() << '\n';
  return 0;
}
```

Output

```text
after title: 1
after grid: 2
after control: 3
after view: 4
```

### Example 3

Example 3 reads every accessor and shows that the version does not move.

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
  board.grid(1, 2);
  const auto before = board.version();
  (void)board.key();
  (void)board.title();
  (void)board.nrows();
  (void)board.ncols();
  (void)board.tiles();
  std::cout << "version unchanged by reads: " << (board.version() == before) << '\n';
  return 0;
}
```

Output

```text
version unchanged by reads: 1
```

## See Also

- [`title`](title.md) -- one of the changing calls.
- [`Session`](../bi-session.md) -- the session's own change counter.
