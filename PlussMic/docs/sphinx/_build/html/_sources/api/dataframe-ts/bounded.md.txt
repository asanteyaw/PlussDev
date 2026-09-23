<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: e20e0347dcc6 -->

# DataFrameTsAccessor::bounded

[Back to DataFrameTsAccessor API Reference](../../dataframe_ts_accessor_api_reference)

## Signature

```cpp
DataFrameTsAccessor::bounded(const WindowBoundsFn& bounds, WindowStatistic statistic = WindowStatistic::Mean, const WindowOptions& options = {}) const -> DataFrame
```

## What It Does

The most general window primitive: instead of a fixed trailing size
([`rolling()`](rolling.md)), a growing size ([`expanding()`](expanding.md)), or a
duration, `bounded()` takes a callback that computes each row's `[start, end)`
window bounds directly, given the case count and `options`. This lets you express
window shapes none of the other accessor methods can — for example, a window that
deliberately excludes the current row.

## Parameters

**bounds** (`const WindowBoundsFn&`)

A callback `(std::int64_t size, const WindowOptions& options) -> pair<vector<int64_t>, vector<int64_t>>`
returning one `[start, end)` pair per row. Must not be empty.

**statistic** (`WindowStatistic`, default `Mean`)

The statistic computed over each row's custom window — see [`rolling()`](rolling.md)
for the full set.

**options** (`const WindowOptions&`)

Passed through to `bounds` unchanged, and used for `min_count`/`ddof` when
evaluating the statistic — see [`rolling()`](rolling.md).

## Returns

`DataFrame`
    A new frame, same shape and case labels as the accessor's bound features, with
    each feature replaced by its custom-bounded window statistic.

## Examples

Example 1. A "leave-one-out expanding" window — everything strictly before the
current row, excluding the row itself.

```cpp
using namespace pluss::table;

auto readings = DataFrame({
    {"sensor_id", Thread(pluss::make_series({1, 2, 3, 4}))},
    {"load",      Thread(pluss::make_series({10.0, 20.0, 15.0, 30.0}))},
});

auto exclude_current = [](std::int64_t size, const WindowOptions&) {
  std::vector<std::int64_t> starts(size, 0);
  std::vector<std::int64_t> ends(size);
  for (std::int64_t row = 0; row < size; ++row) {
    ends[row] = row;  // stop before the current row
  }
  return std::make_pair(starts, ends);
};

auto out = readings.features({"load"}).ts().bounded(exclude_current);
std::cout << out;
```

### Output

```text
===========
     | load
-----------
0    | NULL
1    | 10  
2    | 15  
3    | 15  
-----------
[4 cs x 1 feat]
```

Row `0`'s window is `[0, 0)` — empty, since there's nothing before it — so with
`min_count = 1` it's `NULL`. Row `1`'s window is `[0, 1)`, just `load[0] = 10`. Row
`2`'s window is `[0, 2)`, the mean of `10` and `20` = `15`. Row `3`'s window is
`[0, 3)`, the mean of `10`, `20`, `15` = `15`. Unlike
[`expanding()`](expanding.md), each row's window here never includes its own value
— useful for leave-one-out baselines or detecting how far a row deviates from
everything observed strictly before it.

Example 2. The same custom bounds, with `statistic = Sum` instead of the
default `Mean`.

```cpp
auto summed = readings.features({"load"}).ts().bounded(exclude_current, WindowStatistic::Sum);
std::cout << summed;
```

### Output

```text
===========
     | load
-----------
0    | NULL
1    | 10  
2    | 30  
3    | 45  
-----------
[4 cs x 1 feat]
```

Same leave-one-out windows as Example 1, just summed instead of averaged: row
`1` is `load[0] = 10`; row `2` is `10 + 20 = 30`; row `3` is
`10 + 20 + 15 = 45`. Row `0`'s window is still empty, so it's `NULL`
regardless of which statistic is requested.

Example 3. Pass an empty `WindowBoundsFn`, to see the call throw.

```cpp
using namespace pluss::table;

WindowBoundsFn empty_fn;

try {
  auto out = readings.features({"load"}).ts().bounded(empty_fn);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
DataFrameTsAccessor::bounded: bounds callback is empty
```

A default-constructed `std::function` holds no callable target; `bounded()`
checks for that up front rather than crashing when it tries to invoke `bounds`.

## Notes

- `bounds` is called once per `bounded()` invocation (not once per row) and must
  return one `[start, end)` pair per row, each satisfying
  `0 <= start <= end <= size`.
- `min_count`/`ddof` from `options` still apply to whatever window each row ends up
  with, exactly as in [`rolling()`](rolling.md) — an empty or too-small window
  still produces `NULL` rather than an error.
- This is the primitive [`rolling()`](rolling.md), [`expanding()`](expanding.md),
  and [`grouped()`](grouped.md) are all built from with a fixed bounds strategy;
  reach for it directly only when none of those shapes fit.

## See Also

- [`DataFrameTsAccessor::rolling`](rolling.md), [`DataFrameTsAccessor::expanding`](expanding.md):
  the fixed-size and growing-from-start window shapes this method generalizes.
- [`DataFrameTsAccessor::weigh`](weigh.md): a fixed-size window with per-position
  weights instead of custom bounds.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="asfreq.html">
    <span class="label">Previous</span>
    <span class="title">asfreq</span>
  </a>
  <a class="next-link" href="calendar.html">
    <span class="label">Next</span>
    <span class="title">calendar</span>
  </a>
</div>
