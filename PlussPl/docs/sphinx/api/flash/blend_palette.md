<!-- pluss-hand-authored -->

# flash::blend_palette

[Back to flash API Reference](../../flash_api_reference)

## Signature

```cpp
PaletteRamp flash::blend_palette(std::vector<Rgba> colors, std::string name = "blend")
```

## What It Does

Builds a `PaletteRamp` directly from an explicit list of colors — no interpolation or
generation, the `colors` vector becomes the ramp's `stops()` verbatim (in order).

## Parameters

**colors** (`std::vector<Rgba>`)
    The ramp's stops, used as-is. If empty, falls back to a fixed two-color default
    (`rgb(42,95,153)` → `rgb(229,107,111)`, a blue-to-coral gradient) rather than
    producing an empty ramp — see the edge case below.

**name** (`std::string`, default `"blend"`)
    The returned ramp's `name()`.

## Returns

`PaletteRamp`
    A ramp whose `stops()` is exactly `colors` (or the two-color fallback).

## Examples

Example 1. Three explicit colors, named.

```cpp
using namespace pluss::pl;

auto blend_custom = flash::blend_palette(
    {Rgba::from_u8(255, 0, 0), Rgba::from_u8(0, 255, 0), Rgba::from_u8(0, 0, 255)}, "rgb");
std::cout << "blend_palette(3 colors) stops=" << blend_custom.stops().size() << " name=" << blend_custom.name() << "\n";
```

Output:

```text
blend_palette(3 colors) stops=3 name=rgb
```

Example 2 (edge case). An empty `colors` vector does not produce an empty ramp — it
falls back to a fixed two-color default.

```cpp
using namespace pluss::pl;

auto blend_default = flash::blend_palette({});
std::cout << "blend_palette(empty) stops=" << blend_default.stops().size() << " name=" << blend_default.name() << "\n";
```

Output:

```text
blend_palette(empty) stops=2 name=blend
```

## Notes

- Not `noexcept`.
- Never touches a `Pane`/`Studio`/`Trace` — a pure value-producing function.
- [`flash::xkcd_palette`](xkcd_palette.md) and [`flash::crayon_palette`](crayon_palette.md)
  are both implemented as fixed calls to this function with hardcoded 3-color lists.

## See Also

- [`flash::xkcd_palette`](xkcd_palette.md), [`flash::crayon_palette`](crayon_palette.md):
  fixed-content callers of this function.
- [`flash::color_palette`](color_palette.md): looks up a ramp by name instead of
  building one from explicit colors.
- `PaletteRamp::sample(double)` (declared in `pluss/pl/studio.h`, not `flash::`):
  interpolates between a ramp's stops — see that header for the full `PaletteRamp`
  interface.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="flash_theme.html">
    <span class="label">Previous</span>
    <span class="title">flash_theme</span>
  </a>
  <a class="next-link" href="hls_palette.html">
    <span class="label">Next</span>
    <span class="title">hls_palette</span>
  </a>
</div>
