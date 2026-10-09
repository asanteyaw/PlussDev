# `pluss::bi::Studio::export_html`

[Back to `Studio`](../bi-studio.md) · [PlussBI API Reference](../bi.md)

```cpp
#include <pluss/bi/specs.h>

void export_html(std::string_view path,
                 std::string_view title = {},
                 std::int64_t width = 1280,
                 std::int64_t height = 820,
                 DashboardChrome chrome = DashboardChrome::Sidebar) const;
```

`export_html` writes the studio as a standalone HTML document. The title appears in the page. The width and height set the frame the
dashboard lays out in. The chrome chooses the layout: `Sidebar` by default, or `None` for a single column.

## Overloads

This function has one overload.

## Parameters

| Parameter | Meaning |
|---|---|
| `path` | The file to write. |
| `title` | The page title. Empty uses the studio key. |
| `width`, `height` | The frame size. Both must be positive. |
| `chrome` | `Sidebar` (default) or `None`. |

## Returns

Nothing. The call writes the file.

## Errors

A zero or negative width or height is refused, and no file is written.

## Practical Examples

### Example 1

Example 1 exports a document with a title and checks that it has a root element and carries the title.

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

### Example 2

Example 2 exports with each chrome. Only the sidebar chrome adds a sidebar element to the markup.

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

### Example 3

Example 3 exports with a zero width. The call is refused and writes no file.

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

## See Also

- [`DashboardChrome`](../bi-dashboard-chrome.md) -- the layout choice.
- [`show`](show.md) -- the window form.
