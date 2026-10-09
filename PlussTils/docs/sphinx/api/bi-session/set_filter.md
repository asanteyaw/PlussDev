# `pluss::bi::Session::set_filter`

[Back to `Session`](../bi-session.md) · [PlussBI API Reference](../bi.md)

```cpp
Session& set_filter(Filter value);
```

`set_filter` stores a filter under its key. A filter narrows what a view shows
when a studio renders the session, so this is how a reader's choice of a
category or a range reaches the dashboard. The call returns the session itself,
so several changes can be chained.

Storage is keyed. The filter's `key` names the slot, and a second filter with the
same key replaces the first. A session therefore never holds two filters under
one key, and reusing a key is how you change a reader's choice without clearing
it first. Each accepted call bumps the session's version by one, even when it
replaces an earlier filter.

The filter must name both a key and a feature. The key identifies the slot, and
the feature is the data column the filter applies to. An empty key or an empty
feature is refused before anything is stored, so a refused call leaves the session
and its version unchanged.

## Parameters

| Parameter | Meaning |
|---|---|
| `value` | The filter to store. Its `key` selects the slot, and its `feature` names the column. Its `op` and `payload` describe the test, and `scope` and `enabled` are stored with it unchanged. |

The call takes its filter by value and moves it into the session, so the
argument you pass is not shared with the session afterwards.

## Returns

A reference to the session, so calls chain. The reference refers to the same
session the call was made on.

## Errors

The call refuses a filter with an empty `key`, and it refuses a filter with an
empty `feature`. Both refusals happen before storage and leave the session
unchanged. 
## Practical Examples

### Example 1: Store a filter

A text equality filter on the region column narrows a view to north. The filter is
stored under its key, and the session reports one filter.

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
  // Filters and parameters live in separate maps. Each change to either bumps the one version.
  bi::Session session;
  session.set_param("threshold", 2.5);
  session.set_filter(bi::Filter{
      .key = "region_is_north",
      .feature = "region",
      .op = bi::FilterOp::Eq,
      .payload = bi::FilterScalar{std::string("north")},
  });
  std::cout << "params: " << session.params().size() << ", filters: " << session.filters().size() << '\n';
  std::cout << "empty after two changes: " << session.empty() << '\n';
  std::cout << "version: " << session.version() << '\n';
  return 0;
}
```

Output

```text
params: 1, filters: 1
empty after two changes: false
version: 2
```

### Example 2: Replace by key

Filters are keyed. Setting a second filter under the same key replaces the first,
so a reader who changes a choice keeps one filter, not two.

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
  // A filter is stored under its key. Setting the same key again replaces the earlier filter.
  session.set_filter(bi::Filter{.key = "region", .feature = "region", .op = bi::FilterOp::Eq,
                                .payload = bi::FilterScalar{std::string("north")}});
  session.set_filter(bi::Filter{.key = "region", .feature = "region", .op = bi::FilterOp::Eq,
                                .payload = bi::FilterScalar{std::string("south")}});
  std::cout << "filters after two sets with one key: " << session.filters().size() << '\n';
  std::cout << "has region: " << session.has_filter("region") << '\n';
  std::cout << "version after two sets: " << session.version() << '\n';
  return 0;
}
```

Output

```text
filters after two sets with one key: 1
has region: true
version after two sets: 2
```

The two calls produce one filter, and the session's version is two because each
accepted call counts as a change, even the one that replaced the first filter.

### Example 3: Refused filters

An empty key or an empty feature is refused, and the session is left as it was.

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
  // A filter must name its key and its feature. Either empty name is refused.
  bi::Session session;
  std::cout << "empty key rejected: " << throws([&] {
    session.set_filter(bi::Filter{.key = "", .feature = "region"});
  }) << '\n';
  std::cout << "empty feature rejected: " << throws([&] {
    session.set_filter(bi::Filter{.key = "region", .feature = ""});
  }) << '\n';
  std::cout << "session still empty: " << session.empty() << '\n';
  std::cout << "version unchanged by rejected calls: " << (session.version() == 0) << '\n';
  return 0;
}
```

Output

```text
empty key rejected: true
empty feature rejected: true
session still empty: true
version unchanged by rejected calls: true
```

Both refusals throw, the session stays empty, and its version stays at zero. A
caller can therefore retry with a valid filter without worrying about partial
state.

## See Also

- [`Session`](../bi-session.md) -- the session that stores filters and parameters.
- [`set_param`](set_param.md) -- store a parameter instead of a filter.
- [`clear_param`](clear_param.md) -- remove a parameter.
