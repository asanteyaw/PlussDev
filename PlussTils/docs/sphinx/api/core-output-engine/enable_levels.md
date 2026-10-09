# `pluss::tils::OutputEngine::enable_levels`

[Back to `OutputEngine`](../core-output-engine.md) · [Core Utilities API Reference](../../api_reference.md)

```cpp
OutputEngine& enable_levels(bool enabled = true) noexcept;
```

`enable_levels` turns the level tag on or off for log lines. With the tag off, a log line is written as its text alone, and it is still filtered by the minimum level. Plain `write` and `writeln` calls never carry a tag, whatever this setting is.

## Parameters

This member takes no parameters.

| `enabled` | True writes level tags on log lines. False writes them without a tag. Default true. |

## Returns

The engine, so the call chains.

## Errors

None.

## Practical Examples

### Example 1

Example 1 turns the tags off and writes an info line.

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
  out.info("no tag on this line");
  return 0;
}
```

Output

```text
no tag on this line
```

The info line is written without its tag.

### Example 2

Example 2 turns the tags off, then on again.

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
  out.warn("untagged");
  out.enable_levels(true);
  out.warn("tagged again");
  return 0;
}
```

Output

```text
untagged
[WARNING] tagged again
```

The first warning has no tag, and the second has its tag again.

### Example 3

Example 3 shows that plain writes never carry a level.

```cpp
#include <pluss/tils/aux/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writeln("plain write has no level");
  out.enable_levels(true);
  out.writeln("still no level, because writeln is not a log call");
  return 0;
}
```

Output

```text
plain write has no level
still no level, because writeln is not a log call
```

A plain write is not a log call, so it has no tag whether levels are on or off.

## See Also

- [`OutputEngine`](../core-output-engine.md)
