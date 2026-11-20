<!-- pluss-hand-authored -->

# flash::crayon_palette

[Back to flash API Reference](../../flash_api_reference)

## Signature

```cpp
PaletteRamp flash::crayon_palette()
```

## What It Does

A fixed, no-argument three-color ramp — `return blend_palette({red, yellow, green},
"crayon");` via [`flash::blend_palette`](blend_palette.md), with hardcoded colors
(`rgb(239,71,111)`, `rgb(255,209,102)`, `rgb(6,214,160)`). The sibling of
[`flash::xkcd_palette`](xkcd_palette.md), same pattern, different fixed colors.

## Parameters

None.

## Returns

`PaletteRamp`
    Named `"crayon"`, always the same three fixed colors.

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
describe(flash::crayon_palette());
```

Output:

```text
3 stops; first (239, 71, 111), last (6, 214, 160)
```

Example 2. The ramp carries its name.

```cpp
std::cout << "name \"" << flash::crayon_palette().name() << "\"\n";
```

Output:

```text
name "crayon"
```

Example 3. The palette takes no arguments, so there is nothing to refuse; to use a ramp with a different number of stops, sample it at positions between 0 and 1.

```cpp
const auto ramp = flash::crayon_palette();
const auto mid = ramp.sample(0.5);
std::cout << static_cast<int>(mid.red * 255 + 0.5) << ", " << static_cast<int>(mid.green * 255 + 0.5) << ", " << static_cast<int>(mid.blue * 255 + 0.5) << "\n";
```

Output:

```text
255, 209, 102
```

## Notes

- Not `noexcept`.
- Never touches a `Pane`/`Studio`/`Trace` — a pure value-producing function.
- Takes no arguments — the three colors are fixed, with no selection mechanism.

## See Also

- [`flash::blend_palette`](blend_palette.md): the function this one calls with a
  hardcoded color list.
- [`flash::xkcd_palette`](xkcd_palette.md): the sibling fixed-content wrapper this
  page's example compares against.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="xkcd_palette.html">
    <span class="label">Previous</span>
    <span class="title">xkcd_palette</span>
  </a>
  <a class="next-link" href="get_dataset_names.html">
    <span class="label">Next</span>
    <span class="title">get_dataset_names</span>
  </a>
</div>
