# `pluss::tils::Sign`

[Back to Core Utilities](core.md) · [Core Utilities API Reference](../api_reference.md)

`Sign` decides whether a positive number shows a sign. `NegativeOnly` is the default: only negative numbers show a minus. `Always` adds a plus to positive numbers. `SpaceForPositive` writes a space in place of a sign on positive numbers, which keeps positive and negative numbers aligned by their digits.

## Values

| Value | Meaning |
|---|---|
| `Inherit` | Use the engine baseline or built-in sign policy. |
| `NegativeOnly` | Show a minus only for negative numbers. Default. |
| `Always` | Also show a plus for positive numbers. |
| `SpaceForPositive` | Use a leading space for positive numbers to align digits with signed negatives. |

## Practical Examples

### Example 1

Example 1 shows all three styles on a positive and a negative number.

```cpp
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writeln("NegativeOnly:", 5, -5, PrintOptions{.sign = Sign::NegativeOnly});
  out.writeln("Always:", 5, -5, PrintOptions{.sign = Sign::Always});
  out.writeln("SpaceForPositive:", 5, -5, PrintOptions{.sign = Sign::SpaceForPositive});
  return 0;
}
```

Output

```text
NegativeOnly: 5 -5
Always: +5 -5
SpaceForPositive:  5 -5
```

Only Always and SpaceForPositive mark the positive number; NegativeOnly leaves it bare.

## See Also

- [Number formatting](core-print-options/numbers.md)
