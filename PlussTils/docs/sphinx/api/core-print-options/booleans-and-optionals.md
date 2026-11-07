# `PrintOptions: booleans and optionals`

[Back to `PrintOptions`](../core-print-options.md) · [Core Utilities API Reference](../../api_reference.md)

```cpp
#include <pluss/tils/print_options.h>
```

`bool_style` chooses how `true` and `false` are written; `bool_true` and `bool_false` give the text for `BoolStyle::Custom`. `null_text` is the text for an empty `optional`, and `optional_tag` wraps a present value in `Some(...)` and an empty one as `None`, instead of writing the value or the null text plainly.

## Parameters

See each field below.

## Returns

Not applicable; this page covers a group of fields.

## Errors

None, unless noted for a specific field.

## Practical Examples

### Example 1

Example 1 shows the four built-in boolean styles.

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

Each style writes true and false in its own words.

### Example 2

Example 2 sets custom text for the custom style.

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

The custom words replace true and false.

### Example 3

Example 3 compares the default and a custom null text.

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
  out.writeln("default:", std::optional<int>{});
  out.writeln("custom:", std::optional<int>{}, PrintOptions{.null_text = "<none>"});
  return 0;
}
```

Output

```text
default: nullopt
custom: <none>
```

The default reads nullopt, and the custom text replaces it.

### Example 4

Example 4 wraps optionals in the tagged form.

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
  out.writeln("untagged:", std::optional<int>{7}, std::optional<int>{});
  out.writeln("tagged:", std::optional<int>{7}, std::optional<int>{}, PrintOptions{.optional_tag = true});
  return 0;
}
```

Output

```text
untagged: 7 nullopt
tagged: Some(7) None
```

The present value is wrapped in Some, and the empty one reads None, instead of the plain value or the null text.

## See Also

- [`BoolStyle`](../core-bool-style.md)
