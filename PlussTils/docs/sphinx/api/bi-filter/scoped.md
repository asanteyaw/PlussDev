# `pluss::bi::Filter::scoped`

[Back to PlussBI API Reference](../bi.md)

```cpp
#include <pluss/bi/specs.h>

Filter scoped(std::string_view value) const;
```

`scoped` returns a copy of the filter with its scope set to the given name. The original filter is
unchanged. A scoped filter applies only to queries that carry the same scope, so one session can hold
filters meant for different views without them interfering.

## Overloads

This function has one overload.

## Parameters

| Parameter | Meaning |
|---|---|
| `value` | The scope name to attach. |

## Returns

A new `Filter` whose `scope` holds the name.

## Errors

A scoped filter on a feature the table lacks is refused when the query carries the same scope. It is ignored when the query carries a different scope or none.

## Practical Examples

### Example 1

Example 1 applies a scoped filter to an overview that has no scope. The filter is ignored, so all six rows remain.

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

### Example 2

Example 2 shows the scope rule for a missing feature. A query with a different scope ignores the filter; a query with the same scope refuses it.

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

### Example 3

Example 3 shows that `scoped` returns a copy and leaves the original unscoped.

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

## See Also

- [`Filter`](../bi-filter.md) -- the `scope` field.
- [`Session`](../bi-session.md) -- stores the filters a studio applies.
