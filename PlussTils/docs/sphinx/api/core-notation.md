# `pluss::tils::Notation`

[Back to Core Utilities](core.md) · [Core Utilities API Reference](../api_reference.md)

`Notation` chooses how a number is written. `Fixed`, `Scientific`, and `General` are the usual floating-point forms. `Hex`, `Octal`, and `Binary` write an integer in another base. `Percent` multiplies by 100 and appends a percent sign. `Metric` scales the number with an SI suffix, using the same scale `Humanize::Metric` uses; it only scales upward, so a value under one stays in base units.

## Values

| Value | Meaning |
|---|---|
| `Inherit` | Use the engine baseline or built-in numeric notation. |
| `Auto` | Choose a suitable form automatically. |
| `Fixed` | Use fixed-point decimal notation. |
| `Scientific` | Use scientific notation. |
| `General` | Use general floating-point notation. |
| `Hex` | Write an integer in base 16. |
| `Percent` | Multiply by 100 and append a percent sign. |
| `Metric` | Scale upward with an SI suffix. |
| `Binary` | Write an integer in base 2. |
| `Octal` | Write an integer in base 8. |

## Practical Examples

### Example 1

Example 1 shows every notation on a representative number.

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
  out.writeln("Auto:", 3.5, PrintOptions{.notation = Notation::Auto});
  out.writeln("Fixed:", 3.14159, PrintOptions{.notation = Notation::Fixed, .precision = 2});
  out.writeln("Scientific:", 123456.789, PrintOptions{.notation = Notation::Scientific, .precision = 2});
  out.writeln("General:", 123456.789, PrintOptions{.notation = Notation::General, .precision = 4});
  out.writeln("Hex:", 255, PrintOptions{.notation = Notation::Hex});
  out.writeln("Percent:", 0.1275, PrintOptions{.notation = Notation::Percent, .precision = 1});
  out.writeln("Metric:", 12345.0, PrintOptions{.notation = Notation::Metric});
  out.writeln("Binary:", 10, PrintOptions{.notation = Notation::Binary});
  out.writeln("Octal:", 64, PrintOptions{.notation = Notation::Octal});
  return 0;
}
```

Output

```text
Auto: 3.5
Fixed: 3.14
Scientific: 1.23e+05
General: 1.235e+05
Hex: ff
Percent: 12.8%
Metric: 12.3k
Binary: 1010
Octal: 100
```

Each notation produces its own written form.

### Example 2

Example 2 compares Notation::Metric with Humanize::Metric on the same number.

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
  // Both reach the same scaling helper, so Notation::Metric and Humanize::Metric agree.
  out.writeln("via notation:", 12345.0, PrintOptions{.notation = Notation::Metric});
  out.writeln("via humanize:", 12345, PrintOptions{.humanize = Humanize::Metric});
  out.writeln("small values are not scaled down:", 0.0025, PrintOptions{.notation = Notation::Metric});
  return 0;
}
```

Output

```text
via notation: 12.3k
via humanize: 12.3k
small values are not scaled down: 0.0
```

Both reach the same scaled text, because they share the same scaling code.

## See Also

- [Number formatting](core-print-options/numbers.md)
- [`Humanize`](core-humanize.md)
