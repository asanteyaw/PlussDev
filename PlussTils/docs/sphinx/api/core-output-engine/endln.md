# `pluss::tils::OutputEngine::endln`

[Back to `OutputEngine`](../core-output-engine.md) · [Core Utilities API Reference](../../api_reference.md)

```cpp
OutputEngine& endln();
```

`endln` writes a newline and nothing else. It ends a line that was started with `write`, `writef`, or a chain of plain writes. It honors the auto-flush setting.

## Parameters

This member takes no parameters.


## Returns

The engine, so the call chains.

## Errors

None.

## Practical Examples

### Example 1

Example 1 ends a line that was started with write.

```cpp
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.write("partial");
  out.endln();
  out.writeln("whole");
  return 0;
}
```

Output

```text
partial
whole
```

The partial line is finished by endln.

### Example 2

Example 2 writes a newline on its own.

```cpp
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  std::ostringstream sink;
  OutputEngine out{sink};
  out.endln();
  std::cout << "bytes written by endln alone: " << sink.str().size() << '\n';
  return 0;
}
```

Output

```text
bytes written by endln alone: 1
```

A call with nothing else writes exactly one byte, the newline.

### Example 3

Example 3 ends a chained line and starts the next.

```cpp
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.write("row:").write(" ", 7).endln().writeln("next row");
  return 0;
}
```

Output

```text
row:  7
next row
```

The write adds a space and the value, and endln finishes the row.

## See Also

- [`OutputEngine`](../core-output-engine.md)
