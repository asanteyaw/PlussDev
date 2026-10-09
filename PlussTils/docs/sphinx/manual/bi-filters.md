# PlussBI: Filters

```{raw} html
<meta name="pluss-doc-kind" content="guide">
```

[Back to PlussTils](../index.md) · [PlussBI API Reference](../api/bi.md) · [PlussBI: A Guide](bi.md)

A filter says which rows a view should keep. It names a feature, an operator, and a payload, and the session stores it. Use a filter when a reader narrows the data; the factories below cover numbers, text, timelines, durations, sets, ranges, and scoped filters, with one example for each.

## The mental model

A filter is three things: a feature (the column), an operator (what to compare), and a payload (the value). The column's kind decides which payloads are valid. A number compares with a number column, a timeline compares with a timeline column, and a duration compares with a duration column. A payload of the wrong kind is refused when the studio renders, not when you build the filter.

Two rules hold for every filter. A filter on a feature the table does not have leaves the rows alone, unless the filter is scoped to a query that has that feature. And bounds are read as given for numbers, but timeline and duration bounds are put in order first.

## Worked tour

Every example is a complete program, and the output under it is copied from that program's run.

## Example 1: Equality on a number

The equality factory keeps rows whose feature equals the payload. This example compares the amount column with 30 and with 35.

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

Exactly one row has an amount of 30, and none has 35. Equality on a number is an exact match.

## Example 2: Equality on text

Text equality works the same way. This example keeps the rows whose region is north, then rows whose region is west.

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

Three rows are north. No row is west, so the second filter keeps none.

## Example 3: A feature the table lacks is ignored

A filter on a feature that is not in the table does nothing, so the rows stay. This example filters on an unknown feature.

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

All six rows remain. The filter is ignored, not refused, because the table simply has no such column.

## Example 4: Equality on a timeline

A timeline filter compares instants. This example keeps the row placed on March 15, 2024, and then tests a day that has no row.

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

The March row matches, and the next day does not.

## Example 5: A timeline payload can be ISO text

The payload can be written as an ISO date string, which is often easier to read than a timeline value. This example uses text.

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

The ISO text matches the row.

## Example 6: A number is refused as a timeline payload

A number is not a temporal bound. This example passes one to a timeline filter, and the studio refuses it when it renders.

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

The refusal is reported, so a number cannot silently mean a date.

## Example 7: Equality on a duration

A duration filter compares spans of time, such as days. This example keeps the row whose lead is exactly three days.

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

One row has a lead of three days.

## Example 8: A number is refused as a duration payload

A bare number has no unit, so it is not a duration. This example passes one, and the studio refuses it.

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

The refusal is reported.

## Example 9: Inequality on a number

Not-equal keeps every row except those that match. This example excludes amount 30.

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
  // ne keeps every row whose value differs from the number.
  std::cout << "amount not 30: " << visible_rows({bi::Filter::ne("amount", 30.0)}) << '\n';
  return 0;
}
```

Output

```text
amount not 30: 5
```

Five rows remain, because exactly one row had an amount of 30.

## Example 10: Inequality on text

Not-equal works on text too. This example excludes north.

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
  std::cout << "region not north: " << visible_rows({bi::Filter::ne("region", std::string_view("north"))}) << '\n';
  return 0;
}
```

Output

```text
region not north: 3
```

Three rows remain, the ones that are not north.

## Example 11: Inequality against an absent value

A not-equal filter keeps rows that do not match, and a value that no row has excludes nothing. This example excludes 35, which no row has.

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
  // A value that occurs nowhere excludes nothing.
  std::cout << "amount not 35: " << visible_rows({bi::Filter::ne("amount", 35.0)}) << '\n';
  return 0;
}
```

Output

```text
amount not 35: 6
```

All six rows remain.

## Example 12: Inequality on a duration

On a duration, not-equal removes the exact span and keeps the rest. This example excludes a lead of three days.

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
  std::cout << "lead not 3 days: " << visible_rows({bi::Filter::ne("lead", days(3))}) << '\n';
  return 0;
}
```

