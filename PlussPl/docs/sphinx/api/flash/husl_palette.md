<!-- pluss-hand-authored -->

# flash::husl_palette

[Back to flash API Reference](../../flash_api_reference)

## Signature

```cpp
PaletteRamp flash::husl_palette(std::int64_t n = 6)
```

## What It Does

A simplified wrapper: `return hls_palette(n, 0.74,
0.55);` — [`flash::hls_palette`](hls_palette.md) with fixed `saturation = 0.74`,
`lightness = 0.55` (higher saturation, slightly higher lightness than
`hls_palette`'s own `0.68`/`0.52` defaults), no way to override either.

```{important} Not an actual HUSL colorspace implementation
Despite the name (evoking the HUSL perceptually-uniform color space), this
function is a plain HLS hue sweep with different fixed parameters -- the same
conversion hls_palette itself uses, not a distinct HUSL algorithm.
The name is a naming convenience, not a claim of colorspace equivalence.
```

## Parameters

**n** (`std::int64_t`, default `6`)

Forwarded to [`flash::hls_palette`](hls_palette.md) — see that page for the
minimum.

## Returns

`PaletteRamp`
    Named `"hls"` (inherited from `hls_palette`, not `"husl"`).

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

Example 1. The default six evenly spaced hues.

```cpp
describe(flash::husl_palette());
```

Output:

```text
6 stops; first (225, 55, 55), last (225, 55, 225)
```

Example 2. Three hues.

```cpp
describe(flash::husl_palette(3));
```

Output:

```text
3 stops; first (225, 55, 55), last (55, 55, 225)
```

Example 3. Twelve hues for a many-category chart.

```cpp
describe(flash::husl_palette(12));
```

Output:

```text
12 stops; first (225, 55, 55), last (225, 55, 140)
```

Example 4. Zero colours is refused (the message names the generator it forwards to).

```cpp
try { flash::husl_palette(0); } catch (const pluss::Error &e) { std::cout << e.message() << "\n"; }
```

Output:

```text
flash::hls_palette: n must be at least 1
```

## Notes

- Not `noexcept`.
- Never touches a `Pane`/`Studio`/`Trace` — a pure value-producing function.
- Returned ramp's `name()` is `"hls"`, not `"husl"` — see the field description above;
  this is `hls_palette`'s own hardcoded name, unchanged by this wrapper.

## See Also

- [`flash::hls_palette`](hls_palette.md): the function this one forwards to with fixed
  parameters.
- [`flash::cubehelix_palette`](cubehelix_palette.md): a sibling fixed-parameter wrapper
  around the same underlying function.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="hls_palette.html">
    <span class="label">Previous</span>
    <span class="title">hls_palette</span>
  </a>
  <a class="next-link" href="cubehelix_palette.html">
    <span class="label">Next</span>
    <span class="title">cubehelix_palette</span>
  </a>
</div>
