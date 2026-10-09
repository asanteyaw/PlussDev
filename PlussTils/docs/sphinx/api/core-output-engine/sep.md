# `pluss::tils::OutputEngine::sep`

[Back to `OutputEngine`](../core-output-engine.md) · [Core Utilities API Reference](../../api_reference.md)

```cpp
OutputEngine& sep(std::string s);
```

`sep` sets the separator that joins the values of a `write` or `writeln` call. The default is a single space. The separator is engine state, so it applies to every later call until it is changed, and a per-call `PrintOptions` may override it for one call.

## Parameters

This member takes no parameters.

| `s` | The text placed between values. |

## Returns

The engine, so the call chains.

## Errors

None.

## Practical Examples

### Example 1

Example 1 writes three values with the default separator.

```cpp
#include <pluss/tils/aux/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writeln("a", "b", "c");
  return 0;
}
```

Output

```text
a b c
```

A single space joins the values.

### Example 2

Example 2 sets a longer separator and writes three names.

```cpp
#include <pluss/tils/aux/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.sep(" | ");
  out.writeln("north", "south", "east");
  return 0;
}
```

Output

```text
north | south | east
```

The separator appears between each pair of values.

### Example 3

Example 3 keeps the separator across calls.

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
  out.sep(", ");
  out.writeln(1, 2, 3);
  out.writeln(4, 5);
  std::cout << sink.str();
  return 0;
}
```

Output

```text
1, 2, 3
4, 5
```

Both lines use the separator set earlier, because it is engine state.

## See Also

- [`OutputEngine`](../core-output-engine.md)
