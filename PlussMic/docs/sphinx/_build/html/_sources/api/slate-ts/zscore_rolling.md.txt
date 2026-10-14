<!-- pluss-hand-authored -->

# TimeSeriesAccessor::zscore_rolling

[Back to TimeSeriesAccessor API Reference](../../slate_ts_accessor_api_reference)

## Signature

```cpp
TimeSeriesAccessor::zscore_rolling(std::int64_t window, std::int64_t min_periods = 1, double epsilon = 1e-12) const -> Slate
```

## What It Does

Computes a **rolling z-score**: at each position, `(x - rolling_mean) /
rolling_std`, using a trailing window of `window` positions ending at (and
including) that position. The rolling mean and standard deviation are computed
with a **population** convention (dividing by the window's observed count, not
count `- 1`) — there is no `ddof` parameter here, unlike
[`rolling()`](rolling.md)'s `Var`/`Std`. A position is `NULL` if the rolling
standard deviation isn't usefully positive (`|std| <= epsilon`, including a
perfectly flat window where `std = 0`), isn't finite, or `x` itself isn't
finite at that position.

```{important} a single missing value poisons every window it touches
Unlike [`rolling()`](rolling.md)/[`expanding()`](expanding.md)/
[`grouped()`](grouped.md), this method's underlying engine does **not** skip
missing values within a window — it accumulates every position in the window
with plain floating-point arithmetic, so one missing entry makes the mean *and*
std (and therefore the z-score) come back `NULL` for every position whose
window still contains it, not just that one position. The effect clears on its
own once the missing position scrolls out of the trailing window. See Example 2.
```

## Parameters

**window** (`std::int64_t`)
    The trailing window size, in positions. Must be positive, or it throws.

**min_periods** (`std::int64_t`, default `1`)
    The minimum number of positions that must actually exist in a window near
    the start of the slate (a purely structural count based on how much of the
    slate has been seen so far — see the important callout above for why this
    is *not* the same as "non-missing observations").

**epsilon** (`double`, default `1e-12`)
    The smallest rolling standard deviation still treated as usefully nonzero.
    Must be positive, or it throws.

## Returns

`Slate`
    A new slate, same length as the bound source, holding the rolling z-score
    at each position.

## Examples

Example 1. A rolling z-score over a clean (no missing values) slate.

```cpp
using namespace pluss::table;

auto values = Slate(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));
auto out = values.ts().zscore_rolling(3);
std::cout << out;
```

### Output

```text
0  NULL
1  1
2  1.224744871
3  1.224744871
4  1.224744871

ptype: Series
```

Position `0`'s window is just itself — a single-value window always has a
standard deviation of exactly `0`, which is below `epsilon`, so it's `NULL`
regardless of `min_periods`. Position `1`'s window is `(10, 20)`: mean `15`,
population std `5`, so `(20 - 15) / 5 = 1`. Position `2` onward see a full
3-position window (`10, 20, 30` then sliding forward): population std
`≈8.165` in every case, so each new value lands `≈1.2247` standard deviations
above its own rolling mean.

Example 2. A single masked-missing value poisons every window that still
contains it, then clears once it scrolls out.

```cpp
auto with_gap = Slate(
    pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}),
    pluss::make_series(std::vector<bool>{true, false, true, true, true})
);
auto out2 = with_gap.ts().zscore_rolling(3);
std::cout << out2;
```

### Output

```text
0  NULL
1  NULL
2  NULL
3  NULL
4  1.224744871

ptype: Series
```

Position `1` is `NULL` for the usual reason (its own value is missing).
Positions `2` and `3` are `NULL` too, even though `x[2]` (`30`) and `x[3]`
(`40`) are both real, valid values — their windows (`[0, 2]` and `[1, 3]`
respectively) still contain the missing position `1`, which poisons the mean
and std for the whole window. Position `4`'s window (`[2, 4]`) no longer
touches the gap at all, so it recovers a clean result, identical to Example 1's
steady-state value.

Example 3. A non-positive `epsilon` throws.

```cpp
try {
  auto bad = values.ts().zscore_rolling(3, 1, 0.0);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
subsp::stats::ts::zscore_rolling: epsilon must be > 0
```

## Notes

- The rolling mean/std here always use a **population** convention (divide by
  count, not count `- 1`) — there's no `ddof` to change that, unlike
  [`rolling()`](rolling.md)'s `Var`/`Std`.
- A window whose standard deviation is `0` (every value in it identical) — or
  below `epsilon` for any other reason — produces `NULL`, not a division by a
  near-zero number.
- See the important callout above: a missing value inside a window poisons that
  window's mean/std entirely, rather than being skipped and shrinking the
  effective window the way [`rolling()`](rolling.md) does.
- `window` must be `> 0`, or it throws `"mov*: window must be > 0"`. `epsilon`
  must be `> 0`, or it throws
  `"subsp::stats::ts::zscore_rolling: epsilon must be > 0"`.

## See Also

- [`TimeSeriesAccessor::rolling`](rolling.md): the fixed-window engine used for
  most other window statistics, which does skip missing values within a
  window rather than poisoning it.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="timed.html">
    <span class="label">Previous</span>
    <span class="title">timed</span>
  </a>
</div>
