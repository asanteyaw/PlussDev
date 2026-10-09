# PlussBI: Studios

```{raw} html
<meta name="pluss-doc-kind" content="guide">
```

[Back to PlussTils](../index.md) · [PlussBI API Reference](../api/bi.md) · [PlussBI: A Guide](bi.md)

A studio owns the boards, the session, and the source tables, and it turns them into what a reader sees. This chapter covers every studio member: naming, sources, boards, the session and its filters, rendering and its cache, saving and restoring, the four export formats, serving, and showing. It ends with the render results and the dashboard chrome.

## The mental model

A studio is the one object you hold. It reads its tables from a source, keeps its boards and session, and renders them on demand. Rendering reads the source and the session and produces a result: the boards, their tiles, and cache statistics. It never changes the boards. A studio with no source refuses to render, and a studio that is saved does not save its tables.

Exports and serving are renders written out in another form. The same render feeds an HTML page, an SVG, a PNG, a PDF, or a local server, so a dashboard looks the same wherever you put it.

## Worked tour

Every example is a complete program, and the output under it is copied from that program's run.

## Example 1: A studio has a key

A studio is named by its key, and a new studio has no boards and an empty session. This example builds one and reads each.

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
  // A studio is created with a key and starts with no sources, boards, or filters.
  bi::Studio studio("sales_studio");
  std::cout << "key: " << studio.key() << '\n';
  std::cout << "boards: " << studio.boards().size() << '\n';
  std::cout << "session empty: " << studio.session().empty() << '\n';
  return 0;
}
```

Output

```text
key: sales_studio
boards: 0
session empty: 1
```

The key is kept, there are no boards, and the session is empty.

## Example 2: A default studio key

A studio built without a key takes the default key. This example builds one with no argument.

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
  // A default-constructed studio is keyed "studio", so an unnamed studio still has a key.
  bi::Studio studio;
  std::cout << "default key: " << studio.key() << '\n';
  return 0;
}
```

Output

```text
default key: studio
```

The default key is studio.

## Example 3: The key is not part of the render

Two studios over the same source render the same rows, whatever their keys. This example renders two studios with different keys.

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
  // The key names the studio. It does not change what the boards render.
  const auto nexus = make_nexus();
  bi::Studio first = overview_studio(nexus);
  bi::Studio second("another_name");
  second.source(nexus);
  second.board("overview").title("Sales overview").grid(1, 1);
  second.board("overview").view(bi::table("rows").query(bi::from("sales")).place(0, 0));
  std::cout << "same rows under either key: " << (rows_shown(first) == rows_shown(second)) << '\n';
  return 0;
}
```

Output

```text
same rows under either key: 1
```

The two studios show the same rows.

## Example 4: A source makes rendering possible

A studio reads its data from a source, set with source. This example sets one and renders a table.

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
  // Once the studio has a source and a board with a view, rendering shows the table's rows.
  const auto nexus = make_nexus();
  const auto studio = overview_studio(nexus);
  std::cout << "rows shown: " << rows_shown(studio) << '\n';
  return 0;
}
```

Output

```text
rows shown: 3
```

The table shows three rows from the source.

## Example 5: A session carries across renders

The studio keeps its session between renders. This example renders twice without changing anything.

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
  // The session belongs to the studio. Each render reads it, so a filter set once applies to every later render.
  const auto nexus = make_nexus();
  bi::Studio studio = overview_studio(nexus);
  studio.set_filter(bi::Filter::eq("region", std::string("north")));
  std::cout << "first render: " << rows_shown(studio) << '\n';
  std::cout << "second render: " << rows_shown(studio) << '\n';
  return 0;
}
```

Output

```text
first render: 2
second render: 2
```

Both renders show the same number of rows.

## Example 6: Asking for a board creates or reuses it

Asking for a board by key returns the existing board if there is one. This example asks twice for the same key.

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
  // The first call with a key creates the board. Later calls with the same key return that board.
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  bi::Board& first = studio.board("overview");
  bi::Board& again = studio.board("overview");
  std::cout << "same board: " << (&first == &again) << '\n';
  std::cout << "boards after two calls: " << studio.boards().size() << '\n';
  return 0;
}
```

Output

```text
same board: 1
boards after two calls: 1
```

Both calls return the same board, and only one board exists.

## Example 7: Boards are listed in creation order

The board list follows creation order, not the order of the names. This example creates three boards in a deliberate order and lists them.

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
  bi::Studio studio("sales_studio");
  studio.board("second_created_first_listed");
  studio.board("alpha");
  studio.board("zeta");
  for (const auto& board : studio.boards()) std::cout << board.key() << '\n';
  return 0;
}
```

Output

```text
second_created_first_listed
alpha
zeta
```

The first board created is listed first, although its name sorts last.

## Example 8: A studio starts with no boards

A new studio lists no boards. This example checks it.

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
  bi::Studio studio("sales_studio");
  std::cout << "boards on a new studio: " << studio.boards().size() << '\n';
  return 0;
}
```