Output

```text
lead not 3 days: 5
```

Five rows remain, so only the three-day row was removed.

## Example 13: Greater than on a number

Greater-than keeps rows strictly above the payload. This example keeps amounts above 30.

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
  // gt keeps rows strictly above the bound. A row equal to the bound is excluded.
  std::cout << "amount above 30: " << visible_rows({bi::Filter::gt("amount", 30.0)}) << '\n';
  return 0;
}
```

Output

```text
amount above 30: 3
```

Three rows are above 30. The row at exactly 30 is not included.

## Example 14: Greater than on text

Text comparisons are lexicographic: they follow alphabetical order. This example keeps regions after north.

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
  // The gt factory accepts only numbers and durations. A text ordering is built directly as a Filter.
  bi::Filter after_north{.key = "after_north", .feature = "region", .op = bi::FilterOp::Gt,
                         .payload = bi::FilterScalar{std::string("north")}};
  std::cout << "region after north: " << visible_rows({after_north}) << '\n';
  return 0;
}
```

Output

```text
region after north: 2
```

Two rows are after north in the alphabet, the south rows. East sorts before north, so it is not counted.

## Example 15: A wrong payload is refused for greater-than

A number bound on a text column does not compare, so the studio refuses it when it renders.

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
  // A numeric bound on a text feature is refused when the studio renders.
  std::cout << "numeric bound on text refused: "
            << throws([] { (void)visible_rows({bi::Filter::gt("region", 30.0)}); }) << '\n';
  return 0;
}
```

Output

```text
numeric bound on text refused: 1
```

The refusal is reported.

## Example 16: Greater than on a duration

On durations, greater-than compares spans of time. This example keeps leads above three days.

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
  // gt with a Duration compares each lead against the bound.
  std::cout << "lead gt 3 days: " << visible_rows({bi::Filter::gt("lead", days(3))}) << '\n';
  return 0;
}
```

Output

```text
lead gt 3 days: 3
```

Three rows have a lead longer than three days.

## Example 17: Greater than or equal on a number

Greater-than-or-equal includes the boundary value. This example keeps amounts of at least 30.

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
  // ge keeps rows at or above the bound. A row equal to the bound is included.
  std::cout << "amount at least 30: " << visible_rows({bi::Filter::ge("amount", 30.0)}) << '\n';
  return 0;
}
```

Output

```text
amount at least 30: 4
```

Four rows qualify: the row at exactly 30 and the three above it.

## Example 18: Greater than or equal around a boundary

This example moves the boundary above and below the largest amount, to show what counts as at least.

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
  std::cout << "amount at least 60: " << visible_rows({bi::Filter::ge("amount", 60.0)}) << '\n';
  std::cout << "amount at least 70: " << visible_rows({bi::Filter::ge("amount", 70.0)}) << '\n';
  return 0;
}
```

Output

```text
amount at least 60: 1
amount at least 70: 0
```

One row has an amount of at least 60, and none has at least 70.

## Example 19: Less than on a number

Less-than keeps rows strictly below the payload. This example keeps amounts below 30.

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
  // lt keeps rows strictly below the bound. A row equal to the bound is excluded.
  std::cout << "amount below 30: " << visible_rows({bi::Filter::lt("amount", 30.0)}) << '\n';
  return 0;
}
```

Output

```text
amount below 30: 2
```

Two rows are below 30.

## Example 20: Less than or equal on a number

Less-than-or-equal includes the boundary. This example keeps amounts of at most 30.

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

Three rows qualify, the row at exactly 30 and the two below it.

## Example 21: Between is inclusive

Between keeps rows from the first bound to the second, and both bounds count. This example uses 20 and 40.

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

Three rows fall between 20 and 40, the 20, the 30, and the 40 rows.

## Example 22: Numeric bounds are not reordered

A numeric range reads its bounds exactly as given. This example passes 40 first and 20 second.

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

Zero rows are kept, because a range from 40 down to 20 holds nothing. This is the one place where a reversed range selects nothing.

## Example 23: A range outside the data is empty

A range that holds no value keeps no rows. This example uses 100 and 200.

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

No row falls in that range, so zero rows are kept.

## Example 24: Between on a timeline

Between on a timeline keeps dates from the first bound to the second, both included. This example uses February 1 and April 30.

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

Three rows fall in that window.

## Example 25: Timeline bounds are put in order

Unlike numbers, timeline bounds are ordered before they are used. This example gives April 30 first and February 1 second.

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

The same three rows are kept. Swapping timeline bounds does not change the result.

## Example 26: Between on a duration

A duration window works like a time window but uses spans. This example keeps leads from two to four days.

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
  // between_duration keeps leads from the first bound to the second, both included.
  std::cout << "2 to 4 days: " << visible_rows({bi::Filter::between_duration("lead", days(2), days(4))}) << '\n';
  return 0;
}
```

