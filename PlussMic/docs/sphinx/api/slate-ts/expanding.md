<!-- pluss-hand-authored -->

# TimeSeriesAccessor::expanding

[Back to TimeSeriesAccessor API Reference](../../slate_ts_accessor_api_reference)

## Signature

```cpp
Slate TimeSeriesAccessor::expanding(WindowStatistic statistic = WindowStatistic::Mean, const StatisticOptions& options = {}) const
```

## What It Does

Computes a **growing (expanding) window statistic**: position `i`'s window is
every position from the very start of the slate through `i`, so the window
widens by one position at a time rather than staying a fixed size. It's
implemented as [`rolling()`](rolling.md) with the window fixed at the full
slate length (`max(1, size())`) and `centered = false`, `closure = Right` — the
same trailing-window engine, just wide enough that every position's window
always starts at position `0`.

## Parameters

**statistic** (`WindowStatistic`, default `Mean`)
    The statistic computed over each expanding window: `Sum`, `Mean`, `Minimum`,
    `Maximum`, `Var`, `Std`, `Sem`, `Skew`, or `Kurtosis`.

**options** (`const StatisticOptions&`)
    `ddof` (default `1`): delta degrees of freedom for `Var`/`Std`. `min_count`
    (default `1`): minimum non-missing observations required for a position's
    output to be non-`NULL`.

## Returns

`Slate`
    A new slate, same length as the bound source, holding the expanding
    statistic at each position.

## Examples

Example 1. A growing mean — every position's output is the mean of everything
seen so far.

```cpp
using namespace pluss::table;

auto values = Slate(pluss::make_series({10.0, 20.0, 30.0, 40.0}));
auto out = values.ts().expanding();
std::cout << out;
```

### Output

```text
0  10
1  15
2  20
3  25

ptype: Series
```

Position `2`'s window is everything from position `0` through `2`:
`mean(10, 20, 30) = 20`.

Example 2. A growing sample variance — position `0` is `NULL` since a single
observation and the default `ddof = 1` can't produce a defined variance.

```cpp
auto out2 = values.ts().expanding(WindowStatistic::Var);
std::cout << out2;
```

### Output

```text
0  NULL
1  50
2  100
3  166.6666667

ptype: Series
```

Position `1`'s variance is over `(10, 20)`: `50`. Position `2` is over
`(10, 20, 30)`: `100`. Position `3` is over all four values: `166.667`.

Example 3. `min_count` can require more observations before a position counts as
valid, independent of `ddof`.

```cpp
auto out3 = values.ts().expanding(WindowStatistic::Mean, StatisticOptions{.min_count = 3});
std::cout << out3;
```

### Output

```text
0  NULL
1  NULL
2  20
3  25

ptype: Series
```

Positions `0` and `1` — which have only `1` and `2` observations, respectively —
are `NULL` since `min_count = 3` now requires at least three.

Example 4. A pre-existing missing entry is skipped, not treated as a real
observation.

```cpp
auto with_gap = Slate(
    pluss::make_series({10.0, 20.0, 30.0, 40.0}),
    pluss::make_series(std::vector<bool>{true, false, true, true})
);
auto out4 = with_gap.ts().expanding();
std::cout << out4;
```

### Output

```text
0  10
1  10
2  20
3  26.66666667

ptype: Series
```

Position `1` (the masked-missing `20.0`) doesn't participate — the mean stays
`10`. Position `2` is `mean(10, 30) = 20`, excluding the missing entry.
Position `3` is `mean(10, 30, 40) = 26.667`.

## Notes

- Equivalent to [`rolling(max(1, size), statistic, options)`](rolling.md) with
  `centered = false` and `closure = Right` — the same trailing-window engine,
  just wide enough to always start at position `0`.
- `Var`/`Std` additionally require the observation count to exceed `ddof`
  (default `1`), independent of `min_count`.
- A pre-existing missing entry in the source slate is skipped within every
  window that would otherwise contain it — see Example 4.

## See Also

- [`TimeSeriesAccessor::rolling`](rolling.md): the general fixed-window engine
  this method is a special case of.
- [`TimeSeriesAccessor::ewm`](ewm.md): an alternative "everything seen so far"
  statistic that weights recent observations more heavily instead of weighting
  every observation equally.
- [`Slate::cumsum`](../slate/cumsum.md): the running total at every position —
  `expanding(WindowStatistic::Sum)` computes the same thing through a different
  engine.

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
