# `pluss::bi::Session`

[Back to PlussBI API Reference](bi.md)

```cpp
#include <pluss/bi/session.h>

class pluss::bi::Session;
```

`Session` holds the choices a reader has made on a dashboard: the filters that
narrow a view, and the named parameters a control or a caller sets. It is the
only part of PlussBI that changes between renders. A studio reads its session
each time it renders, so a change to the session changes the next render and
nothing else.

A session is a plain value. It owns its filters and parameters, copying a
session copies them, and changing a copy does not change the original. The
class holds no tables, no handles, and no references, so a session can be
stored, compared by its contents, and passed between threads under ordinary
copying rules.

Every change bumps a single version counter. The counter starts at zero and
grows by one for each mutating call, including a call that clears a key that
was never set. Reading a value never changes it. Watch the version when you
need to know whether a session moved since you last looked, and compare it
against a value you saved.

## Methods

| Method | Purpose |
|---|---|
| [`set_filter`](bi-session/set_filter.md) | Store a filter under its key, replacing any earlier filter with that key |
| [`clear_filter`](bi-session/clear_filter.md) | Remove the filter stored under one key |
| [`clear_filters`](bi-session/clear_filters.md) | Remove every filter in one change |
| [`has_filter`](bi-session/has_filter.md) | Report whether a filter is stored under a key |
| [`filter`](bi-session/filter.md) | Read a copy of the filter stored under a key |
| [`filters`](bi-session/filters.md) | Read the whole filter map, sorted by key |
| [`set_param`](bi-session/set_param.md) | Store a numeric or text parameter under a key |
| [`clear_param`](bi-session/clear_param.md) | Remove one parameter |
| [`clear_params`](bi-session/clear_params.md) | Remove every parameter in one change |
| [`has_param`](bi-session/has_param.md) | Report whether a parameter is stored under a key |
| [`param`](bi-session/param.md) | Read a copy of the parameter stored under a key |
| [`params`](bi-session/params.md) | Read the whole parameter map, sorted by key |
| [`empty`](bi-session/empty.md) | Report whether the session holds no filters and no parameters |
| [`version`](bi-session/version.md) | Read the change counter |

## Practical Examples

### Example 1: A new session

A new session has no filters and no parameters, and its version is zero. Check
this state first when you start a dashboard, so you know what the session holds
before you add to it.

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
  // A new session has no filters and no parameters, and it starts at version zero.
  bi::Session session;
  std::cout << "empty: " << session.empty() << '\n';
  std::cout << "version: " << session.version() << '\n';
  std::cout << "filters: " << session.filters().size() << ", params: " << session.params().size() << '\n';
  return 0;
}
```

Output

```text
empty: true
version: 0
filters: 0, params: 0
```

The session is empty, its version is zero, and it holds no filters or parameters.
This is the state a studio starts from when no reader has chosen anything yet.

### Example 2: A copy is independent

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
  // A session is a value. A copy keeps its own state, so changing one does not change the other.
  bi::Session original;
  original.set_param("region", std::string("north"));
  bi::Session copy = original;
  copy.set_param("region", std::string("south"));
  std::cout << "original region: " << std::get<std::string>(*original.param("region")) << '\n';
  std::cout << "copy region: " << std::get<std::string>(*copy.param("region")) << '\n';
  std::cout << "copy version after its own change: " << copy.version() << '\n';
  std::cout << "original version unchanged: " << (original.version() == 1) << '\n';
  return 0;
}
```

Output

```text
original region: north
copy region: south
copy version after its own change: 2
original version unchanged: true
```

Copy a session when you want to try a change without affecting the session the
reader is using. The copy carries the same filters and parameters, and each
session keeps its own version counter from that point on.


The copy changes to south while the original stays north. The copy's version
moved to two, because it had one change inherited from the original and then its
own change. The original's version is still one. Sessions are values, so keep the
one the reader sees and derive new ones from it.

### Example 3: Filters and parameters are separate

Filters narrow the rows a view shows. Parameters carry values that a control or a
caller uses directly. A session can hold both, and each change to either bumps the
same version counter.

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

The session holds one parameter and one filter, is not empty, and its version is
two after the two changes. Reading the filter and the parameter separately tells
you what kind of state each one is.

### Example 10: Reads do not change the session

Reading a session is safe to do often. Asking whether a key exists, reading a value,
or listing the maps never moves the version, so a check that compares versions
sees only real changes.

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
  // Reading the session, its maps, or its version never changes the version.
  bi::Session session;
  session.set_param("threshold", 2.5);
  const auto before = session.version();
  (void)session.param("threshold");
  (void)session.has_param("threshold");
  (void)session.params();
  (void)session.filters();
  std::cout << "version unchanged by reads: " << (session.version() == before) << '\n';
  std::cout << "version: " << session.version() << '\n';
  return 0;
}
```

Output

```text
version unchanged by reads: true
version: 1
```

Reading the parameter, testing for it, and listing both maps leave the version at
one. Only the mutating calls change it.

## Errors

A session does not throw on ordinary changes. The methods that take a key refuse
an empty key, and `set_filter` also refuses a filter with an empty feature. The
refusals in Example 5 and in the method pages leave the session empty and its
version unchanged.

## See Also

- [`set_filter`](bi-session/set_filter.md) -- store a filter by key.
- [`set_param`](bi-session/set_param.md) -- store a numeric or text parameter.
- [`clear_param`](bi-session/clear_param.md) -- remove a parameter.
- [`version`](bi-session/version.md) -- the change counter every method moves.
- [`Studio`](bi.md) -- the studio that renders a session's current choices.