Output

```text
2 to 4 days: 3
```

Three rows fall between two and four days, both included.

## Example 27: Duration bounds are put in order

Duration bounds are ordered too. This example gives four days first and two days second.

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
  // Unlike numeric ranges, duration bounds are swapped when they are given in reverse.
  std::cout << "4 to 2 days: " << visible_rows({bi::Filter::between_duration("lead", days(4), days(2))}) << '\n';
  return 0;
}
```

Output

```text
4 to 2 days: 3
```

The same three rows are kept, as with timelines.

## Example 28: Membership in a set of numbers

The in factory keeps rows whose value is one of a list. This example keeps amounts of 10 or 60.

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
  // in keeps rows whose value is any member of the set.
  std::cout << "amount in {10, 60}: " << visible_rows({bi::Filter::in("amount", std::vector<double>{10.0, 60.0})}) << '\n';
  return 0;
}
```

Output

```text
amount in {10, 60}: 2
```

Two rows match, the ones with amounts of 10 and 60.

## Example 29: Membership in a set of text

The same works for text. This example keeps regions that are north or east.

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
  std::cout << "region in {north, east}: "
            << visible_rows({bi::Filter::in("region", std::vector<std::string>{"north", "east"})}) << '\n';
  return 0;
}
```

Output

```text
region in {north, east}: 4
```

Four rows match, the three north rows and the east row.

## Example 30: An empty set is refused

An empty set has no members to match. This example passes an empty list, and the studio refuses it when it renders.

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
  // An empty set has no members to match, and the studio refuses it when it renders.
  std::cout << "empty set refused: "
            << throws([] { (void)visible_rows({bi::Filter::in("amount", std::vector<double>{})}); }) << '\n';
  return 0;
}
```

Output

```text
empty set refused: 1
```

The refusal is reported, so an accidental empty list does not silently hide every row.

## Example 31: A scoped filter is ignored by a query with another scope

A scoped filter applies only to views whose query carries the same scope. This example checks a view that has no matching scope.

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
  // A scoped filter applies only when the query carries the same scope. The overview has none.
  std::cout << "scoped filter with no query scope: "
            << visible_rows({bi::Filter::eq("amount", 30.0).scoped("overview")}) << '\n';
  return 0;
}
```

Output

```text
scoped filter with no query scope: 6
```

All six rows remain, because the filter does not apply to this query.

## Example 32: A scoped filter on a missing feature

A scoped filter on a feature the table lacks is ignored when the scope differs, and refused when the scope matches. The author meant it for that query, so the refusal points out the mistake.

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

// The same missing-feature filter, on a view whose query carries the matching scope.
std::int64_t scoped_view_rows(const std::string& scope_name) {
  const auto nexus = make_nexus();
  bi::Studio studio("sales_studio");
  studio.source(nexus);
  studio.board("overview").grid(1, 1);
  studio.board("overview").view(bi::table("rows").query(bi::from("sales").scope_as(scope_name)).place(0, 0, 1, 1));
  studio.set_filter(bi::Filter::eq("weight", 1.0).scoped("overview"));
  return studio.render().boards.front().tiles.front().table->height();
}

int main() {
  // A scoped filter on a feature the table lacks is ignored when the query has a different scope.
  std::cout << "other scope ignores it: " << scoped_view_rows("other") << '\n';
  // It is refused when the query carries the same scope, because the author meant it to apply.
  std::cout << "matching scope refused: " << throws([] { (void)scoped_view_rows("overview"); }) << '\n';
  return 0;
}
```

