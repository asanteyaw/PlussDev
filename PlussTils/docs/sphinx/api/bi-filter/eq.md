# `pluss::bi::Filter::eq`

[Back to PlussBI API Reference](../bi.md)

```cpp
#include <pluss/bi/specs.h>

static Filter eq(std::string_view feature, double value);
static Filter eq(std::string_view feature, std::string_view value);
static Filter eq(std::string_view feature, const pluss::Timeline& value);
static Filter eq(std::string_view feature, const pluss::Duration& value);
```

`eq` builds a filter that keeps rows whose feature equals the value. It sets the operator to `Eq`
and stores the value as the payload. The key defaults to the feature name, so a second `eq` on the
same feature replaces the first in a session.

Equality on numbers is exact. Equality on text compares the whole string. Equality on a timeline or a
duration uses the same rule, and the value's kind must match the column's kind. A feature that the table
does not have is ignored when the filter is unscoped, so an unknown name does not hide every row.

## Overloads

| Overload | Value type | Column kind |
|---|---|---|
| `eq(feature, double)` | number | numeric |
| `eq(feature, std::string_view)` | text | text |
| `eq(feature, const pluss::Timeline&)` | timeline | timeline |
| `eq(feature, const pluss::Duration&)` | duration | duration |

## Parameters

| Parameter | Meaning |
|---|---|
| `feature` | The column name to test. |
| `value` | The number, text, timeline, or duration to match exactly. |

## Returns

A filter with the `Eq` operator and the given payload.

## Errors

The factory does not check the feature. A payload whose kind does not match the column is refused when the studio renders.

## Practical Examples

### Example 1

Example 1 keeps the rows whose amount is exactly 30. Only one row has that amount, and an amount that appears nowhere keeps none.

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
  // eq on a numeric feature keeps rows whose value equals the number exactly.
  std::cout << "amount equals 30: " << visible_rows({bi::Filter::eq("amount", 30.0)}) << '\n';
  std::cout << "amount equals 35: " << visible_rows({bi::Filter::eq("amount", 35.0)}) << '\n';
  return 0;
}
```

Output

```text
amount equals 30: 1
amount equals 35: 0
```

### Example 2

Example 2 keeps the rows whose region is exactly north. Three rows match, and a region that appears nowhere keeps none.

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
  // eq on a text feature keeps rows whose text matches exactly.
  std::cout << "region equals north: " << visible_rows({bi::Filter::eq("region", std::string_view("north"))}) << '\n';
  std::cout << "region equals west: " << visible_rows({bi::Filter::eq("region", std::string_view("west"))}) << '\n';
  return 0;
}
```

Output

```text
region equals north: 3
region equals west: 0
```

### Example 3

Example 3 shows that a feature the table does not have is ignored when the filter is unscoped, so all six rows remain.

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
  // A session filter on a feature the table does not have is ignored when it is unscoped.
  std::cout << "unknown feature leaves rows: " << visible_rows({bi::Filter::eq("weight", 1.0)}) << '\n';
  return 0;
}
```

Output

```text
unknown feature leaves rows: 6
```

### Example 4

Example 4 keeps the row whose date is exactly March 15, 2024. A timeline bound is compared with every row of a timeline column, so one bound selects the rows equal to that instant. The next day matches no row.

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
  // eq with a Timeline keeps the row whose date is exactly that instant.
  std::cout << "placed on 2024-03-15: " << visible_rows({bi::Filter::eq("placed", pluss::make_timeline({"2024-03-15"}))}) << '\n';
  std::cout << "placed on 2024-03-16: " << visible_rows({bi::Filter::eq("placed", pluss::make_timeline({"2024-03-16"}))}) << '\n';
  return 0;
}
```

Output

```text
placed on 2024-03-15: 1
placed on 2024-03-16: 0
```

### Example 5

Example 5 builds the same timeline filter from an ISO text payload. A text bound in ISO form is read as the same instant on a timeline column.

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
  // A text payload in ISO form is read as the same instant on a timeline column.
  bi::Filter f{.key = "placed", .feature = "placed", .op = bi::FilterOp::Eq,
               .payload = bi::FilterScalar{std::string("2024-03-15")}};
  std::cout << "ISO text matches the row: " << visible_rows({f}) << '\n';
  return 0;
}
```

Output

```text
ISO text matches the row: 1
```

### Example 6

Example 6 shows that a number is refused as a timeline bound.

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
  // A number is not a temporal bound, so the studio refuses it when it renders.
  bi::Filter f{.key = "placed", .feature = "placed", .op = bi::FilterOp::Eq, .payload = bi::FilterScalar{2.0}};
  std::cout << "numeric bound on a timeline refused: " << throws([&] { (void)visible_rows({f}); }) << '\n';
  return 0;
}
```

Output

```text
numeric bound on a timeline refused: 1
```

### Example 7

Example 7 keeps the row whose lead is exactly three days. A duration bound is compared with every row of a duration column, so one bound selects by length of time.

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
  // eq with a Duration keeps the row whose lead is exactly that length.
  std::cout << "lead of 3 days: " << visible_rows({bi::Filter::eq("lead", days(3))}) << '\n';
  return 0;
}
```

Output

```text
lead of 3 days: 1
```

### Example 8

Example 8 shows that a length no lead equals keeps no row.

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
  std::cout << "lead of 7 days: " << visible_rows({bi::Filter::eq("lead", days(7))}) << '\n';
  return 0;
}
```

Output

```text
lead of 7 days: 0
```

### Example 9

Example 9 shows that a bare number is refused as a duration bound. A duration must carry its unit.

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
  // A bare number is not a Duration, so the studio refuses it when it renders.
  bi::Filter f{.key = "lead", .feature = "lead", .op = bi::FilterOp::Eq, .payload = bi::FilterScalar{3.0}};
  std::cout << "bare number on a duration refused: " << throws([&] { (void)visible_rows({f}); }) << '\n';
  return 0;
}
```

Output

```text
bare number on a duration refused: 1
```

## See Also

- [`ne`](ne.md) -- the negation.
- [`in`](in.md) -- equality against a set.
- [`between_time`](between_time.md) and [`between_duration`](between_duration.md) -- range forms for timelines and durations.
- [`Filter`](../bi-filter.md) -- the value the factory returns.