Output

```text
boards on a new studio: 0
```

The count is zero.

## Example 9: Rendering without a source is refused

A studio has nothing to draw until it has a source. This example renders one that has none.

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
  // A studio with boards but no source cannot render.
  bi::Studio studio("sales_studio");
  studio.board("overview").grid(1, 1);
  studio.board("overview").view(bi::table("rows").query(bi::from("sales")).place(0, 0));
  std::cout << "render without source refused: " << throws([&] { (void)studio.render(); }) << '\n';
  return 0;
}
```

Output

```text
render without source refused: 1
```

The studio refuses to render.

## Example 10: A second source replaces the first

Setting a source again replaces the data the studio reads. This example renders from one source, then from a replacement.

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
  // Setting a source replaces the earlier one. The next render reads the new data.
  const auto nexus = make_nexus();
  bi::Studio studio = overview_studio(nexus);
  std::cout << "rows from the first source: " << rows_shown(studio) << '\n';
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame larger({
      {"region", Slate(pluss::make_note({"north", "south", "north", "east"}))},
      {"amount", Slate(pluss::make_series(std::vector<double>{10, 20, 30, 40}, {4}, cfg))},
  });
  const pluss::bundle::Nexus replacement({{"sales", larger}});
  studio.source(replacement);
  std::cout << "rows from the replacement source: " << rows_shown(studio) << '\n';
  return 0;
}
```

Output

```text
rows from the first source: 3
rows from the replacement source: 4
```

The first source shows three rows, and the replacement shows four.

## Example 11: Replacing the session

Setting the session replaces the studio's choices, and the next render applies them. This example sets a session with a south filter, then replaces it with an empty one.

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
  // session(value) replaces the studio's session. The next render applies the new filters.
  const auto nexus = make_nexus();
  bi::Studio studio = overview_studio(nexus);
  bi::Session chosen;
  chosen.set_filter(bi::Filter::eq("region", std::string("south")));
  studio.session(chosen);
  std::cout << "rows with the south filter: " << rows_shown(studio) << '\n';
  studio.session(bi::Session{});
  std::cout << "rows after replacing with an empty session: " << rows_shown(studio) << '\n';
  return 0;
}
```

Output

```text
rows with the south filter: 1
rows after replacing with an empty session: 3
```

With the south filter the view shows one row. With the empty session it shows all three.

## Example 12: A filter narrows a studio's rows

A filter set on the studio applies to the next render. This example renders, sets a north filter, and renders again.

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
  const auto nexus = make_nexus();
  bi::Studio studio = overview_studio(nexus);
  std::cout << "rows before: " << rows_shown(studio) << '\n';
  studio.set_filter(bi::Filter::eq("region", std::string("north")));
  std::cout << "rows after north filter: " << rows_shown(studio) << '\n';
  return 0;
}
```

Output

```text
rows before: 3
rows after north filter: 2
```

The rows drop from three to two.

## Example 13: Setting a filter with the same key replaces it

A second filter under the same key replaces the first, so the studio holds one. This example sets two filters under one key.

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
  // A filter is stored under its key. A second filter with the same key replaces the first.
  const auto nexus = make_nexus();
  bi::Studio studio = overview_studio(nexus);
  studio.set_filter(bi::Filter::eq("region", std::string("north")));
  studio.set_filter(bi::Filter::eq("region", std::string("south")));
  std::cout << "filters held: " << studio.session().filters().size() << '\n';
  std::cout << "rows shown: " << rows_shown(studio) << '\n';
  return 0;
}
```

Output

```text
filters held: 1
rows shown: 1
```

One filter is held and the view shows one row.

## Example 14: A filter on an unknown feature is ignored

A filter on a feature no table has is ignored, so the rows stay. This example sets one.

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
  // An unscoped filter on a feature the table lacks is ignored, so the rows are unchanged.
  const auto nexus = make_nexus();
  bi::Studio studio = overview_studio(nexus);
  studio.set_filter(bi::Filter::eq("weight", 1.0));
  std::cout << "rows shown with an unknown feature: " << rows_shown(studio) << '\n';
  return 0;
}
```

Output

```text
rows shown with an unknown feature: 3
```

All three rows remain.

## Example 15: Clearing an absent filter is accepted

Clearing a key that was never set is not an error. This example clears one.

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
  const auto nexus = make_nexus();
  bi::Studio studio = overview_studio(nexus);
  studio.clear_filter("never_set");
  std::cout << "clearing an absent key is accepted: " << (rows_shown(studio) == 3) << '\n';
  return 0;
}
```

Output

```text
clearing an absent key is accepted: 1
```

The call is accepted.

## Example 16: Rendering the default way

The render returns the studio's key, the session, the boards, and each board's title. This example renders and reads them.

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

The studio key, one session filter, one board, and the board's title are all in the result.

## Example 17: A warm cache reuses tiles

The first render computes every tile, and a second render with no change reuses them. This example renders twice and reads the cache statistics.

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
  // The cache counts what the render computed. A second render with nothing changed computes no tiles.
  const auto nexus = make_nexus();
  bi::Studio studio = overview_studio(nexus);
  const auto first = studio.render().cache;
  const auto second = studio.render().cache;
  std::cout << "first render tile misses: " << first.tile_misses << ", hits: " << first.tile_hits << '\n';
  std::cout << "second render tile misses: " << second.tile_misses << ", hits: " << second.tile_hits << '\n';
  return 0;
}
```

