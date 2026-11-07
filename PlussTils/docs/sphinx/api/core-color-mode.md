# `pluss::tils::ColorMode`

[Back to Core Utilities](core.md) · [Core Utilities API Reference](../api_reference.md)

```cpp
enum class ColorMode : std::uint8_t {
  Auto, Always, Never
};
```

`ColorMode` decides whether ANSI escape sequences are written. `Auto`, the default, writes them only when the sink is a real terminal. `Always` forces them on, which is useful for a test that wants to see the escapes. `Never` suppresses them even on a terminal.

## Parameters

`ColorMode` has no parameters. It is an enumeration.

## Returns

A value of this type.

## Errors

None.

## Practical Examples

### Example 1

Example 1 shows the default behavior off a terminal, where nothing is written.

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
  out.writeln("quiet", PrintOptions{.color_mode = ColorMode::Never, .style = Style{.fg = Color::Red}});
  return 0;
}
```

Output

```text
quiet
```

No escape sequence appears, because Never suppresses them.

### Example 2

Example 2 forces color on.

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
  std::ostringstream captured;
  OutputEngine cap{captured};
  cap.writeln("styled", PrintOptions{.color_mode = ColorMode::Always, .style = Style{.fg = Color::Green, .bold = true}});
  std::string text = captured.str();
  for (char& c : text) if (c == '\x1b') c = '!';
  std::cout << text;
  return 0;
}
```

Output

```text
![1;32mstyled![0m
```

The escape sequence is written, with the escape byte shown as an exclamation mark here for readability.

## See Also

- [Color](core-print-options/color.md)
