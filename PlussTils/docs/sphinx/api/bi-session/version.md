# `pluss::bi::Session::version`

[Back to `Session`](../bi-session.md) · [PlussBI API Reference](../bi.md)

```cpp
#include <pluss/bi/session.h>

std::uint64_t version() const noexcept;
```

`version` returns the session's change counter. It starts at zero and grows by
one for each accepted mutating call. Compare two readings of it to learn whether
the session changed between them.

Each accepted call counts once, including clears of keys that were never set. A
refused call throws before storage and does not count. Reading the version never
changes it.

## Parameters

This function takes no parameters.

## Returns

The number of accepted changes since the session was created.

## Errors

This call does not throw.

## Practical Examples

### Example 1

shows a new session starts at zero.

```cpp
#include <pluss/bi/session.h>
#include <pluss/bi/specs.h>

#include <iostream>
#include <string>
#include <variant>

namespace bi = pluss::bi;

template <class Fn>
bool throws(Fn fn) {
  try {
    fn();
  } catch (const std::exception&) {
    return true;
  }
  return false;
}

bi::Filter region_filter(const std::string& key, const std::string& value) {
  return bi::Filter{.key = key, .feature = "region", .op = bi::FilterOp::Eq,
                    .payload = bi::FilterScalar{value}};
}

int main() {
  bi::Session session;
  std::cout << "new session version: " << session.version() << '\n';
  return 0;
}
```

Output

```text
new session version: 0
```

### Example 2

shows that each kind of change moves the counter by one.

```cpp
#include <pluss/bi/session.h>
#include <pluss/bi/specs.h>

#include <iostream>
#include <string>
#include <variant>

namespace bi = pluss::bi;

template <class Fn>
bool throws(Fn fn) {
  try {
    fn();
  } catch (const std::exception&) {
    return true;
  }
  return false;
}

bi::Filter region_filter(const std::string& key, const std::string& value) {
  return bi::Filter{.key = key, .feature = "region", .op = bi::FilterOp::Eq,
                    .payload = bi::FilterScalar{value}};
}

int main() {
  bi::Session session;
  session.set_param("a", 1.0);
  std::cout << "after set_param: " << session.version() << '\n';
  session.set_filter(region_filter("region", "north"));
  std::cout << "after set_filter: " << session.version() << '\n';
  session.clear_param("a");
  std::cout << "after clear_param: " << session.version() << '\n';
  session.clear_filters();
  std::cout << "after clear_filters: " << session.version() << '\n';
  return 0;
}
```

Output

```text
after set_param: 1
after set_filter: 2
after clear_param: 3
after clear_filters: 4
```

### Example 3

shows that a refused call leaves it alone.

```cpp
#include <pluss/bi/session.h>
#include <pluss/bi/specs.h>

#include <iostream>
#include <string>
#include <variant>

namespace bi = pluss::bi;

template <class Fn>
bool throws(Fn fn) {
  try {
    fn();
  } catch (const std::exception&) {
    return true;
  }
  return false;
}

bi::Filter region_filter(const std::string& key, const std::string& value) {
  return bi::Filter{.key = key, .feature = "region", .op = bi::FilterOp::Eq,
                    .payload = bi::FilterScalar{value}};
}

int main() {
  std::cout << std::boolalpha;
  // A refused change throws before it is stored, so the version does not move.
  bi::Session session;
  session.set_param("a", 1.0);
  const auto before = session.version();
  std::cout << "refused: " << throws([&] { session.set_param("", 2.0); }) << '\n';
  std::cout << "version unchanged: " << (session.version() == before) << '\n';
  return 0;
}
```

Output

```text
refused: true
version unchanged: true
```

## See Also

- [`Session`](../bi-session.md) -- the class that owns this operation.
