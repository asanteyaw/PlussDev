<!-- pluss-hand-authored -->

# Series::aminmax

[Back to Series API Reference](../../series_api_reference)

## Signature

```cpp
std::pair<Series, Series> Series::aminmax() const
std::pair<Series, Series> Series::aminmax(std::int64_t dim, bool keepdim = false) const
```

## What It Does

Returns the smallest and the largest value together, as one call.

`aminmax()` exists because "what is the range of this data" is a single
question, and answering it by calling [`amin()`](amin.md) and
[`amax()`](amax.md) separately walks the tensor twice and reads as two
unrelated statements. `aminmax()` walks it once and hands back a
`std::pair<Series, Series>` — `.first` is the minimum, `.second` is the maximum
— so the pair travels together into whatever needs both: an axis range for a
plot, the denominator of a min-max rescale, a sanity-check bound, the endpoints
for [`histogram()`](histogram.md).

Two forms:

- **`aminmax()`** collapses the whole tensor to two scalars.
- **`aminmax(dim, keepdim)`** reduces one axis. Each result has the input shape
  with `dim` removed, or kept as a length-1 axis when `keepdim` is `true`.

The order in the pair is always `(min, max)`.

## Parameters

**dim** (`std::int64_t`): the axis to reduce. Negative values count from the
end. Omit it (call the no-argument form) to reduce every axis.

**keepdim** (`bool`): when `true`, the reduced axis stays in the result as a
length-1 dimension so the outputs still broadcast against the input. Default
`false`.

## Returns

`std::pair<Series, Series>`
    `{min, max}`. Both pieces share the input's dtype and platform. The
    whole-tensor form yields two 0-D scalars; the `dim` form yields two series
    shaped like the input with `dim` reduced.

Structured bindings read cleanly: `auto [lo, hi] = x.aminmax();`.

## Gradient

Both channels are differentiable. In a reverse pass:

- the gradient arriving at `.first` (the min) is routed to the input position
  that held the minimum,
- the gradient arriving at `.second` (the max) is routed to the position that
  held the maximum,
- every other position receives `0`.

If several positions tie for the extremum, the incoming gradient is split
evenly among them (the same rule [`amin()`](amin.md) / [`amax()`](amax.md)
use). Integer inputs carry no gradient.

## Examples

Example 1. The whole-tensor form: two scalars.

```cpp
auto [lo, hi] =
    pluss::make_series({3.0, 1.0, 4.0, 1.0, 5.0, 9.0, 2.0, 6.0}).aminmax();

std::cout << "min=" << lo;
std::cout << "max=" << hi;
```

### Output

```text
min=pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=1max=pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=9
```

Example 2. `dim = 0` reduces down each column; the axis is dropped.

```cpp
auto m = pluss::make_series({
    {1.0, 5.0, 2.0},
    {8.0, 3.0, 7.0},
    {4.0, 0.0, 9.0},
});
auto [lo, hi] = m.aminmax(0);

std::cout << "min=" << lo;
std::cout << "max=" << hi;
```

### Output

```text
min=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[1, 0, 2]max=pluss::Series(shape=[3], strides=[1], dtype=float64, platform=cpu, numel=3, contiguous=true)
data=[8, 5, 9]
```

Column `0` ranges `1..8`, column `1` ranges `0..5`, column `2` ranges `2..9`.

Example 3. `keepdim = true` keeps the reduced axis as length `1`, so the
results still line up against the input for broadcasting.

```cpp
auto m = pluss::make_series({{1.0, 5.0, 2.0}, {8.0, 3.0, 7.0}});
auto [lo, hi] = m.aminmax(1, true);

std::cout << "min=" << lo;
std::cout << "max=" << hi;
```

### Output

```text
min=pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[1],
  [3]]max=pluss::Series(shape=[2,1], strides=[1,1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[[5],
  [8]]
```

Example 4. Because the pair travels together, the span is one subtraction.

```cpp
auto [lo, hi] =
    pluss::make_series({{2.0, 9.0, 4.0}, {6.0, 1.0, 8.0}}).aminmax(1);

pluss::series::print(std::cout, hi.sub(lo));
```

### Output

```text
Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[7.0000, 7.0000]
```

Both rows happen to span exactly `7`.

