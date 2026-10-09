# `pluss::bi::Session::clear_filter`

[Back to `Session`](../bi-session.md) · [PlussBI API Reference](../bi.md)

```cpp
#include <pluss/bi/session.h>

Session& clear_filter(std::string_view key);
```

`clear_filter` removes the filter stored under a key. It is the way to drop one
of a reader's narrowing choices while keeping the others. The key is matched
exactly, and removing it leaves every other filter in place.

Clearing counts as a change even when no filter was stored under the key. The
version increases by one either way, so a watcher comparing versions sees the
clear. The call returns the session itself, so it chains with other changes.

Clearing never touches parameters. Filters and parameters are separate maps, and
this call reads and writes only the filter map.

## Parameters

| Parameter | Meaning |
|---|---|
| `key` | The key of the filter to remove. It may name a key that holds no filter. |

## Returns

A reference to the session, so calls chain.

## Errors

This call does not refuse any key and does not throw.

## Practical Examples

### Example 1

removes one of two filters and shows the other survives.

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
  // clear_filter removes the filter stored under one key and leaves other filters alone.
  bi::Session session;
  session.set_filter(region_filter("region", "north"));
  session.set_filter(region_filter("region_b", "south"));
  session.clear_filter("region");
  std::cout << "region still stored: " << session.has_filter("region") << '\n';
  std::cout << "region_b still stored: " << session.has_filter("region_b") << '\n';
  std::cout << "filters left: " << session.filters().size() << '\n';
  return 0;
}
```

Output

```text
region still stored: false
region_b still stored: true
filters left: 1
```

### Example 2

shows that clearing an absent key is accepted and still moves the version.

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
  // Clearing a key that holds no filter is not an error, and it still counts as a change.
  bi::Session session;
  session.set_filter(region_filter("region", "north"));
  const auto before = session.version();
  session.clear_filter("never_set");
  std::cout << "no error for absent key: true\n";
  std::cout << "version moved: " << (session.version() == before + 1) << '\n';
  std::cout << "original filter untouched: " << session.has_filter("region") << '\n';
  return 0;
}
```

Output

```text
no error for absent key: true
version moved: true
original filter untouched: true
```

### Example 3

shows that parameters are not affected.

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
  // clear_filter only touches filters. Parameters stay in place.
  bi::Session session;
  session.set_param("threshold", 2.5);
  session.set_filter(region_filter("region", "north"));
  session.clear_filter("region");
  std::cout << "parameter kept: " << session.has_param("threshold") << '\n';
  std::cout << "filters empty: " << session.filters().empty() << '\n';
  return 0;
}
```

Output

```text
parameter kept: true
filters empty: true
```

## See Also

- [`Session`](../bi-session.md) -- the class that owns this operation.
