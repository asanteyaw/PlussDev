# `pluss::bi::Session::filter`

[Back to `Session`](../bi-session.md) · [PlussBI API Reference](../bi.md)

```cpp
#include <pluss/bi/session.h>

std::optional<Filter> filter(std::string_view key) const;
```

`filter` returns the filter stored under a key, wrapped in an optional. When no
filter is stored under the key, the result is empty.

The returned value is a copy. It is a snapshot of the filter at the moment of the
call, so a later change to the session does not change what you already hold.

The call only reads, and it never changes the version.

## Parameters

| Parameter | Meaning |
|---|---|
| `key` | The key of the filter to read. |

## Returns

An optional holding a copy of the stored filter, or an empty optional when no filter is stored under `key`.

## Errors

This call does not throw.

## Practical Examples

### Example 1

reads a stored filter and its fields.

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
  // filter returns the stored filter wrapped in an optional. It is a copy of the stored value.
  bi::Session session;
  session.set_filter(region_filter("region", "north"));
  const auto found = session.filter("region");
  std::cout << "found: " << found.has_value() << '\n';
  std::cout << "key: " << found->key << '\n';
  std::cout << "feature: " << found->feature << '\n';
  std::cout << "value: " << std::get<std::string>(std::get<pluss::bi::FilterScalar>(found->payload)) << '\n';
  return 0;
}
```

Output

```text
found: true
key: region
feature: region
value: north
```

### Example 2

shows that absent and cleared keys both return an empty optional.

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
  std::cout << "absent key has no value: " << !session.filter("region").has_value() << '\n';
  session.set_filter(region_filter("region", "north"));
  session.clear_filter("region");
  std::cout << "cleared key has no value: " << !session.filter("region").has_value() << '\n';
  return 0;
}
```

Output

```text
absent key has no value: true
cleared key has no value: true
```

### Example 3

shows that the returned value is a copy.

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
  // The returned filter is a copy. Changing the session afterwards does not change it.
  bi::Session session;
  session.set_filter(region_filter("region", "north"));
  auto copy = session.filter("region");
  session.set_filter(region_filter("region", "south"));
  std::cout << "copy still north: "
            << (std::get<std::string>(std::get<pluss::bi::FilterScalar>(copy->payload)) == "north") << '\n';
  std::cout << "session now south: "
            << (std::get<std::string>(std::get<pluss::bi::FilterScalar>(session.filter("region")->payload)) == "south") << '\n';
  return 0;
}
```

Output

```text
copy still north: true
session now south: true
```

## See Also

- [`Session`](../bi-session.md) -- the class that owns this operation.