Output

```text
first render tile misses: 1, hits: 0
second render tile misses: 0, hits: 1
```

The first render computed one tile and reused none. The second computed none and reused one.

## Example 18: Dependencies show which tiles use a feature

After a filter, the render lists which tiles depend on each feature. This example renders with a filter and reads the dependency for region.

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

Two tiles depend on region.

## Example 19: A refresh recomputes every tile

Refreshing throws away the cached tiles, so the next render computes them again. This example renders warm, refreshes, and renders again.

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
  // refresh drops the cached query results. The next render computes them again.
  const auto nexus = make_nexus();
  bi::Studio studio = overview_studio(nexus);
  (void)studio.render();
  const auto warm = studio.render().cache;
  studio.refresh();
  const auto cold = studio.render().cache;
  std::cout << "warm render misses: " << warm.query_misses << '\n';
  std::cout << "render after refresh misses: " << cold.query_misses << '\n';
  return 0;
}
```

Output

```text
warm render misses: 0
render after refresh misses: 1
```

The warm render missed none, and the render after the refresh missed one.

## Example 20: A refresh does not change the output

Refreshing changes how the studio computes, not what it shows. This example checks the rows before and after a refresh.

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
  // Refreshing changes how work is cached, not what is shown.
  const auto nexus = make_nexus();
  bi::Studio studio = overview_studio(nexus);
  const auto before = rows_shown(studio);
  studio.refresh();
  std::cout << "rows unchanged by refresh: " << (rows_shown(studio) == before) << '\n';
  return 0;
}
```

Output

```text
rows unchanged by refresh: 1
```

The rows are the same before and after.

## Example 21: A changed session invalidates the cache

A session change makes the cached tiles stale. This example renders warm, sets a filter, and renders again.

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
  // A session change invalidates the cached tiles, so the next render computes them again.
  const auto nexus = make_nexus();
  bi::Studio studio = overview_studio(nexus);
  (void)studio.render();
  const auto steady = studio.render().cache;
  studio.set_filter(bi::Filter::eq("region", std::string("north")));
  const auto changed = studio.render().cache;
  std::cout << "steady render tile misses: " << steady.tile_misses << '\n';
  std::cout << "render after a filter tile misses: " << changed.tile_misses << '\n';
  return 0;
}
```

Output

```text
steady render tile misses: 0
render after a filter tile misses: 1
```

The warm render missed none. The render after the filter missed one.

## Example 22: Saving a studio

Saving writes the boards and the session to a file, and loading brings them back. This example saves and loads, then renders with the source.

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
  // save writes the studio to a file. Loading it with the same data gives back the same rows.
  const auto nexus = make_nexus();
  bi::Studio studio = overview_studio(nexus);
  const fs::path dir = private_dir("pluss_studio_save_example");
  studio.save((dir / "studio.bi").string());
  std::cout << "file written: " << fs::exists(dir / "studio.bi") << '\n';
  const bi::Studio restored = bi::Studio::load((dir / "studio.bi").string(), nexus);
  std::cout << "rows after load: " << rows_shown(restored) << '\n';
  fs::remove_all(dir);
  return 0;
}
```

Output

```text
file written: 1
rows after load: 3
```

The file is written, and the loaded studio renders three rows.

## Example 23: A saved studio keeps its filter

The session is part of the saved file, so a filter survives a round trip. This example saves a filtered studio and loads it.

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
  const auto nexus = make_nexus();
  bi::Studio studio = overview_studio(nexus);
  studio.set_filter(bi::Filter::eq("region", std::string("north")));
  const fs::path dir = private_dir("pluss_studio_save_filter_example");
  studio.save((dir / "studio.bi").string());
  const bi::Studio restored = bi::Studio::load((dir / "studio.bi").string(), nexus);
  std::cout << "filter kept: " << restored.session().has_filter("region") << '\n';
  std::cout << "rows under the kept filter: " << rows_shown(restored) << '\n';
  fs::remove_all(dir);
  return 0;
}
```

Output

```text
filter kept: 1
rows under the kept filter: 2
```

The filter is kept, and the loaded studio shows two rows under it.

## Example 24: Loading a studio

Loading restores the boards, and rendering needs the source again. This example loads a studio and renders it with data.

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
  const auto nexus = make_nexus();
  bi::Studio studio = overview_studio(nexus);
  const fs::path dir = private_dir("pluss_studio_load_example");
  studio.save((dir / "studio.bi").string());
  const bi::Studio loaded = bi::Studio::load((dir / "studio.bi").string(), nexus);
  std::cout << "boards loaded: " << loaded.boards().size() << '\n';
  std::cout << "rows shown: " << rows_shown(loaded) << '\n';
  fs::remove_all(dir);
  return 0;
}
```

