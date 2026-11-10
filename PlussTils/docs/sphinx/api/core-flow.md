# `pluss::tils::flow` and `FlowBox`

[Back to Core Utilities](core.md) · [Core Utilities API Reference](../api_reference.md)

```cpp
#include <pluss/tils/flow.h>

template <class T>
class FlowBox;

template <class T>

inline constexpr FlowFn flow{};
```

`flow(x)` is the value-pipeline API. It returns `FlowBox<T>`. The separate [`suppose`](core-suppose.md) API checks named premises, derivations, and conclusions for a concrete Pluss value.

The box holds one whole value, including a whole container. `then(fn)` invokes `fn` once with that value and returns a new box holding the result; it does not map over a container's elements. `transform(fn)` is the same operation. `tap(fn)` invokes `fn` for an effect and returns the same box. `value()` and `unwrap()` expose the stored value, and `into<U>()` converts it. A callback receiving a mutable reference can modify an lvalue box, so `then` and `tap` are not inherently nonmutating.

## Parameters

| Parameter | Meaning |
|---|---|
| `value` (of `flow`) | Any value. The box stores a decayed copy, or moves the value in when it is an rvalue. |
| `To` (of `as`) | The type the new box holds. It must be constructible from the given value. |

## Returns

`flow(x)` returns a `FlowBox<std::decay_t<T>>`. `flow.as<To>(x)` returns a `FlowBox<To>`. The members return a box, a reference, or a value as described in each example.

## Errors

`into<U>` and `flow.as<To>` are checked at compile time: the conversion must be constructible, or the program does not compile.

## Practical Examples

### Example 1

Example 1 wraps a string and reads it back.

```cpp
#include <pluss/tils/flow.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // flow wraps any value, not only the core types, and the box returns the value it holds.
  auto text = tils::flow(std::string("abc"));
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

The box returns the held value, and its length is read from it.

### Example 2

Example 2 applies `then` to a box and keeps the original.

```cpp
#include <pluss/tils/flow.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // then runs the function on the held value and returns a new box. The original keeps its value.
  auto base = tils::flow(3);
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

The new box holds the doubled value, and the original still holds three.

### Example 3

Example 3 shows that `transform` and `then` produce the same box.

```cpp
#include <pluss/tils/flow.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // transform is the same operation under a second name, so either spelling gives the same box.
  auto base = tils::flow(std::string("ab"));
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

Both spellings give the same result.

### Example 4

Example 4 changes the type of the held value through `then`.

```cpp
#include <pluss/tils/flow.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // The result type follows the function. A string can become a length, a number, or anything else.
  auto text = tils::flow(std::string("hello"));
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

The result is a length, a `size_t`, so the type follows the function.

### Example 5

Example 5 runs `tap` for its effect and keeps the value.

```cpp
#include <pluss/tils/flow.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // tap runs a function on the held value and returns the same box, so the value is unchanged.
  auto items = tils::flow(std::vector<int>{1, 2, 3});
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

The side effect runs, and the value is the same afterwards.

### Example 6

Example 6 converts a copy with `into<int>`.

```cpp
#include <pluss/tils/flow.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // into<U> converts the held value to U and returns a box of U. The original is kept.
  const auto ratio = tils::flow(3.7);
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

The converted box holds the integer, and the original keeps its double.

### Example 7

Example 7 builds a box of a chosen type with `as`.

