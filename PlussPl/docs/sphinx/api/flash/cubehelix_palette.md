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
Despite the name (evoking Dave Green's genuine cubehelix color scheme -- a spiral
through color space designed to be monotonic in perceived brightness even in
grayscale), this function is the same plain HLS hue sweep flash::hls_palette/
flash::husl_palette use, just with different fixed saturation/lightness. It has no
brightness-monotonicity guarantee — the name is a naming convenience, not a claim of
algorithmic equivalence.
```

## Parameters

**n** (`std::int64_t`, default `6`)
    Forwarded to [`flash::hls_palette`](hls_palette.md) — see that page for the
    clamping behavior (`n < 2` silently becomes `2`).

## Returns

`PaletteRamp`
    Named `"hls"` (inherited from `hls_palette`, not `"cubehelix"`).

## Examples

Example 1 (verification, not a distinct rendering). Genuinely different stops from both
`hls_palette`'s default and `husl_palette`'s output.

```cpp
using namespace pluss::pl;

auto hls_default = flash::hls_palette();
auto husl = flash::husl_palette(6);
auto cube = flash::cubehelix_palette(6);
std::cout << "cubehelix_palette(6) vs hls_palette(6) differ: "
          << (cube.stops()[0].red != hls_default.stops()[0].red || cube.stops()[0].green != hls_default.stops()[0].green) << "\n";
std::cout << "husl_palette(6) vs cubehelix_palette(6) differ: "
          << (husl.stops()[0].red != cube.stops()[0].red || husl.stops()[0].green != cube.stops()[0].green) << "\n";
```

Output:

```text
cubehelix_palette(6) vs hls_palette(6) differ: 1
husl_palette(6) vs cubehelix_palette(6) differ: 1
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
