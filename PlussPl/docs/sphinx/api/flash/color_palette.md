<!-- pluss-hand-authored -->

# flash::color_palette

[Back to flash API Reference](../../flash_api_reference)

## Signature

```cpp
PaletteRamp flash::color_palette(std::string_view name = "ocean")
```

## What It Does

A single entry point covering **two** distinct
sources by name: [`flash::flash_palette`](flash_palette.md)'s six categorical names
(`"deep"`, `"muted"`, `"pastel"`, `"bright"`, `"dark"`, `"colorblind"`), plus four of
`PaletteRamp`'s own built-in gradient ramps by name (`"thermal"`, `"lagoon"`,
`"meadow"`, `"neutral"`) and `"ocean"` (the default). Any other string is an error that lists
the valid names, so a mistyped name is reported instead of silently drawing the default ramp.

## Parameters

**name** (`std::string_view`, default `"ocean"`)
    Case-sensitive. See the dispatch table below.

## Dispatch table

| `name` | Resolves to |
|---|---|
| `"deep"`, `"muted"`, `"pastel"`, `"bright"`, `"dark"`, `"colorblind"` | `flash::flash_palette(name)` — see [that page](flash_palette.md) |
| `"thermal"` | `PaletteRamp::thermal()` |
| `"lagoon"` | `PaletteRamp::lagoon()` |
| `"meadow"` | `PaletteRamp::meadow()` |
| `"neutral"` | `PaletteRamp::neutral()` |
| `"ocean"` (the default) | `PaletteRamp::ocean()` |
| anything else | `pluss::Error` listing the valid names |

## Returns

`PaletteRamp`
    The resolved ramp, retaining whatever `name()` its source constructor gives it
    (e.g. `"deep"`/`"muted"`/... for the `flash_palette`-routed names, `"thermal"`/
    `"lagoon"`/`"meadow"`/`"neutral"`/`"ocean"` for the `PaletteRamp`-static-routed
    ones).

## Examples

Example 1. A `flash_palette`-routed name and a `PaletteRamp`-static-routed name.

```cpp
using namespace pluss::pl;

auto c1 = flash::color_palette("muted");
auto c2 = flash::color_palette("thermal");
std::cout << "color_palette(\"muted\") name=" << c1.name() << "\n";
std::cout << "color_palette(\"thermal\") name=" << c2.name() << "\n";
```

Output:

```text
color_palette("muted") name=muted
color_palette("thermal") name=thermal
```

Example 2 (edge case). The default is `"ocean"`; a mistyped name is an error that lists the valid ones.

```cpp
using namespace pluss::pl;

auto c3 = flash::color_palette();
std::cout << "color_palette() default name=" << c3.name() << "\n";
try {
  flash::color_palette("thermall");
} catch (const pluss::Error &e) {
  std::cout << e.message() << "\n";
}
```

Output:

```text
color_palette() default name=ocean
flash::color_palette: unknown palette 'thermall'; the palettes are deep, muted, pastel, bright, dark, colorblind, thermal, lagoon, meadow, neutral and ocean
```

## Notes

- Not `noexcept`.
- Contrast with [`flash::mpl_palette`](mpl_palette.md), which dispatches a different
  set of colormap-family names against the same five `PaletteRamp` statics plus
  `lagoon` — the two functions' dispatch tables do not overlap in name coverage.
- Never touches a `Pane`/`Studio`/`Trace` — a pure value-producing function.

## See Also

- [`flash::flash_palette`](flash_palette.md): what the six categorical names route to.
- [`flash::mpl_palette`](mpl_palette.md): a sibling dispatcher covering a different
  colormap-family name set instead.
- [`flash::blend_palette`](blend_palette.md): builds a custom ramp from explicit
  colors, rather than looking one up by name.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="flash_palette.html">
    <span class="label">Previous</span>
    <span class="title">flash_palette</span>
  </a>
  <a class="next-link" href="flash_theme.html">
    <span class="label">Next</span>
    <span class="title">flash_theme</span>
  </a>
</div>
