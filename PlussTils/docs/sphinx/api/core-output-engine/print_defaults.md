# `pluss::tils::OutputEngine::print_defaults`

[Back to `OutputEngine`](../core-output-engine.md) · [Core Utilities API Reference](../../api_reference.md)

```cpp
OutputEngine& print_defaults(PrintOptions po);
PrintOptions print_defaults() const;
```

`print_defaults` sets a baseline of `PrintOptions` that applies to every call on the engine. A call can still pass its own options, and the call's options win for the fields they set. Passing an empty `PrintOptions` clears the baseline. The getter returns the stored baseline.

## Parameters

This member takes no parameters.

| `po` | The baseline. An empty value clears it. |

## Returns

The setter returns the engine. The getter returns the stored `PrintOptions`.

## Errors

None.

## Practical Examples

### Example 1

Example 1 sets a grouping baseline that every call uses.

```cpp
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.grouped('_'));
  out.writeln("baseline grouping:", 1000000);
  out.writeln("again:", 2500000);
  return 0;
}
```

Output

```text
baseline grouping: 1_000_000
again: 2_500_000
```

Both numbers are grouped with underscores, with no option on either call.

### Example 2

Example 2 sets a baseline and then overrides it on one call.

```cpp
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.fixed(1));
  out.writeln("engine baseline:", 3.14159);
  out.writeln("call override:", 3.14159, PrintOptions{}.fixed(4));
  return 0;
}
```

Output

```text
engine baseline: 3.1
call override: 3.1416
```

The baseline prints one decimal, and the call's option prints four.

### Example 3

Example 3 clears the baseline with an empty value.

```cpp
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

using namespace pluss::tils;

int main() {
  OutputEngine out;
  out.print_defaults(PrintOptions{}.grouped(','));
  out.print_defaults(PrintOptions{});
  out.writeln("baseline cleared:", 1234567);
  std::cout << "stored baseline is empty: " << (out.print_defaults().grouping == '\0') << '\n';
  return 0;
}
```

Output

```text
baseline cleared: 1234567
stored baseline is empty: 1
```

After clearing, the number prints plain, and the stored baseline reads back empty.

## See Also

- [`OutputEngine`](../core-output-engine.md)
