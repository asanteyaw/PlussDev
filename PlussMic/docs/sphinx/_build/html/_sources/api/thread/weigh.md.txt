<!-- pluss-hand-authored -->

# Thread::weigh

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::weigh(pluss::RangeView<double> weights, WindowStatistic statistic = WindowStatistic::Mean, std::int64_t min_count = 1) const -> Thread
```

## What It Does

Computes a **weighted trailing window** statistic over the thread — like a
fixed-width rolling window, but instead of every position in the window
counting equally, each contributes according to `weights`. The window width is
implicitly `weights.size()`; `weights[0]` applies to the *oldest* position in
each window and `weights.back()` applies to the *most recent* (current)
position — the natural left-to-right reading order.

Near the start of the thread, where a full window isn't yet available, the
missing leading positions are treated as absent (contributing zero weight)
rather than zero-valued — so early rows still compute a properly normalized
weighted average over however many real positions are actually available,
they just do so over a *smaller* effective window. Missing (`NULL`) entries
within a window are excluded the same way.

```{note} weights are ignored for Minimum/Maximum
For `WindowStatistic::Minimum` and `WindowStatistic::Maximum`, the actual
`weights` *values* don't affect the result at all — only `weights.size()`
does, since "weighted minimum/maximum" isn't a well-defined idea. These two
statistics fall back to a plain (unweighted) rolling min/max over a window the
same width as `weights`.
```

## Parameters

**weights** (`pluss::RangeView<double>`)
    The per-position weights, oldest to newest. Implicitly sets the window
    width to `weights.size()`. Must be non-empty, or it throws.

**statistic** (`WindowStatistic`, default `Mean`)
    Which statistic to compute inside each weighted window: `Sum`, `Mean`,
    `Minimum`, `Maximum`, `Var`, or `Std` (`Minimum`/`Maximum` ignore the
    weight values — see the note above).

**min_count** (`std::int64_t`, default `1`)
    The smallest number of observed (weighted-eligible) positions in a window
    still considered valid. Must be `> 0` and `<= weights.size()`, or it
    throws.

## Returns

`Thread`
    A new thread the same length as the source, holding the weighted
    statistic at each position.

## Examples

Example 1. A weighted moving average — later (more recent) positions in each
window count for more.

```cpp
using namespace pluss::table;

auto readings = Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0, 50.0}));
auto out = readings.weigh({1.0, 2.0, 3.0});
std::cout << out;
```

### Output

```text
0  10
1  16
2  23.33333333
3  33.33333333
4  43.33333333

ptype: Series
```

Position `2` onward has a full 3-wide window (weights `1, 2, 3` applied to the
oldest, middle, and newest of the three values respectively): `(10*1 + 20*2 +
30*3) / (1+2+3) = 140/6 ≈ 23.333`. Position `0` has only itself available (the
two leading window slots don't exist yet), so it's weighted only by `3` (the
weight for "most recent"), normalizing to exactly `10`. Position `1` has two
real values, weighted by `2` and `3` (the window's last two weights): `(10*2 +
20*3) / (2+3) = 80/5 = 16`.

Example 2. `min_count` can require a fuller window before a position counts as
valid.

```cpp
auto out2 = readings.weigh({1.0, 2.0, 3.0}, WindowStatistic::Mean, 3);
std::cout << out2;
```

### Output

```text
0  NULL
1  NULL
2  23.33333333
3  33.33333333
4  43.33333333

ptype: Series
```

With `min_count = 3` (the full window width), positions `0` and `1` — which
only had `1` and `2` real values available, respectively — are `NULL`.

Example 3. `Sum` reports the weighted total directly, without normalizing by
the weight total.

```cpp
auto out3 = readings.weigh({1.0, 2.0, 3.0}, WindowStatistic::Sum);
std::cout << out3;
```

### Output

```text
0  30
1  80
2  140
3  200
4  260

ptype: Series
```

These are the same weighted sums computed for Example 1's means — just not
divided by the weight total (`6` for a full window).

## Notes

- `weights[0]` applies to the oldest position in each window, `weights.back()`
  to the most recent — the window width is always `weights.size()`.
- Near the start of the thread, unavailable leading positions contribute zero
  weight (not a zero value), so early results are still properly normalized
  weighted averages over a smaller effective window.
- `Minimum`/`Maximum` ignore the actual weight values — see the important note
  above.
- `weights` must be non-empty, or it throws `"Thread::weigh: weights cannot be
  empty"`. `min_count` must be in `(0, weights.size()]`, or it throws
  `"Thread::weigh: invalid min_count"`.
- Restricted to numeric `Series` payloads.

## See Also

- [`Thread::bounded`](bounded.md): the more general custom-window method this
  builds a fixed, weighted trailing window on top of.
- [`Thread::ts`](ts.md): the time-series accessor's `rolling()` provides an
  *unweighted* fixed-width rolling window.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="view.html">
    <span class="label">Previous</span>
    <span class="title">view</span>
  </a>
  <a class="next-link" href="window.html">
    <span class="label">Next</span>
    <span class="title">window</span>
  </a>
</div>
