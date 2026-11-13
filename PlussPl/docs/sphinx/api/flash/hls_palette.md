<!-- pluss-hand-authored -->

# flash::hls_palette

[Back to flash API Reference](../../flash_api_reference)

## Signature

```cpp
PaletteRamp flash::hls_palette(
    std::int64_t n = 6, double saturation = 0.68, double lightness = 0.52)
```

## What It Does

A generated ramp: `n` colors evenly spaced around the
HSL hue wheel (`hue = i / n` for `i` in `[0, n)`), all sharing the same `saturation`
and `lightness`. [`flash::husl_palette`](husl_palette.md) and
[`flash::cubehelix_palette`](cubehelix_palette.md) are both implemented as fixed calls
to this function with different hardcoded `saturation`/`lightness` pairs — see those
pages.

## Parameters

**n** (`std::int64_t`, default `6`)
    Number of evenly-spaced hues to generate, at least `1`; `0` or a negative value throws `pluss::Error`.

**saturation** (`double`, default `0.68`)
    HSL saturation, shared by every generated stop.

**lightness** (`double`, default `0.52`)
    HSL lightness, shared by every generated stop.

## Returns

`PaletteRamp`
    Named `"hls"` regardless of `n`/`saturation`/`lightness` — every ramp this function
    (and its two callers below) produces shares that one name; only the `stops()`
    content differs.

## Examples

Example 1. Default parameters, and an explicit larger `n`.

```cpp
using namespace pluss::pl;

auto hls_default = flash::hls_palette();
auto hls_n10 = flash::hls_palette(10);
std::cout << "hls_palette() default stops=" << hls_default.stops().size() << " name=" << hls_default.name() << "\n";
std::cout << "hls_palette(10) stops=" << hls_n10.stops().size() << "\n";
```

Output:

```text
hls_palette() default stops=6 name=hls
hls_palette(10) stops=10
```

Example 2 (a comparison, not a new picture). `husl_palette`/`cubehelix_palette`
both differ from `hls_palette`'s own default output, and from each other —
confirming their fixed `saturation`/`lightness` overrides take effect.

```cpp
using namespace pluss::pl;

auto hls_default = flash::hls_palette();
auto husl = flash::husl_palette(6);
auto cube = flash::cubehelix_palette(6);

// All three share the same n and hue sweep; only saturation/lightness differ, so
// comparing any one stop's color is sufficient to detect a difference.
std::cout << "husl_palette(6) vs hls_palette(6) differ: "
          << (husl.stops()[0].red != hls_default.stops()[0].red || husl.stops()[0].green != hls_default.stops()[0].green) << "\n";
std::cout << "cubehelix_palette(6) vs hls_palette(6) differ: "
          << (cube.stops()[0].red != hls_default.stops()[0].red || cube.stops()[0].green != hls_default.stops()[0].green) << "\n";
```

Output:

```text
husl_palette(6) vs hls_palette(6) differ: 1
cubehelix_palette(6) vs hls_palette(6) differ: 1
```

## Notes

- Not `noexcept`.
- Never touches a `Pane`/`Studio`/`Trace` — a pure value-producing function.
- Hue spacing is always `i / n` (never `i / (n - 1)`), so the wheel wraps evenly without
  duplicating the starting hue at the end — the same "non-inclusive endpoint" spacing
  convention used elsewhere in `flash::` (e.g.
  [`flash::hyper_animation_sequence`](hyper_animation_sequence.md)'s default
  `include_endpoint = false`).

## See Also

- [`flash::husl_palette`](husl_palette.md), [`flash::cubehelix_palette`](cubehelix_palette.md):
  fixed-saturation/lightness callers of this function.
- [`flash::light_palette`](light_palette.md), [`flash::dark_palette`](dark_palette.md):
  a different generation strategy — interpolating toward/from a single target color,
  rather than sweeping hue at fixed saturation/lightness.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="blend_palette.html">
    <span class="label">Previous</span>
    <span class="title">blend_palette</span>
  </a>
  <a class="next-link" href="husl_palette.html">
    <span class="label">Next</span>
    <span class="title">husl_palette</span>
  </a>
</div>
