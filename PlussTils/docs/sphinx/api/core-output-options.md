# `pluss::tils::OutputOptions`

[Back to Core Utilities](core.md) · [Core Utilities API Reference](../api_reference.md)

```cpp
struct OutputOptions {
  std::string separator{" "};
  bool levels_enabled{true};
  std::array<std::string, 5> level_prefixes;
  OutputLevel min_level{OutputLevel::Trace};
  bool auto_flush{false};
};
```

`OutputOptions` is the bulk configuration of an engine. It holds the separator, whether level tags are written, the tag for each level, the minimum level, and whether each line is flushed. Build one, change the fields you need, and pass it to the constructor or to `set_options`. `OutputEngine::options` returns the current values in this form.

## Parameters

| Field | Meaning |
|---|---|
| `separator` | The text between values. Default one space. |
| `levels_enabled` | Whether log lines carry a tag. Default true. |
| `level_prefixes` | The tag for each level, in level order. |
| `min_level` | The lowest level that is written. Default trace. |
| `auto_flush` | Whether each line is flushed as it is written. Default false. |

## Returns

A plain value. Copying it copies every field.

## Errors

None.

## Practical Examples

### Example 1

Example 1 reads the defaults of a new options value.

```cpp
#include <pluss/tils/aux/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  std::cout << std::boolalpha;
  const OutputOptions options;
  std::cout << "separator: [" << options.separator << "]\n";
  std::cout << "levels enabled: " << options.levels_enabled << '\n';
  std::cout << "auto flush: " << options.auto_flush << '\n';
  std::cout << "info prefix: " << options.level_prefixes[2] << '\n';
  return 0;
}
```

Output

```text
separator: [ ]
levels enabled: true
auto flush: false
info prefix: [INFO]
```

The defaults are a single space, tags on, and the standard info tag.

### Example 2

Example 2 replaces every level tag with a short one.

```cpp
#include <pluss/tils/aux/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputOptions options;
  options.level_prefixes = {"trc", "dbg", "inf", "wrn", "err"};
  OutputEngine out{std::cout, options};
  out.debug("short prefixes");
  out.error("short prefixes again");
  return 0;
}
```

Output

```text
dbg short prefixes
err short prefixes again
```

Each log line uses the short tag for its level.

### Example 3

Example 3 sets a minimum level and turns on auto flush.

```cpp
#include <pluss/tils/aux/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  std::ostringstream sink;
  OutputOptions options;
  options.min_level = OutputLevel::Info;
  options.auto_flush = true;
  OutputEngine out{sink, options};
  out.debug("below the minimum");
  out.info("at the minimum");
  std::cout << sink.str();
  return 0;
}
```

Output

```text
[INFO] at the minimum
```

Debug is dropped, and the info line is kept.

## See Also

- [`OutputEngine::set_options`](core-output-engine/set_options.md)
- [`OutputEngine::options`](core-output-engine/options.md)
