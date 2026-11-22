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

The ramp's stops, used as-is. At least one colour is required: an empty list is an
error (see the edge case below). A single colour gives a constant ramp.

**name** (`std::string`, default `"blend"`)

The returned ramp's `name()`.

## Returns

`PaletteRamp`
    A ramp whose `stops()` is exactly `colors`.

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

Example 2 (edge case). An empty list is an error, and a single colour is a valid constant ramp.

```cpp
using namespace pluss::pl;

try {
  flash::blend_palette({});
} catch (const pluss::Error &e) {
  std::cout << e.message() << "\n";
}
auto one = flash::blend_palette({Rgba::from_u8(1, 2, 3)});
const auto mid = one.sample(0.5);
std::cout << "one colour, sampled at 0.5: " << static_cast<int>(mid.red * 255 + 0.5) << "," << static_cast<int>(mid.green * 255 + 0.5) << "," << static_cast<int>(mid.blue * 255 + 0.5) << "\n";
```

Output:

```text
flash::blend_palette: at least one colour is required
one colour, sampled at 0.5: 1,2,3
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
