<!-- pluss-hand-authored -->

# flash::cubehelix_palette

[Back to flash API Reference](../../flash_api_reference)

## Signature

```cpp
PaletteRamp flash::cubehelix_palette(std::int64_t n = 6)
```

## What It Does

A simplified wrapper: `return hls_palette(n,
0.58, 0.46);` — [`flash::hls_palette`](hls_palette.md) with fixed `saturation = 0.58`,
`lightness = 0.46` (lower saturation and lightness than both `hls_palette`'s own
defaults and [`flash::husl_palette`](husl_palette.md)'s), no way to override either.

```{important} Not an actual cubehelix algorithm
Despite the name (evoking Dave Green's cubehelix color scheme -- a spiral
through color space designed to be monotonic in perceived brightness even in
grayscale), this function is the same plain HLS hue sweep flash::hls_palette/
flash::husl_palette use, just with different fixed saturation/lightness. It has no
brightness-monotonicity guarantee — the name is a naming convenience, not a claim of
algorithmic equivalence.
```

## Parameters

**n** (`std::int64_t`, default `6`)
    Forwarded to [`flash::hls_palette`](hls_palette.md) — see that page for the
    minimum.

## Returns

`PaletteRamp`
    Named `"hls"` (inherited from `hls_palette`, not `"cubehelix"`).

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

Example 1. The default six stops.

```cpp
describe(flash::cubehelix_palette());
```

Output:

```text
6 stops; first (185, 49, 49), last (185, 49, 185)
```

Example 2. Four stops.

```cpp
describe(flash::cubehelix_palette(4));
```

Output:

```text
4 stops; first (185, 49, 49), last (117, 49, 185)
```

Example 3. Nine stops.

```cpp
describe(flash::cubehelix_palette(9));
```

Output:

```text
9 stops; first (185, 49, 49), last (185, 49, 140)
```

Example 4. Zero colours is refused (the message names the generator it forwards to).

```cpp
try { flash::cubehelix_palette(0); } catch (const pluss::Error &e) { std::cout << e.message() << "\n"; }
```

Output:

```text
flash::hls_palette: n must be at least 1
```

## Notes

- Not `noexcept`.
- Never touches a `Pane`/`Studio`/`Trace` — a pure value-producing function.
- Returned ramp's `name()` is `"hls"`, not `"cubehelix"`.

## See Also

- [`flash::hls_palette`](hls_palette.md): the function this one forwards to with fixed
  parameters.
- [`flash::husl_palette`](husl_palette.md): a sibling fixed-parameter wrapper around
  the same underlying function.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="husl_palette.html">
    <span class="label">Previous</span>
    <span class="title">husl_palette</span>
  </a>
  <a class="next-link" href="light_palette.html">
    <span class="label">Next</span>
    <span class="title">light_palette</span>
  </a>
</div>
