<!-- pluss-hand-authored -->

# flash::diverging_palette

[Back to flash API Reference](../../flash_api_reference)

## Signature

```cpp
PaletteRamp flash::diverging_palette(Rgba left, Rgba right, std::int64_t n = 7)
```

## What It Does

A two-sided ramp: interpolates from `left` to
white at the midpoint, then from white to `right` — the two-color analogue of
[`flash::light_palette`](light_palette.md)'s single-color-toward-white ramp, useful for
signed data (e.g. negative-to-positive heatmaps) where white should mark the neutral
midpoint.

## Parameters

**left** (`Rgba`)
    The color at index `0`.

**right** (`Rgba`)
    The color at index `n - 1`.

**n** (`std::int64_t`, default `7`)
    Number of stops. Clamped to at least `3` (`std::max<std::int64_t>(n, 3)`) — a
    diverging ramp with fewer than 3 stops couldn't represent both ends plus a distinct
    midpoint.

## Returns

`PaletteRamp`
    Named `"diverging"`. The stop at index `n / 2` (integer division) is the ramp's
    white midpoint.

## Examples

Example 1. Red-to-blue diverging ramp, 7 stops — the midpoint stop confirmed to be
near-white.

```cpp
using namespace pluss::pl;

auto div = flash::diverging_palette(Rgba::from_u8(200, 30, 30), Rgba::from_u8(30, 30, 200), 7);
std::cout << "diverging_palette stops=" << div.stops().size() << " mid(white-ish)="
          << (div.stops()[3].red > 0.85 && div.stops()[3].green > 0.85 && div.stops()[3].blue > 0.85) << "\n";
```

Output:

```text
diverging_palette stops=7 mid(white-ish)=1
```

## Notes

- Not `noexcept`.
- Never touches a `Pane`/`Studio`/`Trace` — a pure value-producing function.
- The midpoint index is `n / 2` via integer division — for even `n`, this lands
  slightly past the true geometric center (e.g. `n = 6` → midpoint index `3`, the
  4th of 6 stops).
- Same plain per-channel linear-mix interpolation as
  [`flash::light_palette`](light_palette.md)/[`flash::dark_palette`](dark_palette.md).

## See Also

- [`flash::light_palette`](light_palette.md), [`flash::dark_palette`](dark_palette.md):
  the single-color-anchored building blocks this function's two-sided interpolation is
  conceptually built from (though implemented directly, not by calling either).

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dark_palette.html">
    <span class="label">Previous</span>
    <span class="title">dark_palette</span>
  </a>
  <a class="next-link" href="mpl_palette.html">
    <span class="label">Next</span>
    <span class="title">mpl_palette</span>
  </a>
</div>