Example 5. `NaN` **propagates**: a single `NaN` anywhere in a reduced group
makes both the min and the max `NaN` for that group.

```cpp
auto [lo, hi] = pluss::make_series(
    {1.0, std::numeric_limits<double>::quiet_NaN(), 3.0}).aminmax();

std::cout << "min=" << lo;
std::cout << "max=" << hi;
```

### Output

```text
min=pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=nanmax=pluss::Series(shape=[], strides=[], dtype=float64, platform=cpu, numel=1, contiguous=true)
data=nan
```

This matches [`amin()`](amin.md) / [`amax()`](amax.md) and the wider
reduction convention — an unknown value makes the extremes unknown. To get the
extremes of the finite values only, drop the `NaN`s first (for example
`x.index_select(0, x.isnan().logical_not().nonzero().squeeze(-1))`) or use a
NaN-aware statistic.

Example 6. Both channels are differentiable, each routing its gradient to the
position of its extremum. Here `min = 1` is tied between positions `1` and `3`.

```cpp
auto x = pluss::make_series({3.0, 1.0, 4.0, 1.0, 5.0});
x.tracks_slope_(true);

auto [lo, hi] = x.aminmax();
(lo.mul(10.0).add(hi.mul(100.0))).reverse();

pluss::series::print(std::cout, x.sensitivity());
```

### Output

```text
Series(shape=[5], strides=[1], dtype=float64, platform=cpu, numel=5, contiguous=true)
data=[0.0000, 5.0000, 0.0000, 5.0000, 100.0000]
```

The `100` weight on `max` lands entirely on position `4` (the value `5`). The
`10` weight on `min` is split between the two tied `1`s, `5` each.

Example 7. Reducing an empty tensor is an error — there is no minimum or
maximum to report.

```cpp
try {
  auto range = pluss::series::zeros({0}).aminmax();
  (void)range;
} catch (const std::exception& e) {
  std::string msg = e.what();
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
aminmax: cannot reduce over an empty tensor
```

Example 8. Negative `dim` counts from the end.

```cpp
auto [lo, hi] = pluss::make_series({{1.0, 5.0}, {8.0, 3.0}}).aminmax(-1);

std::cout << "min=" << lo;
std::cout << "max=" << hi;
```

### Output

```text
min=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[1, 3]max=pluss::Series(shape=[2], strides=[1], dtype=float64, platform=cpu, numel=2, contiguous=true)
data=[5, 8]
```

## Gotchas And Failure Modes

- The pair is `(min, max)`, in that order. `auto [lo, hi]` is the natural
  binding; `auto [hi, lo]` compiles and silently swaps the meaning.
- `NaN` propagates: any `NaN` in a reduced group makes both outputs `NaN` for
  that group. Filter `NaN` first if you want the finite extremes.
- Reducing an empty tensor, or an empty `dim`, raises.
- There is no `keepdim` on the whole-tensor form — it always produces 0-D
  scalars.
- Complex inputs have no ordering and are rejected.
- Both outputs carry gradient; do not assume `aminmax()` is a detached
  read-only summary.

## Summary

`aminmax()` is the minimum and maximum in a single reduction:

- returns `std::pair<Series, Series>` = `{min, max}` — bind as
  `auto [lo, hi] = ...`
- whole-tensor form gives two scalars; `aminmax(dim, keepdim)` reduces one axis
- `NaN` propagates (matches [`amin`](amin.md) / [`amax`](amax.md))
- both channels differentiable; gradient routes to the extremum position, ties
  shared
- empty input raises

## See Also

- [`Series::amin`](amin.md) / [`Series::amax`](amax.md): the two halves on their
  own, when you only need one.
- [`Series::max`](max.md) / [`Series::min`](min.md): the value/index pair form
  (`max_pair` / `min_pair`) when you also want *where* the extremum is.
- [`Series::histogram`](histogram.md): a common consumer — `aminmax()` gives the
  default bin range.
- [`Series::clamp_min`](clamp_min.md) / [`Series::clamp_max`](clamp_max.md):
  apply a bound rather than measure one.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="amin.html">
    <span class="label">Previous</span>
    <span class="title">amin</span>
  </a>
  <a class="next-link" href="angle.html">
    <span class="label">Next</span>
    <span class="title">angle</span>
  </a>
</div>
