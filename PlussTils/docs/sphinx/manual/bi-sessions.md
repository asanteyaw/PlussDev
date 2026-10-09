# PlussBI: Sessions and parameters

```{raw} html
<meta name="pluss-doc-kind" content="guide">
```

[Back to PlussTils](../index.md) · [PlussBI API Reference](../api/bi.md) · [PlussBI: A Guide](bi.md)

A session holds what a reader has chosen. A filter narrows the rows a view shows, and a parameter is a named value that a view or a control can read. Use the session when the choice should survive a save and should change the next render, not the board. This chapter covers every session operation and every parameter operation, in the order you would reach for them.

## The mental model

A session is a record of choices, and it is versioned. Every change that stores or removes a choice moves the version forward by exactly one. Reads never move it. A call the session refuses, such as an empty key, never moves it either. That makes the version a reliable signal: if it has not moved, nothing a render depends on has changed.

Filters and parameters are two separate namespaces. A filter and a parameter can share a key without touching each other, because the session stores them apart.

## Worked tour

Every example is a complete program, and the output under it is copied from that program's run.

## Example 1: A new session is empty

Start with a session you have just made. Before any change, it holds no filters and no parameters, and its version is zero. Every later example measures its version from this baseline.

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

Both counts are zero and the version is zero. You can rely on that starting point when you save a session and load it later.

## Example 2: Filters and parameters live side by side

A session can hold a filter and a parameter at once. This example stores one of each and reads both counts back.

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

The session holds one parameter and one filter. Two changes moved the version to two, one for each store.

## Example 3: A copy of a session is independent

A session is a value. Copying it gives you a second record that you can change without touching the first. This example changes the copy and then reads both.

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

The original still says north, and the copy says south. The copy's version moved on its own, and the original's did not.

## Example 4: Setting a filter with the same key replaces it

A filter is stored under its key. Setting a second filter under the same key does not add a second one. This example sets two filters under one key.

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

After two sets, the session still holds one filter. The version reads two, because each set counted as a change.

## Example 5: A filter with an empty key or feature is refused

The session checks names before it stores anything. This example passes an empty key, then an empty feature, and then reads the state.

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

Both calls were refused, the session is still empty, and the version did not move. A refusal is never a half-applied change.

## Example 6: A parameter can hold a number or text

A parameter is typed by the value you store. This example stores a number and some text, then reads each back in its own type.

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
  // A parameter holds either a number or text. The overload you call picks the type it stores.
  bi::Session session;
  session.set_param("threshold", 2.5);
  session.set_param("region", std::string_view("north"));
  std::cout << "threshold is numeric: " << std::holds_alternative<double>(*session.param("threshold")) << '\n';
  std::cout << "region is text: " << std::holds_alternative<std::string>(*session.param("region")) << '\n';
  std::cout << "threshold value: " << std::get<double>(*session.param("threshold")) << '\n';
  return 0;
}
```

Output

```text
threshold is numeric: true
region is text: true
threshold value: 2.5
```

The threshold is numeric and the region is text. Reading the number gives 2.5.

## Example 7: Setting a parameter with a new type replaces the old one

Storing a parameter under an existing key replaces the stored value, including its type. This example changes a parameter from a number to text.

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
  // Setting an existing key with the other type replaces the stored value and its type.
  bi::Session session;
  session.set_param("mode", 1.0);
  session.set_param("mode", std::string("fast"));
  std::cout << "mode is now text: " << std::holds_alternative<std::string>(*session.param("mode")) << '\n';
  std::cout << "still one parameter: " << (session.params().size() == 1) << '\n';
  return 0;
}
```

Output

```text
mode is now text: true
still one parameter: true
```

The mode is text now, and there is still only one parameter. The key did not change, only the value and its type did.

## Example 8: A parameter with an empty key is refused

