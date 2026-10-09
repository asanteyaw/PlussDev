# PlussTils: Values in pipelines, suppose and let

```{raw} html
<meta name="pluss-doc-kind" content="guide">
```

[Back to PlussTils](../index.md) · [PlussTils Core Manual](core.md)

Two small utilities wrap a value so that you can work with it in steps. `suppose` wraps any value and lets you chain changes to it. `let` binds one of the core payload types, a series, a note, a timeline, or a duration, and refuses to rebind it. Use `suppose` when you are shaping a value with a sequence of steps, and `let` when a value should stay fixed once it is named.

## The mental model

Both utilities are boxes. You put a value in, and the box gives it back through `value()`, `get()`, or `unwrap()`. The difference is what the box allows. A `suppose` box accepts any type, and every step returns a new box, so the one you started with never changes. A `let` box accepts only the payload types it was built for, and it never accepts a new value: once bound, the name always refers to the same payload.

## Worked tour

Every example is a complete program, and the output under it is copied from that program's run.

## Example 1: suppose wraps any value

A `suppose` box holds any type, not only the core types. This example wraps a string and reads it back, along with its length.

```cpp
#include <pluss/tils/suppose.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // suppose wraps any value, not only the core types, and the box returns the value it holds.
  auto text = tils::suppose(std::string("abc"));
  std::cout << "wrapped text: " << text.value() << '\n';
  std::cout << "wrapped length: " << text.value().size() << '\n';
  return 0;
}
```

Output

```text
wrapped text: abc
wrapped length: 3
```

The box returns the string unchanged, and the length is read from the value it holds.

## Example 2: then returns a new box

`then` runs a function on the held value and returns a new box with the result. The starting box is not touched. This example doubles a value and then reads both boxes.

```cpp
#include <pluss/tils/suppose.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // then runs the function on the held value and returns a new box. The original keeps its value.
  auto base = tils::suppose(3);
  auto doubled = base.then([](int x) { return x * 2; });
  std::cout << "original still holds: " << base.value() << '\n';
  std::cout << "new box holds: " << doubled.value() << '\n';
  return 0;
}
```

Output

```text
original still holds: 3
new box holds: 6
```

The original still holds three, and the new box holds six. Every step makes a new box, so the chain never changes what you started with.

## Example 3: transform is the same as then

`transform` is a second name for the same operation. This example applies each name to the same box and compares the results.

```cpp
#include <pluss/tils/suppose.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // transform is the same operation under a second name, so either spelling gives the same box.
  auto base = tils::suppose(std::string("ab"));
  auto via_then = base.then([](const std::string& s) { return s + "cd"; });
  auto via_transform = base.transform([](const std::string& s) { return s + "cd"; });
  std::cout << "then: " << via_then.value() << '\n';
  std::cout << "transform: " << via_transform.value() << '\n';
  return 0;
}
```

Output

```text
then: abcd
transform: abcd
```

Both produce abcd. Pick one name and use it everywhere in a chain.

## Example 4: then can change the type

The type of the new box follows the function's result. This example turns a string into its length, a `size_t`.

```cpp
#include <pluss/tils/suppose.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // The result type follows the function. A string can become a length, a number, or anything else.
  auto text = tils::suppose(std::string("hello"));
  auto length = text.then([](const std::string& s) { return s.size(); });
  std::cout << "length type is size_t: " << std::is_same_v<decltype(length.value()), std::size_t&> << '\n';
  std::cout << "length: " << length.value() << '\n';
  return 0;
}
```

Output

```text
length type is size_t: 1
length: 5
```

The length is a `size_t`, read as five, so the type changed along with the value.

## Example 5: tap runs a side effect and keeps the value

`tap` runs a function on the held value for its effect, such as logging, and returns the same box. This example taps a vector and then reads its size.

```cpp
#include <pluss/tils/suppose.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // tap runs a function on the held value and returns the same box, so the value is unchanged.
  auto items = tils::suppose(std::vector<int>{1, 2, 3});
  items.tap([](const std::vector<int>& v) { std::cout << "tapped size: " << v.size() << '\n'; });
  std::cout << "value unchanged: " << items.value().size() << '\n';
  return 0;
}
```

Output

```text
tapped size: 3
value unchanged: 3
```

The tap printed the size, and the value still has three elements afterwards.

## Example 6: into converts a copy

`into<U>` converts the held value to `U` and returns a box of `U`. The original keeps its type and value. This example converts 3.7 to an integer.

