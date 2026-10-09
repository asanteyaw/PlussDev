# PlussBI: Boards, tiles, and layout

```{raw} html
<meta name="pluss-doc-kind" content="guide">
```

[Back to PlussTils](../index.md) · [PlussBI API Reference](../api/bi.md) · [PlussBI: A Guide](bi.md)

A board is one page of a dashboard: a key, a title, a grid, and an ordered list of tiles. Each tile is a view or a control, placed in a cell of the grid. This chapter covers every board member, the tile, and the layout, so you can decide how a page is built and how it is checked.

## The mental model

A board is a grid with a fixed number of rows and columns, and its tiles are kept in the order you added them. A tile names a cell with its row and column, and its spans say how many cells it covers. Tiles may overlap, and a tile may not sit outside the grid.

Every change to a board moves its version forward by one. Reads do not. The version tells you whether the board you rendered is the one you think it is.

## Worked tour

Every example is a complete program, and the output under it is copied from that program's run.

## Example 1: A new board has defaults

A board starts with a key, an empty title, a one-by-one grid, no tiles, and version zero. This example reads each default.

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

Every default is what you would expect, so a board needs only a key to exist.

## Example 2: Building a board in order

Tiles are kept in the order they were added, and each one remembers whether it is a view or a control. This example adds a control and then a view.

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

The first tile is the control and the second is the view, in the order they were added.

## Example 3: A board renders what it was built with

The render reports the same key, title, grid, and tile count as the board. This example builds a board with two tiles and renders it.

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

The render shows the title and the grid of one row by two, with two tiles.

## Example 4: A board's key is kept as given

A key is stored exactly as you wrote it, including spaces and digits. This example builds a board with a key that has both, and finds it again by that key.

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
  // The key is stored exactly as it was given.
  bi::Board board("Overview 2024");
  std::cout << "key: " << board.key() << '\n';
  std::cout << "same key for a studio lookup: " << (bi::Studio("s").board("Overview 2024").key() == "Overview 2024") << '\n';
  return 0;
}
```

Output

```text
key: Overview 2024
same key for a studio lookup: 1
```

The key is unchanged, and a lookup by the same key finds the board.

## Example 5: A board with no title shows its key

When a board has no title, the page uses its key as the heading. This example renders a board with no title and checks the page.

```cpp
#include <pluss/bi/bi.h>
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
  // The rendered title stays empty when no title is set. The page heading falls back to the key.
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 1);
  studio.board("overview").view(bi::table("rows").query(bi::from("sales")).place(0, 0));
  std::cout << "rendered title empty: " << studio.render().boards.front().title.empty() << '\n';
  namespace fs = std::filesystem;
  const fs::path dir = fs::temp_directory_path() / "pluss_board_key_example";
  fs::remove_all(dir);
  fs::create_directories(dir);
  const std::string path = (dir / "page.html").string();
  studio.export_html(path, "Board title check", 900, 600);
  std::ifstream in(path);
  const std::string html((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  std::cout << "heading uses the key: " << (html.find("<h2>overview</h2>") != std::string::npos) << '\n';
  fs::remove_all(dir);
  return 0;
}
```

Output

```text
rendered title empty: 1
heading uses the key: 1
```

The rendered title is empty and the heading uses the key.

## Example 6: An empty key is refused

A board needs a key to refer to it. This example builds a board with an empty key and renders.

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
  // An empty key is refused when the studio renders, not when the board is created.
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("").grid(1, 1);
  std::cout << "empty board key refused at render: " << throws([&] { (void)studio.render(); }) << '\n';
  return 0;
}
```

Output

```text
empty board key refused at render: 1
```

The studio refuses the empty key at render.

## Example 7: Setting a title

The title is the heading of the board. This example sets one, reads it back, and replaces it.

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

The title reads back, and the replacement takes its place.

## Example 8: A title change counts as a change

Changing a title moves the board's version, even if you set the same title again. This example sets a title twice.

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

Each set moved the version by one.

## Example 9: The default number of rows

A board starts with one row. This example reads the default.

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
  std::cout << "default rows: " << board.nrows() << '\n';
  return 0;
}
```

Output

```text
default rows: 1
```

The default is one row.

## Example 10: Rows after setting the grid

The grid call sets both the rows and the columns. This example sets a grid of three rows by two columns, then reads them.

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
  board.grid(3, 2);
  std::cout << "rows after grid(3, 2): " << board.nrows() << '\n';
  std::cout << "columns after grid(3, 2): " << board.ncols() << '\n';
  return 0;
}
```

