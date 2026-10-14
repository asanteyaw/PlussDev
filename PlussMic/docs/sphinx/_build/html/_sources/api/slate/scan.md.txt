<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: b55bab1c8862 -->

# Slate::scan

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
Slate::scan(ScanStatistic statistic, bool skip_nulls = true) const -> Slate
```

## What It Does

Computes a **running** (cumulative) statistic down the slate — the value at
each position reflects every observed entry up to and including it, not just
that position alone. [`cumsum()`](cumsum.md), [`cumprod()`](cumprod.md),
[`cummin()`](cummin.md), and [`cummax()`](cummax.md) are thin wrappers around
this method for each `ScanStatistic`; this page documents the shared mechanics
all four rely on. Restricted to numeric `Series` payloads; every other kind or
dtype throws.

```{note} the throw message always says "Slate::scan"
`cumsum()`/`cumprod()`/`cummin()`/`cummax()` each delegate straight to
`scan(...)`, passing the literal string `"Slate::scan"` as the error context —
so calling any of the four wrapper methods on a non-numeric slate throws
`"Slate::scan: expected a real numeric Series Slate"`, not a message naming
the specific method you actually called.
```

Internally, missing entries are first replaced with the operation's
**identity element** before the running computation runs — `0` for `Sum`, `1`
for `Product`, `+∞` for `Minimum`, `-∞` for `Maximum` — so a missing entry
never resets or corrupts the running value for the positions after it. What
happens to *that position's own* output value is then controlled separately by
`skip_nulls`:

- **`skip_nulls = true`** (the default): each output position's validity
  mirrors its *own* input position — a position that was missing in the input
  stays missing in the output, but the running computation for every
  **later** position still correctly includes whatever value came before it
  (accumulated through the identity-substituted missing spot).
- **`skip_nulls = false`**: once any position is missing, **every position
  after it** becomes missing too — the missing entry "poisons" the running
  computation forward, even though the identity-substituted arithmetic
  underneath is still technically well-defined. This matches the common
  spreadsheet/pandas convention that a running total is undefined once a gap
  appears in the input, unless you explicitly ask to skip past it.

## Parameters

**statistic** (`ScanStatistic`)
    Which running computation to perform: `Sum`, `Product`, `Minimum`, or
    `Maximum`.

**skip_nulls** (`bool`, default `true`)
    `true`: a missing input position stays missing in the output, but later
    positions' running values are unaffected. `false`: a missing input
    position makes every subsequent output position missing too.

## Returns

`Slate`
    A slate the same length as the source, holding the running statistic at
    each position.

## Examples

Example 1. Running sum with `skip_nulls = true` (the default) — the missing
position stays missing, but the run continues correctly underneath it.

```cpp
using namespace pluss::table;

auto readings = Slate(
    pluss::make_series({10.0, 20.0, 30.0, 40.0}),
    pluss::make_series(std::vector<bool>{true, false, true, true})
);
auto out = readings.scan(ScanStatistic::Sum);
std::cout << out;
```

### Output

```text
0  10
1  NULL
2  40
3  80

ptype: Series
```

Position `1` (missing) is hidden as `NULL`, but position `2`'s running sum is
`40` (`10 + 30`, correctly skipping the missing `20.0`), and position `3`'s is
`80` (`10 + 30 + 40`) — the gap doesn't reset or break the running total for
what follows.

Example 2. The same input with `skip_nulls = false` — the missing position
poisons every position after it.

```cpp
auto out2 = readings.scan(ScanStatistic::Sum, false);
std::cout << out2;
```

### Output

```text
0  10
1  NULL
2  NULL
3  NULL

ptype: Series
```

Position `1` is still `NULL` (there was nothing to sum there), but now
positions `2` and `3` are also `NULL` — even though their underlying running
totals are the same `40`/`80` computed in Example 1, `skip_nulls = false`
suppresses them because a gap appeared earlier in the run.

Example 3. Restricted to numeric `Series` payloads — the throw message always
reads `"Slate::scan"`.

```cpp
auto words = Slate(pluss::make_note({"a", "b"}));
try {
  auto bad = words.scan(ScanStatistic::Sum);
} catch (const std::exception& e) {
  std::cout << e.what();
}
```

### Output

```text
Slate::scan: expected a real numeric Series Slate
```

## Notes

- Missing entries are substituted with the operation's identity element before
  the running computation runs, so later positions' values are always
  arithmetically correct underneath — `skip_nulls` only controls which
  positions are *displayed* as missing, not what gets computed.
- `skip_nulls = false` propagates missingness forward from the first gap
  onward, matching the common "undefined once a gap appears" convention.
- Restricted to numeric `Series` payloads; anything else throws
  `"Slate::scan: expected a real numeric Series Slate"` — this exact message,
  regardless of whether you call `scan()` directly or through `cumsum()`
  /`cumprod()`/`cummin()`/`cummax()`.

## See Also

- [`Slate::cumsum`](cumsum.md), [`Slate::cumprod`](cumprod.md),
  [`Slate::cummin`](cummin.md), [`Slate::cummax`](cummax.md): the four
  fixed-statistic wrappers around this method.
- [`Slate::sum`](sum.md), [`Slate::prod`](prod.md), [`Slate::min`](min.md),
  [`Slate::max`](max.md): the single-final-value counterparts, rather than a
  running value at every position.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="sample.html">
    <span class="label">Previous</span>
    <span class="title">sample</span>
  </a>
  <a class="next-link" href="se.html">
    <span class="label">Next</span>
    <span class="title">se</span>
  </a>
</div>
