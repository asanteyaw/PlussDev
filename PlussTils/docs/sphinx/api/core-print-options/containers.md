# `PrintOptions: containers, maps, and tuples`

[Back to `PrintOptions`](../core-print-options.md) · [Core Utilities API Reference](../../api_reference.md)

```cpp
#include <pluss/tils/print_options.h>
```

These fields shape how a list, a map, or a tuple is written: the separators between items and between a map's key and value, the brackets for each container kind, the `pair_style` for a `std::pair`, a cap on how many items are shown, whether a map is sorted by key, whether a list shows its indices, and the multiline layout with its indent and trailing comma.

## Parameters

See each field below.

## Returns

Not applicable; this page covers a group of fields.

## Errors

None, unless noted for a specific field.

## Practical Examples

### Example 1

Example 1 changes the item separator and the key-value separator.

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
  out.writeln("items:", std::vector<int>{1, 2, 3}, PrintOptions{.item_sep = std::string(" ; ")});
  out.writeln("map:", std::map<std::string, int>{{"a", 1}}, PrintOptions{.kv_sep = std::string(" -> ")});
  return 0;
}
```

Output

```text
items: [1 ; 2 ; 3]
map: {a -> 1}
```

The list items are joined with a semicolon, and the map uses an arrow between its key and value.

### Example 2

Example 2 sets custom brackets for a list, a map, and a tuple.

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
  out.writeln("list:", std::vector<int>{1, 2, 3}, PrintOptions{.list_brackets = std::array<std::string, 2>{"<< ", " >>"}});
  out.writeln("map:", std::map<std::string, int>{{"a", 1}}, PrintOptions{.map_brackets = std::array<std::string, 2>{"{ ", " }"}});
  out.writeln("tuple:", std::tuple{1, 2, 3}, PrintOptions{.tuple_brackets = std::array<std::string, 2>{"[|", "|]"}});
  return 0;
}
```

Output

```text
list: << 1, 2, 3 >>
map: { a: 1 }
tuple: [|1, 2, 3|]
```

Each container kind uses its own bracket pair instead of the default square or curly brackets.

### Example 3

Example 3 shows all four pair styles.

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

The same pair is written as a tuple, with a colon, with an arrow, or with an equals sign.

### Example 4

Example 4 caps a list and compares the plain and the verbose ellipsis.

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
  const std::vector<int> v{1, 2, 3, 4, 5, 6, 7, 8};
  out.writeln("capped:", v, PrintOptions{.max_items = 3});
  out.writeln("capped with a count:", v, PrintOptions{.max_items = 3, .more_verbose = true});
  return 0;
}
```

Output

```text
capped: [1, 2, 3, …]
capped with a count: [1, 2, 3, … (+5 more)]
```

The capped list ends with an ellipsis, and the verbose form also says how many items were left out.

### Example 5

Example 5 limits how deep a nested value is shown.

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
  const std::vector<std::vector<int>> nested{{1, 2}, {3, 4}};
  out.writeln("full depth:", nested);
  out.writeln("depth 1:", nested, PrintOptions{.max_depth = 1});
  return 0;
}
```

Output

```text
full depth: [[1, 2], [3, 4]]
depth 1: [[…], […]]
```

At the default depth the nested lists print in full, and at depth one each inner list collapses to an ellipsis.

### Example 6

Example 6 sorts a map by its key.

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
  const std::map<std::string, int> m{{"c", 3}, {"a", 1}, {"b", 2}};
  out.writeln("map is already ordered by its own key type, so this is about larger keys:", m, PrintOptions{.sort_keys = true});
  return 0;
}
```

Output

```text
map is already ordered by its own key type, so this is about larger keys: {a: 1, b: 2, c: 3}
```

The entries come out in key order.

### Example 7

Example 7 numbers the items of a list.

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
  out.writeln("names:", std::vector<std::string>{"zero", "one", "two"}, PrintOptions{.show_indices = true});
  return 0;
}
```

Output

```text
names: [0: zero, 1: one, 2: two]
```

Each item is shown with its position.

### Example 8

Example 8 shows the multiline layout, a custom indent, and a trailing comma.

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
  out.writeln("default indent:", std::vector<int>{1, 2, 3}, PrintOptions{.multiline = true});
  out.writeln("custom indent:", std::vector<int>{1, 2, 3}, PrintOptions{.multiline = true, .indent = "    "});
  out.writeln("trailing comma:", std::vector<int>{1, 2}, PrintOptions{.multiline = true, .trailing_comma = true});
  return 0;
}
```

Output

```text
default indent: [
  1,
  2,
  3
]
custom indent: [
    1,
    2,
    3
]
trailing comma: [
  1,
  2,
]
```

Each item is on its own line, the indent matches what was set, and the trailing comma appears only when it is requested.

## See Also

- [`PairStyle`](../core-pair-style.md)