Output

```text
other scope ignores it: 6
matching scope refused: 1
```

A different scope ignores the filter and keeps all six rows. The matching scope refuses it.

## Example 33: Scoping returns a copy

The scoped factory returns a new filter and leaves the original alone. This example scopes a filter and checks both.

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
  std::cout << std::boolalpha;
  const bi::Filter base = bi::Filter::eq("amount", 30.0);
  const bi::Filter scoped = base.scoped("overview");
  std::cout << "base unscoped: " << !base.scope.has_value() << '\n';
  std::cout << "copy scoped: " << scoped.scope.has_value() << '\n';
  return 0;
}
```

Output

```text
base unscoped: true
copy scoped: true
```

The original is unscoped, and the copy is scoped.

## Example 34: The operator enumeration

Each operator has a fixed number. This example prints the first eight values, so you can read a stored operator back as a number.

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
  // The enumerators have fixed numeric values, in declaration order.
  std::cout << "Eq=" << static_cast<int>(bi::FilterOp::Eq)
            << " Ne=" << static_cast<int>(bi::FilterOp::Ne)
            << " Gt=" << static_cast<int>(bi::FilterOp::Gt)
            << " Ge=" << static_cast<int>(bi::FilterOp::Ge) << '\n';
  std::cout << "Lt=" << static_cast<int>(bi::FilterOp::Lt)
            << " Le=" << static_cast<int>(bi::FilterOp::Le)
            << " In=" << static_cast<int>(bi::FilterOp::In)
            << " Between=" << static_cast<int>(bi::FilterOp::Between) << '\n';
  return 0;
}
```

Output

```text
Eq=0 Ne=1 Gt=2 Ge=3
Lt=4 Le=5 In=6 Between=7
```

Equal is zero, not equal is one, and the others follow in order through between.

## Example 35: The comparison operators in action

The six comparison operators can be used directly on a filter. This example applies each one to the amount column with the same payload, 30.

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
  // Each comparison operator selects rows against the amount column.
  std::cout << "Eq 30: " << visible_rows({bi::Filter{.key = "a", .feature = "amount", .op = bi::FilterOp::Eq, .payload = bi::FilterScalar{30.0}}}) << '\n';
  std::cout << "Ne 30: " << visible_rows({bi::Filter{.key = "a", .feature = "amount", .op = bi::FilterOp::Ne, .payload = bi::FilterScalar{30.0}}}) << '\n';
  std::cout << "Gt 30: " << visible_rows({bi::Filter{.key = "a", .feature = "amount", .op = bi::FilterOp::Gt, .payload = bi::FilterScalar{30.0}}}) << '\n';
  std::cout << "Ge 30: " << visible_rows({bi::Filter{.key = "a", .feature = "amount", .op = bi::FilterOp::Ge, .payload = bi::FilterScalar{30.0}}}) << '\n';
  std::cout << "Lt 30: " << visible_rows({bi::Filter{.key = "a", .feature = "amount", .op = bi::FilterOp::Lt, .payload = bi::FilterScalar{30.0}}}) << '\n';
  std::cout << "Le 30: " << visible_rows({bi::Filter{.key = "a", .feature = "amount", .op = bi::FilterOp::Le, .payload = bi::FilterScalar{30.0}}}) << '\n';
  return 0;
}
```

Output

```text
Eq 30: 1
Ne 30: 5
Gt 30: 3
Ge 30: 4
Lt 30: 2
Le 30: 3
```

Each operator keeps the rows you would expect for the value thirty: equal one, not equal five, greater three, greater or equal four, less two, and less or equal three.

## Example 36: The set and range operators

The in operator and the between operator take a set or a pair of bounds. This example applies each one.

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
  // In takes a set of values; Between takes a range whose ends are both included.
  const bi::Filter in = bi::Filter::in("amount", std::vector<double>{10.0, 60.0});
  const bi::Filter between = bi::Filter::between("amount", 20.0, 40.0);
  std::cout << "In {10, 60}: " << visible_rows({in}) << '\n';
  std::cout << "Between 20 and 40: " << visible_rows({between}) << '\n';
  return 0;
}
```

