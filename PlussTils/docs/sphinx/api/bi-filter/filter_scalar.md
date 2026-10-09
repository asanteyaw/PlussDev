# `pluss::bi::FilterScalar`

[Back to PlussBI API Reference](../bi.md)

```cpp
#include <pluss/bi/specs.h>

using FilterScalar = std::variant<double, std::string, pluss::Timeline, pluss::Duration>;
```

`FilterScalar` holds one bound of a filter. It is a variant of four kinds: a number, a text value, a
timeline instant, and a duration. The kind you store must match the column the filter tests: numbers
for numeric columns, text or ISO-form text for text and timeline columns, and durations for duration
columns. A text payload is read as an instant on a timeline column, but a number is never read as a
duration. Check the alternative with `std::holds_alternative` before you read it with `std::get`.

## Parameters

| Alternative | Used for |
|---|---|
| `double` | numeric bounds and numeric equality |
| `std::string` | text values, and ISO-form instants on timeline columns |
| `pluss::Timeline` | a single instant on a timeline column |
| `pluss::Duration` | a single length of time on a duration column |

## Returns

Not a function. `FilterScalar` is a type alias for a variant.

## Errors

A bound whose kind does not match the column is refused when the studio renders. Reading an alternative that is not held throws.

## Practical Examples

### Example 1

Example 1 builds one value of each alternative and checks that each one holds the kind it was built with.

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
  std::cout << std::boolalpha;
  // FilterScalar holds one bound, which can be a number, text, a timeline, or a duration.
  bi::FilterScalar number = 2.5;
  bi::FilterScalar text = std::string("north");
  bi::FilterScalar when = pluss::make_timeline({"2024-03-15"});
  bi::FilterScalar span = days(3);
  std::cout << "number: " << std::holds_alternative<double>(number) << '\n';
  std::cout << "text: " << std::holds_alternative<std::string>(text) << '\n';
  std::cout << "timeline: " << std::holds_alternative<pluss::Timeline>(when) << '\n';
  std::cout << "duration: " << std::holds_alternative<pluss::Duration>(span) << '\n';
  return 0;
}
```

Output

```text
number: true
text: true
timeline: true
duration: true
```

### Example 2

Example 2 shows that the kind must match the column. A text bound on a numeric column is refused, while the same column accepts a numeric bound.

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
  // The bound's type must match the column. A text bound on a numeric column is refused.
  bi::Filter f{.key = "amount", .feature = "amount", .op = bi::FilterOp::Eq, .payload = bi::FilterScalar{std::string("30")}};
  std::cout << "text bound on a numeric column refused: " << throws([&] { (void)visible_rows({f}); }) << '\n';
  std::cout << "numeric bound on the same column: " << visible_rows({bi::Filter::eq("amount", 30.0)}) << '\n';
  return 0;
}
```

Output

```text
text bound on a numeric column refused: 1
numeric bound on the same column: 1
```

### Example 3

Example 3 reads a stored number back with `std::get` after checking its alternative. Reading the wrong alternative throws.

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
  // Read a stored bound back with std::get, after checking its alternative.
  bi::FilterScalar bound = 40.0;
  if (std::holds_alternative<double>(bound)) {
    std::cout << "bound value: " << std::get<double>(bound) << '\n';
  }
  std::cout << "wrong type read throws: " << throws([&] { (void)std::get<std::string>(bound); }) << '\n';
  return 0;
}
```

Output

```text
bound value: 40
wrong type read throws: 1
```

## See Also

- [`Filter`](../bi-filter.md) -- the struct whose payload holds this type.
- [`FilterRange`](filter_range.md) -- two scalars forming a range.
- [`FilterSet`](filter_set.md) -- several members forming a set.
