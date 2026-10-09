# `pluss::tils::OutputEngine::writef`

[Back to `OutputEngine`](../core-output-engine.md) · [Core Utilities API Reference](../../api_reference.md)

```cpp
template <class... Args>
OutputEngine& writef(std::string_view pattern, Args&&... args);
```

`writef` formats a pattern with its arguments and writes the result without a newline. Each `{}` takes the next argument, and a `{:spec}` takes a format specification for its argument. A trailing `PrintOptions` applies to the arguments of that call.

## Parameters

This member takes no parameters.

| `pattern` | The pattern, with `{}` placeholders and `{{` for a literal brace. |
| `args` | The values for the placeholders. |

## Returns

The engine, so the call chains.

## Errors

A pattern whose placeholder count disagrees with the arguments is refused in strict mode, and otherwise prints the marker or ignores the extra argument.

## Practical Examples

### Example 1

Example 1 fills two placeholders.

```cpp
#include <pluss/tils/aux/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writef("{} of {} done", 3, 10);
  out.endln();
  return 0;
}
```

Output

```text
3 of 10 done
```

The arguments fill the placeholders in order.

### Example 2

Example 2 uses format specifications for width, alignment, and zero padding.

```cpp
#include <pluss/tils/aux/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writef("[{:>6}] [{:<6}] [{:06.2f}]", "right", "left", 3.14159);
  out.endln();
  return 0;
}
```

Output

```text
[ right] [left  ] [003.14]
```

Each placeholder carries its own alignment and width.

### Example 3

Example 3 passes a trailing options value with the argument.

```cpp
#include <pluss/tils/aux/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writef("value {}", 1234567.5, PrintOptions{}.fixed(2).grouped(','));
  out.endln();
  return 0;
}
```

Output

```text
value 1,234,567.50
```

The number is grouped and rounded to two decimals.

## See Also

- [`OutputEngine`](../core-output-engine.md)