Output

```text
boards loaded: 1
rows shown: 3
```

The loaded studio has one board and shows three rows.

## Example 25: Loading without data

The file holds the dashboard, not the tables. A loaded studio with no source has its boards but cannot render. This example checks both.

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
  // Loading without a nexus restores the boards and the session, but not the tables. Rendering then fails.
  const auto nexus = make_nexus();
  bi::Studio studio = overview_studio(nexus);
  const fs::path dir = private_dir("pluss_studio_load_nodata_example");
  studio.save((dir / "studio.bi").string());
  const bi::Studio loaded = bi::Studio::load((dir / "studio.bi").string());
  std::cout << "boards loaded: " << loaded.boards().size() << '\n';
  std::cout << "render without data refused: " << throws([&] { (void)loaded.render(); }) << '\n';
  fs::remove_all(dir);
  return 0;
}
```

Output

```text
boards loaded: 1
render without data refused: 1
```

The board is loaded, and the render is refused without data.

## Example 26: Loading a missing file is refused

Loading a file that does not exist is refused. This example loads one.

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
  const fs::path dir = private_dir("pluss_studio_missing_example");
  std::cout << "missing file refused: " << throws([&] { (void)bi::Studio::load((dir / "absent.bi").string()); }) << '\n';
  fs::remove_all(dir);
  return 0;
}
```

Output

```text
missing file refused: 1
```

The load is refused.

## Example 27: Restoring a session from a file

Restoring brings back a session from a file without touching the boards. This example renders with a filter, saves, clears, and restores.

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
  // restore_session replaces only the session. The boards stay as they are.
  const auto nexus = make_nexus();
  bi::Studio source = overview_studio(nexus);
  source.set_filter(bi::Filter::eq("region", std::string("north")));
  const fs::path dir = private_dir("pluss_studio_restore_example");
  source.save((dir / "studio.bi").string());
  bi::Studio target = overview_studio(nexus);
  std::cout << "rows before restore: " << rows_shown(target) << '\n';
  target.restore_session((dir / "studio.bi").string());
  std::cout << "rows after restore: " << rows_shown(target) << '\n';
  fs::remove_all(dir);
  return 0;
}
```

Output

```text
rows before restore: 3
rows after restore: 2
```

Before the restore the view shows three rows, and after the restore two.

## Example 28: Restoring keeps the boards

A session restore leaves the boards alone. This example checks the boards after a restore.

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
  const auto nexus = make_nexus();
  bi::Studio source = overview_studio(nexus);
  const fs::path dir = private_dir("pluss_studio_restore_boards_example");
  source.save((dir / "studio.bi").string());
  bi::Studio target("sales_studio");
  target.source(nexus);
  target.board("other").grid(1, 1);
  target.restore_session((dir / "studio.bi").string());
  std::cout << "boards after restore: " << target.boards().size() << '\n';
  std::cout << "first board is still other: " << (target.boards().front().key() == "other") << '\n';
  fs::remove_all(dir);
  return 0;
}
```

Output

```text
boards after restore: 1
first board is still other: 1
```

The studio still has its one board, and it is the same board.

## Example 29: A standalone HTML document

Exporting HTML writes a complete page with a title. This example exports one and checks it.

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
  // export_html writes a standalone document. The title you pass appears in the page.
  const auto nexus = make_nexus();
  bi::Studio studio = overview_studio(nexus);
  const fs::path dir = private_dir("pluss_studio_html_example");
  studio.export_html((dir / "overview.html").string(), "Quarterly sales", 900, 600);
  const std::string html = read_file(dir / "overview.html");
  std::cout << "file written: " << fs::exists(dir / "overview.html") << '\n';
  std::cout << "document has a root element: " << (html.find("<html") != std::string::npos) << '\n';
  std::cout << "document carries the title: " << (html.find("Quarterly sales") != std::string::npos) << '\n';
  fs::remove_all(dir);
  return 0;
}
```

Output

```text
file written: 1
document has a root element: 1
document carries the title: 1
```

The file is written, it has a root element, and it carries the title.

## Example 30: The chrome decides the layout

The dashboard chrome sets whether the page has a sidebar. This example exports with none and with a sidebar, and checks each page.

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
  // The chrome decides the layout. Sidebar chrome adds a sidebar element; None does not.
  const auto nexus = make_nexus();
  bi::Studio studio = overview_studio(nexus);
  const fs::path dir = private_dir("pluss_studio_chrome_example");
  studio.export_html((dir / "none.html").string(), "T", 900, 600, bi::DashboardChrome::None);
  studio.export_html((dir / "sidebar.html").string(), "T", 900, 600, bi::DashboardChrome::Sidebar);
  std::cout << "None has a sidebar element: " << (read_file(dir / "none.html").find("class=\"bi-sidebar\"") != std::string::npos) << '\n';
  std::cout << "Sidebar has a sidebar element: " << (read_file(dir / "sidebar.html").find("class=\"bi-sidebar\"") != std::string::npos) << '\n';
  fs::remove_all(dir);
  return 0;
}
```

