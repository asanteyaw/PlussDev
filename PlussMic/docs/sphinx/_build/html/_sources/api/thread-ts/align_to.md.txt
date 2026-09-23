<!-- pluss-hand-authored -->

# TimeSeriesAccessor::align_to

[Back to TimeSeriesAccessor API Reference](../../thread_ts_accessor_api_reference)

## Signatures

```cpp
TimeSeriesAccessor::align_to(const Thread& source_axis, const Thread& target_axis, TsAlignMode mode = TsAlignMode::Exact) const -> Thread
TimeSeriesAccessor::align_to(const Thread& source_axis, const Thread& target_axis, TsAlignMode mode, GapFillMode fill_mode, std::optional<double> constant = std::nullopt) const -> Thread
```

## What It Does

Reindexes the bound thread's values — currently living at `source_axis` ticks —
onto an entirely new `target_axis`, matching each target tick to a `source_axis`
tick according to `mode`:

- **`Exact`** (the default): a target tick is matched only to a source tick with
  the exact same value; anything unmatched is `NULL`.
- **`Backward`**: each target tick takes the most recent source value at or
  before it (a "last known value" lookup).
- **`Forward`**: each target tick takes the next source value at or after it.
- **`Nearest`**: each target tick takes whichever source value is closest in
  time, before or after — ties break toward the earlier source tick.

The `fill_mode`/`constant` overload additionally runs
[`fill_gaps(fill_mode, constant)`](fill_gaps.md) on the result afterward,
filling in whatever's still `NULL` after alignment.

## Parameters

**source_axis** (`const Thread&`)
    The time axis (`Timeline`/`TimeIndex`) the bound thread's own values
    currently live at. Must be the same length as the bound thread.

**target_axis** (`const Thread&`)
    The new time axis (`Timeline`/`TimeIndex`) to reindex onto. Determines the
    result's length.

**mode** (`TsAlignMode`, default `Exact`)
    The matching strategy described above.

**fill_mode** (`GapFillMode`, fill overload)
    The gap-fill strategy applied after alignment — same as
    [`fill_gaps()`](fill_gaps.md)'s `mode`.

**constant** (`std::optional<double>`, fill overload)
    The fill value when `fill_mode` is `GapFillMode::Constant`. Required in
    that case.

## Returns

`Thread`
    A new thread, length `target_axis.size()`, holding the bound values
    reindexed according to `mode` (and optionally gap-filled).

## Examples

Example 1. Reindex a sparse thread onto a regular daily grid, comparing all
four alignment modes.

```cpp
using namespace pluss::table;

auto source_axis = Thread(pluss::make_timeline(
    {"2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", "2024-01-05T00:00:00Z"}));
auto prices = Thread(pluss::make_series({100.0, 102.0, 110.0}));

auto grid = Thread(pluss::make_timeline({
    "2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z", "2024-01-03T00:00:00Z",
    "2024-01-04T00:00:00Z", "2024-01-05T00:00:00Z"}));

auto exact = prices.ts().align_to(source_axis, grid, TsAlignMode::Exact);
std::cout << exact;
```

### Output (Exact)

```text
0  100
1  102
2  NULL
3  NULL
4  110

ptype: Series
```

`Exact` only fills in the grid ticks that have an exact match in `source_axis`
(`2024-01-01`, `2024-01-02`, `2024-01-05`); the ticks with no source row at all
(`2024-01-03`/`2024-01-04`) are `NULL`.

```cpp
auto backward = prices.ts().align_to(source_axis, grid, TsAlignMode::Backward);
auto forward = prices.ts().align_to(source_axis, grid, TsAlignMode::Forward);
auto nearest = prices.ts().align_to(source_axis, grid, TsAlignMode::Nearest);
std::cout << backward;
std::cout << forward;
std::cout << nearest;
```

### Output (Backward / Forward / Nearest)

```text
0  100
1  102
2  102
3  102
4  110

ptype: Series
0  100
1  102
2  110
3  110
4  110

ptype: Series
0  100
1  102
2  102
3  110
4  110

ptype: Series
```

`Backward` carries `2024-01-02`'s `102` forward through `2024-01-03`/
`2024-01-04`, since that's the most recent known value at or before each of
those ticks. `Forward` instead reaches ahead to `2024-01-05`'s `110` for both.
`Nearest` splits the difference: `2024-01-03` is one day from `2024-01-02` and
two from `2024-01-05`, so it takes `102`; `2024-01-04` is the mirror case and
takes `110`.

Example 2. The `fill_mode`/`constant` overload fills every still-unmatched
position in one call.

```cpp
auto filled = prices.ts().align_to(
    source_axis, grid, TsAlignMode::Exact,
    pluss::subsp::stats::ts::GapFillMode::Constant, -1.0);
std::cout << filled;
```

### Output

```text
0  100
1  102
2  -1
3  -1
4  110

ptype: Series
```

Same `Exact` alignment as before, but `GapFillMode::Constant` immediately fills
both still-`NULL` positions with `-1` — equivalent to calling the 3-argument
overload and then [`fill_gaps()`](fill_gaps.md) yourself.

Example 3. A `source_axis` whose length doesn't match the bound thread throws.

```cpp
auto short_axis = Thread(pluss::make_timeline({"2024-01-01T00:00:00Z"}));
try {
  auto bad = prices.ts().align_to(short_axis, grid);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
TimeSeriesAccessor::align_to: right values and right time axis length mismatch
```

## Notes

- `source_axis` must be the same length as the bound thread — it pairs
  one-to-one with the thread's own values, position by position.
- `Nearest` breaks a tie between two equally-close source ticks toward the
  *earlier* one.
- The result's length always matches `target_axis.size()`, regardless of the
  bound thread's own length.
- The fill overload is exactly the 3-argument overload followed by
  [`fill_gaps(fill_mode, constant)`](fill_gaps.md) — there's no option to
  "align without filling" beyond simply calling the 3-argument overload
  directly.

## See Also

- [`TimeSeriesAccessor::fill_gaps`](fill_gaps.md): the gap-fill step the
  `fill_mode`/`constant` overload applies automatically.
- [`TimeSeriesAccessor::resample`](resample.md): reindexes onto a
  frequency-derived axis while also aggregating, rather than a 1:1 lookup.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="add.html">
    <span class="label">Previous</span>
    <span class="title">add</span>
  </a>
  <a class="next-link" href="diff.html">
    <span class="label">Next</span>
    <span class="title">diff</span>
  </a>
</div>
