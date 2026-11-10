# `pluss::bi::TileRender`

[Back to PlussBI API Reference](bi.md)

`TileRender` is one tile of a rendered board. It records the tile's key, layout, and kind, whether it is a control, the active filter a control has set,
and the table of rows a table view shows. A table tile holds the rows it shows.

## Fields

| Field | Meaning |
|---|---|
| `key`, `layout` | The tile key and placement. |
| `kind` | The view kind, such as a detail table. |
| `is_control` | True for control tiles. |
| `active_filter` | The filter a control has set, when one applies. |
| `table` | The rows a table view shows. Present for table tiles. |

## Practical Examples

### Example 1

Example 1 reads the kind and control flags of a view tile and a control tile, and the active filter on the control.

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
  // Each rendered tile records its kind, whether it is a control, and any filter that a control has set.
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 2);
  studio.board("overview").view(bi::table("rows").query(bi::from("sales")).place(0, 0));
  studio.board("overview").control(bi::category("pick", "region", "Region").place(0, 1));
  studio.set_filter(bi::Filter::eq("region", std::string("north")));
  const auto rendered = studio.render();
  const auto& tiles = rendered.boards.front().tiles;
  std::cout << "view tile kind is detail table: " << (tiles[0].kind == bi::ViewKind::DetailTable) << '\n';
  std::cout << "view tile is a control: " << tiles[0].is_control << '\n';
  std::cout << "control tile is a control: " << tiles[1].is_control << '\n';
  std::cout << "control tile has an active filter: " << tiles[1].active_filter.has_value() << '\n';
  return 0;
}
```

Output

```text
view tile kind is detail table: 1
view tile is a control: 0
control tile is a control: 1
control tile has an active filter: 1
```

### Example 2

Example 2 reads the rows a table tile holds after a filter.

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
  // A table tile carries the rows it shows, after the session filters are applied.
  const auto nexus = make_nexus();
  bi::Studio studio = overview_studio(nexus);
  studio.set_filter(bi::Filter::eq("region", std::string("south")));
  const auto rendered = studio.render();
  const auto& tile = rendered.boards.front().tiles.front();
  std::cout << "tile key: " << tile.key << '\n';
  std::cout << "rows held by the tile: " << tile.table->height() << '\n';
  return 0;
}
```

Output

```text
tile key: rows
rows held by the tile: 1
```

### Example 3

Example 3 reads the dependency map to see which tiles read region.

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
  // After a filter, the render lists which tiles depend on each feature.
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 2);
  studio.board("overview").view(bi::table("rows").query(bi::from("sales")).place(0, 0));
  studio.board("overview").control(bi::category("pick", "region", "Region").place(0, 1));
  studio.set_filter(bi::Filter::eq("region", std::string("north")));
  const auto rendered = studio.render();
  for (const auto& [feature, tiles] : rendered.dependencies) {
    std::cout << feature << " is used by " << tiles.size() << " tiles\n";
  }
  return 0;
}
```

Output

```text
region is used by 2 tiles
```

## See Also

- [`BoardRender`](bi-board-render.md) -- holds the tiles.
- [`Tile`](bi-tile.md) -- the board's tile before rendering.
