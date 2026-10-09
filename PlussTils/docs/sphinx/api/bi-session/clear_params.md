# `pluss::bi::Session::clear_params`

[Back to `Session`](../bi-session.md) · [PlussBI API Reference](../bi.md)

```cpp
#include <pluss/bi/session.h>

Session& clear_params();
```

`clear_params` removes every parameter in the session in one call. It is the
parameter counterpart of `clear_filters`, and it leaves filters in place.

The whole clear is one change, so the version increases by one for the call
whether the session held zero or many parameters. The call returns the session
itself.

## Parameters

This function takes no parameters.

## Returns

A reference to the session, so calls chain.

## Errors

This call does not throw.

## Practical Examples

### Example 1

removes every parameter.

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
  session.set_param("region", std::string("north"));
  session.clear_params();
  std::cout << "params left: " << session.params().size() << '\n';
  std::cout << "empty: " << session.empty() << '\n';
  return 0;
}
```

Output

```text
params left: 0
empty: true
```

### Example 2

shows that filters survive the call.

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
  session.clear_params();
  std::cout << "filter kept: " << session.has_filter("region") << '\n';
  std::cout << "empty: " << session.empty() << '\n';
  return 0;
}
```

Output

```text
filter kept: true
empty: false
```

### Example 3

shows the version rule for empty and non-empty parameter maps.

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
  session.clear_params();
  std::cout << "version after clear on empty params: " << session.version() << '\n';
  session.set_param("a", 1.0);
  session.set_param("b", 2.0);
  const auto before = session.version();
  session.clear_params();
  std::cout << "version moved by one for the whole clear: " << (session.version() == before + 1) << '\n';
  return 0;
}
```

Output

```text
version after clear on empty params: 1
version moved by one for the whole clear: true
```

## See Also

- [`Session`](../bi-session.md) -- the class that owns this operation.
