<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 851cf787a3b3 -->

# DataFrameTsAccessor::weigh

[Back to DataFrameTsAccessor API Reference](../../dataframe_ts_accessor_api_reference)

## Signature

```cpp
DataFrameTsAccessor::weigh(pluss::RangeView<double> weights, WindowStatistic statistic = WindowStatistic::Mean, std::int64_t min_count = 1) const -> DataFrame
```

## What It Does

A trailing rolling statistic where each position in the window is scaled by an
explicit weight instead of contributing equally. `weights.size()` fixes the window
size; `weights[0]` applies to the oldest row in the window and `weights.back()`
applies to the current row. Rows at the very start of the series see a
partially-filled window — the missing (not-yet-reached) positions simply don't
contribute, and the remaining weights are **not renormalized to add up to their
original total** beyond dividing by whatever weight total is actually present, so
early rows are naturally computed from fewer, more-recent weights.

`WindowStatistic::Minimum`/`Maximum` ignore the weight *magnitudes* entirely (only
the window width `weights.size()` matters) and fall back to the same trailing
extremum [`rolling()`](rolling.md) computes.

## Parameters

**weights** (`pluss::RangeView<double>`)

The per-position weight kernel, oldest-to-newest. Must not be empty.

**statistic** (`WindowStatistic`, default `Mean`)

`Sum`, `Mean`, `Minimum`, or `Maximum`. (`Var`/`Std`/`Sem`/`Skew`/`Kurtosis` are not
supported by this method.)

**min_count** (`std::int64_t`, default `1`)

Minimum non-null, in-window observations required for a row's output to be
non-`NULL`. Must be between `1` and `weights.size()`.

## Returns

`DataFrame`
    A new frame, same shape and case labels as the accessor's bound features, with
    each feature replaced by its weighted rolling statistic.

## Examples

Example 1. A 2-element weighted moving average that favors the most recent value.

```cpp
using namespace pluss::table;

auto readings = DataFrame({
    {"sensor_id", Slate(pluss::make_series({1, 2, 3, 4}))},
    {"load",      Slate(pluss::make_series({10.0, 20.0, 15.0, 30.0}))},
});

auto out = readings.features({"load"}).ts().weigh({0.25, 0.75});
std::cout << out;
```

### Output

```text
============
     | load 
------------
0    | 10   
1    | 17.5 
2    | 16.25
3    | 26.25
------------
[4 cs x 1 feat]
```

Row `0` only has the current value in its window (there's no earlier row to pair
with weight `0.25`), so its weighted mean is just `10`, using only the `0.75`
weight. Row `1` blends `10` (weight `0.25`) and `20` (weight `0.75`):
`(10 * 0.25 + 20 * 0.75) / (0.25 + 0.75) = 17.5`. Row `2` blends `20` and `15`:
`(20 * 0.25 + 15 * 0.75) / 1.0 = 16.25`. Row `3` blends `15` and `30`:
`(15 * 0.25 + 30 * 0.75) / 1.0 = 26.25`.

Example 2. `WindowStatistic::Minimum` ignores the weight magnitudes entirely —
two very different weight vectors of the same size produce identical output.

```cpp
auto min_a = readings.features({"load"}).ts().weigh({0.25, 0.75}, WindowStatistic::Minimum);
std::cout << min_a;

auto min_b = readings.features({"load"}).ts().weigh({5.0, 100.0}, WindowStatistic::Minimum);
std::cout << min_b;
```

### Output

```text
===========
     | load
-----------
0    | 10  
1    | 10  
2    | 15  
3    | 15  
-----------
[4 cs x 1 feat]
===========
     | load
-----------
0    | 10  
1    | 10  
2    | 15  
3    | 15  
-----------
[4 cs x 1 feat]
```

`{0.25, 0.75}` and `{5.0, 100.0}` are wildly different weight vectors, but both
have size `2` — since only `weights.size()` (the window width) matters for
`Minimum`/`Maximum`, both calls produce the exact same trailing 2-row minimum
as plain [`rolling(2, Minimum)`](rolling.md).

Example 3. An empty `weights` list, and an out-of-range `min_count`, each throw.

```cpp
try {
  auto out = readings.features({"load"}).ts().weigh({});
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}

try {
  auto out = readings.features({"load"}).ts().weigh({0.25, 0.75}, WindowStatistic::Mean, 3);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Slate::weigh: weights cannot be empty
Slate::weigh: invalid min_count
```

An empty weight kernel has no window width to speak of, so it's rejected
outright. `min_count` must be between `1` and `weights.size()` inclusive; `3`
exceeds the 2-element kernel's own size, so that throws too.

## Notes

- Unlike an unweighted [`rolling()`](rolling.md), the weights let recent
  observations count more (or less) than older ones within the same window — a
  common building block for smoothing filters distinct from
  [`ewm()`](ewm.md)'s continuously-decaying kernel.
- `Minimum`/`Maximum` only use `weights.size()` as the window width; the actual
  weight values are ignored for those two statistics.
- The result keeps the accessor's original case labels; nothing is dropped or
  re-indexed.

## See Also

- [`DataFrameTsAccessor::rolling`](rolling.md): the unweighted (equal-contribution)
  trailing window statistic this method generalizes.
- [`DataFrameTsAccessor::ewm`](ewm.md): a different, continuously-decaying weighting
  scheme rather than a fixed, explicit kernel.
- [`DataFrameTsAccessor::bounded`](bounded.md): fully custom window bounds instead
  of a fixed-size weighted kernel.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="timed.html">
    <span class="label">Previous</span>
    <span class="title">timed</span>
  </a>
  <a class="next-link" href="window.html">
    <span class="label">Next</span>
    <span class="title">window</span>
  </a>
</div>
