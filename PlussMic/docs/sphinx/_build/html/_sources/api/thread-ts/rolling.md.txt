<!-- pluss-hand-authored -->

# TimeSeriesAccessor::rolling

[Back to TimeSeriesAccessor API Reference](../../thread_ts_accessor_api_reference)

## Signatures

```cpp
TimeSeriesAccessor::rolling(std::int64_t window, WindowStatistic statistic = WindowStatistic::Mean, const WindowOptions& options = {}) const -> Thread
TimeSeriesAccessor::rolling(const Thread& axis, std::string_view duration, WindowStatistic statistic = WindowStatistic::Mean, const StatisticOptions& options = {}, WindowClosure closure = WindowClosure::Right) const -> Thread
```

## What It Does

Computes a **trailing rolling window statistic** over the bound thread, in one of
two shapes:

- **Fixed row count** (`rolling(window, statistic, options)`): each position's
  window is the `window` positions ending at (and including) that position. The
  very first `window - 1` positions use whatever's available rather than waiting
  for a full window — position `0`'s window is just itself, position `1`'s is
  positions `0..1`, and so on, until position `window - 1` and beyond see a full
  window. A position's output is `NULL` only if the number of non-missing values
  it can see falls below `options.min_count`.
- **Calendar duration** (`rolling(axis, duration, statistic, options, closure)`):
  the same trailing-window idea, but each position's window spans a fixed span of
  time (like `"2D"`) looked up against an explicit time `axis`, rather than a
  fixed row count. `axis` must be `Timeline` or `TimeIndex`, the same length as
  the bound thread, non-missing, and sorted ascending.

## Parameters

**window** (`std::int64_t`, fixed-window overload)
    The trailing window size, in positions. Must be positive, or it throws.

**statistic** (`WindowStatistic`, default `Mean`)
    The statistic computed over each window: `Sum`, `Mean`, `Minimum`, `Maximum`,
    `Var`, `Std`, `Sem`, `Skew`, or `Kurtosis`.

**options** (`const WindowOptions&`, fixed-window overload)

- **`min_count`** (`std::int64_t`, default `1`): minimum non-missing observations
  required for a position's output to be non-`NULL`.
- **`ddof`** (`std::int64_t`, default `1`): delta degrees of freedom for
  `Var`/`Std`.
- **`centered`** (`bool`, default `false`): whether the window is centered on the
  position instead of trailing it.
- **`closure`** (`WindowClosure`, default `Right`): which end(s) of the window are
  inclusive.

**axis** (`const Thread&`, calendar-duration overload)
    An explicit time axis (`Timeline` or `TimeIndex`), the same length as the
    bound thread, non-missing and sorted ascending.

**duration** (`std::string_view`, calendar-duration overload)
    A duration string (for example `"2D"`, `"12H"`, `"30min"`) defining each
    position's time-based window width.

**options** (`const StatisticOptions&`, calendar-duration overload)
    `ddof` and `min_count`, same meaning as above; only `Sum`, `Mean`, `Var`, and
    `Std` are supported for this overload.

**closure** (`WindowClosure`, calendar-duration overload, default `Right`)
    Which end of the time window is inclusive.

## Returns

`Thread`
    A new thread, same length as the bound source, holding the rolling statistic
    at each position.

## Examples

Example 1. A fixed-size trailing rolling mean, including its partial windows.

```cpp
using namespace pluss::table;

auto load = Thread(pluss::make_series({10.0, 20.0, 15.0, 30.0}));
auto out = load.ts().rolling(2);
std::cout << out;
```

### Output

```text
0  10
1  15
2  17.5
3  22.5

ptype: Series
```

With the default `min_count = 1`, position `0` doesn't wait for a full 2-position
window — its output is just itself (`10`). Position `1` sees a full window
(`10, 20`, mean `15`); position `2` sees `(20, 15)` → `17.5`; position `3` sees
`(15, 30)` → `22.5`.

Example 2. The same window with `WindowStatistic::Std`, where `ddof = 1` bites at
position `0`.

```cpp
auto rolled_std = load.ts().rolling(2, WindowStatistic::Std);
std::cout << rolled_std;
```

### Output

```text
0  NULL
1  7.071067812
2  3.535533906
3  10.60660172

ptype: Series
```

Position `0`'s window has only 1 observation, and the default `ddof = 1`
requires more than `1` observation for a defined variance/std, so it's `NULL`
even though `min_count = 1` alone would have allowed it.

Example 3. `options.centered = true` — the window straddles each position
instead of trailing it.

```cpp
auto centered = load.ts().rolling(3, WindowStatistic::Mean, WindowOptions{.centered = true});
std::cout << centered;
```

### Output

```text
0  15
1  15
2  21.66666667
3  22.5

ptype: Series
```

With `window = 3` and `centered = true`, position `0`'s window is positions `0..1`
(`10, 20` → `15`, clipped at the series start); position `1` is positions `0..2`
(`10, 20, 15` → `15`); position `2` is positions `1..3` (`20, 15, 30` →
`21.667`); position `3` is positions `2..3` (`15, 30` → `22.5`, clipped at the
end).

Example 4. A pre-existing missing entry is skipped, not treated as a real
observation, and doesn't need `min_count` lowered to be seen at all.

```cpp
auto with_gap = Thread(
    pluss::make_series({10.0, 20.0, 30.0, 40.0}),
    pluss::make_series(std::vector<bool>{true, false, true, true})
);
auto out2 = with_gap.ts().rolling(2);
std::cout << out2;
```

### Output

```text
0  10
1  10
2  30
3  35

ptype: Series
```

Position `1`'s window (`x[0..1]`) has only one real observation (`x[1]` is
missing), so its mean is just `10`. Position `2`'s window (`x[1..2]`) also has
only one real observation (`30`), so it's `30` rather than an average involving
the missing entry. Position `3`'s window (`x[2..3]`) is fully observed:
`(30 + 40) / 2 = 35`.

Example 5. A non-positive `window` throws.

```cpp
try {
  auto bad = load.ts().rolling(0);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Thread::moving: window must be > 0
```

A window of `0` (or negative) has no meaningful trailing span, so it's rejected
outright. The error is reported under `Thread::moving` — the private engine this
method (and [`expanding()`](expanding.md)) delegate to internally.

## Notes

- The fixed-window overload's default options (`min_count = 1`, `centered =
  false`, `closure = Right`) mean the first `window - 1` positions use a growing
  partial window rather than becoming `NULL` — set `min_count = window` to
  require a full window everywhere instead.
- `Var`/`Std` additionally require the window's observation count to exceed
  `ddof` (default `1`), independent of `min_count`.
- A pre-existing missing entry in the source thread is skipped within any window
  that contains it (it doesn't count toward `min_count`, and doesn't contribute
  to the statistic) — see Example 4.
- The calendar-duration overload requires `axis` to already be `Timeline` or
  `TimeIndex`, sorted ascending with no missing entries, and only supports
  `Sum`/`Mean`/`Var`/`Std` — see [`TimeSeriesAccessor::timed`](timed.md) for a
  fully worked duration-window example (using a plain nanosecond axis, which is
  easier to verify by hand than calendar timestamps) and its exact window-boundary
  rules.

## See Also

- [`TimeSeriesAccessor::timed`](timed.md): the calendar-duration engine this
  overload's second signature delegates to, with a fully worked example.
- [`TimeSeriesAccessor::grouped`](grouped.md): the same fixed-window statistic,
  computed independently per group instead of over the whole thread.
- [`TimeSeriesAccessor::expanding`](expanding.md): the same engine with the
  window growing from the start of the thread instead of a fixed size.

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
