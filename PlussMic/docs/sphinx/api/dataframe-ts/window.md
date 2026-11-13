<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 1ddf5b60da76 -->

# DataFrameTsAccessor::window

[Back to DataFrameTsAccessor API Reference](../../dataframe_ts_accessor_api_reference)

## Signatures

```cpp
DataFrame DataFrameTsAccessor::window(pluss::RangeView<DataFrame::WindowSpec> specs, bool keep_input = true) const
DataFrameTsWindowAccessor DataFrameTsAccessor::window(std::string_view duration, WindowClosure closure = WindowClosure::Right) const
```

## What It Does

Two unrelated shapes share this name:

- **Structured specs** (`window(specs, keep_input)`): delegates to a private
  `DataFrame::window(specs, keep_input)` helper (`DataFrameTsAccessor` is declared
  a `friend` of `DataFrame`, so it can reach it even though it isn't part of
  `DataFrame`'s own public interface) — each `DataFrame::WindowSpec` names one
  feature, a fixed window size, one of
  `RollingSum`/`RollingMean`/`RollingMin`/`RollingMax`, and a `min_periods`,
  producing one computed output column per spec. When `spec.output` is empty, the
  column is auto-named `"{feature}_{op}_{window}"` (for example
  `"load_rollmean_2"`).
- **Calendar duration** (`window(duration, closure)`): enters a
  `DataFrameTsWindowAccessor` scoped to a fixed time span (like `"2D"`) against the
  accessor's time axis, whose own `sum()`/`mean()`/`min()`/`max()`/`count()`
  methods then compute grouped, duration-bucketed aggregates. This overload
  requires `TsOptions.group` to already be set and a genuine time axis (the
  frame's case labels already `Timeline`/`TimeIndex`, or an explicit
  `TsOptions.axis`).

## Parameters

**specs** (`pluss::RangeView<DataFrame::WindowSpec>`, structured overload)

A list of `{output, feature, op, window, min_periods}` records, each describing one
fixed-size rolling column to compute.

**keep_input** (`bool`, structured overload, default `true`)

Whether the original features are kept alongside the newly computed columns
(`true`) or dropped in favor of just the computed columns (`false`).

**duration** (`std::string_view`, calendar-duration overload)

A duration string (for example `"2D"`) defining the fixed time span each bucket in
the resulting `DataFrameTsWindowAccessor` spans.

**closure** (`WindowClosure`, calendar-duration overload, default `Right`)

Which end of each time bucket is inclusive.

## Returns

`DataFrame`
    (Structured overload) A frame combining the original and/or newly computed
    rolling columns.

`DataFrameTsWindowAccessor`
    (Calendar-duration overload) An accessor exposing duration-bucketed
    `sum()`/`mean()`/`min()`/`max()`/`count()`.

## Examples

Example 1. Compute a named rolling-mean column via a structured `WindowSpec`,
keeping the original feature alongside it.

```cpp
using namespace pluss::table;

auto readings = DataFrame({
    {"sensor_id", Slate(pluss::make_series({1, 2, 3, 4}))},
    {"load",      Slate(pluss::make_series({10.0, 20.0, 15.0, 30.0}))},
});

DataFrame::WindowSpec spec;
spec.feature = "load";
spec.op = SlateWindowOp::RollingMean;
spec.window = 2;

auto out = readings.features({"load"}).ts().window({spec});
std::cout << out;
```

### Output

```text
============================
     | load  load_rollmean_2
----------------------------
0    | 10    10             
1    | 20    15             
2    | 15    17.5           
3    | 30    22.5           
----------------------------
[4 cs x 2 feat]
```

`spec.output` was left empty, so the computed column is auto-named
`"load_rollmean_2"` (`{feature}_{op}_{window}`). Its values match
[`rolling(2)`](rolling.md)'s trailing-mean output exactly. Because `keep_input`
defaults to `true`, the original `load` column is kept alongside it rather than
being replaced.

## Notes

- The structured-specs overload only supports `Sum`/`Mean`/`Min`/`Max` (via
  `SlateWindowOp`), not the full `WindowStatistic` set
  [`rolling()`](rolling.md)'s other overloads support.
- The calendar-duration overload's output depends on frequency and calendar
  bucketing conventions outside this reference's current verified scope, so it
  isn't covered by a worked example here.
- `window(specs, keep_input)` and [`rolling(specs, keep_input)`](rolling.md) (the
  structured-specs overload of `rolling()`) are the exact same operation under two
  names.

## See Also

- [`DataFrameTsAccessor::rolling`](rolling.md): the fixed-window and
  calendar-duration statistic methods this method's two overloads parallel, and
  whose own structured-specs overload is this exact same private helper under a
  different public name.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="weigh.html">
    <span class="label">Previous</span>
    <span class="title">weigh</span>
  </a>
  <a class="next-link" href="zscore_rolling.html">
    <span class="label">Next</span>
    <span class="title">zscore_rolling</span>
  </a>
</div>
