# `pluss::tils::SyntaxTheme`

[Back to Core Utilities](core.md) · [Core Utilities API Reference](../api_reference.md)

```cpp
struct SyntaxTheme {
  std::optional<Style> number, string, boolean, null, key, punctuation, field;
};
```

`SyntaxTheme` names a `Style` for each kind of token inside a structured value: numbers, strings, booleans, null, map keys, punctuation, and struct fields. It is a reserved field on `PrintOptions`: a value is accepted and does not refuse to compile or run, but the renderer does not yet apply per-token colors from it.

## Parameters

See the fields below.

## Returns

A value of this type.

## Errors

None.

## Practical Examples

### Example 1

Example 1 sets a theme and checks that it does not yet color a number.

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

The call succeeds, and no per-token escape appears for the number, because this field is reserved.

## See Also

- [Color](core-print-options/color.md)
