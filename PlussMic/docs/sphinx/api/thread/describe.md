<!-- pluss-hand-authored -->

# Thread::describe

[Back to Thread API Reference](../../thread_api_reference)

## Signature

```cpp
Thread::describe(bool render = false) const -> ThreadView
```

## What It Does

Returns a compact statistical summary of one thread, as a `ThreadView` whose case
labels are the names of the computed statistics. The exact statistics depend on
the payload kind:

- **Numeric `Series`**: the same eight statistics, in the same order, as
  [`DataFrame::describe()`](../dataframe/describe.md) computes per column —
  `count`, `mean`, `std` (sample standard deviation, `n - 1` denominator), `min`,
  `25%`, `50%`, `75%` (quantiles via linear interpolation between ranks), `max`.
  Internally this wraps the thread in a one-column `DataFrame`, calls its
  `describe()`, and extracts the single resulting column back out — so it's the
  exact per-column counterpart of that method, not a separately implemented
  calculation.
- **`Note`-kind**: a frequency-oriented summary instead — `count` (non-missing
  entries), `unique` (distinct non-missing values), `top` (the most frequent
  value, ties broken alphabetically), `freq` (that value's count).
- **Anything else** (`Timeline`, `TimeIndex`, non-numeric `Series`): throws —
  `describe()` has no defined summary for those payload kinds.

## Parameters

**render** (`bool`, default `false`)
    See [`head()`](head.md)'s parameter description — auto-render-on-scope-exit
    only; has no effect on what an explicit `std::cout << ...` prints.

## Returns

`ThreadView`
    A summary view with one entry per statistic, labeled by statistic name. It is
    given a display name — the source thread's own [`display_name`](display_name.md)
    if it has one, or `"value"` otherwise — so the render footer always reads
    `Name: {name}, ptype: {kind}` rather than the plain `ptype: {kind}` a default
    thread shows.

## Examples

Example 1. Describe a numeric thread.

```cpp
using namespace pluss::table;

auto scores = Thread(pluss::make_series({10.0, 20.0, 30.0, 40.0}));
auto out = scores.describe(false);
std::cout << out;
```

### Output

```text
count  4
 mean  25
  std  12.90994449
  min  10
  25%  17.5
  50%  25
  75%  32.5
  max  40

Name: value, ptype: Series
```

The leading label column is right-aligned to the widest label in play (`count`,
five characters) — `mean`, `std`, `min`, and each quartile line up one or two
spaces further in than `count`/`max`.

Example 2. Missing values are excluded from every statistic, the same
per-column skip [`DataFrame::describe()`](../dataframe/describe.md) uses.

```cpp
auto with_gap = Thread(pluss::make_series({10.0, 20.0, std::numeric_limits<double>::quiet_NaN(), 40.0}));
auto out2 = with_gap.describe(false);
std::cout << out2;
```

### Output

```text
count  3
 mean  23.33333333
  std  15.27525232
  min  10
  25%  15
  50%  20
  75%  30
  max  40

Name: value, ptype: Series
```

`count` reports `3`, not `4` — the missing entry is dropped before any statistic
is computed, not treated as `0` or left to propagate a `NaN` through the results.

Example 3. Describe a `Note`-kind thread — a frequency summary instead of
numeric statistics.

```cpp
auto cities = Thread(pluss::make_note({"Accra", "Tema", "Accra", "Kumasi", "Tema"}));
auto out3 = cities.describe(false);
std::cout << out3;
```

### Output

```text
 count  5
unique  3
   top  Accra
  freq  2

Name: value, ptype: Note
```

`"Accra"` and `"Tema"` are tied at two occurrences each; ties break
alphabetically, so `"Accra"` (not `"Tema"`) is reported as `top`.

Example 4. Any other payload kind throws.

```cpp
auto events = Thread(pluss::make_timeline({"2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z"}));
try {
  auto bad = events.describe();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Thread::describe supports numeric Series and Note payloads
```

## Notes

- Numeric statistics match [`DataFrame::describe()`](../dataframe/describe.md)
  exactly — `std` uses `n - 1` (sample standard deviation), and quartiles use
  linear interpolation between the two closest ranks, so `50%` coincides with the
  median exactly, including for even-sized samples.
- For `Note`-kind threads, ties for `top` break alphabetically by value.
- The returned view always has a display name (`"value"` if the source thread
  didn't already have one), so the render footer reads `Name: ..., ptype: ...`.
- Restricted to numeric `Series` and `Note` payloads; `Timeline`, `TimeIndex`, and
  non-numeric `Series` all throw `"Thread::describe supports numeric Series and
  Note payloads"`.

## See Also

- [`DataFrame::describe`](../dataframe/describe.md): the same summary computed
  for every numeric feature of a frame at once.
- [`Thread::freq`](freq.md): the full frequency table this method's `Note`-kind
  `top`/`freq` are drawn from.
- [`Thread::mean`](mean.md), [`Thread::std`](std.md),
  [`Thread::percentiles`](percentiles.md): compute one specific numeric statistic
  directly instead of the full summary.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="defined.html">
    <span class="label">Previous</span>
    <span class="title">defined</span>
  </a>
  <a class="next-link" href="display_name.html">
    <span class="label">Next</span>
    <span class="title">display_name</span>
  </a>
</div>
