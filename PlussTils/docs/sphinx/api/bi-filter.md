# `pluss::bi::Filter`

[Back to PlussBI API Reference](bi.md)

```cpp
#include <pluss/bi/specs.h>

struct Filter {
  std::string key;
  std::string feature;
  FilterOp op{FilterOp::Eq};
  std::variant<std::monostate, FilterScalar, FilterSet, FilterRange> payload;
  std::optional<std::string> scope;
  bool enabled{true};
};
```

A `Filter` is one narrowing choice. It names the column it tests (`feature`), the
comparison to make (`op`), the value or values to compare against (`payload`), and
two switches that control where it applies: `scope` and `enabled`. A session stores
filters by `key`, and a studio applies the stored filters to each view's rows when it
renders. The filter never changes the table itself; it only decides which rows a view
shows.

The `key` is the slot the filter occupies in a session. Two filters with the same key
cannot coexist in one session, so the key is how a later choice replaces an earlier
one. The `feature` is the column name, and it is tested against each view's data
when the studio renders.

`enabled` defaults to true. A filter with `enabled` set to false stays in the session
but has no effect on rows. `scope` is empty by default, which makes the filter apply
to any query that is evaluated with the session. Setting a scope restricts the filter
to queries that carry the same scope, and `scoped` is the convenient way to set it.

The static factories `eq`, `ne`, `gt`, `ge`, `lt`, `le`, `between`, and `in` build a
filter with the operator and payload already in place. Build the struct directly when
you need a combination the factories do not offer, such as a text ordering.

## Parameters

| Parameter | Meaning |
|---|---|
| (fields) | Set the fields directly, or use a static factory. The struct is an aggregate. |

## Returns

A `Filter` is a value. Copying it copies every field.

## Errors

Setting an unsupported payload for an operator is refused when the studio renders, not when the filter is built.

## Practical Examples

### Example 1

Example 1 builds a filter with `eq` and reads its fields. The default operator is `Eq`, the filter is enabled, and it has no scope.

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

### Example 2

Example 2 shows that `scoped` returns a new filter and leaves the original unscoped. The copy keeps the feature.

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

### Example 3

Example 3 shows that `enabled = false` removes the filter's effect. The view shows three rows with the filter and all six without it.

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

## See Also

- [`FilterOp`](bi-filter/filter_op.md) -- the comparison a filter performs.
- [`Session`](bi-session.md) -- stores filters by key.
- [`Studio`](bi.md) -- applies the stored filters when it renders.
