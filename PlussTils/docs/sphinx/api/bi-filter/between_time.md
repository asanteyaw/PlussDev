# `pluss::bi::Filter::between_time`

[Back to PlussBI API Reference](../bi.md)

```cpp
#include <pluss/bi/specs.h>

static Filter between_time(std::string_view feature,
                             const pluss::Timeline& low,
                             const pluss::Timeline& high);
```

`between_time` builds a filter that keeps timeline rows within an inclusive range. Both instants are
included. Unlike numeric ranges, timeline bounds are ordered for you: reversed bounds select the same
rows as ordered ones, as the second example shows. Each bound is a single instant, so the range is
inclusive at both ends.

## Overloads

| Overload | Bound type | Column kind |
|---|---|---|
| `between_time(feature, Timeline, Timeline)` | timeline | timeline |

## Parameters

| Parameter | Meaning |
|---|---|
| `feature` | The timeline column to test. |
| `low` | The earlier instant, inclusive. |
| `high` | The later instant, inclusive. |

## Returns

A filter with the `Between` operator and a timeline range payload.

## Errors

A range on a column that is not a timeline is refused when the studio renders.

## Practical Examples

### Example 1

Example 1 keeps the rows dated from February 1 through April 30, 2024. Three rows fall in that range: February 10, March 15, and April 20.

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

// Six sales rows. Regions: north, south, north, east, south, north. Amounts: 10 to 60.
// Dates: one per month from January to June 2024. Leads: 1 to 6 days.
pluss::bundle::Nexus make_nexus() {
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame sales({
      {"region", Slate(pluss::make_note({"north", "south", "north", "east", "south", "north"}))},
      {"amount", Slate(pluss::make_series(std::vector<double>{10, 20, 30, 40, 50, 60}, {6}, cfg))},
      {"placed", Slate(pluss::make_timeline({"2024-01-05", "2024-02-10", "2024-03-15", "2024-04-20", "2024-05-25", "2024-06-30"}))},
      {"lead", Slate(pluss::make_duration(std::vector<std::int64_t>{1, 2, 3, 4, 5, 6}, {6}, pluss::duration::Unit::Days))},
  });
  return pluss::bundle::Nexus({{"sales", sales}});
}

// The number of rows the overview table shows with the given filters on the session.
std::int64_t visible_rows(const std::vector<bi::Filter>& filters) {
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 1);
  studio.board("overview").view(bi::table("rows").query(bi::from("sales")).place(0, 0, 1, 1));
  for (const auto& filter : filters) studio.set_filter(filter);
  return studio.render().boards.front().tiles.front().table->height();
}

pluss::Duration days(std::int64_t n) {
  return pluss::make_duration(std::vector<std::int64_t>{n}, {1}, pluss::duration::Unit::Days);
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
  // between_time keeps dates from the first bound to the second, both included.
  std::cout << "February 1 to April 30: " << visible_rows({bi::Filter::between_time("placed",
      pluss::make_timeline({"2024-02-01"}), pluss::make_timeline({"2024-04-30"}))}) << '\n';
  return 0;
}
```

Output

```text
February 1 to April 30: 3
```

### Example 2

Example 2 passes the bounds in reverse. The result matches Example 1: the range selects the same rows whichever order the bounds are given.

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

// Six sales rows. Regions: north, south, north, east, south, north. Amounts: 10 to 60.
// Dates: one per month from January to June 2024. Leads: 1 to 6 days.
pluss::bundle::Nexus make_nexus() {
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame sales({
      {"region", Slate(pluss::make_note({"north", "south", "north", "east", "south", "north"}))},
      {"amount", Slate(pluss::make_series(std::vector<double>{10, 20, 30, 40, 50, 60}, {6}, cfg))},
      {"placed", Slate(pluss::make_timeline({"2024-01-05", "2024-02-10", "2024-03-15", "2024-04-20", "2024-05-25", "2024-06-30"}))},
      {"lead", Slate(pluss::make_duration(std::vector<std::int64_t>{1, 2, 3, 4, 5, 6}, {6}, pluss::duration::Unit::Days))},
  });
  return pluss::bundle::Nexus({{"sales", sales}});
}

// The number of rows the overview table shows with the given filters on the session.
std::int64_t visible_rows(const std::vector<bi::Filter>& filters) {
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 1);
  studio.board("overview").view(bi::table("rows").query(bi::from("sales")).place(0, 0, 1, 1));
  for (const auto& filter : filters) studio.set_filter(filter);
  return studio.render().boards.front().tiles.front().table->height();
}

pluss::Duration days(std::int64_t n) {
  return pluss::make_duration(std::vector<std::int64_t>{n}, {1}, pluss::duration::Unit::Days);
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
  // For timelines the bounds are not swapped, so reversed bounds select nothing.
  std::cout << "April 30 to February 1: " << visible_rows({bi::Filter::between_time("placed",
      pluss::make_timeline({"2024-04-30"}), pluss::make_timeline({"2024-02-01"}))}) << '\n';
  return 0;
}
```

Output

```text
April 30 to February 1: 3
```

### Example 3

Example 3 uses a range in 2025, after every date in the table, so no rows remain.

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

// Six sales rows. Regions: north, south, north, east, south, north. Amounts: 10 to 60.
// Dates: one per month from January to June 2024. Leads: 1 to 6 days.
pluss::bundle::Nexus make_nexus() {
  using pluss::table::DataFrame;
  using pluss::table::Slate;
  const auto cfg = pluss::Config{}.with_dtype(pluss::ScalarType::Float64).with_platform(pluss::pCPU);
  const DataFrame sales({
      {"region", Slate(pluss::make_note({"north", "south", "north", "east", "south", "north"}))},
      {"amount", Slate(pluss::make_series(std::vector<double>{10, 20, 30, 40, 50, 60}, {6}, cfg))},
      {"placed", Slate(pluss::make_timeline({"2024-01-05", "2024-02-10", "2024-03-15", "2024-04-20", "2024-05-25", "2024-06-30"}))},
      {"lead", Slate(pluss::make_duration(std::vector<std::int64_t>{1, 2, 3, 4, 5, 6}, {6}, pluss::duration::Unit::Days))},
  });
  return pluss::bundle::Nexus({{"sales", sales}});
}

// The number of rows the overview table shows with the given filters on the session.
std::int64_t visible_rows(const std::vector<bi::Filter>& filters) {
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 1);
  studio.board("overview").view(bi::table("rows").query(bi::from("sales")).place(0, 0, 1, 1));
  for (const auto& filter : filters) studio.set_filter(filter);
  return studio.render().boards.front().tiles.front().table->height();
}

pluss::Duration days(std::int64_t n) {
  return pluss::make_duration(std::vector<std::int64_t>{n}, {1}, pluss::duration::Unit::Days);
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
  std::cout << "all of 2025: " << visible_rows({bi::Filter::between_time("placed",
      pluss::make_timeline({"2025-01-01"}), pluss::make_timeline({"2025-12-31"}))}) << '\n';
  return 0;
}
```

Output

```text
all of 2025: 0
```

## See Also

- [`between`](between.md) -- the numeric form, which does not reorder bounds.
- [`eq`](eq.md) -- the single-instant form.
