<!-- pluss-hand-authored -->

# Pane::band_x

[Back to Pane API Reference](../../pane_api_reference)

## Signature

```cpp
Pane& Pane::band_x(Range1d span, std::string label = {})
```

## What It Does

Draws a shaded reference *band* spanning a range on the x-axis — the span-based
counterpart of [`rule_x()`](rule_x.md)'s single-value line, useful for marking a region
(a safe zone, a confidence band, a highlighted period) rather than one exact value. Like
rules, bands **accumulate**: each `band_x()`/[`band_y()`](band_y.md)/
[`band_z()`](band_z.md) call appends to a shared [`bands()`](bands.md) list rather than
replacing a previous entry, and there is no removal API.

```{important} Same range validation as x_limits(): finite and strictly increasing
`span` must be finite on both ends and strictly increasing (`span.low < span.high`), or
this throws `std::invalid_argument("band_x: range must be finite and strictly
increasing")` — the identical check [`x_limits()`](x_limits.md) performs on its own
`Range1d` argument. See Example 2.
```

## Parameters

**span** (`Range1d`)
    The `{low, high}` range to shade. Must be finite and strictly increasing.

**label** (`std::string`, default `{}`)
    Optional text drawn alongside the band. An empty label draws just the shaded region.

## Returns

`Pane&`
    A reference to the same pane, for chaining.

## Examples

Example 1. A labeled band spanning part of the x-axis.

```cpp
using namespace pluss::pl;

auto fig = studio("band_x_basic");
auto& pane = fig.pane2();
pane.trend(std::vector<double>{1.0, 2.0, 3.0, 4.0, 5.0}, std::vector<double>{2.0, 4.0, 3.0, 5.0, 6.0});
pane.band_x(Range1d{2.0, 3.5}, "region A");
fig.show();

std::cout << pane.bands().size() << "\n";
std::cout << pane.bands()[0].span.low << " " << pane.bands()[0].span.high << "\n";
```

### Output

```text
1
2 3.5
```

![Output of Example 1](img/band_x-1-basic.svg)

Example 2. A reversed range throws — the same validation as `x_limits()`.

```cpp
using namespace pluss::pl;

auto fig = studio("band_x_failure");
auto& pane = fig.pane2();
pane.trend(std::vector<double>{1.0, 2.0}, std::vector<double>{1.0, 2.0});

try {
  pane.band_x(Range1d{5.0, 2.0});
} catch (const std::invalid_argument& e) {
  std::cout << e.what() << "\n";
}
```

### Output

```text
band_x: range must be finite and strictly increasing
```

## Notes

- Not `noexcept` (validates the range).
- Bands accumulate across calls, and across axes — same pattern as
  [`rule_x()`](rule_x.md), just with `Range1d` spans instead of single values.
- No removal API exists for individual or all bands once added.

## See Also

- [`Pane::band_y`](band_y.md) / [`Pane::band_z`](band_z.md): the y- and z-axis
  equivalents, appending to the same list.
- [`Pane::bands`](bands.md): reads back every band added so far.
- [`Pane::rule_x`](rule_x.md): a single-value reference line, as opposed to a span.
- [`Pane::x_limits`](x_limits.md): shares the identical range-validation logic.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="rule_z.html">
    <span class="label">Previous</span>
    <span class="title">rule_z</span>
  </a>
  <a class="next-link" href="band_y.html">
    <span class="label">Next</span>
    <span class="title">band_y</span>
  </a>
</div>
