# `PrintOptions: number formatting`

[Back to `PrintOptions`](../core-print-options.md) · [Core Utilities API Reference](../../api_reference.md)

```cpp
#include <pluss/tils/print_options.h>
```

These fields choose how a number is written: the `notation`, its `precision`, the `sign` style, the `alt` and `uppercase` flags, the `grouping` character and `group_size`, the `decimal_point` character, `trim_trailing_zeros`, and the text for not-a-number and infinite values. `Notation::Metric` scales the number itself with an SI suffix; it reuses the same scale as `Humanize::Metric` and only scales upward, so a value under one stays in base units.

## Parameters

See each field below.

## Returns

Not applicable; this page covers a group of fields.

## Errors

None, unless noted for a specific field.

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

Each notation produces its own form, from fixed decimals to a hexadecimal digit string.

### Example 2

Example 2 compares the default fixed precision with an explicit one.

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
  out.writeln("default fixed precision:", 1.0 / 3.0, PrintOptions{.notation = Notation::Fixed});
  out.writeln("precision 2:", 1.0 / 3.0, PrintOptions{.notation = Notation::Fixed, .precision = 2});
  return 0;
}
```

Output

```text
default fixed precision: 0.333333
precision 2: 0.33
```

The default is six digits, and setting precision to two shortens it.

### Example 3

Example 3 shows the three sign styles on a positive and a negative number.

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

Only the negative number ever shows a minus by default; the other styles add more.

### Example 4

Example 4 adds the base prefix to hexadecimal, octal, and binary numbers.

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
  out.writeln("hex without alt:", 255, PrintOptions{.notation = Notation::Hex});
  out.writeln("hex with alt:", 255, PrintOptions{.notation = Notation::Hex, .alt = true});
  out.writeln("octal with alt:", 64, PrintOptions{.notation = Notation::Octal, .alt = true});
  out.writeln("binary with alt:", 10, PrintOptions{.notation = Notation::Binary, .alt = true});
  return 0;
}
```

Output

```text
hex without alt: ff
hex with alt: 0xff
octal with alt: 0o100
binary with alt: 0b1010
```

Each base gets its own prefix, 0x, 0o, or 0b.

### Example 5

Example 5 shows uppercase affecting hex digits and a scientific exponent.

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
  out.writeln("hex lower:", 3735928559U, PrintOptions{.notation = Notation::Hex, .alt = true});
  out.writeln("hex upper:", 3735928559U, PrintOptions{.notation = Notation::Hex, .alt = true, .uppercase = true});
  out.writeln("scientific upper:", 12345.678, PrintOptions{.notation = Notation::Scientific, .precision = 2, .uppercase = true});
  return 0;
}
```

Output

```text
hex lower: 0xdeadbeef
hex upper: 0xDEADBEEF
scientific upper: 1.23E+04
```

The hex digits and the exponent letter both switch to uppercase.

### Example 6

Example 6 groups digits with two different characters and group sizes.

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
  out.writeln("comma:", 1234567, PrintOptions{.grouping = ','});
  out.writeln("underscore:", 1234567, PrintOptions{.grouping = '_'});
  out.writeln("group size 4:", 123456789, PrintOptions{.grouping = ',', .group_size = 4});
  return 0;
}
```

Output

```text
comma: 1,234,567
underscore: 1_234_567
group size 4: 1,2345,6789
```

The default group size is three digits; setting it to four changes where the separators fall.

### Example 7

Example 7 changes the decimal point character.

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
  out.writeln("default point:", 3.5, PrintOptions{.notation = Notation::Fixed, .precision = 2});
  out.writeln("comma point:", 3.5, PrintOptions{.notation = Notation::Fixed, .precision = 2, .decimal_point = ','});
  return 0;
}
```

Output

```text
default point: 3.50
comma point: 3,50
```

The fraction is separated by a comma instead of a period.

### Example 8

Example 8 trims trailing zeros from a fixed value.

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
  out.writeln("not trimmed:", 1.2, PrintOptions{.notation = Notation::Fixed, .precision = 4});
  out.writeln("trimmed:", 1.2, PrintOptions{.notation = Notation::Fixed, .precision = 4, .trim_trailing_zeros = true});
  return 0;
}
```

Output

```text
not trimmed: 1.2000
trimmed: 1.2
```

The untrimmed value keeps its zeros, and the trimmed one drops them.

### Example 9

Example 9 sets custom text for not-a-number and both infinities.

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
  out.writeln(std::numeric_limits<double>::quiet_NaN(),
              std::numeric_limits<double>::infinity(),
              -std::numeric_limits<double>::infinity(),
              PrintOptions{.nan_text = "missing", .pos_inf_text = "overflow", .neg_inf_text = "underflow"});
  return 0;
}
```

Output

```text
missing overflow underflow
```

Each special value prints the text given for it.

## See Also

- [`Notation`](../core-notation.md)
- [`Sign`](../core-sign.md)
- [Humanized units](humanize.md)
