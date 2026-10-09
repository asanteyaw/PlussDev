# `pluss::bi::Session::set_param`

[Back to `Session`](../bi-session.md) · [PlussBI API Reference](../bi.md)

```cpp
Session& set_param(std::string_view key, double value);
Session& set_param(std::string_view key, std::string_view value);
```

`set_param` stores a named parameter. A parameter is a single value a control or
a caller reads by name, and it is stored as either a number or text. The overload
you call decides which: a `double` argument stores a number, and a string argument
stores text. Parameters are separate from filters, so a parameter never narrows
rows by itself.

Storage is keyed. The key names the slot, and setting a key that already holds a
parameter replaces it, including when the new value has the other type. A session
never holds two parameters under one key. Each accepted call bumps the version by
one. The call returns the session itself, so several parameters can be set in one
expression.

The key must not be empty. An empty key is refused before storage, so a refused
call leaves the session and its version unchanged.

## Parameters

| Parameter | Meaning |
|---|---|
| `key` | The parameter name. It must not be empty. |
| `value` | A `double` to store a number, or a `std::string_view` to store text. The text is copied into the session. |

## Returns

A reference to the session, so calls chain.

## Errors

The call refuses an empty `key`, for either overload, and throws before anything
is stored. The version is unchanged after a refused call.

## Practical Examples

### Example 1: A numeric and a text parameter

The two overloads store values of different types. Read them back with the type
the value was stored as.

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

The threshold is numeric and reads back as 2.5, and the region is text. The
variant holds the type that matches the overload you called.

### Example 2: Replacing a value with the other type

Setting an existing key again replaces the stored value and its type, so one key
never holds two values at once.

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

The key first held a number and then text. The session still holds exactly one
parameter, and it is now text.

### Example 3: Refused keys

An empty key is refused for both overloads, and the version does not move.

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

Both refusals throw and the version stays at zero, so a failed call never looks
like a change to a watcher.

## See Also

- [`Session`](../bi-session.md) -- the session that stores parameters.
- [`clear_param`](clear_param.md) -- remove a parameter.
- [`set_filter`](set_filter.md) -- store a filter, which narrows rows.
