# `pluss::tils::Humanize`

[Back to Core Utilities](core.md) · [Core Utilities API Reference](../api_reference.md)

```cpp
enum class Humanize : std::uint8_t {
  None, BytesIEC, BytesSI, Metric, DurationNs
};
```

`Humanize` scales a number and appends a unit. `BytesIEC` uses binary units (KiB, MiB, ...). `BytesSI` uses decimal units (kB, MB, ...). `Metric` uses plain SI prefixes (k, M, G, ...), the same scale `Notation::Metric` uses. `DurationNs` treats the number as a count of nanoseconds and scales it up through microseconds, milliseconds, and seconds.

## Parameters

`Humanize` has no parameters. It is an enumeration.

## Returns

A value of this type.

## Errors

None.

## Practical Examples

### Example 1

Example 1 scales byte counts in binary units.

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
  out.writeln("1536 bytes:", 1536, PrintOptions{.humanize = Humanize::BytesIEC});
  out.writeln("3 GiB:", 3221225472LL, PrintOptions{.humanize = Humanize::BytesIEC});
  return 0;
}
```

Output

```text
1536 bytes: 1.5 KiB
3 GiB: 3.0 GiB
```

The value scales from bytes up to gibibytes as the count grows.

### Example 2

Example 2 scales byte counts in decimal units.

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
  out.writeln("1500 bytes:", 1500, PrintOptions{.humanize = Humanize::BytesSI});
  out.writeln("2.5 MB:", 2500000LL, PrintOptions{.humanize = Humanize::BytesSI});
  return 0;
}
```

Output

```text
1500 bytes: 1.5 kB
2.5 MB: 2.5 MB
```

The same magnitude reads differently in decimal units than in binary ones.

### Example 3

Example 3 scales a duration in nanoseconds.

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
  out.writeln("2500 ns:", 2500, PrintOptions{.humanize = Humanize::DurationNs});
  out.writeln("2.5 ms:", 2500000LL, PrintOptions{.humanize = Humanize::DurationNs});
  out.writeln("90 s:", 90000000000LL, PrintOptions{.humanize = Humanize::DurationNs});
  return 0;
}
```

Output

```text
2500 ns: 2.5 µs
2.5 ms: 2.5 ms
90 s: 90.0 s
```

The duration scales up through the unit table as the count grows.

## See Also

- [Humanized units](core-print-options/humanize.md)
