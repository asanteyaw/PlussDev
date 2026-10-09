# `pluss::bi::Studio::show`

[Back to `Studio`](../bi-studio.md) · [PlussBI API Reference](../bi.md)

```cpp
#include <pluss/bi/specs.h>

bool show(std::string_view title = {},
          std::int64_t width = 1280,
          std::int64_t height = 820,
          bool launch = true,
          DashboardChrome chrome = DashboardChrome::Sidebar) const;
```

`show` renders the studio and then opens it in a native window. With `launch` set to false, it renders and checks the studio and returns true
without opening anything. That is the form to use in tests and in headless programs.

## Overloads

This function has one overload.

## Parameters

| Parameter | Meaning |
|---|---|
| `title` | The window title. Empty uses the studio key. |
| `width`, `height` | The window size. |
| `launch` | Open the window. When false, the call only renders and checks. |
| `chrome` | `Sidebar` (default) or `None`. |

## Returns

`true` when the window opened, or when `launch` is false and the studio rendered. Otherwise `false`.

## Errors

A studio with no source is refused. A board that fails its checks is refused too.

## Practical Examples

### Example 1

Example 1 checks a studio without opening a window. The call succeeds.

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

### Example 2

Example 2 checks a studio with no source. The call is refused.

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

### Example 3

Example 3 checks a filtered studio. The filter applies and the call succeeds.

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

## See Also

- [`export_html`](export_html.md) -- writes the same document to a file.
- [`start_serving`](start_serving.md) -- serves the page over HTTP.
