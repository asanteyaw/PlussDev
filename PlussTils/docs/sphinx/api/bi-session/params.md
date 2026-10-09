# `pluss::bi::Session::params`

[Back to `Session`](../bi-session.md) · [PlussBI API Reference](../bi.md)

```cpp
#include <pluss/bi/session.h>

const std::map<std::string, ParamValue>& params() const noexcept;
```

`params` returns the session's parameter map, keyed by parameter name. Like
`filters`, the map is sorted by key and iterates in a stable order.

The reference refers to the session's own storage, so copy it if you need the
entries after the session changes. The call only reads, and it never changes the
version.

## Parameters

This function takes no parameters.

## Returns

A const reference to the map of key to parameter value.

## Errors

This call does not throw.

## Practical Examples

### Example 1

walks the map in key order and tells each value's type.

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
  session.set_param("zeta", 1.0);
  session.set_param("alpha", std::string("north"));
  for (const auto& [key, value] : session.params()) {
    std::cout << key << ": " << (std::holds_alternative<double>(value) ? "number" : "text") << '\n';
  }
  return 0;
}
```

Output

```text
alpha: text
zeta: number
```

### Example 2

shows an empty map and then one entry.

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
  std::cout << "empty map: " << session.params().empty() << '\n';
  session.set_param("threshold", 2.5);
  std::cout << "size after one set: " << session.params().size() << '\n';
  return 0;
}
```

Output

```text
empty map: true
size after one set: 1
```

### Example 3

shows that reads leave the version alone.

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
  session.set_param("threshold", 2.5);
  const auto before = session.version();
  std::size_t count = 0;
  for (const auto& entry : session.params()) {
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
