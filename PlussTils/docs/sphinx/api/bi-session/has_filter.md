# `pluss::bi::Session::has_filter`

[Back to `Session`](../bi-session.md) · [PlussBI API Reference](../bi.md)

```cpp
#include <pluss/bi/session.h>

bool has_filter(std::string_view key) const;
```

`has_filter` reports whether a filter is stored under a key. Use it to decide
whether a reader has narrowed a view on that key before you read the filter or
render with it.

The call only reads. It never changes the session or its version, so you can
check as often as you need.

An empty key is never stored, so the call returns false for it rather than
throwing.

## Parameters

| Parameter | Meaning |
|---|---|
| `key` | The key to test. Any string is accepted; an empty string reports absent. |

## Returns

`true` when a filter is stored under `key`, otherwise `false`.

## Errors

This call does not throw.

## Practical Examples

### Example 1

shows membership before and after a store.

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
  // has_filter reports whether a filter is stored under a key.
  bi::Session session;
  std::cout << "before any filter: " << session.has_filter("region") << '\n';
  session.set_filter(region_filter("region", "north"));
  std::cout << "after storing: " << session.has_filter("region") << '\n';
  std::cout << "other key: " << session.has_filter("other") << '\n';
  return 0;
}
```

Output

```text
before any filter: false
after storing: true
other key: false
```

### Example 2

shows that the version does not move.

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
  // Testing for a filter is a read. It never changes the version.
  bi::Session session;
  session.set_filter(region_filter("region", "north"));
  const auto before = session.version();
  (void)session.has_filter("region");
  (void)session.has_filter("missing");
  std::cout << "version unchanged: " << (session.version() == before) << '\n';
  return 0;
}
```

Output

```text
version unchanged: true
```

### Example 3

shows that an empty key reports absent instead of throwing.

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
  // An empty key is never stored, so has_filter reports false rather than throwing.
  bi::Session session;
  std::cout << "empty key does not throw: " << !throws([&] { (void)session.has_filter(""); }) << '\n';
  std::cout << "empty key reports absent: " << !session.has_filter("") << '\n';
  return 0;
}
```

Output

```text
empty key does not throw: true
empty key reports absent: true
```

## See Also

- [`Session`](../bi-session.md) -- the class that owns this operation.
