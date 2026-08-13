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

Example 1 (verification, not a distinct rendering). Genuinely different colors from
[`flash::xkcd_palette`](xkcd_palette.md).

```cpp
using namespace pluss::pl;

auto crayon = flash::crayon_palette();
auto xkcd = flash::xkcd_palette();
std::cout << "crayon_palette name=" << crayon.name() << " stops=" << crayon.stops().size() << "\n";
std::cout << "xkcd_palette vs crayon_palette differ: " << (xkcd.stops()[0].red != crayon.stops()[0].red) << "\n";
```

Output:

```text
crayon_palette name=crayon stops=3
xkcd_palette vs crayon_palette differ: 1
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
