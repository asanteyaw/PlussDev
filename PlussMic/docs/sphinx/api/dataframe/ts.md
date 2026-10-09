<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 39f64dd9bd4f -->

# DataFrame::ts

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::ts() const -> DataFrameTsAccessor
DataFrame::ts(const TsOptions& options) const -> DataFrameTsAccessor
```

## What It Does

Enters the dedicated time-series-*numerics* accessor for the current frame —
period-over-period returns, differencing, lag/lead, rolling and expanding windows,
exponentially-weighted statistics, and more, one method per operation. Unlike
[`tl()`](tl.md) (which requires `Timeline`/`TimeIndex` features), `ts()` requires
**every feature currently on the frame to already be a numeric `Series`** — narrow
to the numeric feature(s) first with [`features()`](features.md), the same idiom
`note()`/`tl()` use for their own kind requirement.

`options` (a `TsOptions`) optionally supplies an explicit time `axis`, a `group` (for
grouped/panel time-series operations), and an `anchor`, letting the accessor's window
and resampling methods reason about actual elapsed time rather than plain row
position. Omitting `options` is equivalent to a default-constructed `TsOptions{}`.

This page previews the accessor's most common entry points. Every one of its methods
has its own dedicated, example-rich reference page — start at the
[DataFrameTsAccessor API Reference](../../dataframe_ts_accessor_api_reference) and
use its left sidebar to jump straight to any method (`lag`, `diff`, `rolling`,
`expanding`, `ewm`, and more).

## Parameters

**options** (`const TsOptions&`, optional)

- **`axis`** (`Slate`, default undefined): an explicit time axis (typically a
  `Timeline`) to align duration-based windows against, instead of plain row order.
- **`group`** (`TsGroup`, default empty): grouping keys for panel-style time series,
  so window/lag operations don't cross group boundaries.
- **`anchor`** (`Slate`, default undefined): a reference point for anchored
  resampling operations.

## Returns

`DataFrameTsAccessor`
    A lightweight accessor bound to the current (all-numeric) frame and `options`.
    Every method on it returns a new `DataFrame`; the source frame is never mutated.

## Examples

Example 1. Compute a lag and a period-over-period difference on a numeric feature —
after selecting it first.

```cpp
using namespace pluss::table;

auto readings = DataFrame({
    {"sensor_id", Slate(pluss::make_series({1, 2, 3, 4}))},
    {"load",      Slate(pluss::make_series({10.0, 20.0, 15.0, 30.0}))},
});
std::cout << readings;
```

### Output (before)

```text
======================
     | sensor_id  load
----------------------
0    | 1          10  
1    | 2          20  
2    | 3          15  
3    | 4          30  
----------------------
[4 cs x 2 feat]
```

```cpp
auto lagged = readings.features({"load"}).ts().lag(1);
std::cout << lagged;
```

### Output

```text
===========
     | load
-----------
0    | NULL
1    | 10  
2    | 20  
3    | 15  
-----------
[4 cs x 1 feat]
```

`sensor_id` is a numeric `Series` too, so `readings.ts()` directly would have
succeeded without throwing — but its accessor methods apply to **every** bound
feature, and lagging `sensor_id` isn't wanted here. `.features({"load"})` narrows
the frame down to just `load` first, so `.lag(1)` only affects that one feature.
`.lag(1)` is equivalent to [`shift(1)`](shift.md) scoped to `load`: each case now
shows the previous case's value, with case `0` becoming `NULL` since it has no
predecessor.

Example 2. Compute the period-over-period difference instead.

```cpp
auto changes = readings.features({"load"}).ts().diff(1);
std::cout << changes;
```

### Output

```text
===========
     | load
-----------
0    | NULL
1    | 10  
2    | -5  
3    | 15  
-----------
[4 cs x 1 feat]
```

`.diff(1)` is `load[i] - load[i - 1]`: case `1` is `20 - 10 = 10`, case `2` is
`15 - 20 = -5`, case `3` is `30 - 15 = 15`; case `0` has no prior case to subtract,
so it's `NULL`.

## Notes

- Every feature on the frame `.ts()` is called on must already be a numeric
  `Series`; select the numeric feature(s) first with [`features()`](features.md) if
  the source frame has other kinds mixed in.
- `.lag(periods)` on a numeric feature behaves identically to
  [`shift(periods)`](shift.md) scoped to that same feature — both introduce `NULL`
  at the vacated end.
- `ts()` on a frame with no features at all, or a frame containing a non-numeric
  feature, throws before constructing the accessor.

## See Also

- [`DataFrame::features`](features.md): the usual way to narrow a frame down to just
  its numeric features before calling `.ts()`.
- [`DataFrame::shift`](shift.md): the lower-level positional shift `.ts().lag()`
  is built on, without the time-series-accessor framing.
- [`DataFrame::tl`](tl.md): the equivalent dedicated accessor for calendar/timestamp
  components on `Timeline`/`TimeIndex` features, instead of numeric time-series
  values.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="truediv.html">
    <span class="label">Previous</span>
    <span class="title">truediv</span>
  </a>
  <a class="next-link" href="unify.html">
    <span class="label">Next</span>
    <span class="title">unify</span>
  </a>
</div>
