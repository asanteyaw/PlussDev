# `pluss::tils::OutputEngine::join`

[Back to `OutputEngine`](../core-output-engine.md) · [Core Utilities API Reference](../../api_reference.md)

```cpp
template <std::ranges::input_range R>
static std::string join(const R& range, std::string_view sep);
```

`join` renders every element of a range and returns the pieces separated by `sep`. Elements render with the engine's default value rendering, the same as [`write`](write.md). `pluss::tils::join` is the same call as a free function.

## Parameters

| Parameter | Meaning |
|---|---|
| `range` | Any input range of renderable values. |
| `sep` | The text placed between elements. |

## Returns

The joined `std::string`. An empty range returns an empty string.

## Errors

This member throws nothing of its own.

## Practical Examples

### Example 1

Example 1 joins integers with a comma and a space.

```cpp
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace pluss::tils;

int main() {
  const std::vector<int> xs{1, 2, 3};
  std::cout << OutputEngine::join(xs, ", ") << '\n';
  return 0;
}
```

Output

```text
1, 2, 3
```

Each element is rendered the way `write` renders it, and the separator goes between elements only.

### Example 2

Example 2 builds the bracketed list that an error message often needs.

```cpp
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace pluss::tils;

int main() {
  const std::vector<std::string> names{"id", "city", "sales"};
  std::cout << OutputEngine::format("columns: [{}]", OutputEngine::join(names, ", ")) << '\n';
  return 0;
}
```

Output

```text
columns: [id, city, sales]
```

`join` returns a string, so it nests inside a `format` call.

### Example 3

Example 3 covers the empty and the single-element range.

```cpp
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace pluss::tils;

int main() {
  std::cout << '[' << OutputEngine::join(std::vector<int>{}, ", ") << "]\n";
  std::cout << '[' << OutputEngine::join(std::vector<double>{2.5}, ", ") << "]\n";
  std::cout << '[' << OutputEngine::join(std::vector<bool>{true, false}, "|") << "]\n";
  return 0;
}
```

Output

```text
[]
[2.5]
[true|false]
```

An empty range gives an empty string, and one element gives no separator.

## See Also

- [`format`](format.md)
- [`write`](write.md)
