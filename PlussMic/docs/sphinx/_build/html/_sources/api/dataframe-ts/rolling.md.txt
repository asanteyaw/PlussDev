<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 3e8d46371453 -->

# DataFrameTsAccessor::rolling

[Back to DataFrameTsAccessor API Reference](../../dataframe_ts_accessor_api_reference)

## Signatures

```cpp
DataFrameTsAccessor::rolling(std::int64_t window, WindowStatistic statistic = WindowStatistic::Mean, const WindowOptions& options = {}) const -> DataFrame
DataFrameTsAccessor::rolling(std::string_view duration, WindowStatistic statistic = WindowStatistic::Mean, const StatisticOptions& options = {}, WindowClosure closure = WindowClosure::Right) const -> DataFrame
DataFrameTsAccessor::rolling(pluss::RangeView<DataFrame::WindowSpec> specs, bool keep_input = true) const -> DataFrame
```

## What It Does

Computes a **trailing rolling window statistic** over every bound feature, in one
of three shapes:

- **Fixed row count** (`rolling(window, statistic, options)`): each row's window is
  the `window` rows ending at (and including) that row. The very first `window - 1`
  rows use whatever's available rather than waiting for a full window — row `0`'s
  window is just itself, row `1`'s is rows `0..1`, and so on, until row
  `window - 1` and beyond see a full window. A row's output is `NULL` only if the
  number of non-null values it can see falls below `options.min_count`.
- **Calendar duration** (`rolling(duration, statistic, options, closure)`): the same
  trailing-window idea, but each row's window spans a fixed span of time (like
  `"2D"`) looked up against the accessor's time axis, rather than a fixed row count.
  This overload requires the frame's case labels (or an explicit `TsOptions.axis`)
  to already be a `Timeline`/`TimeIndex`.
