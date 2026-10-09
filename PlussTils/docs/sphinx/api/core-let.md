# `pluss::tils::let`

[Back to Core Utilities](core.md) · [Core Utilities API Reference](../api_reference.md)

```cpp
#include <pluss/tils/let.h>

template <class T>
concept let_payload;

template <let_payload T>
class let;

template <class T>
inline constexpr bool enable_let = false;
```

`let<T>` binds one core payload and never rebinds it. The payload types are the series, note, timeline, duration, and queue types, plus any type that opts in by specializing `enable_let`. The binding is read-only: `get()`, `*`, and `->` return const access, and assignment to a `let` is removed. Copying a `let` gives a second binding to the same payload. `map` keeps the binding when its result is itself a payload, and otherwise yields the bare value.

## Parameters

| Parameter | Meaning |
|---|---|
| `T` | A payload type. `let_payload<T>` is true for a type whose `enable_let` specialization is true. |
| constructor argument | A payload value, copied or moved in. |

## Returns

`let(value)` returns a binding. `get()` returns a const reference to the payload. `map` returns a `let` or the bare value. `unwrap()` returns the payload by value.

## Errors

Assignment to a `let` does not compile. A type that is not a payload is refused by the constraint on `let`.

## Practical Examples

### Example 1

Example 1 binds a series and reads its count through `get`.

```cpp
#include <pluss/mic.h>
#include <pluss/tils/let.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // let binds a core payload. The binding is read-only, and reading the payload gives the same numbers.
  tils::let values = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}, {3});
  std::cout << "bound count: " << values.get().numel() << '\n';
  return 0;
}
```

Output

```text
bound count: 3
```

The bound series has three elements.

### Example 2

Example 2 maps a series to a series and keeps the binding.

```cpp
#include <pluss/mic.h>
#include <pluss/tils/let.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // map keeps the binding when the function returns a payload type.
  tils::let values = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}, {3});
  auto doubled = values.map([](const pluss::Series& s) { return s * 2.0; });
  std::cout << "map returns a let when the result is a payload: " << std::is_same_v<decltype(doubled), tils::let<pluss::Series>> << '\n';
  return 0;
}
```

Output

```text
map returns a let when the result is a payload: 1
```

The result is a `let`, so chaining continues.

### Example 3

Example 3 maps a series to a count and drops to the bare value.

```cpp
#include <pluss/mic.h>
#include <pluss/tils/let.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // map drops to the bare value when the function returns something that is not a payload.
  tils::let values = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0}, {3});
  auto count = values.map([](const pluss::Series& s) { return s.numel(); });
  std::cout << "map drops to the bare value: " << std::is_same_v<decltype(count), std::int64_t> << '\n';
  std::cout << "count: " << count << '\n';
  return 0;
}
```

Output

```text
map drops to the bare value: 1
count: 3
```

The count is a plain integer.

### Example 4

Example 4 passes a binding to a function that takes a const payload.

```cpp
#include <pluss/mic.h>
#include <pluss/tils/let.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // A let converts implicitly to const T&, so a function that takes a const payload accepts it.
  tils::let values = pluss::make_series(std::vector<double>{1.0, 2.0}, {2});
  auto size_of = [](const pluss::Series& s) { return s.numel(); };
  std::cout << "passed through as const: " << size_of(values) << '\n';
  return 0;
}
```

Output

```text
passed through as const: 2
```

The implicit conversion gives the function the payload.

### Example 5

Example 5 moves the payload out with `unwrap`.

```cpp
#include <pluss/mic.h>
#include <pluss/tils/let.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // unwrap on a binding that is about to end moves the payload out. The binding is then spent.
  tils::let names = pluss::make_note({"north", "south"});
  pluss::Note moved = std::move(names).unwrap();
  std::cout << "moved out count: " << moved.numel() << '\n';
  return 0;
}
```

Output

```text
moved out count: 2
```

The note moves out with its two entries.

### Example 6

Example 6 runs `peek` and keeps the binding.

```cpp
#include <pluss/mic.h>
#include <pluss/tils/let.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // peek runs a function on the payload and returns the same binding, so it can be chained.
  tils::let names = pluss::make_note({"north", "south", "east"});
  names.peek([](const pluss::Note& n) { std::cout << "peeked count: " << n.numel() << '\n'; })
       .peek([](const pluss::Note& n) { std::cout << "peeked again: " << n.numel() << '\n'; });
  std::cout << "binding unchanged: " << names.get().numel() << '\n';
  return 0;
}
```

Output

```text
peeked count: 3
peeked again: 3
binding unchanged: 3
```

Both peeks read three, and the binding still holds three.

### Example 7

Example 7 checks that assignment is removed.

```cpp
#include <pluss/mic.h>
#include <pluss/tils/let.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // The binding itself cannot change, so assignment does not compile. This check reads the trait, which says so.
  std::cout << "let is copy-assignable: " << std::is_copy_assignable_v<tils::let<pluss::Series>> << '\n';
  std::cout << "let is move-assignable: " << std::is_move_assignable_v<tils::let<pluss::Series>> << '\n';
  return 0;
}
```

