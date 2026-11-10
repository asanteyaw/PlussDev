# `PrintOptions: presets`

[Back to `PrintOptions`](../core-print-options.md) · [Core Utilities API Reference](../../api_reference.md)

Each preset is a complete `PrintOptions` built for one job: `plain()` removes brackets and quoting for a terse message, `pretty_json()` lays a value out like formatted JSON, `repr()` favors an unambiguous debug form, and `table_cell(width)` pads and truncates a value to fit one column.

## Methods

| Method | Meaning |
|---|---|
| `plain()` | Use a terse form without brackets or color. |
| `pretty_json()` | Use multiline, quoted, sorted output with JSON-like values. |
| `repr()` | Favor a debug form with quoted strings and tagged optionals. |
| `table_cell(width)` | Pad and truncate to one column width. |

## Practical Examples

### Example 1

Example 1 writes a list and a map with the plain preset.

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
  out.writeln(std::vector<int>{1, 2, 3}, std::map<std::string, int>{{"a", 1}}, PrintOptions::plain());
  return 0;
}
```

Output

```text
1, 2, 3 a: 1
```

No brackets surround the list or the map, so the line reads like plain text.

### Example 2

Example 2 writes a nested map with the pretty JSON preset.

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
  out.writeln(std::map<std::string, std::vector<int>>{{"b", {3, 4}}, {"a", {1, 2}}}, PrintOptions::pretty_json());
  return 0;
}
```

Output

```text
{
  "a": [
    1,
    2
  ],
  "b": [
    3,
    4
  ]
}
```

The keys are sorted, the value is laid out across several lines, and each key is quoted.

### Example 3

Example 3 writes an optional and an integer with the repr preset.

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
  out.writeln(std::optional<std::string>{"hi"}, 255, PrintOptions::repr());
  return 0;
}
```

Output

```text
Some(hi) 255
```

The optional is wrapped in Some, which is the unambiguous debug form.

### Example 4

Example 4 fits a short and a long value into a ten-character cell.

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
  out.writelnf("[{}]", "name", PrintOptions::table_cell(10));
  out.writelnf("[{}]", "an-overlong-name", PrintOptions::table_cell(10));
  return 0;
}
```

Output

```text
[      name]
[an-overlo…]
```

The short value is padded to the width, and the long one is truncated to fit.

## See Also

- [Containers](containers.md)
- [Field layout](layout.md)
