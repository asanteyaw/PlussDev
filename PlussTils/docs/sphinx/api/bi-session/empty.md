# `pluss::bi::Session::empty`

[Back to `Session`](../bi-session.md) · [PlussBI API Reference](../bi.md)

```cpp
#include <pluss/bi/session.h>

bool empty() const noexcept;
```

`empty` reports whether the session holds no filters and no parameters. A session
that holds either one is not empty.

Emptiness is about content, not history. A session whose changes have all been
cleared is empty again, even though its version has kept counting. The call only
reads.

## Parameters

This function takes no parameters.

## Returns

`true` when there are no filters and no parameters, otherwise `false`.

## Errors

This call does not throw.

## Practical Examples

### Example 1

shows a new session is empty.

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
  std::cout << "new session empty: " << session.empty() << '\n';
  return 0;
}
```

Output

```text
new session empty: true
```

### Example 2

shows that either a filter or a parameter makes it non-empty.

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
  // A session is non-empty when it holds either a filter or a parameter.
  bi::Session session;
  session.set_param("threshold", 2.5);
  std::cout << "with a parameter: " << session.empty() << '\n';
  session.clear_params();
  std::cout << "after clearing parameters: " << session.empty() << '\n';
  session.set_filter(region_filter("region", "north"));
  std::cout << "with a filter: " << session.empty() << '\n';
  return 0;
}
```

Output

```text
with a parameter: false
after clearing parameters: true
with a filter: false
```

### Example 3

shows that clearing everything makes it empty again while the version keeps counting.

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
  session.set_filter(region_filter("region", "north"));
  session.clear_filters();
  session.clear_params();
  std::cout << "empty after clearing everything: " << session.empty() << '\n';
  std::cout << "version kept counting: " << session.version() << '\n';
  return 0;
}
```

Output

```text
empty after clearing everything: true
version kept counting: 4
```

## See Also

- [`Session`](../bi-session.md) -- the class that owns this operation.
