<!-- pluss-hand-authored -->

# DataFrameTsAccessor::align_to

[Back to DataFrameTsAccessor API Reference](../../dataframe_ts_accessor_api_reference)

## Signatures

```cpp
DataFrameTsAccessor::align_to(const Thread& target_axis, TsAlignMode mode = TsAlignMode::Exact) const -> DataFrame
DataFrameTsAccessor::align_to(const Thread& target_axis, TsAlignMode mode, TsReindexFill fill, std::optional<double> constant = std::nullopt) const -> DataFrame
DataFrameTsAccessor::align_to(std::string_view time_feature, const Thread& target_axis, TsAlignMode mode = TsAlignMode::Exact) const -> DataFrame
DataFrameTsAccessor::align_to(std::string_view time_feature, const Thread& target_axis, TsAlignMode mode, TsReindexFill fill, std::optional<double> constant = std::nullopt) const -> DataFrame
```

## What It Does

Reindexes every bound feature onto an entirely new `target_axis`, matching each
target row to a row of this accessor's own time axis according to `mode`:

- **`Exact`**: a target tick is matched only to a source row with the exact same
  tick; anything unmatched is `NULL`.
- **`Backward`**: each target tick takes the most recent source value at or before
  it (a "last known value" lookup).
- **`Forward`**: each target tick takes the next source value at or after it.
- **`Nearest`**: each target tick takes whichever source value is closest in time,
  before or after.

The `fill`/`constant` overloads additionally run [`fill_gaps()`](fill_gaps.md) on
the result afterward, using `fill` (a `TsReindexFill` value) to pick the gap-fill
strategy — `TsReindexFill::None` skips this fill step entirely, leaving any
still-unmatched cells as `NULL`.

## Parameters

**target_axis** (`const Thread&`)

The new time axis (`Timeline`/`TimeIndex`) to reindex onto. Becomes the result's
case labels.

**mode** (`TsAlignMode`, default `Exact`)

The matching strategy described above.

**fill** (`TsReindexFill`, `fill`/`constant` overloads)

The post-alignment gap-fill strategy: `None` (no fill), `Forward`, `Backward`,
`Zero`, or `Constant`.

**constant** (`std::optional<double>`, `fill`/`constant` overloads)

The fill value when `fill == TsReindexFill::Constant`. Required in that case.

**time_feature** (`std::string_view`, two overloads)

An explicit feature name supplying this accessor's own source time axis, instead
of using its case labels or `TsOptions.axis`.

## Returns

`DataFrame`
    A new frame with `target_axis` as its case labels, and every bound feature
    reindexed according to `mode`.

## Examples

Example 1. Reindex onto a regular daily grid using each of the four alignment
modes.

```cpp
using namespace pluss::table;

auto prices = DataFrame(
    {{"value", Thread(pluss::make_series({100.0, 102.0, 110.0}))}},
    Thread(pluss::make_timeline({
        "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", "2024-01-05T00:00:00Z"})));
std::cout << prices;

auto grid = Thread(pluss::make_timeline({
    "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", "2024-01-03T00:00:00Z",
    "2024-01-04T00:00:00Z", "2024-01-05T00:00:00Z"}));

auto exact = prices.ts().align_to(grid, TsAlignMode::Exact);
std::cout << exact;
```

### Output (before)

```text
==================
           | value
------------------
2024-01-01 | 100  
2024-01-02 | 102  
2024-01-05 | 110  
------------------
[3 cs x 1 feat]
```

### Output (Exact)

```text
==================
           | value
------------------
2024-01-01 | 100  
2024-01-02 | 102  
2024-01-03 | NULL 
2024-01-04 | NULL 
2024-01-05 | 110  
------------------
[5 cs x 1 feat]
```

`Exact` only fills in `2024-01-01`, `2024-01-02`, and `2024-01-05`, since those are
the only grid ticks `prices` has an exact match for; `2024-01-03`/`2024-01-04` have
no source row at all, so they're `NULL`.

```cpp
auto backward = prices.ts().align_to(grid, TsAlignMode::Backward);
std::cout << backward;
```

### Output (Backward)

```text
==================
           | value
------------------
2024-01-01 | 100  
2024-01-02 | 102  
2024-01-03 | 102  
2024-01-04 | 102  
2024-01-05 | 110  
------------------
[5 cs x 1 feat]
```

`Backward` instead carries `2024-01-02`'s `102` forward through
`2024-01-03`/`2024-01-04`, since that's the most recent known value at or before
each of those ticks — this is the alignment mode
[`DataFrameTsResampler::ffill()`](../dataframe-ts-resample/ffill.md) is built on.

Example 2. The `time_feature` overload, sourcing the axis from a named column
instead of case labels.

```cpp
auto prices_named = DataFrame(
    {{"price_time", Thread(pluss::make_timeline({
          "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", "2024-01-05T00:00:00Z"}))},
     {"value", Thread(pluss::make_series({100.0, 102.0, 110.0}))}});
std::cout << prices_named;

auto exact_named = prices_named.ts().align_to("price_time", grid, TsAlignMode::Exact);
std::cout << exact_named;
```