A parameter key must have characters in it. This example tries an empty key with a number and with text.

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
  std::cout << "empty key rejected: " << throws([&] { session.set_param("", 1.0); }) << '\n';
  std::cout << "empty key with text rejected: " << throws([&] { session.set_param("", std::string_view("x")); }) << '\n';
  std::cout << "version unchanged by rejected calls: " << (session.version() == 0) << '\n';
  return 0;
}
```

Output

```text
empty key rejected: true
empty key with text rejected: true
version unchanged by rejected calls: true
```

Both calls were refused, and the version did not move.

## Example 9: Clearing a parameter removes it

Clearing removes a parameter so that it reads as absent. This example clears a stored value, then clears a key that was never set.

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

The cleared key is no longer set. Clearing it moved the version, and clearing an absent key moved it again. A clear always counts, so a session that was cleared twice has changed twice.

## Example 10: Reads do not change the version

A render or a lookup reads the session. This example reads several values and checks that the version stays put.

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

The version is still one, so reading the session is free. You can read as often as you like without invalidating anything.

## Example 11: Clearing a parameter leaves the filters alone

The two namespaces are independent. This example clears a parameter while a filter is set.

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

The filter survived the clear. Only the parameter went away.

## Example 12: A cleared key reads as absent, not as empty

Empty text is a value that you chose, and it is different from a key you never set or have cleared. This example checks both.

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

The empty text is present and reads as a value. The cleared key is absent, and it does not read as a value.

## Example 13: An empty session has no content

A session counts as empty only when it holds no filters and no parameters. This example checks the session at each step: with a parameter, after clearing it, and with a filter.

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

While a parameter is stored, the session is not empty. Once cleared, it is empty again. A filter alone also makes it not empty.

## Example 14: Clearing a single filter

Removing one filter by its key leaves the others in place. This example stores two filters and clears one.

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
  // clear_filter removes the filter stored under one key and leaves other filters alone.
  bi::Session session;
  session.set_filter(region_filter("region", "north"));
  session.set_filter(region_filter("region_b", "south"));
  session.clear_filter("region");
  std::cout << "region still stored: " << session.has_filter("region") << '\n';
  std::cout << "region_b still stored: " << session.has_filter("region_b") << '\n';
  std::cout << "filters left: " << session.filters().size() << '\n';
  return 0;
}
```

Output

```text
region still stored: false
region_b still stored: true
filters left: 1
```

The cleared filter is gone, the other one is still stored, and one filter is left.

## Example 15: Clearing all filters at once

Clearing all filters removes every filter in one call. This example stores filters and clears them all.

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
  // clear_filters removes every filter in one call, and the version moves once.
  bi::Session session;
  session.set_filter(region_filter("region", "north"));
  session.set_filter(region_filter("region_b", "south"));
  const auto before = session.version();
  session.clear_filters();
  std::cout << "filters left: " << session.filters().size() << '\n';
  std::cout << "version moved by one: " << (session.version() == before + 1) << '\n';
  return 0;
}
```

Output

```text
filters left: 0
version moved by one: true
```

No filters are left, and the version moved by one, because the clear is one change.

## Example 16: Clearing all parameters at once

Clearing all parameters removes every parameter in one call. This example stores parameters and clears them all.

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

No parameters are left, and the session reads as empty.

## Example 17: Checking whether a filter is set

A membership check tells you whether a key is set, without reading its value. This example checks a key before and after storing it, and checks a different key.

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

The key is absent before, present after, and a different key is still absent.

## Example 18: Checking whether a parameter is set

A parameter has the same membership check. This example checks a parameter key before and after storing it.

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

The key is absent before, present after, and a different key is still absent.

## Example 19: Reading a stored filter

Reading a filter by its key returns the filter you stored, with its feature and value. This example reads one back.

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

The lookup found the filter, and its key, feature, and value match what you stored.

## Example 20: Listing filters by key

Listing filters returns them in key order, so the list is the same every time. This example stores two filters in reverse order and lists them.

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
  // filters returns the whole map. Keys come out in sorted order.
  bi::Session session;
  session.set_filter(region_filter("zeta", "north"));
  session.set_filter(region_filter("alpha", "south"));
  for (const auto& [key, filter] : session.filters()) {
    std::cout << key << ": " << filter.feature << '\n';
  }
  return 0;
}
```

Output

```text
alpha: region
zeta: region
```

The list reads alpha before zeta, although zeta was stored first.

## Example 21: Listing parameters by key

Parameters are listed in key order too. This example stores two parameters in reverse order and lists them with their types.

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

The list reads alpha before zeta, and each entry shows its type.

## Example 22: Reading a parameter back in its type

Reading a parameter returns its value in the type it was stored with. This example reads a number and some text.

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

The number reads back as a number and the text as text. The number's value is 2.5.

## Decisions

| You want | Use | Notes |
|---|---|---|
| A choice a reader makes that should narrow rows | A filter (`Session::set_filter`) | Filters are per column and per operator; see the Filters chapter. |
| A named value a view or control reads | A parameter (`Session::set_param`) | Parameters carry a number or text, and keep their type. |
| Remove one choice | `clear_filter` or `clear_param` | Clearing an absent key still counts as a change. |
| Remove every choice of one kind | `clear_filters` or `clear_params` | One call, one version change. |
| Check whether a key is set without reading it | `has_filter` or `has_param` | A membership check does not move the version. |

## Pitfalls

**Expecting a rejected call to move the version.** session_05 and session_08 show that a refused call leaves the session and its version untouched.

**Expecting a clear of an absent key to be free.** session_09 shows the version moving on a clear of a key that was never set.

**Reading an empty string as a missing value.** session_12 shows that empty text is present, and a cleared key is absent.

## Where next

- [Filters](bi-filters.md) -- every filter factory and its payload.
- [Studios](bi-studios.md) -- how the session feeds a render, and how a saved studio keeps it.
- [PlussBI API Reference](../api/bi.md) -- the session reference.
