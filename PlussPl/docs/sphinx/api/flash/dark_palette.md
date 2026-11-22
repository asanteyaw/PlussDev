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

Number of stops, at least `2`; fewer throws `pluss::Error`.

## Returns

`PaletteRamp`
    Named `"dark"`. `stops()[0]` is exactly black; `stops()[n-1]` is exactly `color`.

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

Example 1. The default six stops, from black to the target colour.

```cpp
describe(flash::dark_palette(blue));
```

Output:

```text
6 stops; first (0, 0, 0), last (30, 80, 200)
```

Example 2. Three stops.

```cpp
describe(flash::dark_palette(blue, 3));
```

Output:

```text
3 stops; first (0, 0, 0), last (30, 80, 200)
```

Example 3. A longer ramp toward a different colour.

```cpp
describe(flash::dark_palette(red, 10));
```

Output:

```text
10 stops; first (0, 0, 0), last (200, 40, 40)
```

Example 4. Fewer than two stops is refused.

```cpp
try { flash::dark_palette(blue, 1); } catch (const pluss::Error &e) { std::cout << e.message() << "\n"; }
```

Output:

```text
flash::dark_palette: n must be at least 2
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
