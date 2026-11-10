# `pluss::bi::Tile`

[Back to PlussBI API Reference](bi.md)

**Header:** `<pluss/bi/specs.h>`

### `Tile` members

| Member | Kind or type | Signature or default |
|---|---|---|
| `key` | Method | `const std::string& key() const noexcept` |
| `layout` | Method | `const LayoutSpec& layout() const noexcept` |
| `view` | Method | `const std::optional<View>& view() const noexcept` |
| `control` | Method | `const std::optional<Control>& control() const noexcept` |
| `from_view` | Method | `static Tile from_view(std::string_view key, LayoutSpec layout, View view)` |
| `from_control` | Method | `static Tile from_control(std::string_view key, LayoutSpec layout, Control control)` |

A `Tile` is one entry in a board's tile list. It records a key, a placement, and exactly one of a view or a
control. A board creates tiles when you add a view or a control, so you rarely build one yourself. The
static factories `from_view` and `from_control` exist for code that assembles tiles directly, and they copy
the layout you pass, so later changes to the view or control do not move the tile.

## Parameters

Not applicable. The method takes no parameters.

## Returns

`from_view` and `from_control` return a tile holding the given content, key, and layout. The accessors return the stored values.

## Errors

These calls do not throw. A tile is checked when a board that holds it is rendered.

## Practical Examples

### Example 1

Example 1 wraps a view in a tile with `from_view`. The tile keeps the view's key and layout, and holds the view.

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
  // Tile::from_view wraps a view with its key and layout. Board does this when you add a view.
  const bi::View view = bi::table("rows").query(bi::from("sales")).place(0, 1, 1, 2);
  const bi::Tile tile = bi::Tile::from_view(view.key(), view.layout(), view);
  std::cout << "key: " << tile.key() << '\n';
  std::cout << "holds a view: " << tile.view().has_value() << '\n';
  std::cout << "layout: row " << tile.layout().row << ", col " << tile.layout().col
            << ", span " << tile.layout().row_span << "x" << tile.layout().col_span << '\n';
  return 0;
}
```

Output

```text
key: rows
holds a view: true
layout: row 0, col 1, span 1x2
```

### Example 2

Example 2 wraps a control in a tile with `from_control`. The tile holds the control and no view.

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
  const bi::Control control = bi::category("pick", "region", "Region").place(0, 0);
  const bi::Tile tile = bi::Tile::from_control(control.key(), control.layout(), control);
  std::cout << "key: " << tile.key() << '\n';
  std::cout << "holds a control: " << tile.control().has_value() << '\n';
  std::cout << "holds a view: " << tile.view().has_value() << '\n';
  return 0;
}
```

Output

```text
key: pick
holds a control: true
holds a view: false
```

### Example 3

Example 3 shows that a tile copies its layout. Moving the view afterwards does not move the tile.

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
  // A tile stores a copy of its layout. Changing the view afterwards does not move the tile.
  bi::View view = bi::table("rows").query(bi::from("sales")).place(0, 0);
  const bi::Tile tile = bi::Tile::from_view(view.key(), view.layout(), view);
  view.place(3, 3);
  std::cout << "tile row after the view moved: " << tile.layout().row << '\n';
  std::cout << "view row: " << view.layout().row << '\n';
  return 0;
}
```

Output

```text
tile row after the view moved: 0
view row: 3
```

## See Also

- [`Board`](bi-board.md) -- the owner of the tile list.
- [`LayoutSpec`](bi-layout-spec.md) -- the placement a tile records.
