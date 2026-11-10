# `pluss::tils::BoolStyle`

[Back to Core Utilities](core.md) · [Core Utilities API Reference](../api_reference.md)

`BoolStyle` chooses the words written for `true` and `false`. The named styles provide fixed vocabularies. `Custom` reads its words from the `bool_true` and `bool_false` fields instead.

## Values

| Value | Meaning |
|---|---|
| `Inherit` | Use the engine baseline, then the built-in true/false form. |
| `TrueFalse` | Write true and false. |
| `OneZero` | Write 1 and 0. |
| `YesNo` | Write yes and no. |
| `OnOff` | Write on and off. |
| `Custom` | Use the bool_true and bool_false fields. |

## Practical Examples

### Example 1

Example 1 shows the four built-in styles.

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
  out.writeln("TrueFalse:", true, false, PrintOptions{.bool_style = BoolStyle::TrueFalse});
  out.writeln("OneZero:", true, false, PrintOptions{.bool_style = BoolStyle::OneZero});
  out.writeln("YesNo:", true, false, PrintOptions{.bool_style = BoolStyle::YesNo});
  out.writeln("OnOff:", true, false, PrintOptions{.bool_style = BoolStyle::OnOff});
  return 0;
}
```

Output

```text
TrueFalse: true false
OneZero: 1 0
YesNo: yes no
OnOff: on off
```

Each style writes its own words for true and false.

### Example 2

Example 2 sets custom words.

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
  out.writeln("Custom:", true, false,
              PrintOptions{.bool_style = BoolStyle::Custom, .bool_true = "OUI", .bool_false = "NON"});
  return 0;
}
```

Output

```text
Custom: OUI NON
```

The custom words replace the built-in ones.

## See Also

- [Booleans and optionals](core-print-options/booleans-and-optionals.md)