Output

```text
In {10, 60}: 2
Between 20 and 40: 3
```

Two rows match the set, and three rows fall in the range from 20 to 40.

## Example 37: A filter scalar can hold four kinds

A payload is one scalar: a number, text, a timeline value, or a duration. This example builds one of each and reports which kind it holds.

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

Each of the four kinds is recognised, so the payload's type decides how it is compared.

## Example 38: The payload type selects the column kind

Because the payload's type decides how it compares, a text bound on a numeric column is refused, and a numeric bound on the same column is accepted. This example checks both.

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

The text bound is refused, and the numeric bound works on the same column.

## Example 39: A numeric set of members

A filter set holds a list of members of one kind. This example builds a numeric set and counts the members and the matching rows.

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
  // FilterSet holds the members of an In filter. A numeric set holds doubles.
  bi::FilterSet members = std::vector<double>{10.0, 60.0};
  std::cout << "numeric set: " << std::holds_alternative<std::vector<double>>(members) << '\n';
  std::cout << "members: " << std::get<std::vector<double>>(members).size() << '\n';
  std::cout << "rows matching the set: " << visible_rows({bi::Filter{.key = "a", .feature = "amount",
      .op = bi::FilterOp::In, .payload = members}}) << '\n';
  return 0;
}
```

Output

```text
numeric set: 1
members: 2
rows matching the set: 2
```

The set has two members, and two rows match it.

## Example 40: A text set of members

A filter set can also hold text members. This example builds a set of two regions.

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
  bi::FilterSet members = std::vector<std::string>{"north", "east"};
  std::cout << "text set: " << std::holds_alternative<std::vector<std::string>>(members) << '\n';
  std::cout << "rows matching the set: " << visible_rows({bi::Filter{.key = "r", .feature = "region",
      .op = bi::FilterOp::In, .payload = members}}) << '\n';
  return 0;
}
```

Output

```text
text set: 1
rows matching the set: 4
```

Four rows match the set of text members.

## Example 41: Numeric bounds

A filter range is a pair of bounds, low and high. This example builds a numeric range and counts the rows between the bounds.

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
  // FilterRange holds the low and high bounds of a Between filter.
  bi::FilterRange range{.low = bi::FilterScalar{20.0}, .high = bi::FilterScalar{40.0}};
  bi::Filter f{.key = "a", .feature = "amount", .op = bi::FilterOp::Between, .payload = range};
  std::cout << "rows between the bounds: " << visible_rows({f}) << '\n';
  return 0;
}
```

Output

```text
rows between the bounds: 3
```

Three rows fall between the bounds.

## Example 42: Temporal bounds from ISO dates

A range can also be built from timeline bounds written as ISO dates. This example uses two dates.

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
  bi::FilterRange range{.low = bi::FilterScalar{std::string("2024-02-01")}, .high = bi::FilterScalar{std::string("2024-04-30")}};
  bi::Filter f{.key = "p", .feature = "placed", .op = bi::FilterOp::Between, .payload = range};
  std::cout << "rows between ISO dates: " << visible_rows({f}) << '\n';
  return 0;
}
```

Output

```text
rows between ISO dates: 3
```

Three rows fall between the two dates.

## Example 43: A range on a text column is refused

