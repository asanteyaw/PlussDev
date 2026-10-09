# `pluss::bi::Studio::start_serving`

[Back to `Studio`](../bi-studio.md) · [PlussBI API Reference](../bi.md)

```cpp
#include <pluss/bi/specs.h>

ServeHandle start_serving(std::string_view title = {},
                           std::int64_t width = 1280,
                           std::int64_t height = 820,
                           std::int64_t port = 0,
                           bool open_browser = true,
                           DashboardChrome chrome = DashboardChrome::Sidebar);
```

`start_serving` starts a local web server for the studio and returns a handle to it. Port zero asks the operating system for a free port.
The page answers over HTTP while the handle is running. The studio must outlive the handle, because every request renders the studio.
Set `open_browser` to false in programs and tests, so no browser opens.

## Overloads

This function has one overload.

## Parameters

| Parameter | Meaning |
|---|---|
| `title` | The page title. Empty uses the studio key. |
| `width`, `height` | The frame size. Both must be positive. |
| `port` | The port to listen on. Zero asks for any free port. |
| `open_browser` | Open the page in the system browser. Default true. |
| `chrome` | `Sidebar` (default) or `None`. |

## Returns

A `ServeHandle`. It reports its port and URL, whether it is running, and it stops the server.

## Errors

A zero or negative width or height is refused. A failure to bind the port is reported as an error.

## Practical Examples

### Example 1

Example 1 starts a server, requests the page over HTTP, and reads the status. The status is 200.

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

### Example 2

Example 2 stops a running server. It reports running before the stop and not running after.

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

### Example 3

Example 3 asks for a zero width. The call is refused and no server starts.

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

## See Also

- [`serve`](serve.md) -- starts a server and blocks.
- [`Studio`](../bi-studio.md) -- the class.
