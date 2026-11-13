<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: a2a995a58544 -->

# Slate::describe

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
SlateView Slate::describe(bool render = false) const
```

## What It Does

Returns a compact statistical summary of one slate, as a `SlateView` whose case
labels are the names of the computed statistics. The exact statistics depend on
the payload kind:

- **Numeric `Series`**: the same eight statistics, in the same order, as
  [`DataFrame::describe()`](../dataframe/describe.md) computes per column —
  `count`, `mean`, `std` (sample standard deviation, `n - 1` denominator), `min`,
  `25%`, `50%`, `75%` (quantiles via linear interpolation between ranks), `max`.
  Internally this wraps the slate in a one-column `DataFrame`, calls its
  `describe()`, and extracts the single resulting column back out — so it's the
  exact per-column counterpart of that method, not a separately implemented
  calculation.
- **`Note`-kind**: a frequency-oriented summary instead — `count` (non-missing
  entries), `unique` (distinct non-missing values), `top` (the most frequent
  value, ties kept in first-occurrence order via [`freq()`](freq.md), not
  alphabetical order), `freq` (that value's count).
- **Anything else** (`Timeline`, `TimeIndex`, non-numeric `Series`): throws —
  `describe()` has no defined summary for those payload kinds.

## Parameters

**render** (`bool`, default `false`)
    See [`head()`](head.md)'s parameter description — auto-render-on-scope-exit
    only; has no effect on what an explicit `std::cout << ...` prints.

## Returns

`SlateView`
    A summary view with one entry per statistic, labeled by statistic name. It is
    given a display name — the source slate's own [`name()`](name.md)
    if it has one, or `"value"` otherwise — so the render footer always reads
    `Name: {name}, ptype: {kind}` rather than the plain `ptype: {kind}` a default
    slate shows.

## Examples

Example 1. Describe a numeric slate.

```cpp
using namespace pluss::table;

auto scores = Slate(pluss::make_series({10.0, 20.0, 30.0, 40.0}));
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
auto with_gap = Slate(pluss::make_series({10.0, 20.0, std::numeric_limits<double>::quiet_NaN(), 40.0}));
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

Example 3. Describe a `Note`-kind slate — a frequency summary instead of
numeric statistics.

```cpp
auto cities = Slate(pluss::make_note({"Accra", "Tema", "Accra", "Kumasi", "Tema"}));
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

`"Accra"` and `"Tema"` are tied at two occurrences each. `top` comes from
[`freq()`](freq.md) internally, whose tie-break keeps whichever value was
**first seen** in the source data — `"Accra"` (row `0`) before `"Tema"` (row
`1`) — not alphabetical order. The two happen to agree here since `"Accra"`
is both first-seen and alphabetically first; that isn't true in general.

Example 4. Any other payload kind throws.

```cpp
auto events = Slate(pluss::make_timeline({"2024-01-01T00:00:00Z", "2024-01-02T00:00:00Z"}));
try {
  auto bad = events.describe();
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Slate::describe supports numeric Series and Note payloads
```

## Notes

- Numeric statistics match [`DataFrame::describe()`](../dataframe/describe.md)
  exactly — `std` uses `n - 1` (sample standard deviation), and quartiles use
  linear interpolation between the two closest ranks, so `50%` coincides with the
  median exactly, including for even-sized samples.
- For `Note`-kind slates, `top` comes from [`freq()`](freq.md); ties for
  `top` keep first-occurrence order, not alphabetical order.
- The returned view always has a display name (`"value"` if the source slate
  didn't already have one), so the render footer reads `Name: ..., ptype: ...`.
- Restricted to numeric `Series` and `Note` payloads; `Timeline`, `TimeIndex`, and
  non-numeric `Series` all throw `"Slate::describe supports numeric Series and
  Note payloads"`.

## See Also

- [`DataFrame::describe`](../dataframe/describe.md): the same summary computed
  for every numeric feature of a frame at once.
- [`Slate::freq`](freq.md): the full frequency table this method's `Note`-kind
  `top`/`freq` are drawn from.
- [`Slate::mean`](mean.md), [`Slate::std`](std.md),
  [`Slate::percentiles`](percentiles.md): compute one specific numeric statistic
  directly instead of the full summary.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="defined.html">
    <span class="label">Previous</span>
    <span class="title">defined</span>
  </a>
  <a class="next-link" href="distinct.html">
    <span class="label">Next</span>
    <span class="title">distinct</span>
  </a>
</div>
