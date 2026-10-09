# `PrintOptions: fluent builders`

[Back to `PrintOptions`](../core-print-options.md) · [Core Utilities API Reference](../../api_reference.md)

```cpp
#include <pluss/tils/print_options.h>
```

Each builder sets one or two related fields and returns the value, so calls chain: `with_sep`, `with_end`, `fixed`, `sci`, `hex`, `percent`, `pad`, `grouped`, `colored`, `quoted`, `upcase`, `pretty`, and `capped`. They are shortcuts for the designated-initializer form, and either form produces the same `PrintOptions`.

## Parameters

See each field below.

## Returns

Not applicable; this page covers a group of fields.

## Errors

None, unless noted for a specific field.

## Practical Examples

### Example 1

Example 1 sets the separator and the end text with builders.

```cpp
#include <pluss/tils/aux/output.h>

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
  out.writeln(1, 2, 3, PrintOptions{}.with_sep(" / "));
  out.writeln("x", PrintOptions{}.with_end("!"));
  return 0;
}
```

Output

```text
1 / 2 / 3
x!
```

Each builder sets the one field it names.

### Example 2

Example 2 sets fixed and scientific notation with their precision in one call each.

```cpp
#include <pluss/tils/aux/output.h>

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
  out.writeln(3.14159, PrintOptions{}.fixed(2));
  out.writeln(98765.0, PrintOptions{}.sci(1));
  return 0;
}
```

Output

```text
3.14
9.9e+04
```

Both the notation and the precision come from the one builder call.

### Example 3

Example 3 turns the base prefix on and off for hexadecimal.

```cpp
#include <pluss/tils/aux/output.h>

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
  out.writeln(4095, PrintOptions{}.hex());
  out.writeln(4095, PrintOptions{}.hex(false));
  return 0;
}
```

Output

```text
0xfff
fff
```

The prefix appears only when the builder's argument is true, which is also the default.

### Example 4

Example 4 sets a percentage with zero decimals.

```cpp
#include <pluss/tils/aux/output.h>

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
  out.writeln(0.5, PrintOptions{}.percent(0));
  return 0;
}
```

Output

```text
50%
```

Half becomes 50 percent, with no fraction shown.

### Example 5

Example 5 pads a value with a chosen width, alignment, and fill character in one call.

```cpp
#include <pluss/tils/aux/output.h>

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
  out.writelnf("[{}]", "hi", PrintOptions{}.pad(6, Align::Right, '*'));
  return 0;
}
```

Output

```text
[****hi]
```

All three settings come from the one builder.

### Example 6

Example 6 groups digits with the default separator and with a custom one.

```cpp
#include <pluss/tils/aux/output.h>

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
  out.writeln(1234567, PrintOptions{}.grouped());
  out.writeln(12345678, PrintOptions{}.grouped('_', 2));
  return 0;
}
```

Output

```text
1,234,567
12_34_56_78
```

The default groups by comma in threes, and the custom call groups by underscore in twos.

### Example 7

Example 7 combines three single-field builders: a color, quoting, and uppercase.

```cpp
#include <pluss/tils/aux/output.h>

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
  std::ostringstream captured;
  OutputEngine cap{captured};
  cap.writeln("green", PrintOptions{.color_mode = ColorMode::Always}.colored(Style{.fg = Color::Green}));
  cap.writeln(std::vector<std::string>{"q"}, PrintOptions{}.quoted());
  cap.writelnf("{}", "shout", PrintOptions{}.upcase());
  std::string text = captured.str();
  for (char& c : text) if (c == '\x1b') c = '!';
  std::cout << text;
  return 0;
}
```

Output

```text
![32mgreen![0m
["q"]
SHOUT
```

Each builder changes only the field it is named for.

### Example 8

Example 8 switches a list to the multiline layout with an indent.

```cpp
#include <pluss/tils/aux/output.h>

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
  out.writeln(std::vector<int>{1, 2, 3}, PrintOptions{}.pretty("  "));
  return 0;
}
```

Output

```text
[
  1,
  2,
  3
]
```

The list prints one item per line, indented by the given string.

### Example 9

Example 9 caps a list to a maximum number of items.

```cpp
#include <pluss/tils/aux/output.h>

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
  out.writeln(std::vector<int>{1, 2, 3, 4, 5}, PrintOptions{}.capped(2));
  return 0;
}
```

Output

```text
[1, 2, …]
```

Only the first two items are shown, followed by an ellipsis.

## See Also

- [Field layout](layout.md)
- [Number formatting](numbers.md)
