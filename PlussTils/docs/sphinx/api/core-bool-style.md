# `pluss::tils::BoolStyle`

[Back to Core Utilities](core.md) · [Core Utilities API Reference](../api_reference.md)

```cpp
enum class BoolStyle : std::uint8_t {
  Inherit, TrueFalse, OneZero, YesNo, OnOff, Custom
};
```

`BoolStyle` chooses the words written for `true` and `false`. The first four are fixed vocabularies. `Custom` reads its words from the `bool_true` and `bool_false` fields instead.

## Parameters

`BoolStyle` has no parameters. It is an enumeration.

## Returns

A value of this type.

## Errors

None.

## Practical Examples

### Example 1

Example 1 shows the four built-in styles.

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
#include <pluss/tils/auxiliary/output.h>

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
