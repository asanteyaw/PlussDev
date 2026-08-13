<!-- pluss-hand-authored -->

# flash::dark_palette

[Back to flash API Reference](../../flash_api_reference)

## Signature

```cpp
PaletteRamp flash::dark_palette(Rgba color, std::int64_t n = 6)
```

## What It Does

A sequential ramp: `n` stops interpolating linearly
from black (`rgb(0,0,0)`) at index `0` to `color` at index `n - 1` — the black-anchored
mirror of [`flash::light_palette`](light_palette.md).

## Parameters

**color** (`Rgba`)
    The target (end) color the ramp approaches.

**n** (`std::int64_t`, default `6`)
    Number of stops. Clamped to at least `2` (`std::max<std::int64_t>(n, 2)`).

## Returns

`PaletteRamp`
    Named `"dark"`. `stops()[0]` is exactly black; `stops()[n-1]` is exactly `color`.

## Examples

Example 1. A ramp toward the same mid-tone blue as
[`flash::light_palette`](light_palette.md)'s example — confirmed to genuinely differ
from it via direct stop comparison.

```cpp
using namespace pluss::pl;

auto light = flash::light_palette(Rgba::from_u8(30, 80, 200), 6);
auto dark = flash::dark_palette(Rgba::from_u8(30, 80, 200), 6);
std::cout << "dark_palette stops=" << dark.stops().size() << " first(black-ish)=" << (dark.stops()[0].red < 0.1) << "\n";
std::cout << "light_palette vs dark_palette differ: "
          << (light.stops()[0].red != dark.stops()[0].red) << "\n";
```

Output:

```text
dark_palette stops=6 first(black-ish)=1
light_palette vs dark_palette differ: 1
```

## Notes

- Not `noexcept`.
- Never touches a `Pane`/`Studio`/`Trace` — a pure value-producing function.
- Same plain per-channel linear-mix interpolation as
  [`flash::light_palette`](light_palette.md), just anchored at black instead of white.

## See Also

- [`flash::light_palette`](light_palette.md): the white-anchored mirror of this
  function.
- [`flash::diverging_palette`](diverging_palette.md): a two-color, white-midpoint
  variant of the same interpolation idea.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="light_palette.html">
    <span class="label">Previous</span>
    <span class="title">light_palette</span>
  </a>
  <a class="next-link" href="diverging_palette.html">
    <span class="label">Next</span>
    <span class="title">diverging_palette</span>
  </a>
</div>