Output

```text
rows after grid(3, 2): 3
columns after grid(3, 2): 2
```

The board has three rows and two columns.

## Example 11: A title does not change the rows

Only the grid changes the number of rows. This example sets a title and checks the row count.

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
  board.grid(2, 4).title("Sales overview");
  std::cout << "rows after a title change: " << board.nrows() << '\n';
  return 0;
}
```

Output

```text
rows after a title change: 2
```

The row count stays at two, the value it had before the title changed.

## Example 12: The default number of columns

A board starts with one column. This example reads the default.

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
  std::cout << "default columns: " << board.ncols() << '\n';
  return 0;
}
```

Output

```text
default columns: 1
```

The default is one column.

## Example 13: Columns after setting the grid

The grid call also sets the columns. This example sets a grid of one row by four columns.

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
  board.grid(1, 4);
  std::cout << "columns after grid(1, 4): " << board.ncols() << '\n';
  return 0;
}
```

Output

```text
columns after grid(1, 4): 4
```

The board has four columns.

## Example 14: Zero columns are refused

A grid needs at least one column. This example sets zero columns and renders.

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
  // A zero column count is accepted by the setter and refused when the studio renders.
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 0);
  std::cout << "zero columns refused at render: " << throws([&] { (void)studio.render(); }) << '\n';
  return 0;
}
```

Output

```text
zero columns refused at render: 1
```

The studio refuses it at render.

## Example 15: Setting the grid shape

The grid sets both dimensions at once, and a second call replaces the first. This example sets two by three, then one by one.

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

The shape reads two by three, and then one by one after the second call.

## Example 16: Zero rows are refused

A grid needs at least one row. This example sets zero rows.

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

The studio refuses it when it renders.

## Example 17: Negative rows are refused

A negative count is no grid at all. This example sets a negative row count.

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

The studio refuses it when it renders.

## Example 18: A new board starts at version zero

The version counts changes to the board. This example checks a board that has changed nothing.

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

The version is zero.

## Example 19: Every change moves the version

Each change to the board adds one to the version, whatever the kind of change. This example makes four changes, a title, a grid, a control, and a view.

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

After each change the version reads one, two, three, and four.

## Example 20: Reads leave the version alone

Rendering and reading a board are not changes. This example reads the board several times.

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

The version is unchanged by reads.

## Example 21: A refused change does not count

A change the board refuses leaves the board and its version as they were. This example makes an invalid change and checks the version.

```cpp
#include <pluss/bi/session.h>
#include <pluss/bi/specs.h>

#include <iostream>
#include <string>
#include <variant>

namespace bi = pluss::bi;

template <class Fn>
bool throws(Fn fn) {
  try {
    fn();
  } catch (const std::exception&) {
    return true;
  }
  return false;
}

bi::Filter region_filter(const std::string& key, const std::string& value) {
  return bi::Filter{.key = key, .feature = "region", .op = bi::FilterOp::Eq,
                    .payload = bi::FilterScalar{value}};
}

int main() {
  std::cout << std::boolalpha;
  // A refused change throws before it is stored, so the version does not move.
  bi::Session session;
  session.set_param("a", 1.0);
  const auto before = session.version();
  std::cout << "refused: " << throws([&] { session.set_param("", 2.0); }) << '\n';
  std::cout << "version unchanged: " << (session.version() == before) << '\n';
  return 0;
}
```

Output

```text
refused: true
version unchanged: true
```

The change was refused and the version did not move.

## Example 22: A control is a tile

A control added to a board becomes a tile with the control's key. This example adds one and checks what the tile holds.

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

The board has one tile, its key is the control's key, and it holds a control and not a view.

