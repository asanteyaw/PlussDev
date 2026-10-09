# `pluss::tils::OutputEngine::to`

[Back to `OutputEngine`](../core-output-engine.md) · [Core Utilities API Reference](../../api_reference.md)

```cpp
OutputEngine& to(std::ostream& os) noexcept;
```

`to` changes where the engine writes. Every later call writes to the new stream until the next `to`. Text that was already written stays in the old stream. The engine keeps a pointer, so the new stream must outlive the engine or the next `to` call.

## Parameters

This member takes no parameters.

| `os` | The new sink. |

## Returns

The engine, so the call chains.

## Errors

None. The call does not check the stream's state.

## Practical Examples

### Example 1

Example 1 redirects the engine to a second stream and back.

```cpp
#include <pluss/tils/aux/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  std::ostringstream first, second;
  OutputEngine out{first};
  out.writeln("to the first sink");
  out.to(second);
  out.writeln("to the second sink");
  std::cout << "first: " << first.str();
  std::cout << "second: " << second.str();
  return 0;
}
```

Output

```text
first: to the first sink
second: to the second sink
```

Each line lands in the stream that was current when it was written.

### Example 2

Example 2 shows that `to` returns the engine itself, so it chains.

```cpp
#include <pluss/tils/aux/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  std::ostringstream sink;
  OutputEngine out;
  OutputEngine& same = out.to(sink);
  std::cout << std::boolalpha << "returns the same engine: " << (&same == &out) << '\n';
  same.writeln("chained");
  std::cout << "sink holds: " << sink.str();
  return 0;
}
```

Output

```text
returns the same engine: true
sink holds: chained
```

The returned reference is the same engine, and the next write goes to the new sink.

### Example 3

Example 3 returns the engine to standard output after capturing.

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
  out.writeln("captured line");
  out.to(std::cout);
  out.writeln("back on standard output");
  std::cout << "sink holds: " << sink.str();
  return 0;
}
```

Output

```text
back on standard output
sink holds: captured line
```

The captured line stays in the string stream, and the later line reaches standard output.

## See Also

- [`OutputEngine`](../core-output-engine.md)
