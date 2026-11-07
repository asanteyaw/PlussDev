# `pluss::tils::OutputEngine::debug`

[Back to `OutputEngine`](../core-output-engine.md) · [Core Utilities API Reference](../../api_reference.md)

```cpp
template <class... Args>
OutputEngine& debug(const Args&... args);
```

`debug` writes its values as a log line of the debug level. It is `log(OutputLevel::Debug, ...)` with the level already chosen. The line is dropped when debug is below the engine's minimum level.

## Parameters

This member takes no parameters.

| `args` | The values to write, joined by the separator. |

## Returns

The engine, so the call chains.

## Errors

None. A line below the minimum is dropped silently.

## Practical Examples

### Example 1

Example 1 writes a debug line with a value.

```cpp
#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.debug("message at debug level", 7);
  return 0;
}
```

Output

```text
[DEBUG] message at debug level 7
```

The line carries the DEBUG tag.

### Example 2

Example 2 sets a minimum level of error and writes a debug line.

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
  out.debug("this line is below the minimum or not");
  std::cout << "captured bytes: " << sink.str().size() << '\n';
  return 0;
}
```

Output

```text
captured bytes: 0
```

Nothing reaches the sink, so the captured length is zero.

### Example 3

Example 3 gives the debug level its own prefix.

```cpp
#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.prefix(OutputLevel::Debug, "[DEBUG*]");
  out.debug("with the custom prefix");
  return 0;
}
```

Output

```text
[DEBUG*] with the custom prefix
```

The custom tag replaces the default for this level only.

## See Also

- [`OutputEngine`](../core-output-engine.md)