### Output (before)

```text
========================
     | price_time  value
------------------------
0    | 2024-01-01  100  
1    | 2024-01-02  102  
2    | 2024-01-05  110  
------------------------
[3 cs x 2 feat]
```

### Output (after)

```text
==================
           | value
------------------
2024-01-01 | 100  
2024-01-02 | 102  
2024-01-03 | NULL 
2024-01-04 | NULL 
2024-01-05 | 110  
------------------
[5 cs x 1 feat]
```

`.ts()` auto-binds only the numeric `value` feature — `price_time` is left alone
on the underlying frame so `align_to("price_time", ...)` can still find it by
name. The result matches the `Exact` output above; `price_time` itself drops out
of the output columns, replaced by `grid` as the new case labels.

Example 3. Use the `fill`/`constant` overload to fill every still-unmatched cell
in one call.

```cpp
auto filled = prices.ts().align_to(grid, TsAlignMode::Exact, TsReindexFill::Constant, -1.0);
std::cout << filled;
```

### Output

```text
==================
           | value
------------------
2024-01-01 | 100  
2024-01-02 | 102  
2024-01-03 | -1   
2024-01-04 | -1   
2024-01-05 | 110  
------------------
[5 cs x 1 feat]
```

Same `Exact` alignment as before, but `TsReindexFill::Constant` immediately
fills both still-`NULL` cells (`2024-01-03`, `2024-01-04`) with `-1` —
equivalent to calling the 2-argument overload and then
[`fill_gaps()`](fill_gaps.md) with `GapFillMode::Constant` yourself.

Example 4. Pass `TsReindexFill::Constant` without a `constant` value, to see it
throw.

```cpp
try {
  auto out = prices.ts().align_to(grid, TsAlignMode::Exact, TsReindexFill::Constant);
} catch (const std::exception& e) {
  std::string msg(e.what());
  std::cout << msg.substr(0, msg.find('\n')) << "\n";
}
```

### Output

```text
DataFrameTsAccessor::align_to: constant fill requires a value
```

`constant` defaults to `std::nullopt`; `TsReindexFill::Constant` needs an
actual value to fill with, so omitting it throws rather than silently falling
back to `0.0`.

## Notes

- `Forward` is the mirror image of `Backward` — each gap takes the *next*
  available value instead of the last one — and is what
  [`DataFrameTsResampler::bfill()`](../dataframe-ts-resample/bfill.md) is built on
  (deliberately the opposite naming: forward-*fill* means "carry backward-looking
  values forward," so it maps to `TsAlignMode::Backward`).
- `Nearest` picks whichever neighbor (before or after) is chronologically closer,
  breaking ties toward the earlier one; it underlies
  [`DataFrameTsResampler::nearest()`](../dataframe-ts-resample/nearest.md).
- The `fill`/`constant` overloads are a convenience for immediately gap-filling the
  reindexed result — equivalent to calling the 2-argument overload and then
  [`fill_gaps()`](fill_gaps.md) yourself, except `TsReindexFill::None` (a value not
  present on `GapFillMode`) explicitly means "don't fill at all."
- [`DataFrameTsAccessor::insert_gaps`](insert_gaps.md) and
  [`DataFrameTsResampler::asfreq`](../dataframe-ts-resample/asfreq.md) are both
  thin wrappers over this method with `TsAlignMode::Exact` and a regular
  frequency-derived `target_axis`.
- `DataFrame::ts()`/`ts(options)` auto-bind only the numeric `Series` columns on
  the calling frame; a named `Timeline`/`TimeIndex` column meant for
  `time_feature` is simply skipped rather than rejected, so a frame can carry
  both value features and a named time-axis column at once (Example 2).
- The `time_feature` overloads consume that column purely as an alignment key —
  it does not appear in the output, which instead takes `target_axis` as its
  new case labels, exactly like the case-labels-only overloads.

## See Also

- [`DataFrameTsAccessor::insert_gaps`](insert_gaps.md): a convenience wrapper that
  builds `target_axis` from a frequency string automatically.
- [`DataFrameTsAccessor::fill_gaps`](fill_gaps.md): the gap-fill step the
  `fill`/`constant` overloads apply automatically.
- [`DataFrameTsResampler::ffill`](../dataframe-ts-resample/ffill.md),
  [`DataFrameTsResampler::bfill`](../dataframe-ts-resample/bfill.md),
  [`DataFrameTsResampler::nearest`](../dataframe-ts-resample/nearest.md): named,
  pre-configured wrappers around the `Backward`/`Forward`/`Nearest` modes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="add.html">
    <span class="label">Previous</span>
    <span class="title">add</span>
  </a>
  <a class="next-link" href="asfreq.html">
    <span class="label">Next</span>
    <span class="title">asfreq</span>
  </a>
</div>
