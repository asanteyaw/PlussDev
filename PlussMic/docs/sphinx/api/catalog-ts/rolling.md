<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a7b3ad2ac228 -->

# CatalogTimeSeriesAccessor::rolling

[Back to CatalogTimeSeriesAccessor API Reference](../../catalog_ts_accessor_api_reference)

## Signature

```cpp
Catalog CatalogTimeSeriesAccessor::rolling(
    std::int64_t window,
    pluss::table::WindowStatistic statistic = pluss::table::WindowStatistic::Mean,
    const pluss::table::WindowOptions& options = {}) const
```

## What It Does

Computes a **trailing rolling window statistic** over every bound key. Each
position's window is the `window` positions ending at (and including) that
position. The very first `window - 1` positions use whatever's available
rather than waiting for a full window — position `0`'s window is just itself,
position `1`'s is positions `0..1`, and so on, until position `window - 1` and
beyond see a full window. A position's output is `NULL` only if the number of
non-missing values it can see falls below `options.min_count`.

```{important} CatalogTimeSeriesAccessor requires every bound key to be numeric Series-kind
Same precondition as every method on this accessor — see
[`CatalogTimeSeriesAccessor::diff`](diff.md)'s own important callout for the
full explanation and how to project down to numeric keys first.
```

```{important} Only the fixed-window overload — no calendar-duration form
Unlike [`Thread::ts().rolling()`](../thread-ts/rolling.md), which also has a
`rolling(axis, duration, ...)` calendar-duration overload,
`CatalogTimeSeriesAccessor::rolling()` only has this one fixed-row-count
signature — a multi-key catalog has no single unambiguous time axis to key a
calendar-duration window off of implicitly. Project down to one key with
[`Catalog::keys(key)`](../catalog/keys.md) to reach [`Stream::ts()`](../stream/ts.md)
for the calendar-duration form.
```

## Parameters

**window** (`std::int64_t`)
    The trailing window size, in positions. Must be positive, or it throws.

**statistic** (`pluss::table::WindowStatistic`, default `Mean`)
    The statistic computed over each window: `Sum`, `Mean`, `Minimum`,
    `Maximum`, `Var`, `Std`, `Sem`, `Skew`, or `Kurtosis`.

**options** (`const pluss::table::WindowOptions&`)
    `min_count` (default `1`): minimum non-missing observations required for a
    position's output to be non-`NULL`. `ddof` (default `1`): delta degrees of
    freedom for `Var`/`Std`. `centered` (default `false`): whether the window
    is centered on the position instead of trailing it. `closure` (default
    `Right`): which end(s) of the window are inclusive.

## Returns

`Catalog`
    A new catalog with every bound key replaced by its rolling statistic at
    each position.

## Examples

Example 1. A fixed-size trailing rolling mean, including its partial windows.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 15.0, 30.0})),
});
auto out = ct.ts().rolling(2);
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [metric]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
metric                   | Series       | Series[float64]  | 4       | [10, 15, 17.5, 22.5]                             
========================================================================================================================
```

With the default `min_count = 1`, position `0` doesn't wait for a full
2-position window — its output is just itself (`10`).

Example 2. The same window with `WindowStatistic::Std`, where `ddof = 1` bites
at position `0`.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 15.0, 30.0})),
});
auto out = ct.ts().rolling(2, pluss::table::WindowStatistic::Std);
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [metric]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
metric                   | Series       | Series[float64]  | 4       | [NA, 7.07106781187, 3.53553390593, 10.6066017178]
========================================================================================================================
```

Position `0`'s window has only 1 observation, and the default `ddof = 1`
requires more than `1` observation for a defined variance/std, so it's `NA`
even though `min_count = 1` alone would have allowed it.

Example 3. `options.centered = true` — the window straddles each position
instead of trailing it.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 15.0, 30.0})),
});
auto out = ct.ts().rolling(3, pluss::table::WindowStatistic::Mean,
                            pluss::table::WindowOptions{.centered = true});
std::cout << out;
```

### Output

```text
========================================================================================================================
kindle::Catalog  keys=1
keys: [metric]
------------------------------------------------------------------------------------------------------------------------
key                      | kind         | dtype            | cs      | preview                                          
------------------------------------------------------------------------------------------------------------------------
metric                   | Series       | Series[float64]  | 4       | [15, 15, 21.6666666667, 22.5]                    
========================================================================================================================
```

With `window = 3` and `centered = true`, position `0`'s window is positions
`0..1` (`10, 20` → `15`, clipped at the series start); position `2` is
positions `1..3` (`20, 15, 30` → `21.667`).

Example 4 (edge case). A non-positive `window` throws.

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 15.0, 30.0})),
});
try {
  auto out = ct.ts().rolling(0);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n'));
}
```

### Output

```text
Thread::moving: window must be > 0
```

A window of `0` (or negative) has no meaningful trailing span, so it's
rejected outright. The error is reported under `Thread::moving` — the private
engine this method delegates to internally.

## Notes

- See the important callouts above — no calendar-duration overload exists at
  this level.
- The default options (`min_count = 1`, `centered = false`, `closure =
  Right`) mean the first `window - 1` positions use a growing partial window
  rather than becoming `NULL` — set `min_count = window` to require a full
  window everywhere instead.
- `Var`/`Std` additionally require the window's observation count to exceed
  `ddof` (default `1`), independent of `min_count`.
- A pre-existing missing entry in a bound key is skipped within any window
  that contains it (it doesn't count toward `min_count`, and doesn't
  contribute to the statistic).

## See Also

- [`Stream::ts`](../stream/ts.md): the single-stream equivalent, which also
  exposes the calendar-duration overload — see the important callout above.
- [`Catalog::ts`](../catalog/ts.md): the entry point this accessor is reached from.
- [`TimeSeriesAccessor::rolling`](../thread-ts/rolling.md): the underlying
  single-thread engine this method's fixed-window form delegates to.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="returns.html">
    <span class="label">Previous</span>
    <span class="title">returns</span>
  </a>
</div>
