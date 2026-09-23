<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 44c70fa1d0a5 -->

# DataFrameTsAccessor::expanding

[Back to DataFrameTsAccessor API Reference](../../dataframe_ts_accessor_api_reference)

## Signature

```cpp
DataFrameTsAccessor::expanding(WindowStatistic statistic = WindowStatistic::Mean, const StatisticOptions& options = {}) const -> DataFrame
```

## What It Does

The same trailing-window engine as [`rolling()`](rolling.md), except the window
always starts at row `0` and grows to include every row up to and including the
current one — equivalent to calling `rolling(ncases(), statistic, {...})` with a
window size at least as large as the whole series.

## Parameters

**statistic** (`WindowStatistic`, default `Mean`)

The statistic computed over each row's growing window — see [`rolling()`](rolling.md)
for the full set.

**options** (`const StatisticOptions&`)

- **`ddof`** (`std::int64_t`, default `1`): delta degrees of freedom for `Var`/`Std`.
- **`min_count`** (`std::int64_t`, default `1`): minimum non-null observations
  required for a row's output to be non-`NULL`.

## Returns

`DataFrame`
    A new frame, same shape and case labels as the accessor's bound features, with
    each feature replaced by its expanding statistic.

## Examples

Example 1. An expanding mean and an expanding standard deviation over the same
feature.

```cpp
using namespace pluss::table;

auto readings = DataFrame({
    {"sensor_id", Thread(pluss::make_series({1, 2, 3, 4}))},
    {"load",      Thread(pluss::make_series({10.0, 20.0, 15.0, 30.0}))},
});

auto expanding_mean = readings.features({"load"}).ts().expanding();
std::cout << expanding_mean;
```

### Output

```text
============
     | load 
------------
0    | 10   
1    | 15   
2    | 15   
3    | 18.75
------------
[4 cs x 1 feat]
```

Row `0`'s window is just `[10]` → `10`. Row `1`'s window is `[10, 20]` → `15`. Row
`2`'s window is `[10, 20, 15]` → `15`. Row `3`'s window is every value so far,
`[10, 20, 15, 30]` → `18.75`.

```cpp
auto expanding_std = readings.features({"load"}).ts().expanding(WindowStatistic::Std);
std::cout << expanding_std;
```

### Output

```text
==================
     | load       
------------------
0    | NULL       
1    | 7.071067812
2    | 5          
3    | 8.539125638
------------------
[4 cs x 1 feat]
```

Row `0` has only 1 observation, and the default `ddof = 1` requires more than that
for a defined standard deviation, so it's `NULL`. Row `1` is the sample std of
`(10, 20)`; row `2` of `(10, 20, 15)`; row `3` of `(10, 20, 15, 30)`.

## Notes

- `expanding()` is exactly [`rolling()`](rolling.md) with a window as large as the
  whole series — every row's output reflects every prior row (plus itself),
  never a fixed-size trailing subset.
- `Var`/`Std` still require the growing window's observation count to exceed
  `ddof` (default `1`), same as [`rolling()`](rolling.md).
- The result keeps the accessor's original case labels; nothing is dropped or
  re-indexed.

## See Also

- [`DataFrameTsAccessor::rolling`](rolling.md): the fixed-size trailing window this
  method is a special case of.
- [`DataFrameTsAccessor::ewm`](ewm.md): a different kind of "whole history so far"
  statistic, weighted by exponential decay instead of equally.
- [`DataFrame::cumsum`](../dataframe/cumsum.md),
  [`DataFrame::scan`](../dataframe/scan.md): related running statistics outside the
  `ts()` accessor.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="ewm.html">
    <span class="label">Previous</span>
    <span class="title">ewm</span>
  </a>
  <a class="next-link" href="fill_gaps.html">
    <span class="label">Next</span>
    <span class="title">fill_gaps</span>
  </a>
</div>
