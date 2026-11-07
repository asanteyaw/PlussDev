# `pluss::tils::OutputEngine::writeln`

[Back to `OutputEngine`](../core-output-engine.md) · [Core Utilities API Reference](../../api_reference.md)

```cpp
template <class... Args>
OutputEngine& writeln(const Args&... args);
```

`writeln` writes its values joined by the separator and ends the line with a newline. A trailing `PrintOptions` applies to the whole call, and its end text is written before the newline. A call with no values writes a blank line.

## Parameters

This member takes no parameters.

| `args` | The values to write. A trailing `PrintOptions` applies to the whole call. |

## Returns

The engine, so the call chains.

## Errors

None.

## Practical Examples

### Example 1

Example 1 writes two lines, the first with three values.

```cpp
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writeln("loaded", 3, "files");
  out.writeln("next line");
  return 0;
}
```

Output

```text
loaded 3 files
next line
```

Each call ends its own line.

### Example 2

Example 2 adds end text through a trailing options value.

```cpp
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writeln("total:", 42, PrintOptions{}.with_end(" (estimated)"));
  return 0;
}
```

Output

```text
total: 42 (estimated)
```

The end text is written before the newline.

### Example 3

Example 3 calls writeln with no values.

```cpp
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writeln("before");
  out.writeln();
  out.writeln("after");
  return 0;
}
```

Output

```text
before

after
```

The empty call writes a blank line between the other two.

## See Also

- [`OutputEngine`](../core-output-engine.md)
