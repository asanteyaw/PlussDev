<!-- pluss-hand-authored -->

# DataFrame::equals

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::equals(const DataFrame& other) const -> bool
DataFrame::equals(const DataFrame& other, double atol, double rtol) const -> bool
```

## What It Does

A whole-frame identity check — `true` only if `other` matches this frame in every
respect: same case count, same feature count, same feature names *in the same order*,
same case-label dtype and validity pattern, and every cell's payload kind, validity,
and value matching. Where [`eq()`](eq.md) returns a per-cell boolean mask, `equals()`
collapses the whole comparison down to one `bool`.

Two comparison modes are available:

- **Exact** (`equals(other)`, the default): every value is compared via its formatted
  string representation. No floating-point tolerance — two numerically-close-but-not-
  identical floats do not compare equal.
- **Tolerant** (`equals(other, atol, rtol)`): for numeric feature cells specifically,
  values are compared with the same `|a - b| <= atol + rtol * max(|a|, |b|)` rule
  `numpy.allclose` uses, absorbing small floating-point discrepancies from independent
  computation paths that should mathematically agree. Non-numeric cells (`Note`,
  `Timeline`, `TimeIndex`, or a numeric/non-numeric mismatch) always fall back to exact
  string comparison even in tolerant mode — `atol`/`rtol` only relax numeric-to-numeric
  comparisons.

```{important} Case labels are always compared exactly, even in tolerant mode
`atol`/`rtol` only relax the *feature cell* comparison. Case labels (and their dtype
and validity pattern) are always compared with exact string equality, regardless of
which overload you call.
```

No alignment happens before either check: `equals()` does not try to reorder features
or match case labels first the way [`add()`](add.md) or [`eq()`](eq.md) can with
`AlignOptions` — the two frames are compared exactly as supplied. If you need to
compare two frames that carry the same data in a different feature or case order, use
[`reorder()`](reorder.md)/[`arrange()`](arrange.md) (or the alignment-aware
[`eq()`](eq.md)) first.

## Parameters

**other** (`const DataFrame&`)

The frame to compare against. Compared exactly as given — feature order and case
order both matter.

**atol** (`double`, tolerant overload only)

The absolute tolerance term. Must be non-negative.

**rtol** (`double`, tolerant overload only)

The relative tolerance term, scaled by the larger of the two values being compared at
each cell. Must be non-negative.

## Returns

`bool`
    `true` only when both frames have identical case count, feature count, feature
    names (in order), case-label dtype and values, and every feature's payload kind,
    validity pattern, and cell values (exactly, or within `atol`/`rtol` for numeric
    cells under the tolerant overload). `false` otherwise.

## Examples

Example 1. A copy always compares equal to its source.

```cpp
using namespace pluss::table;

auto official = DataFrame({
    {"quiz",  pluss::make_series({84.0, 90.0, 84.0})},
    {"final", pluss::make_series({84.0, 88.0, 72.0})},
}, pluss::make_note({"s1", "s2", "s3"}));

auto out = official.equals(official.copy());
std::cout << std::boolalpha << out;
```

### Output

```text
true
```

Example 2. One changed value anywhere is enough to fail the check.

```cpp
auto changed = DataFrame({
    {"quiz",  pluss::make_series({84.0, 90.0, 83.0})},
    {"final", pluss::make_series({84.0, 88.0, 72.0})},
}, pluss::make_note({"s1", "s2", "s3"}));

auto out_changed = official.equals(changed);
std::cout << std::boolalpha << out_changed;
```

### Output

```text
false
```

Only `s3`'s `quiz` value differs (`84` vs `83`) — every other cell matches, but the
exact `equals()` overload has no notion of "mostly the same": any single mismatched
cell fails the whole check.

Example 3. A tiny floating-point discrepancy fails the exact overload but passes the
tolerant one.

```cpp
auto nearly_official = DataFrame({
    {"quiz",  pluss::make_series({84.0000001, 90.0, 84.0})},
    {"final", pluss::make_series({84.0, 88.0, 72.0})},
}, pluss::make_note({"s1", "s2", "s3"}));

auto out_exact = official.equals(nearly_official);
std::cout << std::boolalpha << out_exact << "\n";

auto out_tolerant = official.equals(nearly_official, /* atol = */ 1e-6, /* rtol = */ 0.0);
std::cout << std::boolalpha << out_tolerant;
```

### Output

```text
false
true
```

`s1`'s `quiz` value differs by `1e-7` between the two frames — enough to fail the
exact, string-representation-based check, but well within the `atol = 1e-6` tolerance
passed to the second call, which reports `true`.

## Notes

- The exact overload (`equals(other)`) compares values via their formatted string
  representation, with no floating-point tolerance — two numerically-close-but-not-
  identical floats will **not** compare equal. Use the tolerant overload
  (`equals(other, atol, rtol)`) when small floating-point discrepancies from
  independent computation paths should be treated as equal.
- The tolerant overload's `atol`/`rtol` only apply to numeric-to-numeric feature cell
  comparisons; case labels, non-numeric features, and any cell where one side isn't
  numeric always fall back to exact string comparison.
- Under the tolerant overload, two `NaN` values compare equal to each other (unlike
  ordinary IEEE 754 `NaN != NaN` semantics), and two infinities compare equal only if
  they have the same sign.
- Feature order matters: a frame with the same features in a different order is not
  `equals()`-equal, even though [`eq()`](eq.md) with `AlignBy::Feature` alignment would
  happily compare them.
- Missingness is part of the contract — a valid cell and a `NULL` cell at the same
  position never compare equal, even if the underlying storage happens to hold the
  same bit pattern.
- There is no `AlignOptions` overload; `equals()` never re-aligns before comparing.

## See Also

- [`DataFrame::eq`](eq.md): per-cell equality mask, with optional alignment via
  `AlignOptions`.
- [`DataFrame::copy`](copy.md): produce a structurally identical frame, useful as a
  baseline before mutating and re-checking with `equals()`.
- [`DataFrame::reorder`](reorder.md): normalize feature order before comparing two
  frames that hold the same data in a different arrangement.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="eq.html">
    <span class="label">Previous</span>
    <span class="title">eq</span>
  </a>
  <a class="next-link" href="exp.html">
    <span class="label">Next</span>
    <span class="title">exp</span>
  </a>
</div>
