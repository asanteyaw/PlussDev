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
Despite the name (evoking the genuine HUSL perceptually-uniform color space), this
function is a plain HLS hue sweep with different fixed parameters -- the same
detail::hsl_to_rgb conversion hls_palette itself uses, not a distinct HUSL algorithm.
The name is a naming convenience, not a claim of colorspace equivalence.
```

## Parameters

**n** (`std::int64_t`, default `6`)
    Forwarded to [`flash::hls_palette`](hls_palette.md) — see that page for the
    clamping behavior (`n < 2` silently becomes `2`).

## Returns

`PaletteRamp`
    Named `"hls"` (inherited from `hls_palette`, not `"husl"`).

## Examples

Example 1 (verification, not a distinct rendering). Genuinely different stops from
`hls_palette`'s own default output, confirming the fixed saturation/lightness override
takes effect.

```cpp
using namespace pluss::pl;

auto hls_default = flash::hls_palette();
auto husl = flash::husl_palette(6);
std::cout << "husl_palette(6) vs hls_palette(6) differ: "
          << (husl.stops()[0].red != hls_default.stops()[0].red || husl.stops()[0].green != hls_default.stops()[0].green) << "\n";
```

Output:

```text
husl_palette(6) vs hls_palette(6) differ: 1
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