Output

```text
None has a sidebar element: 0
Sidebar has a sidebar element: 1
```

The page with no chrome has no sidebar, and the page with the sidebar chrome has one.

## Example 31: A zero width HTML export is refused

A width and a height must both be positive. This example exports with a zero width.

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
  // A width and a height must both be positive. The refusal writes no file.
  const auto nexus = make_nexus();
  bi::Studio studio = overview_studio(nexus);
  const fs::path dir = private_dir("pluss_studio_html_zero_example");
  std::cout << "zero width refused: " << throws([&] { studio.export_html((dir / "x.html").string(), "T", 0, 600); }) << '\n';
  std::cout << "no file written: " << !fs::exists(dir / "x.html") << '\n';
  fs::remove_all(dir);
  return 0;
}
```

Output

```text
zero width refused: 1
no file written: 1
```

The export is refused and no file is written.

## Example 32: An SVG document

Exporting SVG writes a vector document. This example exports one and checks its root.

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
  // export_svg writes the studio as one SVG document.
  const auto nexus = make_nexus();
  bi::Studio studio = overview_studio(nexus);
  const fs::path dir = private_dir("pluss_studio_svg_example");
  studio.export_svg((dir / "overview.svg").string());
  const std::string svg = read_file(dir / "overview.svg");
  std::cout << "file written: " << fs::exists(dir / "overview.svg") << '\n';
  std::cout << "contains an svg root: " << (svg.find("<svg") != std::string::npos) << '\n';
  fs::remove_all(dir);
  return 0;
}
```

Output

```text
file written: 1
contains an svg root: 1
```

The file is written and contains an SVG root.

## Example 33: A second SVG export overwrites

A second export to the same path replaces the file. This example exports twice.

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
  const auto nexus = make_nexus();
  bi::Studio studio = overview_studio(nexus);
  const fs::path dir = private_dir("pluss_studio_svg_overwrite_example");
  studio.export_svg((dir / "overview.svg").string());
  const auto first_size = fs::file_size(dir / "overview.svg");
  studio.export_svg((dir / "overview.svg").string());
  std::cout << "second export replaces the file: " << (fs::file_size(dir / "overview.svg") == first_size) << '\n';
  fs::remove_all(dir);
  return 0;
}
```

Output

```text
second export replaces the file: 1
```

The second export replaces the first.

## Example 34: SVG export without a source is refused

Exporting needs a rendered studio, so a studio with no source is refused. This example tries one.

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
  const fs::path dir = private_dir("pluss_studio_svg_nosource_example");
  bi::Studio studio("sales_studio");
  studio.board("overview").grid(1, 1);
  std::cout << "export without source refused: " << throws([&] { studio.export_svg((dir / "x.svg").string()); }) << '\n';
  fs::remove_all(dir);
  return 0;
}
```

Output

```text
export without source refused: 1
```

The export is refused.

## Example 35: A PNG has its signature

A PNG export begins with the PNG signature. This example exports and checks it.

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
  // export_png writes a PNG file, which begins with the PNG signature.
  const auto nexus = make_nexus();
  bi::Studio studio = overview_studio(nexus);
  const fs::path dir = private_dir("pluss_studio_png_example");
  studio.export_png((dir / "overview.png").string());
  const std::string png = read_file(dir / "overview.png");
  std::cout << "PNG signature present: " << (png.compare(0, 4, "\x89PNG") == 0) << '\n';
  fs::remove_all(dir);
  return 0;
}
```

Output

```text
PNG signature present: 1
```

The signature is present.

## Example 36: PNG resolution changes the size

The resolution scales the image. This example exports at 72 and at 288 dots per inch.

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
  // The resolution in dots per inch changes the size of the image. The default is 144.
  const auto nexus = make_nexus();
  bi::Studio studio = overview_studio(nexus);
  const fs::path dir = private_dir("pluss_studio_png_dpi_example");
  studio.export_png((dir / "low.png").string(), 72);
  studio.export_png((dir / "default.png").string());
  studio.export_png((dir / "high.png").string(), 288);
  std::cout << "72 dpi is smaller than default: " << (fs::file_size(dir / "low.png") < fs::file_size(dir / "default.png")) << '\n';
  std::cout << "288 dpi is larger than default: " << (fs::file_size(dir / "high.png") > fs::file_size(dir / "default.png")) << '\n';
  fs::remove_all(dir);
  return 0;
}
```

Output

```text
72 dpi is smaller than default: 1
288 dpi is larger than default: 1
```

The 72 dpi image is smaller than the default, and the 288 dpi image is larger.