```cpp
#include <pluss/tils/suppose.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // into<U> converts the held value to U and returns a box of U. The original is kept.
  const auto ratio = tils::suppose(3.7);
  const auto whole = ratio.into<int>();
  std::cout << "converted: " << whole.value() << '\n';
  std::cout << "original still holds: " << ratio.value() << '\n';
  return 0;
}
```

Output

```text
converted: 3
original still holds: 3.7
```

The converted box holds three, and the original still holds 3.7.

## Example 7: as builds a box from another type

`suppose.as<To>(value)` builds a box of type `To` from a value of another type. This example builds a string from a string literal.

```cpp
#include <pluss/tils/suppose.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // suppose.as<To>(value) builds a box of To from a value of another type.
  auto from_text = tils::suppose.as<std::string>("literal");
  std::cout << "built string: " << from_text.value() << '\n';
  std::cout << "built value is a std::string: " << std::is_same_v<decltype(from_text.value()), std::string&> << '\n';
  return 0;
}
```

Output

```text
built string: literal
built value is a std::string: 1
```

The box holds literal, and its type is `std::string`.

## Example 8: unwrap on a box that is about to end moves the value out

Calling `unwrap` on a box that you no longer need moves the value out without copying it. This example moves a vector out of its box.

```cpp
#include <pluss/tils/suppose.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // Calling unwrap on a box that is about to end moves the value out, so no copy is made.
  auto items = tils::suppose(std::vector<int>{4, 5, 6});
  std::vector<int> taken = std::move(items).unwrap();
  std::cout << "taken size: " << taken.size() << '\n';
  return 0;
}
```

Output

```text
taken size: 3
```

The vector has its three elements, and no copy was made.

## Example 9: let binds a series

A `let` binds a core payload. This example binds a series and reads its count through `get`.

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

## Example 10: map keeps the binding when the result is a payload

`map` applies a function to a `let`. When the result is itself a payload type, the result is a `let` again. This example doubles a series.

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

The result is still a `let`, so you can keep chaining.

## Example 11: map drops to the bare value otherwise

When the result of `map` is not a payload, the result is the bare value. This example counts the elements of a series.

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

The count is a plain integer, and the value is 3.

## Example 12: a let passes where a const payload is expected

A `let` converts implicitly to a constant reference of its payload, so a function that takes a const payload accepts it directly. This example passes one.

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

The function receives the payload and returns its count, two.

## Example 13: unwrap moves the payload out and spends the let

`unwrap` on a `let` that you no longer need moves its payload out. This example moves a note out of its binding.

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

The payload comes out with its two entries.

## Example 14: peek runs a function and keeps the binding

`peek` runs a function on the payload and returns the same binding, so calls can be chained. This example peeks twice and then reads the binding.

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

## Example 15: a let cannot be reassigned

Assignment to a `let` is removed, so the name cannot be pointed at another value. This example checks the traits that say so.

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

Both copy assignment and move assignment are unavailable, so the name always refers to the same payload.

## Example 16: only opted-in types are payloads

A `let` accepts only types that are marked as payloads. This example checks an integer and a series against that rule.

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

An integer is not a payload, and a series is.

## Example 17: timelines and durations are payloads too

Timelines are payloads, so they bind the same way as series. This example binds two dates.

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

The binding holds two dates.

## Example 18: a new type can join by opting in

A type outside the core set becomes a payload by specializing the opt-in trait. This example defines a small struct and opts it in.

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

The custom payload reads back its value, and the trait is true for it.

## Decisions

| You want | Use | Notes |
|---|---|---|
| A value you shape with several steps, of any type | `suppose` | Each step returns a new box, so the starting value is kept. |
| A core payload that must stay the same once named | `let` | The name cannot be reassigned, and the payload is read-only. |
| A plain value you only read once | plain `auto` | A box adds nothing when no step is chained. |
| A chain over a let that should stay a let | `map` returning a payload | A non-payload result drops to the bare value. |

## Pitfalls

**Expecting a suppose step to change the box you started with.** suppose_02 shows the starting box still holds three after the step.

**Expecting a let to be reassigned.** let_07 shows assignment is unavailable, so the name cannot be pointed at a new value.

**Expecting map on a let to always return a let.** let_03 shows a count comes back as a bare integer.

**Expecting any type to be accepted by let.** let_08 shows an integer is refused as a payload.

## Where next

- [SupposeBox and suppose](../api/core-suppose.md) -- the reference for the box and its operations.
- [let](../api/core-let.md) -- the reference for the binding and its payload rules.
- [PlussBI: A Guide](bi.md) -- where the core payloads are used to build dashboards.