A range needs an ordered column of numbers or timelines. This example applies a text range to the region column, and the studio refuses it when it renders.

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
  // A range on a text column is not supported, and the studio refuses it when it renders.
  bi::FilterRange range{.low = bi::FilterScalar{std::string("a")}, .high = bi::FilterScalar{std::string("z")}};
  bi::Filter f{.key = "r", .feature = "region", .op = bi::FilterOp::Between, .payload = range};
  std::cout << "range on a text column refused: " << throws([&] { (void)visible_rows({f}); }) << '\n';
  return 0;
}
```

Output

```text
range on a text column refused: 1
```

The refusal is reported.

## Example 44: A filter's fields

A filter has a key, a feature, an operator, and a payload, and it is enabled by default and unscoped. This example reads each field back from a filter it built.

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
  std::cout << std::boolalpha;
  // A filter names a key and a feature, and it carries an operator and a payload.
  const bi::Filter f = bi::Filter::eq("region", std::string("north"));
  std::cout << "key: " << f.key << '\n';
  std::cout << "feature: " << f.feature << '\n';
  std::cout << "op is Eq: " << (f.op == bi::FilterOp::Eq) << '\n';
  std::cout << "enabled by default: " << f.enabled << '\n';
  std::cout << "has scope: " << f.scope.has_value() << '\n';
  return 0;
}
```

Output

```text
key: region
feature: region
op is Eq: true
enabled by default: true
has scope: false
```

The operator reads back as equal, the filter is enabled, and it has no scope.

## Example 45: A scoped copy keeps the feature

The scoped copy changes only its scope. This example checks the scope of the base and the copy, and that the feature stayed the same.

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
  std::cout << std::boolalpha;
  // scoped returns a new filter with its scope set. The original is unchanged.
  const bi::Filter base = bi::Filter::eq("amount", 30.0);
  const bi::Filter scoped = base.scoped("overview");
  std::cout << "base has scope: " << base.scope.has_value() << '\n';
  std::cout << "copy scope: " << *scoped.scope << '\n';
  std::cout << "copy keeps the feature: " << (scoped.feature == "amount") << '\n';
  return 0;
}
```

Output

```text
base has scope: false
copy scope: overview
copy keeps the feature: true
```

The base has no scope and the copy's scope is overview. The feature is unchanged.

## Example 46: A disabled filter is ignored

A filter can be stored disabled, and then it does not narrow the rows, but it stays in the session. This example applies the same filter enabled and disabled.

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
  std::cout << std::boolalpha;
  // A disabled filter is kept in the session but has no effect on the rows shown.
  bi::Filter f = bi::Filter::eq("region", std::string("north"));
  std::cout << "enabled rows: " << visible_rows({f}) << '\n';
  f.enabled = false;
  std::cout << "disabled rows: " << visible_rows({f}) << '\n';
  return 0;
}
```

Output

```text
enabled rows: 3
disabled rows: 6
```

Enabled, the filter keeps three rows. Disabled, all six rows remain.

## Decisions

| You want | Use | Notes |
|---|---|---|
| Keep rows equal to one value | `Filter::eq` | Use the factory that matches the column kind: number, text, timeline, or duration. |
| Keep a numeric or time window | `between`, `between_time`, or `between_duration` | Numeric bounds are read as given; timeline and duration bounds are put in order. |
| Keep one of a list | `Filter::in` | An empty list is refused. |
| Limit a filter to one query | `Filter::scoped` | The scope decides whether a missing feature is ignored or refused. |
| Turn a filter off without removing it | `enabled` set to false | The filter stays in the session and stops narrowing. |

## Pitfalls

**Expecting a reversed numeric range to behave like a reversed time window.** between_02 keeps zero rows for 40 to 20, while between_time_02 keeps three rows for April 30 to February 1.

**Expecting a text bound to compare as a number.** filter_scalar_02 shows that a text bound on a numeric column is refused, and gt_02 shows that text order is alphabetical.

**Expecting a filter on a missing feature to fail.** eq_03 keeps all six rows, because the feature is missing from the table.

## Where next

- [Sessions and parameters](bi-sessions.md) -- where filters are stored.
- [Views](bi-views.md) -- the views a filter narrows.
- [PlussBI API Reference](../api/bi-filter.md) -- every filter factory, with its refusals.