## Example 37: PNG export without a source is refused

A PNG export needs a source, like every export. This example tries one with none.

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
  const fs::path dir = private_dir("pluss_studio_png_nosource_example");
  bi::Studio studio("sales_studio");
  studio.board("overview").grid(1, 1);
  std::cout << "export without source refused: " << throws([&] { studio.export_png((dir / "x.png").string()); }) << '\n';
  fs::remove_all(dir);
  return 0;
}
```

Output

```text
export without source refused: 1
```

The export is refused.

## Example 38: PDF with the automatic engine

The automatic PDF engine picks a converter for you. This example exports with it and checks the signature.

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
  // Engine Auto produces a PDF document. The file begins with the PDF signature.
  const auto nexus = make_nexus();
  bi::Studio studio = overview_studio(nexus);
  const fs::path dir = private_dir("pluss_studio_pdf_auto_example");
  studio.export_pdf((dir / "overview.pdf").string(), pluss::pl::PdfEngine::Auto);
  const std::string pdf = read_file(dir / "overview.pdf");
  std::cout << "PDF signature present: " << (pdf.compare(0, 4, "%PDF") == 0) << '\n';
  fs::remove_all(dir);
  return 0;
}
```

Output

```text
PDF signature present: 1
```

The PDF signature is present.

## Example 39: PDF with the internal engine

The internal engine writes the PDF without an external converter. This example exports with it.

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
  // Engine Internal produces a PDF document. The file begins with the PDF signature.
  const auto nexus = make_nexus();
  bi::Studio studio = overview_studio(nexus);
  const fs::path dir = private_dir("pluss_studio_pdf_internal_example");
  studio.export_pdf((dir / "overview.pdf").string(), pluss::pl::PdfEngine::Internal);
  const std::string pdf = read_file(dir / "overview.pdf");
  std::cout << "PDF signature present: " << (pdf.compare(0, 4, "%PDF") == 0) << '\n';
  fs::remove_all(dir);
  return 0;
}
```

Output

```text
PDF signature present: 1
```

The PDF signature is present.

## Example 40: PDF converted from SVG

The SVG convert engine draws the SVG and converts it to PDF. This example exports with it.

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
  // Engine SvgConvert produces a PDF document. The file begins with the PDF signature.
  const auto nexus = make_nexus();
  bi::Studio studio = overview_studio(nexus);
  const fs::path dir = private_dir("pluss_studio_pdf_svgconvert_example");
  studio.export_pdf((dir / "overview.pdf").string(), pluss::pl::PdfEngine::SvgConvert);
  const std::string pdf = read_file(dir / "overview.pdf");
  std::cout << "PDF signature present: " << (pdf.compare(0, 4, "%PDF") == 0) << '\n';
  fs::remove_all(dir);
  return 0;
}
```

Output

```text
PDF signature present: 1
```

The PDF signature is present.

## Example 41: The local server answers

Starting a server runs the dashboard on a local page. This example starts one and requests the page.

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
  // start_serving returns a handle to a server on a free local port. The page answers over HTTP.
  const auto nexus = make_nexus();
  bi::Studio studio = overview_studio(nexus);
  auto handle = studio.start_serving("Sales", 900, 600, 0, false, bi::DashboardChrome::None);
  httplib::Client client("127.0.0.1", handle.port());
  const auto response = client.Get("/");
  std::cout << "server running: " << handle.running() << '\n';
  std::cout << "page status: " << (response ? response->status : -1) << '\n';
  handle.stop();
  handle.wait();
  return 0;
}
```

Output

```text
server running: 1
page status: 200
```

The server is running and the page answers 200.

## Example 42: Stopping a server

The handle tells you whether the server is running, and stop ends it. This example checks before and after.

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
  const auto nexus = make_nexus();
  bi::Studio studio = overview_studio(nexus);
  auto handle = studio.start_serving("Sales", 900, 600, 0, false, bi::DashboardChrome::None);
  std::cout << "running before stop: " << handle.running() << '\n';
  handle.stop();
  handle.wait();
  std::cout << "running after stop: " << handle.running() << '\n';
  return 0;
}
```

Output

```text
running before stop: 1
running after stop: 0
```

The server runs before the stop and does not after it.

## Example 43: Serving with a zero size is refused

A served page needs a positive size. This example serves with a zero width.

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
  // A served page needs a positive width and height.
  const auto nexus = make_nexus();
  bi::Studio studio = overview_studio(nexus);
  std::cout << "zero width refused: " << throws([&] {
    auto handle = studio.start_serving("Sales", 0, 600, 0, false, bi::DashboardChrome::None);
  }) << '\n';
  return 0;
}
```

Output

```text
zero width refused: 1
```

The start is refused.

## Example 44: A child process serves the page

Serving blocks until the process stops. This example runs it in a child, waits for the page, and stops the child with a signal.

```cpp
#include <pluss/bi/bi.h>
#include <pluss/bi/serve.h>
#include <pluss/bundle/bundle.h>
#include <pluss/mic.h>