```cpp
#include <pluss/tils/flow.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // flow.as<To>(value) builds a box of To from a value of another type.
  auto from_text = tils::flow.as<std::string>("literal");
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

The box holds the string, built from a literal.

### Example 8

Example 8 moves the value out with `unwrap` on a box that is about to end.

```cpp
#include <pluss/tils/flow.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // Calling unwrap on a box that is about to end moves the value out, so no copy is made.
  auto items = tils::flow(std::vector<int>{4, 5, 6});
  std::vector<int> taken = std::move(items).unwrap();
  std::cout << "taken size: " << taken.size() << '\n';
  return 0;
}
```

Output

```text
taken size: 3
```

The vector is moved out, and its elements are intact.

### Example 9

Example 9 edits the held value through `value()` on an lvalue box.

```cpp
#include <pluss/tils/flow.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // value() on an lvalue box returns a reference to the held value, so a change through it is kept.
  auto counter = tils::flow(10);
  counter.value() += 5;
  std::cout << "held value after an edit: " << counter.value() << '\n';
  return 0;
}
```

Output

```text
held value after an edit: 15
```

The edit is kept, because `value()` returns a reference to the held value.

### Example 10

Example 10 reads the value of a const box.

```cpp
#include <pluss/tils/flow.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // A const box gives a const reference, so the value can be read but not changed through it.
  const auto fixed = tils::flow(std::string("read only"));
  std::cout << "const reference type: " << std::is_const_v<std::remove_reference_t<decltype(fixed.value())>> << '\n';
  std::cout << "read value: " << fixed.value() << '\n';
  return 0;
}
```

Output

```text
const reference type: 1
read value: read only
```

The reference is const, so the value is read only.

### Example 11

Example 11 pushes through `unwrap` on an lvalue box.

```cpp
#include <pluss/tils/flow.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // unwrap on an lvalue box returns a reference, like value(), so the box keeps owning the value.
  auto items = tils::flow(std::vector<int>{1, 2});
  items.unwrap().push_back(3);
  std::cout << "size after push through unwrap: " << items.value().size() << '\n';
  return 0;
}
```

Output

```text
size after push through unwrap: 3
```

The change is visible through the box, because `unwrap` on an lvalue returns a reference.

### Example 12

Example 12 applies `then` to a const box.

```cpp
#include <pluss/tils/flow.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // then works on a const box too. The function receives the held value as a const reference.
  const auto word = tils::flow(std::string("data"));
  auto upper_size = word.then([](const std::string& s) { return s.size() * 2; });
  std::cout << "const box then result: " << upper_size.value() << '\n';
  std::cout << "const box still holds: " << word.value() << '\n';
  return 0;
}
```

Output

```text
const box then result: 8
const box still holds: data
```

The const box keeps its value, and the new box holds the result.

### Example 13

Example 13 runs `tap` on a const box.

```cpp
#include <pluss/tils/flow.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // tap also works on a const box, because it only reads the held value for its side effect.
  const auto words = tils::flow(std::vector<std::string>{"a", "bb"});
  words.tap([](const std::vector<std::string>& v) { std::cout << "const tap count: " << v.size() << '\n'; });
  std::cout << "const box after tap: " << words.value().size() << '\n';
  return 0;
}
```

Output

```text
const tap count: 2
const box after tap: 2
```

The tap sees the value, and the const box is unchanged.

### Example 14

Example 14 chains `tap` and then.

```cpp
#include <pluss/tils/flow.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // A chain can mix tap and then. Each tap sees the value, and the final then decides the result.
  auto start = tils::flow(3);
  auto result = start.tap([](const int& x) { std::cout << "seen: " << x << '\n'; })
                     .then([](const int& x) { return x * x; });
  std::cout << "squared: " << result.value() << '\n';
  return 0;
}
```

Output

```text
seen: 3
squared: 9
```

The tap sees the value first, and the final step decides the result.

### Example 15

Example 15 transforms a temporary box, which moves the value.

```cpp
#include <pluss/tils/flow.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // transform on a temporary box moves the value into the function, so a large value is not copied.
  auto moved = tils::flow(std::vector<int>{1, 2, 3}).transform([](std::vector<int> v) { v.push_back(4); return v; });
  std::cout << "size after transform on a temporary: " << moved.value().size() << '\n';
  return 0;
}
```

Output

```text
size after transform on a temporary: 4
```

The function adds an element to the moved vector, and the result has four.

### Example 16

Example 16 converts a temporary box with `into`.

```cpp
#include <pluss/tils/flow.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // into on a temporary box converts by moving, so the source value is not copied first.
  auto text = tils::flow(std::string("12345")).into<std::string>();
  std::cout << "moved into a box of the same type: " << text.value() << '\n';
  return 0;
}
```

Output

```text
moved into a box of the same type: 12345
```

The conversion moves the string into the new box.

### Example 17

Example 17 builds a double from an integer with `as`.

```cpp
#include <pluss/tils/flow.h>

#include <iostream>
#include <string>
#include <type_traits>
#include <vector>

namespace tils = pluss::tils;

int main() {
  // as<To> builds a box of To from a value of another type, here an integer read as a double.
  auto ratio = tils::flow.as<double>(7);
  std::cout << "as double: " << ratio.value() << '\n';
  std::cout << "type is double: " << std::is_same_v<decltype(ratio.value()), double&> << '\n';
  return 0;
}
```

Output

```text
as double: 7
type is double: 1
```

The box holds the double, and its type is double.

## See Also

- [Value flow: flow and let](core/value-flow.md)
