# `pluss::bi::Session::filters`

[Back to `Session`](../bi-session.md) · [PlussBI API Reference](../bi.md)

```cpp
#include <pluss/bi/session.h>

const std::map<std::string, Filter>& filters() const noexcept;
```

`filters` returns the session's filter map, keyed by filter key. The map is
sorted by key, so iteration order is stable across runs.

The reference refers to the session's own storage. Read it for the duration of
the current use, and make a copy if you need the entries after the session
changes.

The call only reads, and it never changes the version.

## Parameters

This function takes no parameters.

## Returns

A const reference to the map of key to filter.

## Errors

This call does not throw.

## Practical Examples

### Example 1

walks the map and shows the sorted key order.

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
  // filters returns the whole map. Keys come out in sorted order.
  bi::Session session;
  session.set_filter(region_filter("zeta", "north"));
  session.set_filter(region_filter("alpha", "south"));
  for (const auto& [key, filter] : session.filters()) {
    std::cout << key << ": " << filter.feature << '\n';
  }
  return 0;
}
```

Output

```text
alpha: region
zeta: region
```

### Example 2

shows that an empty session gives an empty map.

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
  bi::Session session;
  std::cout << "empty map: " << session.filters().empty() << '\n';
  std::cout << "size: " << session.filters().size() << '\n';
  return 0;
}
```

Output

```text
empty map: true
size: 0
```

### Example 3

shows that reading the map does not move the version.

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
  bi::Session session;
  session.set_filter(region_filter("region", "north"));
  const auto before = session.version();
  std::size_t count = 0;
  for (const auto& entry : session.filters()) {
    (void)entry;
    ++count;
  }
  std::cout << "entries read: " << count << '\n';
  std::cout << "version unchanged: " << (session.version() == before) << '\n';
  return 0;
}
```

Output

```text
entries read: 1
version unchanged: true
```

## See Also

- [`Session`](../bi-session.md) -- the class that owns this operation.