- **Structured specs** (`rolling(specs, keep_input)`): delegates to a private
  `DataFrame::window(specs, keep_input)` helper (`DataFrameTsAccessor` is declared
  a `friend` of `DataFrame`, so it can call it even though it isn't part of
  `DataFrame`'s own public interface) — each `WindowSpec` names one feature, one
  fixed-size window, and one of `RollingSum`/`RollingMean`/`RollingMin`/`RollingMax`,
  producing one output column per spec (auto-named `"{feature}_{op}_{window}"`
  when `spec.output` is empty).

## Parameters

**window** (`std::int64_t`, fixed-window overload)

The trailing window size, in rows. Must be positive.

**statistic** (`WindowStatistic`, default `Mean`)

The statistic computed over each window: `Sum`, `Mean`, `Minimum`, `Maximum`, `Var`,
`Std`, `Sem`, `Skew`, or `Kurtosis`.

**options** (`const WindowOptions&`, fixed-window overload)

- **`min_count`** (`std::int64_t`, default `1`): minimum non-null observations
  required for a row's output to be non-`NULL`.
- **`ddof`** (`std::int64_t`, default `1`): delta degrees of freedom for `Var`/`Std`.
- **`centered`** (`bool`, default `false`): whether the window is centered on the
  row instead of trailing it.
- **`closure`** (`WindowClosure`, default `Right`): which end(s) of the window are
  inclusive.

**duration** (`std::string_view`, calendar-duration overload)

A duration string (for example `"2D"`) defining each row's time-based window width.

**closure** (`WindowClosure`, calendar-duration overload, default `Right`)

Which end of the time window is inclusive.

**specs** (`pluss::RangeView<DataFrame::WindowSpec>`, structured overload)

A list of `{output, feature, op, window, min_periods}` records, each describing one
fixed-size rolling column to compute.

**keep_input** (`bool`, structured overload, default `true`)

Whether the original features are kept alongside the newly computed columns
(`true`) or dropped in favor of just the computed columns (`false`).

## Returns

`DataFrame`
    A new frame with each targeted feature replaced (fixed-window and
    calendar-duration overloads) or a frame combining originals and/or computed
    columns (structured overload).

## Examples

Example 1. A fixed-size trailing rolling mean, including its partial windows.

```cpp
using namespace pluss::table;

auto readings = DataFrame({
    {"sensor_id", Thread(pluss::make_series({1, 2, 3, 4}))},
    {"load",      Thread(pluss::make_series({10.0, 20.0, 15.0, 30.0}))},
});

auto out = readings.features({"load"}).ts().rolling(2);
std::cout << out;
```

### Output

```text
===========
     | load
-----------
0    | 10  
1    | 15  
2    | 17.5
3    | 22.5
-----------
[4 cs x 1 feat]
```

With the default `min_count = 1`, row `0` doesn't wait for a full 2-row window —
its output is just itself (`10`). Row `1` sees a full window (`10, 20`, mean
`15`); row `2` sees `(20, 15)` → `17.5`; row `3` sees `(15, 30)` → `22.5`. This
differs from libraries whose rolling default requires a full window before
producing a value — here, that behavior only kicks in if you raise
`options.min_count` to `window`.

Example 2. The same window with `WindowStatistic::Std`, where `ddof = 1` bites at
row `0`.

```cpp
auto rolled_std = readings.features({"load"}).ts().rolling(2, WindowStatistic::Std);
std::cout << rolled_std;
```

### Output

```text
==================
     | load       
------------------
0    | NULL       
1    | 7.071067812
2    | 3.535533906
3    | 10.60660172
------------------
[4 cs x 1 feat]
```

Row `0`'s window has only 1 observation, and the default `ddof = 1` requires more
than `1` observation for a defined variance/std, so it's `NULL` even though
`min_count = 1` alone would have allowed it. Row `1` computes the sample standard
deviation of `(10, 20)`; row `2` of `(20, 15)`; row `3` of `(15, 30)`.

Example 3. `options.centered = true` — the window straddles each row instead of
trailing it.

```cpp
auto centered = readings.features({"load"}).ts().rolling(
    3, WindowStatistic::Mean, WindowOptions{.centered = true});
std::cout << centered;
```

### Output

```text
==================
     | load       
------------------
0    | 15         
1    | 15         
2    | 21.66666667
3    | 22.5       
------------------
[4 cs x 1 feat]
```

With `window = 3` and `centered = true`, row `0`'s window is rows `0..1`
(`10, 20` → `15`, clipped at the series start); row `1` is rows `0..2`
(`10, 20, 15` → `15`); row `2` is rows `1..3` (`20, 15, 30` →
`21.667`); row `3` is rows `2..3` (`15, 30` → `22.5`, clipped at the end) — each
window is centered on its row rather than trailing it, so both edges of the
series see a truncated window instead of just the leading one.

Example 4. Pass a non-positive `window`, to see it throw.

```cpp
try {
  auto out = readings.features({"load"}).ts().rolling(0);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
Thread::moving: window must be > 0
```

A window of `0` (or negative) has no meaningful trailing span, so it's
rejected outright.

## Notes

- The fixed-window overload's default options (`min_count = 1`, `centered = false`,
  `closure = Right`) mean the first `window - 1` rows use a growing partial window
  rather than becoming `NULL` — set `min_count = window` to require a full window
  everywhere instead.
- `Var`/`Std` additionally require the window's observation count to exceed `ddof`
  (default `1`), independent of `min_count`.
- The calendar-duration overload needs a genuine time axis — either the frame's own
  case labels are `Timeline`/`TimeIndex`, or an explicit `TsOptions.axis` supplies
  one — and is not covered by a worked example here since it depends on frequency
  and calendar conventions outside this reference's current verified scope.
- The structured `specs` overload only supports `Sum`/`Mean`/`Min`/`Max` (via
  `ThreadWindowOp`), not the full `WindowStatistic` set the other two overloads
  support.

## See Also

- [`DataFrameTsAccessor::grouped`](grouped.md): the same fixed-window statistic,
  computed independently per group instead of over the whole feature.
- [`DataFrameTsAccessor::expanding`](expanding.md): the same engine with the window
  growing from the start of the series instead of a fixed size.
- [`DataFrameTsAccessor::rolling_block`](rolling_block.md): computing several
  window sizes and statistics of the same feature at once, each as its own output
  column.
- [`DataFrameTsAccessor::window`](window.md): shares this same structured-specs
  overload under a different public name.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="returns.html">
    <span class="label">Previous</span>
    <span class="title">returns</span>
  </a>
  <a class="next-link" href="rolling_block.html">
    <span class="label">Next</span>
    <span class="title">rolling_block</span>
  </a>
</div>
