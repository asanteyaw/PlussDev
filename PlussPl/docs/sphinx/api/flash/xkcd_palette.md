<!-- pluss-hand-authored -->

# flash::xkcd_palette

[Back to flash API Reference](../../flash_api_reference)

## Signature

```cpp
PaletteRamp flash::xkcd_palette()
```

## What It Does

A fixed, no-argument three-color ramp — `return blend_palette({blue, orange, green},
"xkcd");` via [`flash::blend_palette`](blend_palette.md), with hardcoded colors
(`rgb(49,117,187)`, `rgb(255,160,64)`, `rgb(89,182,116)`). Named for the crowdsourced
XKCD color-name survey these colors evoke — this function has no name-lookup capability
at all, it always returns the same three fixed colors.

## Parameters

None.

## Returns

`PaletteRamp`
    Named `"xkcd"`, always the same three fixed colors.

## Examples

The examples share one small helper that prints the number of stops and the first and last colour of a ramp as red, green, blue between 0 and 255.

```cpp
using namespace pluss::pl;

auto describe = [](const PaletteRamp &ramp) {
  auto c = [](const Rgba &x) {
    return "(" + std::to_string(static_cast<int>(x.red * 255 + 0.5)) + ", " + std::to_string(static_cast<int>(x.green * 255 + 0.5)) + ", " +
           std::to_string(static_cast<int>(x.blue * 255 + 0.5)) + ")";
  };
  const auto &s = ramp.stops();
  std::cout << s.size() << " stops; first " << c(s.front()) << ", last " << c(s.back()) << "\n";
};
const auto blue = Rgba::from_u8(30, 80, 200);
const auto red = Rgba::from_u8(200, 40, 40);
```

Example 1. The ramp has fixed stops: print its size and the first and last colour.

```cpp
describe(flash::xkcd_palette());
```

Output:

```text
3 stops; first (49, 117, 187), last (89, 182, 116)
```

Example 2. The ramp carries its name.

```cpp
std::cout << "name \"" << flash::xkcd_palette().name() << "\"\n";
```

Output:

```text
name "xkcd"
```

Example 3. The palette takes no arguments, so there is nothing to refuse; to use a ramp with a different number of stops, sample it at positions between 0 and 1.

```cpp
const auto ramp = flash::xkcd_palette();
const auto mid = ramp.sample(0.5);
std::cout << static_cast<int>(mid.red * 255 + 0.5) << ", " << static_cast<int>(mid.green * 255 + 0.5) << ", " << static_cast<int>(mid.blue * 255 + 0.5) << "\n";
```

Output:

```text
255, 160, 64
```

## Notes

- Not `noexcept`.
- Never touches a `Pane`/`Studio`/`Trace` — a pure value-producing function.
- This function takes no color-name arguments at all — there is no way to select which
  colors appear; the three colors are fixed.

## See Also

- [`flash::blend_palette`](blend_palette.md): the function this one calls with a
  hardcoded color list.
- [`flash::crayon_palette`](crayon_palette.md): a sibling fixed-content wrapper around
  the same underlying function.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mpl_palette.html">
    <span class="label">Previous</span>
    <span class="title">mpl_palette</span>
  </a>
  <a class="next-link" href="crayon_palette.html">
    <span class="label">Next</span>
    <span class="title">crayon_palette</span>
  </a>
</div>
