# `pluss::bi::Session::clear_filters`

[Back to `Session`](../bi-session.md) · [PlussBI API Reference](../bi.md)

```cpp
#include <pluss/bi/session.h>

Session& clear_filters();
```

`clear_filters` removes every filter in the session in one call. Use it when a
reader resets a dashboard's narrowing choices, and the parameters should remain.

The whole clear is one change. The version increases by one for the call,
regardless of how many filters were removed, and it also increases on a session
that held no filters. The call returns the session itself.

Parameters are not affected. This call empties only the filter map.

## Parameters

This function takes no parameters.

## Returns

A reference to the session, so calls chain.

## Errors

This call does not throw.

## Practical Examples

### Example 1

removes two filters with one call and the version moves once.

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
  // clear_filters removes every filter in one call, and the version moves once.
  bi::Session session;
  session.set_filter(region_filter("region", "north"));
  session.set_filter(region_filter("region_b", "south"));
  const auto before = session.version();
  session.clear_filters();
  std::cout << "filters left: " << session.filters().size() << '\n';
  std::cout << "version moved by one: " << (session.version() == before + 1) << '\n';
  return 0;
}
```

Output

```text
filters left: 0
version moved by one: true
```

### Example 2

shows that clearing an empty session still counts as a change.

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
  // On an empty session, clear_filters still counts as a change.
  bi::Session session;
  session.clear_filters();
  std::cout << "empty after clear: " << session.empty() << '\n';
  std::cout << "version after clear on empty session: " << session.version() << '\n';
  return 0;
}
```

Output

```text
empty after clear: true
version after clear on empty session: 1
```

### Example 3

shows that parameters survive.

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
  // clear_filters does not touch parameters.
  bi::Session session;
  session.set_param("threshold", 2.5);
  session.set_filter(region_filter("region", "north"));
  session.clear_filters();
  std::cout << "parameter kept: " << session.has_param("threshold") << '\n';
  std::cout << "params count: " << session.params().size() << '\n';
  return 0;
}
```

Output

```text
parameter kept: true
params count: 1
```

## See Also

- [`Session`](../bi-session.md) -- the class that owns this operation.
