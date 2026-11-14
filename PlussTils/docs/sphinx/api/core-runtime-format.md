# `pluss::tils::runtime_format`

[Core Utilities API Reference](../api_reference.md) · [Output](core/output.md)

### `runtime_format` fields

| Field | Type | Default |
|---|---|---|
| `runtime_format` | `Method` | `explicit runtime_format(std::string_view text) noexcept` |

`runtime_format` wraps a pattern that is only known at run time, such as a format string read from a configuration file or supplied by a caller. Every formatting call accepts a string literal and checks it while compiling; a pattern held in a variable has to be wrapped so the call knows to check it when it runs instead. The wrapper does not copy the text, so the string must outlive the call.

## Parameters

| Parameter | Meaning |
|---|---|
| `text` | The pattern text. |

## Returns

A value to pass where a pattern goes: to [`format`](core-output-engine/format.md), [`print`](core-print.md), [`println`](core-println.md), `writef`, `writelnf`, and the level-tagged `...f` members.

## Errors

Constructing the wrapper throws nothing. The pattern is checked by the call it is passed to, and a malformed spec throws `pluss::Error`.

## Practical Examples

### Example 1

Example 1 formats with a pattern held in a variable.

```cpp
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace pluss::tils;

int main() {
  const std::string pattern = "{} -> {:.2f}";
  std::cout << OutputEngine::format(runtime_format(pattern), "pi", 3.14159) << '\n';
  return 0;
}
```

Output

```text
pi -> 3.14
```

The wrapper marks the pattern as one that is only known at run time, so the compiler does not try to check it.

### Example 2

Example 2 shows a missing argument in a run-time pattern, printed as the marker the call chooses.

```cpp
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace pluss::tils;

int main() {
  std::ostringstream captured;
  OutputEngine out{captured};
  const std::string pattern = "order {} shipped to {} on {}";
  out.writelnf(runtime_format(pattern), 1001, "Lagos", PrintOptions{.missing_arg = "<?>"});
  std::cout << captured.str();
  return 0;
}
```

Output

```text
order 1001 shipped to Lagos on <?>
```

A literal with the same three placeholders and two arguments would not compile. The run-time pattern is the case the marker is for.

### Example 3

Example 3 shows strict mode refusing a run-time pattern that leaves an argument unused.

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
  OutputEngine out{std::cout};
  const std::string pattern = "order {} shipped";
  try {
    out.writelnf(runtime_format(pattern), 1001, "Lagos", PrintOptions{.strict_args = true});
  } catch (const pluss::Error& error) {
    std::cout << "refused: " << first_line(error.what()) << '\n';
  }
  return 0;
}
```

Output

```text
refused: OutputEngine: format placeholder / argument count mismatch
```

The refusal is a `pluss::Error`, like every error the formatter raises at run time.

## See Also

- [`OutputEngine::format`](core-output-engine/format.md)
- [`OutputEngine::writef`](core-output-engine/writef.md)
