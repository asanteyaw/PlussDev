<!-- pluss-hand-authored -->

# flash::light_palette

[Back to flash API Reference](../../flash_api_reference)

## Signature

```cpp
PaletteRamp flash::light_palette(Rgba color, std::int64_t n = 6)
```

## What It Does

A sequential ramp: `n` stops interpolating linearly
from white (`rgb(255,255,255)`) at index `0` to `color` at index `n - 1`.

## Parameters

**color** (`Rgba`)
    The target (end) color the ramp approaches.

**n** (`std::int64_t`, default `6`)
    Number of stops. Clamped to at least `2` (`std::max<std::int64_t>(n, 2)`).

## Returns

`PaletteRamp`
    Named `"light"`. `stops()[0]` is exactly white; `stops()[n-1]` is exactly `color`.

## Examples

Example 1. A ramp toward a mid-tone blue.

```cpp
using namespace pluss::pl;

auto light = flash::light_palette(Rgba::from_u8(30, 80, 200), 6);
std::cout << "light_palette stops=" << light.stops().size() << " first(white-ish)=" << (light.stops()[0].red > 0.9) << "\n";
```

Output:

```text
light_palette stops=6 first(white-ish)=1
```

## Notes

- Not `noexcept`.
- Never touches a `Pane`/`Studio`/`Trace` — a pure value-producing function.
- The interpolation is a plain per-channel linear mix (`detail::mix`), not a
  perceptually-uniform colorspace blend — intermediate stops are RGB-linear
  interpolations between white and `color`.

## See Also

- [`flash::dark_palette`](dark_palette.md): the black-anchored mirror of this function.
- [`flash::diverging_palette`](diverging_palette.md): a two-color, white-midpoint
  variant of the same interpolation idea.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="cubehelix_palette.html">
    <span class="label">Previous</span>
    <span class="title">cubehelix_palette</span>
  </a>
  <a class="next-link" href="dark_palette.html">
    <span class="label">Next</span>
    <span class="title">dark_palette</span>
  </a>
</div>
