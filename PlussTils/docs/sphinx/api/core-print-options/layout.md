# `PrintOptions: field layout`

[Back to `PrintOptions`](../core-print-options.md) · [Core Utilities API Reference](../../api_reference.md)

```cpp
#include <pluss/tils/print_options.h>
```

These fields apply to each top-level value a call writes: `align`, `width`, `fill`, `truncate`, and `ellipsis`. They are the same fields a `{:spec}` placeholder sets from a compact format spec. Truncation happens before padding, so a value is cut to fit, then padded to the requested width if it is still short.

## Parameters

See each field below.

## Returns

Not applicable; this page covers a group of fields.

## Errors

None, unless noted for a specific field.

## Practical Examples

### Example 1

Example 1 shows the three alignments at the same width.

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
  out.writelnf("[{}]", "hi", PrintOptions{.align = Align::Left, .width = 6});
  out.writelnf("[{}]", "hi", PrintOptions{.align = Align::Right, .width = 6});
  out.writelnf("[{}]", "hi", PrintOptions{.align = Align::Center, .width = 6});
  return 0;
}
```

Output

```text
[hi    ]
[    hi]
[  hi  ]
```

Left pads on the right, right pads on the left, and center splits the padding.

### Example 2

Example 2 sets a minimum width on a short and a long value.

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
  out.writelnf("[{}]", "ok", PrintOptions{.width = 8});
  out.writelnf("[{}]", "already long enough", PrintOptions{.width = 8});
  return 0;
}
```

Output

```text
[ok      ]
[already long enough]
```

The short value is padded to the width, and the long value is left as it is, because width is a minimum, not a limit.

### Example 3

Example 3 changes the pad character.

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
  out.writelnf("[{}]", "hi", PrintOptions{.align = Align::Right, .width = 8, .fill = '.'});
  return 0;
}
```

Output

```text
[......hi]
```

Dots fill the space instead of blanks.

### Example 4

Example 4 cuts a long value to a maximum width.

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
  out.writelnf("{}", "a-very-long-service-name", PrintOptions{.truncate = 12});
  return 0;
}
```

Output

```text
a-very-long…
```

The text is cut and ends with the default ellipsis mark.

### Example 5

Example 5 sets a custom cut marker.

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
  out.writelnf("{}", "a-very-long-service-name", PrintOptions{.truncate = 12, .ellipsis = "~"});
  return 0;
}
```

Output

```text
a-very-long~
```

The custom marker replaces the default ellipsis.

## See Also

- [`Align`](../core-align.md)
