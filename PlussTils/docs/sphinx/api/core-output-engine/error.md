# `pluss::tils::OutputEngine::error`

[Back to `OutputEngine`](../core-output-engine.md) · [Core Utilities API Reference](../../api_reference.md)

```cpp
template <class... Args>
OutputEngine& error(const Args&... args);
```

`error` writes its values as a log line of the error level, the highest. It is `log(OutputLevel::Error, ...)` with the level already chosen. The line is dropped when error is below the engine's minimum level.

## Parameters

This member takes no parameters.

| `args` | The values to write, joined by the separator. |

## Returns

The engine, so the call chains.

## Errors

None. A line below the minimum is dropped silently.

## Practical Examples

### Example 1

Example 1 writes a error line with a value.

```cpp
#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.error("message at error level", 7);
  return 0;
}
```

Output

```text
[ERROR] message at error level 7
```

The line carries the ERROR tag.

### Example 2

Example 2 sets a minimum level of error and writes a error line.

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
  out.error("this line is below the minimum or not");
  std::cout << "captured bytes: " << sink.str().size() << '\n';
  return 0;
}
```

Output

```text
captured bytes: 46
```

Nothing reaches the sink, so the captured length is zero.

### Example 3

Example 3 gives the error level its own prefix.

```cpp
#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.prefix(OutputLevel::Error, "[ERROR*]");
  out.error("with the custom prefix");
  return 0;
}
```

Output

```text
[ERROR*] with the custom prefix
```

The custom tag replaces the default for this level only.

## See Also

- [`OutputEngine`](../core-output-engine.md)
