# `pluss::tils::Color`

[Back to Core Utilities](core.md) · [Core Utilities API Reference](../api_reference.md)

`Color` names a foreground or background color for a `Style`. `Default` means no color is set for that channel. A `fg_rgb` or `bg_rgb` value on the `Style` overrides the named color for that channel, when a 24-bit color is wanted instead of one of these names.

## Values

| Value | Meaning |
|---|---|
| `Default` | Leave this color channel unset. |
| `Black` | Standard black. |
| `Red` | Standard red. |
| `Green` | Standard green. |
| `Yellow` | Standard yellow. |
| `Blue` | Standard blue. |
| `Magenta` | Standard magenta. |
| `Cyan` | Standard cyan. |
| `White` | Standard white. |
| `BrightBlack` | Bright black. |
| `BrightRed` | Bright red. |
| `BrightGreen` | Bright green. |
| `BrightYellow` | Bright yellow. |
| `BrightBlue` | Bright blue. |
| `BrightMagenta` | Bright magenta. |
| `BrightCyan` | Bright cyan. |
| `BrightWhite` | Bright white. |

## Practical Examples

### Example 1

Example 1 checks that two named colors are distinct, and that Default is the style's starting foreground.

```cpp
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>

using namespace pluss::tils;

int main() {
  std::cout << std::boolalpha;
  std::cout << "Red is not Blue: " << (Color::Red != Color::Blue) << '\n';
  std::cout << "Default is the fallback foreground: " << (Style{}.fg == Color::Default) << '\n';
  std::ostringstream captured;
  OutputEngine cap{captured};
  cap.writeln("warning", PrintOptions{.color_mode = ColorMode::Always, .style = Style{.fg = Color::BrightYellow}});
  std::string text = captured.str();
  for (char& c : text) if (c == '\x1b') c = '!';
  std::cout << text;
  return 0;
}
```

Output

```text
Red is not Blue: true
Default is the fallback foreground: true
![94mwarning![0m
```

Red and blue compare unequal, and a fresh style's foreground is Default.

### Example 2

Example 2 shows a bright color escape sequence.

```cpp
#include <pluss/tils/kit/output.h>

#include <iostream>
#include <sstream>

using namespace pluss::tils;

int main() {
  std::cout << std::boolalpha;
  std::cout << "Red is not Blue: " << (Color::Red != Color::Blue) << '\n';
  std::cout << "Default is the fallback foreground: " << (Style{}.fg == Color::Default) << '\n';
  std::ostringstream captured;
  OutputEngine cap{captured};
  cap.writeln("warning", PrintOptions{.color_mode = ColorMode::Always, .style = Style{.fg = Color::BrightYellow}});
  std::string text = captured.str();
  for (char& c : text) if (c == '\x1b') c = '!';
  std::cout << text;
  return 0;
}
```

Output

```text
Red is not Blue: true
Default is the fallback foreground: true
![94mwarning![0m
```

The bright yellow foreground is written as its own escape code.

### Example 3

Example 3 shows a 24-bit color overriding a named one on the same style.

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

The escape sequence carries the exact RGB numbers instead of a named color.

## See Also

- [`Style`](core-style.md)
- [Color](core-print-options/color.md)