#include <cstdint>
#include <csignal>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <httplib.h>
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
  // serve blocks until the process is stopped. This check runs it in a child process, waits for the page to answer,
  // and then stops the child with a signal. The child's output goes to /dev/null, so this example stays deterministic.
  const auto nexus = make_nexus();
  bi::Studio studio = overview_studio(nexus);
  const pid_t child = fork();
  if (child == 0) {
    const int sink = open("/dev/null", O_WRONLY);
    dup2(sink, 1);
    studio.serve("Sales", 900, 600, 18765, false, bi::DashboardChrome::None);
    _exit(0);
  }
  int status = -1;
  for (int attempt = 0; attempt < 100 && status != 200; ++attempt) {
    httplib::Client client("127.0.0.1", 18765);
    const auto response = client.Get("/");
    if (response) status = response->status;
    if (status != 200) usleep(100000);
  }
  std::cout << "page status: " << status << '\n';
  kill(child, SIGTERM);
  int wait_status = 0;
  waitpid(child, &wait_status, 0);
  std::cout << "server stopped by signal: " << WIFSIGNALED(wait_status) << '\n';
  return 0;
}
```

Output

```text
page status: 200
server stopped by signal: 1
```

The page answers 200, and the child stops by signal.

## Example 45: Serving with a zero size is refused

The blocking serve call refuses a zero size as well. This example tries one.

```cpp
#include <pluss/bi/bi.h>
#include <pluss/bi/serve.h>
#include <pluss/bundle/bundle.h>
#include <pluss/mic.h>

#include <cstdint>
#include <csignal>
#include <fcntl.h>
#include <sys/wait.h>
#include <unistd.h>
#include <httplib.h>
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
  // serve checks its size before it blocks, so a zero width is refused without starting a server.
  const auto nexus = make_nexus();
  bi::Studio studio = overview_studio(nexus);
  std::cout << "zero width refused: " << throws([&] { studio.serve("Sales", 0, 600, 18766, false, bi::DashboardChrome::None); }) << '\n';
  return 0;
}
```

Output

```text
zero width refused: 1
```

The serve is refused.

## Example 46: Showing without launching

Show renders the studio and reports success, and with launch off it opens nothing. This example calls it that way. The next example turns launch on.

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

bi::Studio overview_studio(const pluss::bundle::Nexus& nexus) {
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").title("Sales overview").grid(1, 1);
  studio.board("overview").view(bi::table("rows").query(bi::from("sales")).place(0, 0));
  return studio;
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
  // show with launch set to false renders the studio and reports success. Nothing opens.
  const auto nexus = make_nexus();
  const auto studio = overview_studio(nexus);
  std::cout << "show without launch succeeds: " << studio.show("Sales overview", 900, 600, false, bi::DashboardChrome::None) << '\n';
  return 0;
}
```

Output

```text
show without launch succeeds: true
```

Show succeeds without opening a window.

## Example 47: Showing without a source is refused

Show needs a source, so a studio with none is refused. This example tries one.

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

bi::Studio overview_studio(const pluss::bundle::Nexus& nexus) {
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").title("Sales overview").grid(1, 1);
  studio.board("overview").view(bi::table("rows").query(bi::from("sales")).place(0, 0));
  return studio;
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
  bi::Studio bare("bare");
  bare.board("overview").grid(1, 1);
  std::cout << "show without a source refused: " << throws([&] {
    (void)bare.show("Sales overview", 900, 600, false, bi::DashboardChrome::None);
  }) << '\n';
  return 0;
}
```

Output

```text
show without a source refused: true
```

The show is refused.

## Example 48: Showing in a window

Show with launch on writes the dashboard to a temporary HTML file and opens it in your default browser. The call returns once the browser has been asked to open, and it does not wait for the browser to close. This example shows the page this way. It has no captured output, because it opens a browser on the machine that runs it. Copy it and run it yourself to see the page.

```cpp
#include <pluss/bi/bi.h>
#include <pluss/bundle/bundle.h>
#include <pluss/mic.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace bi = pluss::bi;
namespace fs = std::filesystem;

// Six sales rows: regions north, south, north, east, south, north; months jan, jan, feb, feb, mar, mar;
// amounts 10 to 60; units 1 to 6; and one date per row from January to June 2024.
pluss::bundle::Nexus make_nexus() {
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame sales({
      {"region", Slate(pluss::make_note({"north", "south", "north", "east", "south", "north"}))},
      {"month", Slate(pluss::make_note({"jan", "jan", "feb", "feb", "mar", "mar"}))},
      {"amount", Slate(pluss::make_series(std::vector<double>{10, 20, 30, 40, 50, 60}, {6}, cfg))},
      {"units", Slate(pluss::make_series(std::vector<double>{1, 2, 3, 4, 5, 6}, {6}, cfg))},
      {"placed", Slate(pluss::make_timeline({"2024-01-05", "2024-02-10", "2024-03-15", "2024-04-20", "2024-05-25", "2024-06-30"}))},
  });
  return pluss::bundle::Nexus({{"sales", sales}});
}

