# `pluss::tils::OutputEngine::flush`

[Back to `OutputEngine`](../core-output-engine.md) · [Core Utilities API Reference](../../api_reference.md)

```cpp
OutputEngine& flush();
```

`flush` pushes the text the engine has written to its sink. Use it before a long operation, or when the output must be visible before the program continues. The option `flush` on a single call does the same for that call.

## Parameters

This member takes no parameters.


## Returns

The engine, so the call chains.

## Errors

None.

## Practical Examples

### Example 1

Example 1 flushes in the middle of a chain.

```cpp
#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  std::ostringstream sink;
  OutputEngine out{sink};
  out.write("pending").flush().write(" then more").endln();
  std::cout << sink.str();
  return 0;
}
```

Output

```text
pending then more
```

The flush returns the engine, so the chain continues.

### Example 2

Example 2 checks that text written before a flush is in the sink.

```cpp
#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  std::ostringstream sink;
  OutputEngine out{sink};
  out.writeln("line one");
  out.flush();
  std::cout << "sink after flush: " << sink.str();
  return 0;
}
```

Output

```text
sink after flush: line one
```

The sink holds the line after the flush.

### Example 3

Example 3 flushes through a per-call option.

```cpp
#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  std::ostringstream sink;
  OutputEngine out{sink};
  out.writeln("flushed by option", PrintOptions{.flush = true});
  std::cout << "sink holds: " << sink.str();
  return 0;
}
```

Output

```text
sink holds: flushed by option
```

The option flushes after this one call, and the sink holds the line.

## See Also

- [`OutputEngine`](../core-output-engine.md)
