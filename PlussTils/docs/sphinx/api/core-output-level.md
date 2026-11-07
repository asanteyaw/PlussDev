# `pluss::tils::OutputLevel`

[Back to Core Utilities](core.md) · [Core Utilities API Reference](../api_reference.md)

```cpp
enum class OutputLevel : std::size_t {
  Trace = 0,
  Debug = 1,
  Info = 2,
  Warn = 3,
  Error = 4
};
```

`OutputLevel` ranks a log line. Levels are ordered from trace, the least severe, to error, the most. The minimum level of an engine drops every line below it, so ordering is what makes filtering work. The default prefix of each level is shown by the default prefixes of `OutputOptions`.

## Parameters

`OutputLevel` has no parameters. It is an enumeration.

## Returns

A value of `OutputLevel`. Comparison follows the order above.

## Errors

None.

## Practical Examples

### Example 1

Example 1 prints the number of each level.

```cpp
#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  std::cout << "trace: " << static_cast<std::size_t>(OutputLevel::Trace) << '\n';
  std::cout << "debug: " << static_cast<std::size_t>(OutputLevel::Debug) << '\n';
  std::cout << "info: " << static_cast<std::size_t>(OutputLevel::Info) << '\n';
  std::cout << "warn: " << static_cast<std::size_t>(OutputLevel::Warn) << '\n';
  std::cout << "error: " << static_cast<std::size_t>(OutputLevel::Error) << '\n';
  return 0;
}
```

Output

```text
trace: 0
debug: 1
info: 2
warn: 3
error: 4
```

The numbers run from zero for trace to four for error.

### Example 2

Example 2 compares levels to show how filtering is decided.

```cpp
#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  std::cout << std::boolalpha;
  std::cout << "warn is at least info: " << (OutputLevel::Warn > OutputLevel::Info) << '\n';
  std::cout << "debug is at least error: " << (OutputLevel::Debug >= OutputLevel::Error) << '\n';
  return 0;
}
```

Output

```text
warn is at least info: true
debug is at least error: false
```

A warning ranks above info, and a debug line does not reach error.

### Example 3

Example 3 prints the default prefix each level uses.

```cpp
#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  const OutputOptions options;
  for (const auto& prefix : options.level_prefixes) std::cout << prefix << '\n';
  return 0;
}
```

Output

```text
[TRACE]
[DEBUG]
[INFO]
[WARNING]
[ERROR]
```

Each level has a bracketed tag, and the warning tag is written in full.

## See Also

- [`OutputEngine::log`](core-output-engine/log.md)
- [`OutputOptions`](core-output-options.md)
