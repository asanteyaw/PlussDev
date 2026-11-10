# `PrintOptions: the format-string path`

[Back to `PrintOptions`](../core-print-options.md) · [Core Utilities API Reference](../../api_reference.md)

`missing_arg` is the text written for a placeholder with no argument. `strict_args` turns a placeholder count that disagrees with the arguments into an error instead of a silent fallback. `pack` bundles several values into one `ArgPack`, so a trailing `PrintOptions` is never mistaken for a value, and the bundle can be reused across patterns.

## Fields

| Field | Meaning |
|---|---|
| `missing_arg` | Text for an unfilled placeholder; default {}. |
| `strict_args` | Reject a mismatch between placeholders and arguments; default false. |

A placeholder-count mismatch raises an error when `strict_args` is enabled.

## Practical Examples

### Example 1

Example 1 sets the text for a missing argument. A literal pattern with too few arguments does not compile, so the pattern is held in a variable and wrapped in `runtime_format`.

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
  const std::string pattern = "{} {} {}";
  out.writelnf(runtime_format(pattern), "only", "two", PrintOptions{.missing_arg = "<?>"});
  return 0;
}
```

Output

```text
only two <?>
```

The missing third value is shown as the marker instead of being left out.

### Example 2

Example 2 sets strict argument checking on a call with too many arguments.

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
  try {
    out.writelnf("{} {}", 1, 2, 3, PrintOptions{.strict_args = true});
    std::cout << "accepted\n";
  } catch (const std::exception&) {
    std::cout << "refused\n";
  }
  return 0;
}
```

Output

```text
refused
```

The call is refused, instead of silently dropping the extra value.

### Example 3

Example 3 bundles two values with pack and reuses the bundle in two patterns.

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
  auto bundle = pack(std::string("ama"), 7);
  out.writelnf("{} is {}", bundle);
  out.writelnf("{1} then {0}", bundle);
  return 0;
}
```

Output

```text
ama is 7
7 then ama
```

Both patterns read from the same bundle, once in order and once with the values swapped by index.

## See Also

- [`OutputEngine::writef`](../core-output-engine/writef.md)
- [`OutputEngine::writelnf`](../core-output-engine/writelnf.md)
