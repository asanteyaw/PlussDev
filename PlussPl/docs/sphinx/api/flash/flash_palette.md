<!-- pluss-hand-authored -->

# flash::flash_palette

[Back to flash API Reference](../../flash_api_reference)

## Signatures

```cpp
PaletteRamp flash::flash_palette(FlashPalette palette = FlashPalette::Deep)
PaletteRamp flash::flash_palette(std::string_view name)
```

## What It Does

Returns one of six fixed, 10-color `PaletteRamp`s — a named categorical palette. `PaletteRamp` (declared in `pluss/pl/studio.h`, not
`flash::`) is a small value type holding a `name()`, an ordered `stops()` vector of
[`Rgba`](../pane/theme_tokens.md) colors, and a `sample(double)` method that
interpolates between stops — see its own header for the full public interface; this
page describes only what each `flash::` palette-producing function returns.

Two overloads: the `FlashPalette` form, and a `string_view` form that maps common
lowercase names (`"muted"`, `"pastel"`, `"bright"`, `"dark"`, `"colorblind"`) to the
matching enumerator, falling back to `FlashPalette::Deep` for `"deep"` and any
unrecognized name.

## Parameters

**palette** (`FlashPalette`, default `Deep`, `FlashPalette` overload)
    See the value table below.

**name** (`std::string_view`, string overload)
    Case-sensitive; only the five lowercase strings listed above are recognized. Any
    other string (including `"Deep"`, `"DEEP"`, or an empty string) falls back to
    `FlashPalette::Deep`.

## `FlashPalette` values

Each value names a fixed, 10-color `Rgba` sequence (colorblind-friendly palettes are
sourced from established categorical color research):

- **Deep** (the default) — muted-saturation blues, oranges, greens.
- **Muted** — desaturated versions of the same hue family as `Deep`.
- **Pastel** — light, low-saturation tints.
- **Bright** — highly saturated, vivid hues.
- **Dark** — deeply saturated, dark hues.
- **Colorblind** — a palette specifically chosen for colorblind-safe distinguishability.

## Returns

`PaletteRamp`
    A 10-stop ramp named after the resolved `FlashPalette` value (e.g. `"muted"`,
    `"deep"`).

## Examples

Example 1. `Deep` (the default) vs `Muted` — holds different
color stops.

```cpp
using namespace pluss::pl;

auto deep = flash::flash_palette(flash::FlashPalette::Deep);
auto muted = flash::flash_palette(flash::FlashPalette::Muted);
std::cout << "flash_palette(Deep) name=" << deep.name() << " stops=" << deep.stops().size() << "\n";
std::cout << "flash_palette(Muted) name=" << muted.name() << " stops=" << muted.stops().size() << "\n";
std::cout << "flash_palette(Deep) != flash_palette(Muted): " << (deep.stops()[0].red != muted.stops()[0].red) << "\n";
```

Output:

```text
flash_palette(Deep) name=deep stops=10
flash_palette(Muted) name=muted stops=10
flash_palette(Deep) != flash_palette(Muted): 1
```

Example 2. String overload matches its `FlashPalette` equivalent.

```cpp
using namespace pluss::pl;

auto muted_enum = flash::flash_palette(flash::FlashPalette::Muted);
auto muted_str = flash::flash_palette(std::string_view("muted"));
std::cout << "flash_palette(Muted) vs flash_palette(\"muted\") name match: "
          << (muted_str.name() == muted_enum.name()) << "\n";
std::cout << "flash_palette() default name: " << flash::flash_palette().name() << "\n";
```

Output:

```text
flash_palette(Muted) vs flash_palette("muted") name match: 1
flash_palette() default name: deep
```

Example 3 (edge case). An unrecognized string falls back to `Deep`.

```cpp
using namespace pluss::pl;

auto unknown = flash::flash_palette(std::string_view("not-a-real-name"));
std::cout << "flash_palette(unknown-string) falls back to: " << unknown.name() << "\n";
```

Output:

```text
flash_palette(unknown-string) falls back to: deep
```

## Notes

- Not `noexcept` (string overload does string comparisons; `FlashPalette` overload is a `switch`).
- All six palettes are fixed data — nothing about them is computed at runtime beyond
  the `switch`/string-match dispatch.
- `FlashPalette` is unrelated to [`FlashStyle`](set_style.md) — a `FlashStyle` value
  controls grid/foreground/background, never colors, while `FlashPalette` controls only
  categorical accent colors.
- Three lower-level, technically-public helpers back this function and are used
  elsewhere in this family, but are not documented on their own dedicated pages (their
  full behavior is exactly this page's `FlashPalette` value table and string-dispatch
  rule, restated): `flash_palette_kind(name)` (the `string_view` → `FlashPalette`
  lookup this function's string overload uses), `flash_palette_colors(palette)`
  (returns just the raw 10-`Rgba` vector, no `PaletteRamp` wrapper), and
  `flash_palette_ramp(palette)` (wraps `flash_palette_colors`' result into a named
  `PaletteRamp` — this function's `FlashPalette` overload is exactly `return
  flash_palette_ramp(palette);`).

## See Also

- [`flash::color_palette`](color_palette.md): a broader palette-name dispatcher that
  includes these same six names (via this function) alongside additional gradient
  ramps.
- [`flash::flash_theme`](flash_theme.md): builds a full `Theme` (colors + background +
  foreground) from a `FlashStyle`/`FlashPalette` pair using this function internally —
  distinct from [`flash::set_theme`](set_theme.md)'s toggle-only theming.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="save_png.html">
    <span class="label">Previous</span>
    <span class="title">save_png</span>
  </a>
  <a class="next-link" href="color_palette.html">
    <span class="label">Next</span>
    <span class="title">color_palette</span>
  </a>
</div>
