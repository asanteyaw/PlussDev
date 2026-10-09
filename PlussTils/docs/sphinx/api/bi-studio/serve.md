# `pluss::bi::Studio::serve`

[Back to `Studio`](../bi-studio.md) · [PlussBI API Reference](../bi.md)

```cpp
#include <pluss/bi/specs.h>

void serve(std::string_view title = {},
           std::int64_t width = 1280,
           std::int64_t height = 820,
           std::int64_t port = 0,
           bool open_browser = true,
           DashboardChrome chrome = DashboardChrome::Sidebar);
```

`serve` is the command-line wrapper around `start_serving`. It starts the server, prints the URL, and blocks the calling thread until the
server stops. A process stops it by being interrupted, so `serve` returns nothing to inspect. Use `start_serving` when your program must stop
the server itself.

## Overloads

This function has one overload.

## Parameters

| Parameter | Meaning |
|---|---|
| `title`, `width`, `height`, `port`, `open_browser`, `chrome` | The same as `start_serving`. |

## Returns

Nothing. The call returns when the server stops.

## Errors

The same refusals as `start_serving`.

## Practical Examples

The examples run the call in a child process, because the call blocks. The parent checks the page over HTTP, then stops the child with a signal.

### Example 1

The child serves the page, and the parent receives status 200 from it. The parent then stops the child with a signal, which ends the server, and confirms that the child ended by signal.

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

### Example 2

The wrapper announces the URL it is serving before it blocks. The child writes that line to a file, and the parent reads it back.

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
  // The serve wrapper prints the URL it is serving before it blocks. The child writes that line to a file.
  const auto nexus = make_nexus();
  bi::Studio studio = overview_studio(nexus);
  const fs::path dir = private_dir("pluss_serve_announce_example");
  const std::string log = (dir / "child.log").string();
  const pid_t child = fork();
  if (child == 0) {
    const int sink = open(log.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
    dup2(sink, 1);
    studio.serve("Sales", 900, 600, 18767, false, bi::DashboardChrome::None);
    _exit(0);
  }
  int status = -1;
  for (int attempt = 0; attempt < 100 && status != 200; ++attempt) {
    httplib::Client client("127.0.0.1", 18767);
    const auto response = client.Get("/");
    if (response) status = response->status;
    if (status != 200) usleep(100000);
  }
  // The page can answer before the banner is written, so wait for the banner itself before stopping the child.
  for (int attempt = 0; attempt < 100 && read_file(log).find("is live at") == std::string::npos; ++attempt) {
    usleep(100000);
  }
  kill(child, SIGTERM);
  int wait_status = 0;
  waitpid(child, &wait_status, 0);
  const std::string text = read_file(log);
  std::cout << "announces the served URL: " << (text.find("is live at http://127.0.0.1:18767/") != std::string::npos) << '\n';
  fs::remove_all(dir);
  return 0;
}
```

Output

```text
announces the served URL: 1
```

### Example 3

A zero width is refused before the call blocks, so no server starts.

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

## See Also

- [`start_serving`](start_serving.md) -- the call to use when a program must stop the server itself.
