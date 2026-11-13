<!-- pluss-hand-authored -->

# flash::mpl_palette

[Back to flash API Reference](../../flash_api_reference)

## Signature

```cpp
PaletteRamp flash::mpl_palette(std::string_view name = "viridis")
```

```{important} The name selects one of the five built-in ramps
`mpl_palette(name)` maps the name of a familiar colormap family to the built-in `PaletteRamp` whose progression of hue is closest to it; the table below gives the mapping. A name that is not recognised, and the default `"viridis"`, give the lagoon ramp.
```

## What It Does

Maps common, widely-used colormap-family names (`"viridis"`, `"plasma"`, `"blues"`,
and similar) to the closest of PlussPl's five built-in named `PaletteRamp`s (this
library does not ship a full external colormap catalog, so the mapping is
approximate, chosen by hue-progression similarity, not an exact reproduction of any
specific colormap's data).

## Parameters

**name** (`std::string_view`, default `"viridis"`)
    Case-sensitive. See the dispatch table below.

## Dispatch table

| `name` | Resolves to | Rationale |
|---|---|---|
| `"viridis"` (default), `"cividis"` | `PaletteRamp::lagoon()` | indigo → blue → teal → green → yellow — the closest built-in match to viridis's own dark-purple-to-yellow progression |
| `"plasma"`, `"inferno"`, `"magma"`, `"hot"`, `"afmhot"` | `PaletteRamp::thermal()` | dark navy → teal → gold → orange-red — a dark-to-warm progression matching this colormap family |
| `"blues"`, `"cool"`, `"winter"`, `"ocean"` | `PaletteRamp::ocean()` | a monotone dark-to-light blue ramp |
| `"greens"`, `"summer"`, `"spring"` | `PaletteRamp::meadow()` | a green-to-warm-tan ramp |
| `"greys"`, `"gray"`, `"grey"`, `"binary"` | `PaletteRamp::neutral()` | a grayscale ramp |
| anything else | `PaletteRamp::lagoon()` | the same fallback as `"viridis"` |

## Returns

`PaletteRamp`
    The resolved built-in ramp, retaining its own `name()` (`"lagoon"`, `"thermal"`,
    `"ocean"`, `"meadow"`, or `"neutral"` — never `"viridis"`/`"plasma"`/etc.).

## Examples

Example 1. The default (`"viridis"`) and an explicit call with the same string produce
identical output — confirming the default argument and an explicit pass-through agree.

```cpp
using namespace pluss::pl;

auto viridis = flash::mpl_palette("viridis");
auto viridis_default = flash::mpl_palette();
std::cout << "mpl_palette(\"viridis\") name=" << viridis.name() << "\n";
std::cout << "mpl_palette() default name=" << viridis_default.name() << "\n";
std::cout << "mpl_palette(\"viridis\") == mpl_palette() default: " << (viridis.name() == viridis_default.name()) << "\n";
```

Output:

```text
mpl_palette("viridis") name=lagoon
mpl_palette() default name=lagoon
mpl_palette("viridis") == mpl_palette() default: 1
```

Example 2. Four different colormap-family names now resolve to four
different ramps — confirming the fix actually dispatches, rather than collapsing to
one ramp regardless of input.

```cpp
using namespace pluss::pl;

auto plasma = flash::mpl_palette("plasma");
auto blues = flash::mpl_palette("blues");
auto greens = flash::mpl_palette("greens");
auto greys = flash::mpl_palette("greys");
std::cout << "mpl_palette(\"plasma\") name=" << plasma.name() << "\n";
std::cout << "mpl_palette(\"blues\") name=" << blues.name() << "\n";
std::cout << "mpl_palette(\"greens\") name=" << greens.name() << "\n";
std::cout << "mpl_palette(\"greys\") name=" << greys.name() << "\n";

auto viridis = flash::mpl_palette("viridis");
std::cout << "mpl_palette(\"viridis\") vs mpl_palette(\"plasma\") differ: " << (viridis.name() != plasma.name()) << "\n";
std::cout << "mpl_palette(\"plasma\") vs mpl_palette(\"blues\") differ: " << (plasma.name() != blues.name()) << "\n";
std::cout << "mpl_palette(\"blues\") vs mpl_palette(\"greens\") differ: " << (blues.name() != greens.name()) << "\n";
std::cout << "mpl_palette(\"greens\") vs mpl_palette(\"greys\") differ: " << (greens.name() != greys.name()) << "\n";
```

Output:

```text
mpl_palette("plasma") name=thermal
mpl_palette("blues") name=ocean
mpl_palette("greens") name=meadow
mpl_palette("greys") name=neutral
mpl_palette("viridis") vs mpl_palette("plasma") differ: 1
mpl_palette("plasma") vs mpl_palette("blues") differ: 1
mpl_palette("blues") vs mpl_palette("greens") differ: 1
mpl_palette("greens") vs mpl_palette("greys") differ: 1
```

Example 3 (edge case). An unrecognized name falls back to `lagoon`, same as `"viridis"`.

```cpp
using namespace pluss::pl;

auto unknown = flash::mpl_palette("some-unknown-cmap");
std::cout << "mpl_palette(unknown) falls back to name=" << unknown.name() << "\n";
```

Output:

```text
mpl_palette(unknown) falls back to name=lagoon
```

## Notes

- Not `noexcept`.
- Never touches a `Pane`/`Studio`/`Trace` — a pure value-producing function.
- This is an approximate name-family mapping onto five fixed ramps, not a faithful
  reproduction of any specific external colormap's actual color data — two
  colormaps mapped to the same built-in ramp (e.g. `"plasma"` and `"inferno"`, both →
  `thermal`) will be indistinguishable through this function, even though the original
  colormaps differ from each other.
- Contrast with [`flash::color_palette`](color_palette.md), whose own dispatch table
  covers a different (non-overlapping) set of names.

## See Also

- [`flash::color_palette`](color_palette.md): a sibling dispatcher covering
  categorical names and `PaletteRamp` static names directly, rather than
  colormap-family names.
- [`flash::blend_palette`](blend_palette.md): builds an exact custom ramp from
  explicit colors, for when none of this function's five approximations are close
  enough.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="diverging_palette.html">
    <span class="label">Previous</span>
    <span class="title">diverging_palette</span>
  </a>
  <a class="next-link" href="xkcd_palette.html">
    <span class="label">Next</span>
    <span class="title">xkcd_palette</span>
  </a>
</div>
