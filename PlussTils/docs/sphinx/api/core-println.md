# `pluss::tils::println`

[Core Utilities API Reference](../api_reference.md) · [Output](core/output.md)

```cpp
template <class... Args>
void println(std::string_view pattern, const Args&... args);

template <class... Args>
void println(std::ostream& os, std::string_view pattern, const Args&... args);
```

`println` formats a pattern and writes the text followed by a newline, to standard output or to a stream. It is [`print`](core-print.md) with the newline added.

## Parameters

| Parameter | Meaning |
|---|---|
| `os` | The destination stream. Without one, text goes to `std::cout`. |
| `pattern` | The pattern, checked while compiling when it is a string literal. |
| `args` | The values for the placeholders. |

## Returns

Nothing.

## Errors

A literal pattern that disagrees with its arguments does not compile. A [`runtime_format`](core-runtime-format.md) pattern with a malformed spec throws `pluss::Error`.

## Practical Examples

### Example 1

Example 1 prints one line to standard output.

```cpp
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace pluss::tils;

int main() {
  println("{} of {} done", 3, 10);
  return 0;
}
```

Output

```text
3 of 10 done
```

`println` adds the newline after the formatted text.

### Example 2

Example 2 prints a line into a stream.

```cpp
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace pluss::tils;

int main() {
  std::ostringstream captured;
  println(captured, "{:<6}|{:>6}|", "left", "right");
  std::cout << captured.str();
  return 0;
}
```

Output

```text
left  | right|
```

The line ends with a newline.

### Example 3

Example 3 prints two lines in a row.

```cpp
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace pluss::tils;

int main() {
  println("first {}", 1);
  println("second {:.1f}", 2.5);
  return 0;
}
```

Output

```text
first 1
second 2.5
```

Each call is one line.

## See Also

- [`print`](core-print.md)
- [`OutputEngine::writelnf`](core-output-engine/writelnf.md)
