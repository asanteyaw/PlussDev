# `pluss::bi::FilterOp`

[Back to PlussBI API Reference](../bi.md)

`FilterOp` names the comparison a filter performs. Six operators compare one value:
`Eq`, `Ne`, `Gt`, `Ge`, `Lt`, and `Le`. `In` tests membership in a set of values, and
`Between` tests whether a value falls in an inclusive range. Each operator expects a
particular payload: the six comparisons take a scalar, `In` takes a set, and `Between`
takes a range. A payload that does not match the operator is refused when the studio
renders.

The enumerator values are fixed and form the stable identity of each operator. They
are declared in the order shown, starting at zero.

## Values

| Value | Meaning |
|---|---|
| `Eq (0)` | Equal to a scalar. |
| `Ne (1)` | Not equal to a scalar. |
| `Gt (2)` | Strictly greater than a scalar. |
| `Ge (3)` | Greater than or equal to a scalar. |
| `Lt (4)` | Strictly less than a scalar. |
| `Le (5)` | Less than or equal to a scalar. |
| `In (6)` | Equal to any member of a set. |
| `Between (7)` | Within an inclusive range. |

## Constraints

A comparison with a set or range payload, or `In` or `Between` with a scalar payload, is refused when the studio renders.

## Practical Examples

### Example 1

Example 1 prints each enumerator's numeric value. The values are 0 through 7 in declaration order.

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

### Example 2

Example 2 applies the six comparisons to the amount column. The counts are for the six sample rows with amounts 10 through 60, compared against 30.

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

### Example 3

Example 3 applies `In` with a set and `Between` with a range. Both ends of `Between` are included, so the range 20 to 40 keeps the rows at 20, 30, and 40.

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

## See Also

- [`Filter`](../bi-filter.md) -- the value that carries an operator.
- [`in`](in.md) -- the factory for `In`.
- [`between`](between.md) -- the factory for `Between`.
