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
    Number of stops, at least `3` (the ramp needs its two ends and a midpoint); fewer throws `pluss::Error`.

## Returns

`PaletteRamp`
    Named `"diverging"`. The stop at index `n / 2` (integer division) is the ramp's
    white midpoint.

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

Example 1. The default seven stops between two colours, passing through a light midpoint.

```cpp
describe(flash::diverging_palette(blue, red));
```

Output:

```text
7 stops; first (30, 80, 200), last (200, 40, 40)
```

Example 2. The minimum of three stops: both ends and the midpoint.

```cpp
describe(flash::diverging_palette(blue, red, 3));
```

Output:

```text
3 stops; first (30, 80, 200), last (200, 40, 40)
```

Example 3. Swapping the colours reverses the ramp; eleven stops.

```cpp
describe(flash::diverging_palette(red, blue, 11));
```

Output:

```text
11 stops; first (200, 40, 40), last (30, 80, 200)
```

Example 4. Fewer than three stops is refused.

```cpp
try { flash::diverging_palette(blue, red, 2); } catch (const pluss::Error &e) { std::cout << e.message() << "\n"; }
```

Output:

```text
flash::diverging_palette: n must be at least 3
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
