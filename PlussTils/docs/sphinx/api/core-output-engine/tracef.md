# `pluss::tils::OutputEngine::tracef`

[Back to `OutputEngine`](../core-output-engine.md) · [Core Utilities API Reference](../../api_reference.md)

```cpp
template <class... Args>
OutputEngine& tracef(std::string_view pattern, Args&&... args);
```

`tracef` formats a pattern and writes the result as a log line of the trace level, the lowest. It is `logf(OutputLevel::Trace, ...)` with the level already chosen, and it filters and tags in the same way.

## Parameters

This member takes no parameters.

| `pattern` | The pattern, with `{}` placeholders. |
| `args` | The values for the placeholders. |

## Returns

The engine, so the call chains.

## Errors

A pattern whose placeholder count disagrees with the arguments is refused in strict mode.

## Practical Examples

### Example 1

Example 1 writes a formatted trace line.

```cpp
#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.tracef("trace with {} and {}", 3, "args");
  return 0;
}
```

Output

```text
[TRACE] trace with 3 and args
```

The placeholders fill in order, and the line carries the tag.

### Example 2

Example 2 sets a minimum level of error and writes a formatted trace line.

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
  out.tracef("value {}", 9);
  std::cout << "captured bytes: " << sink.str().size() << '\n';
  return 0;
}
```

Output

```text
captured bytes: 0
```

The line is dropped, so the captured length is zero.

### Example 3

Example 3 gives the trace level its own prefix and writes a formatted line.

```cpp
#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.prefix(OutputLevel::Trace, "[TRACE*]");
  out.tracef("formatted {} with a custom prefix", 5);
  return 0;
}
```

Output

```text
[TRACE*] formatted 5 with a custom prefix
```

The formatted line uses the custom tag.

## See Also

- [`OutputEngine`](../core-output-engine.md)
