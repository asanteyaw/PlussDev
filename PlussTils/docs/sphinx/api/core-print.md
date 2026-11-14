# `pluss::tils::print`

[Core Utilities API Reference](../api_reference.md) · [Output](core/output.md)

```cpp
template <class... Args>
void print(std::string_view pattern, const Args&... args);

template <class... Args>
void print(std::ostream& os, std::string_view pattern, const Args&... args);

template <class... Args>
void print(std::FILE* file, std::string_view pattern, const Args&... args);
```

`print` formats a pattern and writes the text, without a newline, to standard output, to a stream, or to a C file handle. It is a free function for a one-off message. To write many lines to one sink with separators, levels, and shared defaults, use an [`OutputEngine`](core-output-engine.md).

## Parameters

| Parameter | Meaning |
|---|---|
| `os`, `file` | The destination. Without one, text goes to `std::cout`. |
| `pattern` | The pattern, checked while compiling when it is a string literal. |
| `args` | The values for the placeholders. |

## Returns

Nothing.

## Errors

A literal pattern that disagrees with its arguments does not compile. A [`runtime_format`](core-runtime-format.md) pattern with a malformed spec throws `pluss::Error`.

## Practical Examples

### Example 1

Example 1 prints to standard output with no engine.

```cpp
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace pluss::tils;

int main() {
  print("{} of {} done\n", 3, 10);
  return 0;
}
```

Output

```text
3 of 10 done
```

`print` writes exactly the formatted text, so the newline is part of the pattern.

### Example 2

Example 2 prints into a stream.

```cpp
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace pluss::tils;

int main() {
  std::ostringstream captured;
  print(captured, "[{:>6.2f}]", 3.14159);
  std::cout << captured.str() << '\n';
  return 0;
}
```

Output

```text
[  3.14]
```

The stream comes first, then the pattern.

### Example 3

Example 3 prints into a C file handle.

```cpp
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace pluss::tils;

int main() {
  print(stdout, "{}-{:03d}\n", "run", 7);
  return 0;
}
```

Output

```text
run-007
```

A `FILE*` works the same way, so a message can go to `stderr` without an engine.

## See Also

- [`println`](core-println.md)
- [`OutputEngine::format`](core-output-engine/format.md)
