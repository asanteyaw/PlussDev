# `pluss::tils::OutputEngine::format`

[Back to `OutputEngine`](../core-output-engine.md) · [Core Utilities API Reference](../../api_reference.md)

```cpp
template <class... Args>
static std::string format(std::string_view pattern, const Args&... args);
```

`format` renders a pattern with its arguments and returns the text. It uses the same brace grammar and the same value rendering as [`writef`](writef.md), but it writes nothing, so it is the member to use for error messages, labels, and any text that is built before it is written. It is static and uses fresh defaults, so no engine's configuration can change the text of a library message. `pluss::tils::format` is the same call as a free function.

## Parameters

| Parameter | Meaning |
|---|---|
| `pattern` | The pattern. `{}` or `{N}` take an argument, `{:spec}` adds a spec, and `{{` and `}}` are literal braces. A string literal is checked while compiling. |
| `args` | The values for the placeholders. A trailing `PrintOptions` is not a value: it applies to the call. |

A spec reads `[fill]align sign # 0 width .precision type`, where `align` is `<`, `>`, or `^`, `sign` is `+`, `-`, or a space, and `width` and `precision` are numbers or a nested `{}`. Integers take `d b B o x X c`, floating-point values take `e E f F g G a A`, and text takes `s` or `?`.

## Returns

The formatted `std::string`.

## Errors

A string-literal pattern whose placeholders and arguments disagree does not compile. The diagnostic names the problem: `format_string_error_missing_argument`, `format_string_error_unmatched_brace`, `format_string_error_bad_spec`, or `format_string_error_mixed_indexing`. A pattern wrapped in [`runtime_format`](../core-runtime-format.md) is checked when the call runs and throws `pluss::Error` for a malformed spec.

## Practical Examples

### Example 1

Example 1 formats two values into a string.

```cpp
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace pluss::tils;

int main() {
  const std::string line = OutputEngine::format("{} of {} done", 3, 10);
  std::cout << line << '\n';
  return 0;
}
```

Output

```text
3 of 10 done
```

The string comes back instead of being written, so it can go into an error message, a label, or a file name.

### Example 2

Example 2 shows the parts of a spec: fill and alignment, sign, base prefix, zero padding, width, precision, and type.

```cpp
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace pluss::tils;

int main() {
  std::cout << OutputEngine::format("[{:>8}] [{:<8}] [{:^8}] [{:*^8}]", "ab", "ab", "ab", "ab") << '\n';
  std::cout << OutputEngine::format("[{:+d}] [{:05d}] [{:#x}] [{:#b}] [{:o}]", 42, 42, 255, 5, 64) << '\n';
  std::cout << OutputEngine::format("[{:.3f}] [{:10.2e}] [{:g}] [{:+.4}]", 3.14159265, 12345.678, 0.0001, 2.0 / 3.0) << '\n';
  std::cout << OutputEngine::format("[{:.3}] [{:?}] [{:s}] [{}]", "truncate", "tab\there", true, 'c') << '\n';
  return 0;
}
```

Output

```text
[      ab] [ab      ] [   ab   ] [***ab***]
[+42] [00042] [0xff] [0b101] [100]
[3.142] [  1.23e+04] [0.0001] [+0.6667]
[tru] ["tab\there"] [true] [c]
```

A number aligns right by default and text aligns left. A zero flag pads after the sign and the base prefix. A precision on text cuts it to that many characters.

### Example 3

Example 3 refers to arguments by position and takes a width and a precision from other arguments.

```cpp
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace pluss::tils;

int main() {
  std::cout << OutputEngine::format("{1} {0} {1}", "a", "b") << '\n';
  std::cout << OutputEngine::format("[{:>{}}]", "ab", 6) << '\n';
  std::cout << OutputEngine::format("[{:.{}f}]", 3.14159265, 2) << '\n';
  std::cout << OutputEngine::format("[{0:{1}.{2}f}]", 3.14159265, 10, 3) << '\n';
  return 0;
}
```

Output

```text
b a b
[    ab]
[3.14]
[     3.142]
```

A nested `{}` reads the next argument as the width or precision, and `{N}` reads argument N. Mixing automatic and numbered references in one pattern does not compile.

### Example 4

Example 4 passes a trailing `PrintOptions`, which applies to the values of that call.

```cpp
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace pluss::tils;

int main() {
  const auto text = OutputEngine::format("{} units, {} revenue", 1234567, 2500.5, PrintOptions{}.fixed(2).grouped(','));
  std::cout << text << '\n';
  return 0;
}
```

Output

```text
1,234,567 units, 2,500.50 revenue
```

The options value is not an argument: it changes how the call renders, and it is not counted against the placeholders.

### Example 5

Example 5 applies one spec to every element of a list.

```cpp
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace pluss::tils;

int main() {
  const std::vector<double> xs{1.25, 2.5, 10.0};
  std::cout << OutputEngine::format("{:.1f}", xs) << '\n';
  std::cout << OutputEngine::format("{:.3f}", std::vector<double>{1, 2}) << '\n';
  return 0;
}
```

Output

```text
[1.2, 2.5, 10.0]
[1.000, 2.000]
```

A list renders in brackets with the number spec applied to each element.

### Example 6

Example 6 builds a pattern at run time, which needs `runtime_format`, and shows a malformed one failing with `pluss::Error`.

```cpp
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace pluss::tils;

std::string first_line(const char* what) {
  std::string s(what);
  return s.substr(0, s.find('\n'));
}

int main() {
  const std::string from_config = "{:>8.2f}";
  std::cout << OutputEngine::format(runtime_format(from_config), 3.14159) << '\n';
  try {
    const std::string bad = "{:q}";
    (void)OutputEngine::format(runtime_format(bad), 1);
  } catch (const pluss::Error& error) {
    std::cout << "refused: " << first_line(error.what()) << '\n';
  }
  return 0;
}
```

Output

```text
    3.14
refused: format: invalid spec 'q' (unknown type)
```

A literal pattern is checked while compiling, so a placeholder with no argument is a compile error. A pattern that arrives at run time is checked when the call runs.

## See Also

- [`join`](join.md)
- [`writef`](writef.md)
- [`runtime_format`](../core-runtime-format.md)
- [`PrintOptions`](../core-print-options.md)
