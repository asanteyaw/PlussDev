# `PrintOptions: humanized units`

[Back to `PrintOptions`](../core-print-options.md) · [Core Utilities API Reference](../../api_reference.md)

```cpp
#include <pluss/tils/print_options.h>
```

`humanize` scales a number and appends a unit: binary bytes, decimal bytes, a metric count, or a duration in nanoseconds scaled up to seconds. `humanize_precision` sets how many decimals the scaled value shows. `Notation::Metric` reaches the same scaling code as `Humanize::Metric`, so the two agree.

## Parameters

See each field below.

## Returns

Not applicable; this page covers a group of fields.

## Errors

None, unless noted for a specific field.

## Practical Examples

### Example 1

Example 1 scales byte counts in binary units.

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

1536 bytes is 1.5 KiB, and the value scales up through the binary units as it grows.

### Example 2

Example 2 scales byte counts in decimal units.

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

1500 bytes is 1.5 kB in the decimal scale, a different unit from the binary one.

### Example 3

Example 3 scales a plain count with metric prefixes.

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
  out.writeln("12345 rows:", 12345, PrintOptions{.humanize = Humanize::Metric});
  out.writeln("6.7 million:", 6700000LL, PrintOptions{.humanize = Humanize::Metric});
  return 0;
}
```

Output

```text
12345 rows: 12.3k
6.7 million: 6.7M
```

12345 reads as 12.3k, and 6.7 million reads as 6.7M.

### Example 4

Example 4 scales a duration given in nanoseconds.

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

The duration scales from nanoseconds up through microseconds, milliseconds, and seconds as it grows.

### Example 5

Example 5 compares the default precision with a wider one.

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
  out.writeln("default precision:", 1536, PrintOptions{.humanize = Humanize::BytesIEC});
  out.writeln("precision 2:", 1536, PrintOptions{.humanize = Humanize::BytesIEC, .humanize_precision = 2});
  return 0;
}
```

Output

```text
default precision: 1.5 KiB
precision 2: 1.50 KiB
```

The default shows one decimal, and setting precision to two shows two.

### Example 6

Example 6 compares the notation and the humanize forms of the metric scale.

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
  // Both reach the same scaling helper, so Notation::Metric and Humanize::Metric agree.
  out.writeln("via notation:", 12345.0, PrintOptions{.notation = Notation::Metric});
  out.writeln("via humanize:", 12345, PrintOptions{.humanize = Humanize::Metric});
  out.writeln("small values are not scaled down:", 0.0025, PrintOptions{.notation = Notation::Metric});
  return 0;
}
```

Output

```text
via notation: 12.3k
via humanize: 12.3k
small values are not scaled down: 0.0
```

Both reach 12.3k for the same number, and a value under one is not scaled down by either.

## See Also

- [`Humanize`](../core-humanize.md)
- [Number formatting](numbers.md)
