# `pluss::bi::Filter::le`

[Back to PlussBI API Reference](../bi.md)

```cpp
#include <pluss/bi/specs.h>

static Filter le(std::string_view feature, double value);
static Filter le(std::string_view feature, const pluss::Duration& value);
```

`le` builds a filter that keeps rows less than or equal to the bound. The row equal to the bound is included. It sets the operator to `Le`
and stores the bound as the payload. The key defaults to the feature name, so a second
`le` on the same feature replaces the first in a session. The factory has no timeline overload.

## Overloads

| Overload | Bound type | Column kind |
|---|---|---|
| `le(feature, double)` | number | numeric |
| `le(feature, const pluss::Duration&)` | duration | duration |

## Parameters

| Parameter | Meaning |
|---|---|
| `feature` | The column name to test. |
| `value` | The bound: a number for a numeric column, or a duration for a duration column. |

## Returns

A filter with the `Le` operator and the given bound.

## Errors

A payload whose kind does not match the column is refused when the studio renders.

## Practical Examples

### Example 1

Example 1 keeps the rows at or below 30. The row at exactly 30 is included, which gives three rows.

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
  // le keeps rows at or below the bound. A row equal to the bound is included.
  std::cout << "amount at most 30: " << visible_rows({bi::Filter::le("amount", 30.0)}) << '\n';
  return 0;
}
```

Output

```text
amount at most 30: 3
```

### Example 2

Example 2 shows the edges: a bound at the smallest amount keeps one row, and a bound above every amount keeps all six.

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
  std::cout << "amount at most 10: " << visible_rows({bi::Filter::le("amount", 10.0)}) << '\n';
  std::cout << "amount at most 70: " << visible_rows({bi::Filter::le("amount", 70.0)}) << '\n';
  return 0;
}
```

Output

```text
amount at most 10: 1
amount at most 70: 6
```

### Example 3

Example 3 shows that a numeric bound on a text column is refused.

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
  std::cout << "numeric bound on text refused: "
            << throws([] { (void)visible_rows({bi::Filter::le("region", 30.0)}); }) << '\n';
  return 0;
}
```

Output

```text
numeric bound on text refused: 1
```

### Example 4

Example 4 keeps the leads at or below three days, which gives three rows. A duration bound is compared with every row of a duration column, so one bound selects by length of time.

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
  // le with a Duration compares each lead against the bound.
  std::cout << "lead le 3 days: " << visible_rows({bi::Filter::le("lead", days(3))}) << '\n';
  return 0;
}
```

Output

```text
lead le 3 days: 3
```

### Example 5

Example 5 shows the edges: a bound of zero days keeps none, and a bound of nine days keeps every row.

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
  std::cout << "lead le 0 days: " << visible_rows({bi::Filter::le("lead", days(0))}) << '\n';
  std::cout << "lead le 9 days: " << visible_rows({bi::Filter::le("lead", days(9))}) << '\n';
  return 0;
}
```

Output

```text
lead le 0 days: 0
lead le 9 days: 6
```

### Example 6

Example 6 shows that a bare number is refused as a duration bound.

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
  bi::Filter f{.key = "lead", .feature = "lead", .op = bi::FilterOp::Le, .payload = bi::FilterScalar{3.0}};
  std::cout << "bare number on a duration refused: " << throws([&] { (void)visible_rows({f}); }) << '\n';
  return 0;
}
```

Output

```text
bare number on a duration refused: 1
```

## See Also

- [`lt`](lt.md) -- the exclusive counterpart.
- [`Filter`](../bi-filter.md) -- the value the factory returns.
