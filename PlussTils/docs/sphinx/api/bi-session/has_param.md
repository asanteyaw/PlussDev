# `pluss::bi::Session::has_param`

[Back to `Session`](../bi-session.md) · [PlussBI API Reference](../bi.md)

```cpp
#include <pluss/bi/session.h>

bool has_param(std::string_view key) const;
```

`has_param` reports whether a parameter is stored under a key. A parameter that
holds an empty text value is present, and a cleared parameter is absent, so this
call answers the question about the key rather than about the value.

The call only reads, and it never changes the version.

## Parameters

| Parameter | Meaning |
|---|---|
| `key` | The parameter key to test. |

## Returns

`true` when a parameter is stored under `key`, otherwise `false`.

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
  bi::Session session;
  std::cout << "before any parameter: " << session.has_param("threshold") << '\n';
  session.set_param("threshold", 2.5);
  std::cout << "after storing: " << session.has_param("threshold") << '\n';
  std::cout << "other key: " << session.has_param("other") << '\n';
  return 0;
}
```

Output

```text
before any parameter: false
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
  bi::Session session;
  session.set_param("threshold", 2.5);
  const auto before = session.version();
  (void)session.has_param("threshold");
  (void)session.has_param("missing");
  std::cout << "version unchanged: " << (session.version() == before) << '\n';
  return 0;
}
```

Output

```text
version unchanged: true
```

### Example 3

shows the difference between an empty value and an absent key.

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
  // A cleared key is absent, while an empty text value is still present.
  bi::Session session;
  session.set_param("note", std::string(""));
  std::cout << "empty text present: " << session.has_param("note") << '\n';
  session.clear_param("note");
  std::cout << "after clear present: " << session.has_param("note") << '\n';
  return 0;
}
```

Output

```text
empty text present: true
after clear present: false
```

## See Also

- [`Session`](../bi-session.md) -- the class that owns this operation.
