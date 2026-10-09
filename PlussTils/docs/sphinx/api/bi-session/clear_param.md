# `pluss::bi::Session::clear_param`

[Back to `Session`](../bi-session.md) · [PlussBI API Reference](../bi.md)

```cpp
Session& clear_param(std::string_view key);
```

`clear_param` removes the parameter stored under a key. After the call, the key
is absent: `has_param` reports false and `param` returns nothing for it. A cleared key reads as absent, which is different from a key that holds an empty text value.

Clearing always counts as a change. The version increases by one whether or not
the key was present, so a watcher that compares versions sees the clear even when
nothing was removed. The call returns the session itself, so clears chain with
other changes.

Clearing does not touch filters. A filter stored under a key is separate state, and
`clear_param` never removes it.

## Parameters

| Parameter | Meaning |
|---|---|
| `key` | The parameter to remove. It may name a key that was never set. |

## Returns

A reference to the session, so calls chain.

## Errors

This call does not refuse any key, including an absent one. It does not throw.

## Practical Examples

### Example 1: Remove a parameter

Clearing a set parameter makes its key absent, and the version moves to reflect the
change.

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

int main() {
  std::cout << std::boolalpha;
  bi::Session session;
  session.set_param("threshold", 2.5);
  session.clear_param("threshold");
  std::cout << "still set: " << session.has_param("threshold") << '\n';
  std::cout << "version after clear: " << session.version() << '\n';
  // Clearing a key that was never set still counts as a change to the session.
  session.clear_param("never_set");
  std::cout << "version after clearing an absent key: " << session.version() << '\n';
  return 0;
}
```

Output

```text
still set: false
version after clear: 2
version after clearing an absent key: 3
```

The parameter is gone after the clear, and the version counts both the set and the
clear. Clearing a key that was never set still increases the version to three,
because a watcher cannot tell an absent clear from a real one by version alone.

### Example 2: A cleared key reads as absent

A key that holds an empty text value is still present, and its value is still there to read. A cleared key is absent, so `has_param` and `param` both report nothing for it.

```cpp
#include <pluss/bi/session.h>
#include <pluss/bi/specs.h>

#include <iostream>
#include <string>
#include <variant>

namespace bi = pluss::bi;

int main() {
  std::cout << std::boolalpha;
  // A cleared key reads as absent. It is not the same as a key that holds an empty text value.
  bi::Session session;
  session.set_param("note", std::string(""));
  std::cout << "empty text is present: " << session.has_param("note") << '\n';
  std::cout << "empty text reads as a value: " << session.param("note").has_value() << '\n';
  session.clear_param("note");
  std::cout << "cleared key is present: " << session.has_param("note") << '\n';
  std::cout << "cleared key reads as a value: " << session.param("note").has_value() << '\n';
  return 0;
}
```

Output

```text
empty text is present: true
empty text reads as a value: true
cleared key is present: false
cleared key reads as a value: false
```

### Example 3: Clears leave filters alone

Clearing a parameter does not remove a filter. The filter stays in the session and keeps narrowing the rows.

```cpp
#include <pluss/bi/session.h>
#include <pluss/bi/specs.h>

#include <iostream>
#include <string>
#include <variant>

namespace bi = pluss::bi;

int main() {
  std::cout << std::boolalpha;
  // Clearing a parameter leaves filters in place, and clearing a filter leaves parameters in place.
  bi::Session session;
  session.set_param("threshold", 2.5);
  session.set_filter(bi::Filter{.key = "region", .feature = "region", .op = bi::FilterOp::Eq,
                                .payload = bi::FilterScalar{std::string("north")}});
  session.clear_param("threshold");
  std::cout << "filter survives a parameter clear: " << session.has_filter("region") << '\n';
  std::cout << "filters after clear: " << session.filters().size() << '\n';
  return 0;
}
```

Output

```text
filter survives a parameter clear: true
filters after clear: 1
```

## See Also

- [`Session`](../bi-session.md) -- the session that stores parameters.
- [`set_param`](set_param.md) -- store a parameter.
- [`set_filter`](set_filter.md) -- store a filter, which `clear_param` never touches.
