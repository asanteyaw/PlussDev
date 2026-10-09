# `pluss::bi::Board::key`

[Back to `Board`](../bi-board.md) · [PlussBI API Reference](../bi.md)

```cpp
#include <pluss/bi/specs.h>

const std::string& key() const noexcept;
```

`key` returns the board's key, exactly as it was given when the board was created. The key identifies the
board inside its studio: asking the studio for the same key returns the same board. When no title is set,
the page heading uses the key in its place.

An empty key is not valid. The studio refuses it when it renders, so a board you never render can carry
an empty key without harm, but a studio that holds one cannot be rendered.

## Parameters

Not applicable. The method takes no parameters.

## Returns

A reference to the key string. The reference is valid as long as the board is.

## Errors

This call does not throw. An empty key is refused later, when the studio renders.

## Practical Examples

### Example 1

Example 1 shows that a key with spaces and digits is stored unchanged, and that the studio returns the board for that same key.

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

### Example 2

Example 2 renders a board with no title. The rendered title is empty, and the page heading shows the key.

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

### Example 3

Example 3 shows that an empty key is refused when the studio renders.

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

## See Also

- [`title`](title.md) -- the heading text that replaces the key when set.
- [`Board`](../bi-board.md) -- the class that owns the key.
