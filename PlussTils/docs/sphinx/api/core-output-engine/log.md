# `pluss::tils::OutputEngine::log`

[Back to `OutputEngine`](../core-output-engine.md) · [Core Utilities API Reference](../../api_reference.md)

```cpp
template <class... Args>
OutputEngine& log(OutputLevel level, const Args&... args);
```

`log` writes its values as a log line of the given level. The line gets the level's tag, unless tags are disabled, and it is dropped without a trace when its level is below the minimum level of the engine.

## Parameters

This member takes no parameters.

| `level` | The level of the line. |
| `args` | The values to write, joined by the separator. |

## Returns

The engine, so the call chains.

## Errors

None. A line below the minimum is dropped silently.

## Practical Examples

### Example 1

Example 1 writes an info line and an error line.

```cpp
#include <pluss/tils/aux/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.log(OutputLevel::Info, "service started on port", 8080);
  out.log(OutputLevel::Error, "could not bind");
  return 0;
}
```

Output

```text
[INFO] service started on port 8080
[ERROR] could not bind
```

Each line carries its level's tag.

### Example 2

Example 2 sets a minimum level of warning and logs at debug and warning.

```cpp
#include <pluss/tils/aux/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  std::ostringstream sink;
  OutputOptions options;
  options.min_level = OutputLevel::Warn;
  OutputEngine out{sink, options};
  out.log(OutputLevel::Debug, "dropped");
  out.log(OutputLevel::Warn, "kept");
  std::cout << sink.str();
  return 0;
}
```

Output

```text
[WARNING] kept
```

The debug line is dropped, and the warning is kept.

### Example 3

Example 3 turns the tags off and logs an info line.

```cpp
#include <pluss/tils/aux/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.enable_levels(false);
  out.log(OutputLevel::Info, "level text removed");
  return 0;
}
```

Output

```text
level text removed
```

The text is written without a tag.

## See Also

- [`OutputEngine`](../core-output-engine.md)
