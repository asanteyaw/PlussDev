# `pluss::tils::OutputEngine::writelnf`

[Back to `OutputEngine`](../core-output-engine.md) · [Core Utilities API Reference](../../api_reference.md)

```cpp
template <class... Args>
OutputEngine& writelnf(std::string_view pattern, Args&&... args);
```

`writelnf` formats a pattern with its arguments and writes the result followed by a newline. It is the usual way to write a table row, because each placeholder can carry its own width and alignment. Placeholders may name an argument by index, so an argument can be used more than once.

## Parameters

This member takes no parameters.

| `pattern` | The pattern, with `{}`, `{N}`, and format specifications. |
| `args` | The values for the placeholders. |

## Returns

The engine, so the call chains.

## Errors

A pattern whose placeholder count disagrees with the arguments is refused in strict mode, and otherwise prints the marker or ignores the extra argument.

## Practical Examples

### Example 1

Example 1 writes two aligned rows.

```cpp
#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writelnf("{:<8}{:>6}", "north", 12);
  out.writelnf("{:<8}{:>6}", "east", 140);
  return 0;
}
```

Output

```text
north       12
east       140
```

The first column is left-aligned, and the second is right-aligned.

### Example 2

Example 2 uses explicit indexes to reorder and repeat arguments.

```cpp
#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writelnf("{1} before {0}", "first", "second");
  out.writelnf("{0} repeated: {0}", "again");
  return 0;
}
```

Output

```text
second before first
again repeated: again
```

The index picks the argument, so the order in the pattern can differ from the order of the values.

### Example 3

Example 3 leaves a placeholder without an argument.

```cpp
#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writelnf("{} and {}", "only one", PrintOptions{.missing_arg = "<?>"});
  return 0;
}
```

Output

```text
only one and <?>
```

### Example 4

Example 4 passes one more argument than the pattern uses. Without strict checking, the extra value is not written.

```cpp
#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out{std::cout};
  // Without strict checking, an extra argument is not written, and the line is otherwise complete.
  out.writelnf("order {} shipped to {}", 1001, "Lagos", "express");
  return 0;
}
```

Output

```text
order 1001 shipped to Lagos
```

The line holds only the two values the pattern names, so the extra value is dropped without an error.

The missing value prints the marker given in the options.

## See Also

- [`OutputEngine`](../core-output-engine.md)
