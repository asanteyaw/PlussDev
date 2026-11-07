# `PrintOptions: joining the values of a call`

[Back to `PrintOptions`](../core-print-options.md) · [Core Utilities API Reference](../../api_reference.md)

```cpp
#include <pluss/tils/print_options.h>
```

These fields decide how the values of one call are joined into a line: `sep` between values, `end` after the last one, `prefix` and `suffix` around everything, `line_prefix` on every physical line, and `flush` to push the line out immediately. All but `flush` are `std::optional<std::string>`; an unset one falls back to the engine's separator or to an empty string.

## Parameters

See each field below.

## Returns

Not applicable; this page covers a group of fields.

## Errors

None, unless noted for a specific field.

## Practical Examples

### Example 1

Example 1 overrides the separator for one call.

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
  out.writeln("a", "b", "c", PrintOptions{.sep = std::string(" / ")});
  out.writeln("a", "b", "c");
  return 0;
}
```

Output

```text
a / b / c
a b c
```

The first line uses the slash separator, and the second reverts to the engine default.

### Example 2

Example 2 adds text after the last value, before the newline.

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
  out.writeln("total:", 42, PrintOptions{.end = std::string(" (estimated)")});
  return 0;
}
```

Output

```text
total: 42 (estimated)
```

The estimated note is written right after the number, still on the same line.

### Example 3

Example 3 wraps the whole line with a prefix and a suffix.

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
  out.writeln("value", PrintOptions{.prefix = std::string(">> "), .suffix = std::string(" <<")});
  return 0;
}
```

Output

```text
>> value <<
```

The prefix opens the line and the suffix closes it.

### Example 4

Example 4 repeats a line prefix on each physical line of a multiline value.

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
  out.writeln("rows:", std::vector<int>{1, 2, 3}, PrintOptions{.line_prefix = std::string("  | "), .multiline = true});
  return 0;
}
```

Output

```text
  | rows: [
  |   1,
  |   2,
  |   3
  | ]
```

Every line of the rendered list begins with the same marker.

### Example 5

Example 5 forces a flush after one call.

```cpp
#include <pluss/tils/auxiliary/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  std::ostringstream sink;
  OutputEngine out{sink};
  out.writeln("flushed now", PrintOptions{.flush = true});
  std::cout << "sink holds: " << sink.str();
  return 0;
}
```

Output

```text
sink holds: flushed now
```

The sink holds the text as soon as the call returns, without a separate flush call.

## See Also

- [`OutputEngine::sep`](../core-output-engine/sep.md)
- [`OutputEngine::flush`](../core-output-engine/flush.md)
