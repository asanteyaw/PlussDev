# `pluss::tils::OutputEngine::prefix`

[Back to `OutputEngine`](../core-output-engine.md) · [Core Utilities API Reference](../../api_reference.md)

```cpp
OutputEngine& prefix(OutputLevel level, std::string p);
```

`prefix` replaces the tag written in front of log lines of one level. Only that level changes. The defaults are `[TRACE]`, `[DEBUG]`, `[INFO]`, `[WARNING]`, and `[ERROR]`. The prefix is written only when level text is enabled.

## Parameters

This member takes no parameters.

| `level` | The level whose tag changes. |
| `p` | The new tag, written as given. |

## Returns

The engine, so the call chains.

## Errors

None.

## Practical Examples

### Example 1

Example 1 replaces the warning tag and leaves the info tag alone.

```cpp
#include <pluss/tils/aux/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.prefix(OutputLevel::Warn, "[!!]");
  out.warn("custom warning tag");
  out.info("info keeps its tag");
  return 0;
}
```

Output

```text
[!!] custom warning tag
[INFO] info keeps its tag
```

The warning carries the custom tag, and the info line keeps its default.

### Example 2

Example 2 replaces the error tag and writes a debug line, which keeps its default.

```cpp
#include <pluss/tils/aux/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  std::ostringstream sink;
  OutputEngine out{sink};
  out.prefix(OutputLevel::Error, "[FAIL]");
  out.error("failed");
  out.debug("debug uses the default");
  std::cout << sink.str();
  return 0;
}
```

Output

```text
[FAIL] failed
[DEBUG] debug uses the default
```

Only the error tag changed.

### Example 3

Example 3 applies a custom prefix to a formatted log line.

```cpp
#include <pluss/tils/aux/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.prefix(OutputLevel::Info, ">>");
  out.infof("loaded {} rows", 42);
  return 0;
}
```

Output

```text
>> loaded 42 rows
```

The formatted info line uses the custom tag.

## See Also

- [`OutputEngine`](../core-output-engine.md)
