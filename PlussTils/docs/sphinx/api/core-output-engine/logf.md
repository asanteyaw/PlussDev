# `pluss::tils::OutputEngine::logf`

[Back to `OutputEngine`](../core-output-engine.md) · [Core Utilities API Reference](../../api_reference.md)

```cpp
template <class... Args>
OutputEngine& logf(OutputLevel level, std::string_view pattern, Args&&... args);
```

`logf` formats a pattern and writes the result as a log line of the given level. It filters by the minimum level in the same way `log` does, and a trailing `PrintOptions` applies to the arguments.

## Parameters

This member takes no parameters.

| `level` | The level of the line. |
| `pattern` | The pattern, with `{}` placeholders. |
| `args` | The values for the placeholders. |

## Returns

The engine, so the call chains.

## Errors

A pattern whose placeholder count disagrees with the arguments is refused in strict mode.

## Practical Examples

### Example 1

Example 1 writes a formatted warning.

```cpp
#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.logf(OutputLevel::Warn, "retry {} of {} after {} ms", 2, 5, 400);
  return 0;
}
```

Output

```text
[WARNING] retry 2 of 5 after 400 ms
```

The placeholders fill in order, and the line carries the warning tag.

### Example 2

Example 2 sets a minimum level of error and logs an info line and an error line.

```cpp
#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  std::ostringstream sink;
  OutputOptions options;
  options.min_level = OutputLevel::Error;
  OutputEngine out{sink, options};
  out.logf(OutputLevel::Info, "dropped {}", 1);
  out.logf(OutputLevel::Error, "kept {}", 2);
  std::cout << sink.str();
  return 0;
}
```

Output

```text
[ERROR] kept 2
```

The info line is dropped, and the error line is kept.

### Example 3

Example 3 passes a trailing options value with a percentage.

```cpp
#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.logf(OutputLevel::Info, "disk {} used", 0.92, PrintOptions{.notation = Notation::Percent, .precision = 0});
  return 0;
}
```

Output

```text
[INFO] disk 92% used
```

The value prints as a percentage with no decimals.

## See Also

- [`OutputEngine`](../core-output-engine.md)
