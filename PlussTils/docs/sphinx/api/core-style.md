# `pluss::tils::Style`

[Back to Core Utilities](core.md) · [Core Utilities API Reference](../api_reference.md)

```cpp
struct Style {
  Color fg{Color::Default};
  Color bg{Color::Default};
  std::optional<std::array<std::uint8_t, 3>> fg_rgb{};
  std::optional<std::array<std::uint8_t, 3>> bg_rgb{};
  bool bold{}, dim{}, italic{}, underline{}, blink{}, reverse{}, strike{};

  constexpr bool empty() const noexcept;
};
```

`Style` holds the colors and attributes for one call's output: a foreground and a background color, an optional 24-bit override for either, and seven boolean attributes. `empty()` reports whether every field is at its default. Two styles compare equal when every field matches.

## Parameters

See the fields below.

## Returns

A value of this type.

## Errors

None.

## Practical Examples

### Example 1

Example 1 checks emptiness and equality.

```cpp
#include <pluss/tils/aux/output.h>

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

A default style is empty, a style with fields set is not, and the two compare unequal.

### Example 2

Example 2 sets a 24-bit foreground, which overrides a named one on the same style.

```cpp
#include <pluss/tils/aux/output.h>

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

The RGB value is what reaches the escape sequence, not the named color set on the same field.

### Example 3

Example 3 applies a style through PrintOptions.

```cpp
#include <pluss/tils/aux/output.h>

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
  cap.writeln("green", PrintOptions{.color_mode = ColorMode::Always}.colored(Style{.fg = Color::Green}));
  cap.writeln(std::vector<std::string>{"q"}, PrintOptions{}.quoted());
  cap.writelnf("{}", "shout", PrintOptions{}.upcase());
  std::string text = captured.str();
  for (char& c : text) if (c == '\x1b') c = '!';
  std::cout << text;
  return 0;
}
```

Output

```text
![32mgreen![0m
["q"]
SHOUT
```

The style's fields become the escape sequence that wraps the line.

## See Also

- [Color](core-print-options/color.md)
- [`Color`](core-color.md)
