# `pluss::tils::OutputEngine`

[Back to Core Utilities](core.md) · [Core Utilities API Reference](../api_reference.md)

```cpp
#include <pluss/tils/aux/output.h>

class OutputEngine;
```

`OutputEngine` writes text to a stream. It can write plain values, formatted lines, and level-tagged log lines, and it applies the engine's separator, level prefixes, minimum level, and default print options to every line. Each engine owns one sink and is not copyable, so two engines cannot share one lock or one configuration by accident. Calls return the engine, so they chain. The engine is safe to share between threads, because each call takes an internal lock.

## Parameters

This member takes no parameters.

| `std::ostream& os` | The sink. The engine keeps a pointer to it, so the stream must outlive the engine. |
| `OutputOptions opts` | The initial separator, level prefixes, minimum level, and flush behavior. |

## Returns

Constructors return an engine. Most members return `OutputEngine&` so calls chain.

## Errors

A format call whose pattern and arguments disagree is refused when strict argument checking is on. Without it, a missing argument prints its marker, and an extra argument is ignored.

## Practical Examples

### Example 1

Example 1 writes with the default constructor, which uses standard output.

```cpp
#include <pluss/tils/aux/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;  // writes to std::cout
  out.writeln("default engine writes", "to standard output");
  return 0;
}
```

Output

```text
default engine writes to standard output
```

The default engine writes to standard output with no configuration.

### Example 2

Example 2 writes into a string stream, which is how a program captures its own output.

```cpp
#include <pluss/tils/aux/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  std::ostringstream captured;
  OutputEngine out{captured};  // writes into the string stream
  out.writeln("held in the capture");
  std::cout << "captured text: " << captured.str();
  return 0;
}
```

Output

```text
captured text: held in the capture
```

The text is held in the stream, so the program can read it back.

### Example 3

Example 3 builds an engine from options that set a minimum level.

```cpp
#include <pluss/tils/aux/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  std::ostringstream captured;
  OutputOptions options;
  options.min_level = OutputLevel::Warn;  // info is dropped
  OutputEngine out{captured, options};
  out.info("dropped");
  out.warn("kept");
  std::cout << "captured: " << captured.str();
  return 0;
}
```

Output

```text
captured: [WARNING] kept
```

The info line is dropped, and the warning is kept.

### Example 4

Example 4 checks that an engine cannot be copied or reassigned.

```cpp
#include <pluss/tils/aux/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  std::cout << std::boolalpha;
  std::cout << "copy constructible: " << std::is_copy_constructible_v<OutputEngine> << '\n';
  std::cout << "copy assignable: " << std::is_copy_assignable_v<OutputEngine> << '\n';
  return 0;
}
```

Output

```text
copy constructible: false
copy assignable: false
```

Both checks are false, so each engine keeps its own sink and configuration.

## See Also

- [`OutputOptions`](core-output-options.md)
- [`OutputLevel`](core-output-level.md)
