# `pluss::tils::PrintOptions`

[Back to Core Utilities](core.md) · [Core Utilities API Reference](../api_reference.md)

```cpp
#include <pluss/tils/print_options.h>

struct PrintOptions;
```

`PrintOptions` is the per-call formatting spec that `OutputEngine` reads. A value you pass as the last argument of a call describes that whole call: every top-level value written by that call is rendered under it. A value you store with `OutputEngine::print_defaults` becomes the engine's baseline, and a call's own options still win for the fields they set; a field neither sets falls back to a built-in default.

Most fields are plain values with an obvious off state: zero, an empty string, or `false`. A few fields are enumerations with an explicit `Inherit` value, which means the same thing: fall back to the baseline, then to the built-in default. The fields are grouped by what they affect, and each group has its own reference page, linked below.

## Parameters

`PrintOptions` has no constructor parameters. Build one with aggregate initialization, set the fields you need, and pass it as the last argument of a call, or store it with `print_defaults`.

## Returns

A plain value. Copying it copies every field. The fluent builders return `PrintOptions&`, so calls chain.

## Errors

None from the struct itself. `OutputEngine::writef` and `writelnf` refuse a pattern whose placeholder count disagrees with the arguments when `strict_args` is set.

## Practical Examples

### Example 1

Example 1 reads the defaults of a freshly built value.

```cpp
#include <pluss/tils/aux/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  std::cout << std::boolalpha;
  const PrintOptions p;
  std::cout << "sep is unset: " << !p.sep.has_value() << '\n';
  std::cout << "width: " << p.width << '\n';
  std::cout << "align is Inherit: " << (p.align == Align::Inherit) << '\n';
  std::cout << "grouping is none: " << (p.grouping == '\0') << '\n';
  return 0;
}
```

Output

```text
sep is unset: true
width: 0
align is Inherit: true
grouping is none: true
```

Every optional field is unset, the width is zero, the alignment is Inherit, and no grouping character is set.

### Example 2

Example 2 builds a value with designated initializers and passes it to a call.

```cpp
#include <pluss/tils/aux/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  const PrintOptions p{.notation = Notation::Fixed, .precision = 2, .grouping = ','};
  out.writeln("total:", 1234567.5, p);
  return 0;
}
```

Output

```text
total: 1,234,567.50
```

The number is shown fixed to two decimals and grouped by thousands, both set by the one value.

### Example 3

Example 3 sets an engine baseline, then overrides every field the baseline set.

```cpp
#include <pluss/tils/aux/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.grouped('_').fixed(1));
  out.writeln("baseline only:", 1234567.25);
  out.writeln("call overrides both fields:", 1234567.25, PrintOptions{}.grouped(',').fixed(3));
  return 0;
}
```

Output

```text
baseline only: 1_234_567.2
call overrides both fields: 1,234,567.250
```

The baseline formats the first line, and the call's own options replace both fields for the second.

### Example 4

Example 4 sets only one field on the call, leaving the rest of the baseline in place.

```cpp
#include <pluss/tils/aux/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.grouped('_').fixed(1));
  // The call sets only the precision; the grouping still comes from the baseline.
  out.writeln("partial override:", 1234567.25, PrintOptions{}.fixed(0));
  return 0;
}
```

Output

```text
partial override: 1_234_567
```

The precision changes to the call's value, and the grouping still comes from the baseline, because the call never set it.

### Example 5

Example 5 leaves alignment at Inherit and only sets a width.

```cpp
#include <pluss/tils/aux/output.h>

#include <iostream>
#include <limits>
#include <optional>
#include <sstream>
#include <string>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  // align is left at Inherit, so a width still pads, using the built-in default alignment.
  out.writelnf("[{}]", "hi", PrintOptions{.width = 6});
  return 0;
}
```

Output

```text
[hi    ]
```

The text still pads, using the built-in default alignment, which is left.

## See Also

- [Joining](core-print-options/joining.md)
- [Layout](core-print-options/layout.md)
- [Numbers](core-print-options/numbers.md)
- [Humanized units](core-print-options/humanize.md)
- [Booleans and optionals](core-print-options/booleans-and-optionals.md)
- [Strings](core-print-options/strings.md)
- [Containers](core-print-options/containers.md)
- [Color](core-print-options/color.md)
- [The format-string path](core-print-options/format-path.md)
- [Fluent builders](core-print-options/builders.md)
- [Presets](core-print-options/presets.md)
- [`OutputEngine`](core-output-engine.md)
