# `pluss::bi::BoardRender`

[Back to PlussBI API Reference](bi.md)

`BoardRender` is one board inside a `StudioRender`. Its `title` is the title as set, which is empty when none was set; the page heading
falls back to the key. Its `rows` and `cols` are the grid, and its `tiles` are in the order the board holds them.

## Fields

| Field | Meaning |
|---|---|
| `key`, `title` | The board key and its title as set. |
| `rows`, `cols` | The grid shape. |
| `tiles` | The rendered tiles, in board order. |

## Practical Examples

### Example 1

Example 1 renders a board and reads its title, grid, and tile count from the render.

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
  // A render carries the studio key, the session it used, and one entry per board.
  const auto nexus = make_nexus();
  bi::Studio studio = overview_studio(nexus);
  studio.set_filter(bi::Filter::eq("region", std::string("north")));
  const auto rendered = studio.render();
  std::cout << "key: " << rendered.key << '\n';
  std::cout << "session filters: " << rendered.session.filters().size() << '\n';
  std::cout << "boards: " << rendered.boards.size() << ", first title: " << rendered.boards.front().title << '\n';
  return 0;
}
```

Output

```text
key: sales_studio
session filters: 1
boards: 1, first title: Sales overview
```

### Example 2

Example 2 builds a board with a title, a control, and a view, then reads the same values back from its render.

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

### Example 3

Example 3 chains the title and grid, then reads them from the rendered board.

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
  // A board returned by the studio can be configured in one chain.
  bi::Studio studio("sales_studio");
  studio.board("overview").title("Sales overview").grid(2, 3);
  const auto& board = studio.boards().front();
  std::cout << "title: " << board.title() << ", grid: " << board.nrows() << " by " << board.ncols() << '\n';
  return 0;
}
```

Output

```text
title: Sales overview, grid: 2 by 3
```

## See Also

- [`StudioRender`](bi-studio-render.md) -- holds these.
- [`TileRender`](bi-tile-render.md) -- the tiles.