## Example 23: A rendered control is marked as a control

Once rendered, each tile says whether it is a control. This example renders a control tile and a view tile.

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

The first tile is a control and the second is not.

## Example 24: A view is a tile

A view added to a board becomes a tile that holds the view. This example adds one.

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

The board has one tile, and it holds a view and not a control.

## Example 25: Tiles may overlap

Two tiles may share a cell. This example places two views on one cell.

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

Both tiles render over the same cell.

## Example 26: A tile outside the grid is refused

A tile must sit inside the grid. This example places a view beyond the grid's columns.

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

The studio refuses it when it renders.

## Example 27: Tiles keep insertion order

Tiles come out in the order they were added. This example adds three tiles and lists them.

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

The tiles read first, second, and third, in order.

## Example 28: An empty board has no tiles

A board with no tiles has none to list, and it still renders. This example renders an empty board.

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

The board holds zero tiles and renders with zero tiles.

## Example 29: A tile is a view or a control, never both

Each tile holds exactly one of the two. This example checks a control tile and a view tile.

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

The control holds a control and no view, and the view holds a view and no control.

## Example 30: A tile from a view

A tile built from a view keeps the view's key, the view, and its layout. This example checks each.

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

The tile holds the view, uses its key, and carries its layout: row zero, column one, a span of one by two.

## Example 31: A tile from a control

A tile built from a control holds the control and no view. This example checks it.

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

The tile holds a control and no view.

## Example 32: A tile copies its layout

A tile takes a copy of the layout at the time it was made, so moving the view afterwards does not move the tile. This example moves the view after building the tile.

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

The tile keeps row zero, and the view is at row three. The layout was copied, not shared.

## Example 33: The layout defaults

A layout starts at row zero, column zero, with spans of one, and no minimum height. This example reads the defaults.

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

The defaults are row zero, column zero, and a span of one by one, and no minimum height is set.

## Example 34: Placing sets the fields

Placing a view sets its row, column, and spans. This example places a view at row two, column one, spanning three columns.

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

The layout reads row two, column one, and a span of one by three.

## Example 35: Placing does not set a minimum height

Place sets the cell and the spans, and leaves the minimum height alone. This example places a view and checks its minimum height.

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

The minimum height is not set by place.

## Example 36: Setting a minimum height on a view and a control

A view and a control each keep their own minimum height. This example sets 320 on the view and 180 on the control.

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

The view keeps 320 and the control keeps 180.

## Example 37: A zero minimum height is refused

A minimum height must be positive. This example passes zero.

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

The setter refuses it.

## Example 38: The layout factory

The at factory builds a layout from a cell, with spans that default to one. This example builds one at row one, column two.

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

The layout is at row one and column two, with spans of one by one.

## Example 39: Explicit spans with the factory

The at factory takes spans as well. This example builds a layout spanning two rows and three columns.

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

The span reads two by three.

## Example 40: A zero span is refused

A span must be at least one. This example builds a layout with a zero row span and places a view with it.

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

The studio refuses the zero span at render.

## Decisions

| You want | Use | Notes |
|---|---|---|
| A page of dashboards | one `Board` per page, keyed | A board's key is how you find it again. |
| Two tiles to share a space | overlapping `place` calls | Overlap is allowed; the render draws both. |
| A tile that should keep its spot | a copied `LayoutSpec` | Moving the view later does not move a tile that already holds a copy. |
| A tile that takes more height | `min_height` on the view or control | Each keeps its own value; place does not change it. |

## Pitfalls

**Expecting a title change to change the rows.** nrows_03 shows that the row count is unchanged after a title change.

**Expecting a tile to follow its view after the view moves.** tile_03 shows the tile keeps its copied layout.

**Expecting a grid of zero to render as empty.** grid_02 and ncols_03 show that a zero dimension is refused at render.

## Where next

- [Views](bi-views.md) -- the tiles that show data.
- [Controls](bi-controls.md) -- the tiles that change the session.
- [Studios](bi-studios.md) -- boards inside a studio.
- [PlussBI API Reference](../api/bi-board.md) -- every board member.
