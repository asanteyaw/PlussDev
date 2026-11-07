# `pluss::tils::OutputEngine::set_options`

[Back to `OutputEngine`](../core-output-engine.md) · [Core Utilities API Reference](../../api_reference.md)

```cpp
OutputEngine& set_options(OutputOptions opts);
```

`set_options` replaces the engine's whole configuration with an `OutputOptions` value: the separator, the level prefixes, whether levels are enabled, the minimum level, and the flush behavior. It is the bulk form of the individual setters, so a configuration can be built once and applied to several engines.

## Parameters

This member takes no parameters.

| `opts` | The configuration to apply. |

## Returns

The engine, so the call chains.

## Errors

None.

## Practical Examples

### Example 1

Example 1 applies an options value with a minimum level of error.

```cpp
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputOptions options;
  options.min_level = OutputLevel::Error;
  OutputEngine out;
  out.set_options(options);
  out.info("filtered out");
  out.error("kept");
  return 0;
}
```

Output

```text
[ERROR] kept
```

The info line is dropped, and the error line is kept.

### Example 2

Example 2 applies a separator through the options value.

```cpp
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputOptions options;
  options.separator = " / ";
  OutputEngine out;
  out.set_options(options);
  out.writeln("x", "y", "z");
  return 0;
}
```

Output

```text
x / y / z
```

The values are joined with the new separator.

### Example 3

Example 3 chains a write after applying options.

```cpp
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  OutputOptions options;
  options.auto_flush = true;
  out.set_options(options).writeln("chained after set_options");
  return 0;
}
```

Output

```text
chained after set_options
```

The call returns the engine, so the write follows in the same statement.

## See Also

- [`OutputEngine`](../core-output-engine.md)
