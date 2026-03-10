# Flash Theming API

`flash` theming is typed (`enum class` + option structs), not string-driven for style/context selection.

## Core enums

```cpp
enum class FlashContext : std::uint8_t {
  Inherit,
  Paper,
  Notebook,
  Talk,
  Poster,
};

enum class FlashStyle : std::uint8_t {
  Darkgrid,
  Lightgrid,
  Sandgrid,
  Dark,
  Light,
  Sand,
};

enum class FlashPalette : std::uint8_t {
  Deep,
  Muted,
  Pastel,
  Bright,
  Dark,
  Colorblind,
};
```

Shorthand namespaces are available:

```cpp
fc::Talk      // FlashContext::Talk
fs::Sandgrid  // FlashStyle::Sandgrid
fp::Muted     // FlashPalette::Muted
```

## Theme options

```cpp
struct FlashThemeOptions {
  FlashStyle style{FlashStyle::Lightgrid};
  FlashContext context{FlashContext::Inherit};
  bool borderless{false};
  bool horizontal_grid{false};
  std::optional<KeyPlace> legend_place{std::nullopt};
  std::optional<KeyDock> legend_dock{std::nullopt};
  std::optional<KeyShape> legend_shape{std::nullopt};
  std::optional<double> width{std::nullopt};
  std::optional<double> height{std::nullopt};
};
```

## Main theming functions

```cpp
Studio& set(Studio& studio, Theme theme = Theme::paper());
Studio& set_theme(Studio& studio, Theme theme = Theme::paper());

Pane& set_style(Pane& pane, GridStyle style = GridStyle::Major);
Pane& set_style(Pane& pane, FlashStyle style);

Studio& set_context(Studio& studio, ...);
Studio& set_theme(Studio& studio, const FlashThemeOptions& options);
Pane& set_theme(Pane& pane, const FlashThemeOptions& options);

Pane& horizontal_grid(Pane& pane, bool enable = true);
Pane& tickless(Pane& pane, bool enable = true);
Pane& despine(Pane& pane, bool enable = true);
```

## Behavior and precedence

`set_theme(studio, FlashThemeOptions)` applies:

- context scaling (unless `context == FlashContext::Inherit`)
- style policy defaults for panes in that studio
- optional figure extent (`width`, `height`)
- optional legend overrides

Important:

- `FlashContext::Inherit` means “do not reset context scales/extent from current studio state”.
- `borderless` and `horizontal_grid` are style-policy toggles; they do not replace `Studio` theme object identity.
- You can still set local pane-level overrides after studio-level theming.

## Example

```cpp
#include <pluss/pl/flash.h>

using namespace pluss::pl;
using namespace pluss::pl::flash;

Studio fig("flash_theme_demo", Theme::paper());
set_theme(fig, FlashThemeOptions{
    .style = FlashStyle::Sandgrid,
    .context = FlashContext::Talk,
    .borderless = true,
    .horizontal_grid = true,
    .legend_dock = KeyDock::East,
    .width = 1320.0,
    .height = 840.0,
});

auto& p = fig.pane2("trend");
lineplot(p,
         std::vector<double>{0, 1, 2, 3, 4},
         std::vector<double>{0.2, 0.7, 1.0, 0.4, -0.1});
fig.save_pdf("flash_theme_demo.pdf");
```
