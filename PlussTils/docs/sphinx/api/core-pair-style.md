# `pluss::tils::PairStyle`

[Back to Core Utilities](core.md) · [Core Utilities API Reference](../api_reference.md)

`PairStyle` chooses how a `std::pair` is written: as a tuple, with a colon, with an arrow, or with an equals sign between the two values.

## Values

| Value | Meaning |
|---|---|
| `Inherit` | Use the engine baseline or built-in pair style. |
| `Tuple` | Write the pair as a tuple. |
| `Colon` | Separate the elements with a colon. |
| `Arrow` | Separate the elements with an arrow. |
| `Equals` | Separate the elements with an equals sign. |

## Practical Examples

### Example 1

Example 1 shows all four styles on the same pair.

```cpp
#include <pluss/tils/kit/output.h>

#include <array>
#include <iostream>
#include <limits>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writeln("Tuple:", std::pair{1, 2}, PrintOptions{.pair_style = PairStyle::Tuple});
  out.writeln("Colon:", std::pair{1, 2}, PrintOptions{.pair_style = PairStyle::Colon});
  out.writeln("Arrow:", std::pair{1, 2}, PrintOptions{.pair_style = PairStyle::Arrow});
  out.writeln("Equals:", std::pair{1, 2}, PrintOptions{.pair_style = PairStyle::Equals});
  return 0;
}
```

Output

```text
Tuple: (1, 2)
Colon: 1: 2
Arrow: 1 => 2
Equals: 1=2
```

Each style places a different separator between the two values.

## See Also

- [Containers](core-print-options/containers.md)
