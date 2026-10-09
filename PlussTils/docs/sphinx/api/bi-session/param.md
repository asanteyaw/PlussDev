# `pluss::bi::Session::param`

[Back to `Session`](../bi-session.md) · [PlussBI API Reference](../bi.md)

```cpp
#include <pluss/bi/session.h>

std::optional<ParamValue> param(std::string_view key) const;
```

`param` returns the parameter stored under a key, wrapped in an optional. The
value is a variant that holds either a `double` or a `std::string`, so check the
alternative before you read it. An absent key returns an empty optional.

The returned value is a copy, and the call only reads, never changing the version.

## Parameters

| Parameter | Meaning |
|---|---|
| `key` | The parameter key to read. |

## Returns

An optional holding a copy of the stored `ParamValue`, or an empty optional when no parameter is stored under `key`.

## Errors

This call does not throw.

## Practical Examples

### Example 1

reads a numeric and a text parameter and checks each alternative.

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
  // param returns the stored value as a variant. Check its alternative to know the type.
  bi::Session session;
  session.set_param("threshold", 2.5);
  session.set_param("region", std::string("north"));
  const auto threshold = session.param("threshold");
  const auto region = session.param("region");
  std::cout << "threshold holds a number: " << std::holds_alternative<double>(*threshold) << '\n';
  std::cout << "region holds text: " << std::holds_alternative<std::string>(*region) << '\n';
  std::cout << "threshold value: " << std::get<double>(*threshold) << '\n';
  return 0;
}
```

Output

```text
threshold holds a number: true
region holds text: true
threshold value: 2.5
```

### Example 2

shows that absent and cleared keys return an empty optional.

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
  std::cout << "absent parameter has no value: " << !session.param("threshold").has_value() << '\n';
  session.set_param("threshold", 2.5);
  session.clear_param("threshold");
  std::cout << "cleared parameter has no value: " << !session.param("threshold").has_value() << '\n';
  return 0;
}
```

Output

```text
absent parameter has no value: true
cleared parameter has no value: true
```

### Example 3

shows that an empty text value is still a value.

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
  // An empty text value is a value. It is not the same as no value.
  bi::Session session;
  session.set_param("note", std::string(""));
  const auto note = session.param("note");
  std::cout << "empty text is a value: " << note.has_value() << '\n';
  std::cout << "empty text is text: " << std::holds_alternative<std::string>(*note) << '\n';
  std::cout << "length: " << std::get<std::string>(*note).size() << '\n';
  return 0;
}
```

Output

```text
empty text is a value: true
empty text is text: true
length: 0
```

## See Also

- [`Session`](../bi-session.md) -- the class that owns this operation.
