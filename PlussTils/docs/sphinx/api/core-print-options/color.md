# `PrintOptions: color`

[Back to `PrintOptions`](../core-print-options.md) · [Core Utilities API Reference](../../api_reference.md)

```cpp
#include <pluss/tils/print_options.h>
```

`color_mode` decides whether ANSI escape sequences are written at all: `Auto` writes them only when the sink is a real terminal, `Always` forces them, and `Never` suppresses them. `style` sets the colors and attributes for the whole call's output, and `syntax` is a reserved field for coloring each kind of token inside a structured value; it is accepted today, but it does not yet change what is written.

## Parameters

See each field below.

## Returns

Not applicable; this page covers a group of fields.

## Errors

None, unless noted for a specific field.

## Practical Examples

### Example 1

Example 1 shows the default behavior off a terminal.

```cpp
#include <pluss/tils/auxiliary/output.h>

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

No escape sequence is written, because the mode is set to Never.

### Example 2

Example 2 forces color on and shows the escape sequence.

```cpp
#include <pluss/tils/auxiliary/output.h>

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

The escape byte is replaced with an exclamation mark here so the sequence is readable; a real terminal would read it as the escape character.

### Example 3

Example 3 combines several style fields and checks equality.

```cpp
#include <pluss/tils/auxiliary/output.h>

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
  std::cout << std::boolalpha;
  Style s{.fg = Color::Blue, .bg = Color::Black, .bold = true, .underline = true};
  std::cout << "empty default: " << Style{}.empty() << '\n';
  std::cout << "empty with fields set: " << s.empty() << '\n';
  std::cout << "equal to itself: " << (s == s) << '\n';
  std::cout << "different from default: " << !(s == Style{}) << '\n';
  return 0;
}
```

Output

```text
empty default: true
empty with fields set: false
equal to itself: true
different from default: true
```

A default style is empty, and a style with fields set is not. Two default styles are equal, and the set style is not equal to the default.

### Example 4

Example 4 sets a 24-bit color, which overrides a named one.

```cpp
#include <pluss/tils/auxiliary/output.h>

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
  std::ostringstream captured;
  OutputEngine cap{captured};
  Style rgb{};
  rgb.fg = Color::Red;  // overridden by fg_rgb below
  rgb.fg_rgb = std::array<std::uint8_t, 3>{255, 128, 0};
  cap.writeln("orange", PrintOptions{.color_mode = ColorMode::Always, .style = rgb});
  std::string text = captured.str();
  std::cout << "escape names the 24-bit color: " << (text.find("255;128;0") != std::string::npos) << '\n';
  return 0;
}
```

Output

```text
escape names the 24-bit color: 1
```

The escape sequence carries the exact RGB numbers, not the name.

### Example 5

Example 5 sets a syntax theme and checks that it does not yet color a number.

```cpp
#include <pluss/tils/auxiliary/output.h>

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
  std::ostringstream captured;
  OutputEngine cap{captured};
  SyntaxTheme theme;
  theme.number = Style{.fg = Color::Cyan};
  cap.writeln("value:", 42, PrintOptions{.color_mode = ColorMode::Always, .syntax = theme});
  // The struct is accepted and does not refuse to compile or run, but it does not yet color the number.
  std::string text = captured.str();
  std::cout << "no per-token escape for the number yet: " << (text.find("36m42") == std::string::npos) << '\n';
  return 0;
}
```

Output

```text
no per-token escape for the number yet: 1
```

The call succeeds, but no per-token escape appears for the number, because this field is reserved.

## See Also

- [`ColorMode`](../core-color-mode.md)
- [`Color`](../core-color.md)
- [`Style`](../core-style.md)
- [`SyntaxTheme`](../core-syntax-theme.md)
