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

Example 1.

```cpp
using namespace pluss::pl;

auto xkcd = flash::xkcd_palette();
std::cout << "xkcd_palette name=" << xkcd.name() << " stops=" << xkcd.stops().size() << "\n";
```

Output:

```text
xkcd_palette name=xkcd stops=3
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
