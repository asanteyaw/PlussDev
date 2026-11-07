# `pluss::tils::Align`

[Back to Core Utilities](core.md) · [Core Utilities API Reference](../api_reference.md)

```cpp
enum class Align : std::uint8_t {
  Inherit, Left, Right, Center, Numeric
};
```

`Align` sets how a value is padded to a field width. `Inherit` falls back to the baseline, then to the built-in default, which is left. `Numeric` is accepted and currently renders the same as `Right`; alignment is applied to the already-rendered text, so it does not yet pin a sign to the field's edge separately from the digits.

## Parameters

`Align` has no parameters. It is an enumeration.

## Returns

A value of this type.

## Errors

None.

## Practical Examples

### Example 1

Example 1 shows the three distinct alignments at one width.

```cpp
#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.writelnf("[{}]", "hi", PrintOptions{.align = Align::Left, .width = 6});
  out.writelnf("[{}]", "hi", PrintOptions{.align = Align::Right, .width = 6});
  out.writelnf("[{}]", "hi", PrintOptions{.align = Align::Center, .width = 6});
  return 0;
}
```

Output

```text
[hi    ]
[    hi]
[  hi  ]
```

Left, right, and center each pad differently.

### Example 2

Example 2 leaves alignment at Inherit and only sets a width.

```cpp
#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  // align is left at Inherit, so a width still pads, using the built-in default alignment.
  out.writelnf("[{}]", "hi", PrintOptions{.width = 6});
  return 0;
}
```

Output

```text
[hi    ]
```

The value still pads, using the built-in default of left.

### Example 3

Example 3 compares right alignment with numeric alignment on the same negative value.

```cpp
#include <pluss/tils/auxiliary/output.h>

#include <iostream>

using namespace pluss::tils;

int main() {
  std::cout << std::boolalpha;
  OutputEngine out;
  out.writelnf("right:   [{}]", -42, PrintOptions{.align = Align::Right, .width = 8});
  out.writelnf("numeric: [{}]", -42, PrintOptions{.align = Align::Numeric, .width = 8});
  return 0;
}
```

Output

```text
right:   [     -42]
numeric: [     -42]
```

Both produce the same padded text, because numeric is not yet distinct from right.

## See Also

- [Field layout](core-print-options/layout.md)
