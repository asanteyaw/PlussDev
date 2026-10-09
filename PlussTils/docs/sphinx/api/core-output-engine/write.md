# `pluss::tils::OutputEngine::write`

[Back to `OutputEngine`](../core-output-engine.md) · [Core Utilities API Reference](../../api_reference.md)

```cpp
template <class... Args>
OutputEngine& write(const Args&... args);
```

`write` writes its values joined by the separator, with no newline at the end. Use it to build a line in several calls, and finish the line with `endln` or a `writeln`.

## Parameters

This member takes no parameters.

| `args` | The values to write. A trailing `PrintOptions` applies to the whole call. |

## Returns

The engine, so the call chains.

## Errors

None.

## Practical Examples

### Example 1

Example 1 writes a line in two calls and ends it with endln.

```cpp
#include <pluss/tils/aux/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.write("no newline here, ");
  out.write("the rest continues");
  out.endln();
  return 0;
}
```

Output

```text
no newline here, the rest continues
```

Two calls make one line, and endln ends it.

### Example 2

Example 2 writes three values joined by the separator.

```cpp
#include <pluss/tils/aux/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.write(1, 2, 3);
  out.endln();
  return 0;
}
```

Output

```text
1 2 3
```

The values are separated by a single space, and no newline is added.

### Example 3

Example 3 chains several writes in one statement.

```cpp
#include <pluss/tils/aux/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.write("a").write("b").write("c").endln();
  return 0;
}
```

Output

```text
abc
```

The three strings run together, because nothing is added between them.

## See Also

- [`OutputEngine`](../core-output-engine.md)