// The text of the page a studio exports, for checks on what is shown.
std::string page_text(const bi::Studio& studio) {
  const fs::path dir = fs::temp_directory_path() / "pluss_bi_guide_page";
  fs::remove_all(dir);
  fs::create_directories(dir);
  studio.export_html((dir / "page.html").string(), "T", 900, 600, bi::DashboardChrome::None);
  std::ifstream in((dir / "page.html").string());
  std::string text((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  fs::remove_all(dir);
  return text;
}


int main() {
  // show with launch on writes the dashboard to a temporary HTML file and opens it in the default browser.
  // The call does not wait for the browser to close, so this program ends as soon as the browser is asked to open.
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").title("Sales overview").grid(1, 1);
  studio.board("overview").view(bi::table("rows").query(bi::from("sales")).title("Rows").place(0, 0));
  studio.show("Sales overview", 900, 600, true, bi::DashboardChrome::Sidebar);
  return 0;
}
```

Output

```text
(no captured output: this program opens the dashboard in your default browser)
```

Nothing is printed. The dashboard opens in the default browser, which is the one thing that makes this different from the no-launch examples above.

## Example 49: Showing a filtered studio

A filtered studio shows its filtered rows when shown. This example filters, then shows.

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

bi::Studio overview_studio(const pluss::bundle::Nexus& nexus) {
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").title("Sales overview").grid(1, 1);
  studio.board("overview").view(bi::table("rows").query(bi::from("sales")).place(0, 0));
  return studio;
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
  // The session filters apply when show renders, so a filtered studio still succeeds.
  const auto nexus = make_nexus();
  auto studio = overview_studio(nexus);
  studio.set_filter(bi::Filter::eq("region", std::string("north")));
  std::cout << "filtered studio shows: " << studio.show("Sales overview", 900, 600, false, bi::DashboardChrome::None) << '\n';
  return 0;
}
```

Output

```text
filtered studio shows: true
```

The filtered studio shows.

## Example 50: The chrome values

The chrome has two values: none and sidebar. This example prints both.

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
  // DashboardChrome selects the page layout. None is a single column; Sidebar adds the board tabs and filter panel.
  std::cout << "None=" << static_cast<int>(bi::DashboardChrome::None)
            << " Sidebar=" << static_cast<int>(bi::DashboardChrome::Sidebar) << '\n';
  return 0;
}
```

Output

```text
None=0 Sidebar=1
```

None is zero and sidebar is one.

## Example 51: The default chrome is the sidebar

A studio that names no chrome exports with the sidebar. This example exports with the default and checks for one.

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
  // export_html uses Sidebar chrome unless you pass another value.
  const auto nexus = make_nexus();
  bi::Studio studio = overview_studio(nexus);
  const fs::path dir = private_dir("pluss_studio_chrome_default_example");
  studio.export_html((dir / "default.html").string(), "T", 900, 600);
  std::cout << "default export has a sidebar element: " << (read_file(dir / "default.html").find("class=\"bi-sidebar\"") != std::string::npos) << '\n';
  fs::remove_all(dir);
  return 0;
}
```

Output

```text
default export has a sidebar element: 1
```

The default export has a sidebar.

## Example 52: A rendered view tile

Each rendered tile records its kind, whether it is a control, and any filter that a control has set. This example renders a view and a control.

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

The view tile is a detail table and not a control. The control tile is a control and has an active filter.

## Example 53: A rendered table tile

A rendered table tile holds its rows. This example renders one and reads its key and rows.

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

The tile key is rows, and it holds rows.

## Decisions

| You want | Use | Notes |
|---|---|---|
| Show the same dashboard in another place | `export_html`, `export_svg`, `export_png`, or `export_pdf` | A zero size is refused, and no file is written. |
| Keep a dashboard for later | `save` and `load` | Loading needs the source again; the file holds no tables. |
| Keep only the choices, not the boards | `save` then `restore_session` | Restoring leaves the boards alone. |
| Let a reader use a dashboard in a browser now | `serve` or `start_serving` | `start_serving` returns a handle you can stop. |
| Open the page once, on this machine | `show` | With launch off, show writes the page and opens nothing. |

## Pitfalls

**Expecting a loaded studio to render without its data.** studio_load_02 shows the boards load and the render is refused.

**Expecting a board list to be sorted by name.** studio_board_02 lists boards in creation order.

**Expecting a refresh to change what is shown.** studio_refresh_02 shows that the rows are the same before and after.

**Expecting show to wait for the browser.** The launch example returns as soon as the browser is asked to open, so keep the program alive if you need it to stay up.

## Where next

- [Boards, tiles, and layout](bi-boards.md) -- what a studio holds.
- [Sessions and parameters](bi-sessions.md) -- the choices a studio renders.
- [PlussBI API Reference](../api/bi-studio.md) -- every studio member.