Output

```text
let is copy-assignable: 0
let is move-assignable: 0
```

Copy and move assignment are unavailable.

### Example 8

Example 8 checks which types are payloads.

```cpp
#include <pluss/mic.h>
#include <pluss/tils/let.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // Only the core payload types are opted in, so a plain int is not a let payload.
  std::cout << "int is a let payload: " << tils::let_payload<int> << '\n';
  std::cout << "series is a let payload: " << tils::let_payload<pluss::Series> << '\n';
  return 0;
}
```

Output

```text
int is a let payload: 0
series is a let payload: 1
```

An integer is refused, and a series is accepted.

### Example 9

Example 9 binds a timeline.

```cpp
#include <pluss/mic.h>
#include <pluss/tils/let.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // Timelines and durations are payloads too, so they can be bound the same way.
  tils::let dates = pluss::make_timeline({"2024-01-05", "2024-02-10"});
  std::cout << "timeline bound count: " << dates.get().numel() << '\n';
  return 0;
}
```

Output

```text
timeline bound count: 2
```

The timeline binds its two dates.

### Example 10

Example 10 binds a queue -- a dynamic holder over the other four payload
types.

```cpp
#include <pluss/mic.h>
#include <pluss/queue.h>
#include <pluss/tils/let.h>

#include <iostream>

namespace tils = pluss::tils;

int main() {
  // Queue is a payload too -- a dynamic holder over Series/Note/Timeline/
  // Duration, for code that doesn't know which of the four it has until
  // runtime. Binding it works the same as any other payload.
  tils::let boxed = pluss::Queue(pluss::make_series({1.0, 2.0, 3.0}));
  std::cout << "kind: " << boxed.get().kind_name() << '\n';
  std::cout << "numel: " << boxed.get().numel() << '\n';
  return 0;
}
```

Output

```text
kind: Series
numel: 3
```

`let` treats the `Queue` as one opaque payload; it does not know or care
that the `Queue` itself holds a `Series` underneath. See PlussQit's own
Queue API Reference for `Queue`'s full behavior -- `kind()`, `invoke()`,
its operators, and the rest.

### Example 11

Example 11 opts a custom struct into `let`.

```cpp
#include <pluss/mic.h>
#include <pluss/tils/let.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

struct Reading {
  double value;
};

// Any type can join the family by specializing enable_let. Nothing else is needed.
template <>
inline constexpr bool pluss::tils::enable_let<Reading> = true;

int main() {
  tils::let reading = Reading{2.5};
  std::cout << "custom payload value: " << reading.get().value << '\n';
  std::cout << "custom payload is a let payload: " << tils::let_payload<Reading> << '\n';
  return 0;
}
```

Output

```text
custom payload value: 2.5
custom payload is a let payload: 1
```

The struct becomes a payload once `enable_let` is specialized.

### Example 12

Example 12 reads the payload through `get`, which is a const reference.

```cpp
#include <pluss/mic.h>
#include <pluss/tils/let.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // get returns a const reference to the payload. The reference is valid for as long as the let lives.
  tils::let names = pluss::make_note({"north", "south"});
  const auto& payload = names.get();
  std::cout << "payload count: " << payload.numel() << '\n';
  std::cout << "reference is const: " << std::is_const_v<std::remove_reference_t<decltype(payload)>> << '\n';
  return 0;
}
```

Output

```text
payload count: 2
reference is const: 1
```

The reference is const, so the payload is read only.

### Example 13

Example 13 reads the payload through `*` and `->`.

```cpp
#include <pluss/mic.h>
#include <pluss/tils/let.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // * and -> read the same payload that get returns, so either spelling reads the same numbers.
  tils::let values = pluss::make_series(std::vector<double>{1.0, 2.0, 3.0, 4.0}, {4});
  std::cout << "through *: " << (*values).numel() << '\n';
  std::cout << "through ->: " << values->numel() << '\n';
  return 0;
}
```

Output

```text
through *: 4
through ->: 4
```

Both read the same count.

### Example 14

Example 14 makes a copy of a binding.

```cpp
#include <pluss/mic.h>
#include <pluss/tils/let.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // A copy of a let gives a second binding that reads the same payload. Both names report the same count.
  tils::let first = pluss::make_note({"a", "b", "c"});
  tils::let second = first;
  std::cout << "first count: " << first.get().numel() << '\n';
  std::cout << "second count: " << second.get().numel() << '\n';
  return 0;
}
```

Output

```text
first count: 3
second count: 3
```

The copy reads the same count as the original.

### Example 15

Example 15 views a binding as a const payload.

```cpp
#include <pluss/mic.h>
#include <pluss/tils/let.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // A let converts implicitly to a const reference of its payload, which is what allows passing it on.
  tils::let names = pluss::make_note({"north", "south"});
  const pluss::Note& view = names;
  std::cout << "viewed count: " << view.numel() << '\n';
  return 0;
}
```

Output

```text
viewed count: 2
```

The conversion gives a const reference to the note.

## See Also

- [Value flow: let and suppose](core/value-flow.md)
