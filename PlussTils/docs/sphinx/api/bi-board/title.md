# `pluss::bi::Board::title`

[Back to `Board`](../bi-board.md) · [PlussBI API Reference](../bi.md)

```cpp
#include <pluss/bi/specs.h>

const std::string& title() const noexcept;
Board& title(std::string_view value);
```

`title` reads or sets the board's title. The setter stores the text and returns the board, so it can be
chained with `grid`. A title is shown as the board's heading. When no title is set, the heading shows the
board's key instead, as the key page shows.

## Overloads

| Overload | Effect |
|---|---|
| `title()` | Reads the stored title. An empty string means no title is set. |
| `title(value)` | Stores the title and bumps the version. Returns the board. |

## Parameters

| Parameter | Meaning |
|---|---|
| `value` | The title text. It may be empty, which clears the heading back to the key. |

## Returns

The stored title as a string reference, or the board itself from the setter.

## Errors

This call does not throw and does not check the text.

## Practical Examples

### Example 1

Example 1 sets a title and reads it back. A second call replaces the title.

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
  board.title("Sales overview");
  std::cout << "title: " << board.title() << '\n';
  board.title("Revenue");
  std::cout << "replaced title: " << board.title() << '\n';
  return 0;
}
```

Output

```text
title: Sales overview
replaced title: Revenue
```

### Example 2

Example 2 shows that the rendered board carries the title as set.

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
    studio.board("overview").title("Sales overview").grid(1, 1);
    studio.board("overview").view(bi::table("rows").query(bi::from("sales")).place(0, 0));
  });
  std::cout << "rendered title: " << rendered.boards.front().title << '\n';
  return 0;
}
```

Output

```text
rendered title: Sales overview
```

### Example 3

Example 3 shows that each title call counts as a change to the board's version, even when the title repeats.

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
  const auto before = board.version();
  board.title("Sales overview");
  std::cout << "version moved by one: " << (board.version() == before + 1) << '\n';
  board.title("Sales overview");
  std::cout << "setting the same title again moves it again: " << (board.version() == before + 2) << '\n';
  return 0;
}
```

Output

```text
version moved by one: 1
setting the same title again moves it again: 1
```

## See Also

- [`key`](key.md) -- the fallback heading.
- [`version`](version.md) -- the change counter.
