# `pluss::bi::Filter::between`

[Back to PlussBI API Reference](../bi.md)

```cpp
#include <pluss/bi/specs.h>

static Filter between(std::string_view feature, double low, double high);
```

`between` builds a filter that keeps numeric rows within an inclusive range. Both bounds are included.
Numeric bounds are not reordered: when low is greater than high, no row can satisfy the range, and the
result is empty. Timeline and duration ranges behave differently; they are documented on
[`between_time`](between_time.md) and [`between_duration`](between_duration.md).

## Overloads

| Overload | Bound type | Column kind |
|---|---|---|
| `between(feature, double, double)` | number | numeric |
| `between_time(feature, Timeline, Timeline)` | timeline | timeline (see [`between_time`](between_time.md)) |
| `between_duration(feature, Duration, Duration)` | duration | duration (see [`between_duration`](between_duration.md)) |

## Parameters

| Parameter | Meaning |
|---|---|
| `feature` | The numeric column to test. |
| `low` | The lower bound, inclusive. |
| `high` | The upper bound, inclusive. |

## Returns

A filter with the `Between` operator and a numeric range payload.

## Errors

A range payload on a text column is refused when the studio renders. The factory does not check the column.

## Practical Examples

### Example 1

Example 1 keeps the rows from 20 through 40, both included, which gives three rows.

```cpp
#include <pluss/bi/bi.h>
#include <pluss/bundle/bundle.h>
#include <pluss/mic.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <variant>
#include <vector>

namespace bi = pluss::bi;

// Six sales rows: regions north, south, north, east, south, north; amounts 10 to 60.
pluss::bundle::Nexus make_nexus() {
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame sales({
      {"region", Slate(pluss::make_note({"north", "south", "north", "east", "south", "north"}))},
      {"amount", Slate(pluss::make_series(std::vector<double>{10, 20, 30, 40, 50, 60}, {6}, cfg))},
  });
  return pluss::bundle::Nexus({{"sales", sales}});
}

// The number of rows the overview table shows when the given filters are set on the session.
std::int64_t visible_rows(const std::vector<bi::Filter>& filters) {
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 1);
  studio.board("overview").view(bi::table("rows").query(bi::from("sales")).place(0, 0, 1, 1));
  for (const auto& filter : filters) studio.set_filter(filter);
  return studio.render().boards.front().tiles.front().table->height();
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
  // between keeps rows from low to high, and both ends are included.
  std::cout << "between 20 and 40: " << visible_rows({bi::Filter::between("amount", 20.0, 40.0)}) << '\n';
  return 0;
}
```

Output

```text
between 20 and 40: 3
```

### Example 2

Example 2 passes the bounds in reverse order. Numeric bounds are not swapped, so the range is empty and no rows remain.

```cpp
#include <pluss/bi/bi.h>
#include <pluss/bundle/bundle.h>
#include <pluss/mic.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <variant>
#include <vector>

namespace bi = pluss::bi;

// Six sales rows: regions north, south, north, east, south, north; amounts 10 to 60.
pluss::bundle::Nexus make_nexus() {
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame sales({
      {"region", Slate(pluss::make_note({"north", "south", "north", "east", "south", "north"}))},
      {"amount", Slate(pluss::make_series(std::vector<double>{10, 20, 30, 40, 50, 60}, {6}, cfg))},
  });
  return pluss::bundle::Nexus({{"sales", sales}});
}

// The number of rows the overview table shows when the given filters are set on the session.
std::int64_t visible_rows(const std::vector<bi::Filter>& filters) {
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 1);
  studio.board("overview").view(bi::table("rows").query(bi::from("sales")).place(0, 0, 1, 1));
  for (const auto& filter : filters) studio.set_filter(filter);
  return studio.render().boards.front().tiles.front().table->height();
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
  // For numeric features the bounds are not swapped. Reversed bounds select nothing.
  std::cout << "between 40 and 20: " << visible_rows({bi::Filter::between("amount", 40.0, 20.0)}) << '\n';
  return 0;
}
```

Output

```text
between 40 and 20: 0
```

### Example 3

Example 3 uses a range above every amount, so no rows remain.

```cpp
#include <pluss/bi/bi.h>
#include <pluss/bundle/bundle.h>
#include <pluss/mic.h>

#include <cstdint>
#include <iostream>
#include <string>
#include <variant>
#include <vector>

namespace bi = pluss::bi;

// Six sales rows: regions north, south, north, east, south, north; amounts 10 to 60.
pluss::bundle::Nexus make_nexus() {
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame sales({
      {"region", Slate(pluss::make_note({"north", "south", "north", "east", "south", "north"}))},
      {"amount", Slate(pluss::make_series(std::vector<double>{10, 20, 30, 40, 50, 60}, {6}, cfg))},
  });
  return pluss::bundle::Nexus({{"sales", sales}});
}

// The number of rows the overview table shows when the given filters are set on the session.
std::int64_t visible_rows(const std::vector<bi::Filter>& filters) {
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 1);
  studio.board("overview").view(bi::table("rows").query(bi::from("sales")).place(0, 0, 1, 1));
  for (const auto& filter : filters) studio.set_filter(filter);
  return studio.render().boards.front().tiles.front().table->height();
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
  std::cout << "between 100 and 200: " << visible_rows({bi::Filter::between("amount", 100.0, 200.0)}) << '\n';
  return 0;
}
```

Output

```text
between 100 and 200: 0
```

## See Also

- [`between_time`](between_time.md) and [`between_duration`](between_duration.md) -- the temporal and duration forms.
- [`in`](in.md) -- the set-membership alternative.
- [`FilterRange`](filter_range.md) -- the payload this factory builds.
