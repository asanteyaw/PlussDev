<!-- pluss-hand-authored -->
<!-- pluss-signature-hash: 16b42e4326c4 -->

# Slate::corr

[Back to Slate API Reference](../../slate_api_reference)

## Signature

```cpp
Slate::corr(const Slate& other, SlateCorrMethod method = SlateCorrMethod::Pearson) const -> double
```

## What It Does

Computes the correlation between this slate and `other`, as a plain `double`
(unlike most of the reduction family, this one doesn't return a
`SlateValue`). Three methods are available:

- **`Pearson`** (the default): ordinary linear correlation — sensitive to the
  actual shape of the relationship, not just its direction.
- **`Spearman`**: rank correlation — correlates the *ranks* of the two
  slates instead of their raw values, so it only cares whether the
  relationship is monotonic, not whether it's linear.
- **`KendallTau`**: rank-concordance correlation — a different, more
  robust-to-outliers measure of monotonic association.

Both slates must be numeric `Series`, or it throws. If either slate has
explicit case labels, the two are first **aligned by case label** rather than
raw position — see the important callout below, since this changes the result
compared to a naive positional pairing. After alignment, missing/non-finite
entries in either slate are excluded pairwise (a position missing in *either*
slate drops that pair from both). If fewer than 2 valid pairs remain, or
either slate has zero variance across its valid pairs, the result is `NaN`
(never thrown).

```{important} case-label alignment changes the result
If both slates carry explicit case labels, `corr()` (and
[`covar()`](covar.md)/[`corr_with()`](corr_with.md)) pairs entries by
*matching label*, not by raw position — the same way two `DataFrame` columns
with mismatched row order still combine correctly. Two slates of equal length
with no explicit labels pair positionally instead. Example 3 below shows how
different these two outcomes can be for the same underlying data.
```

## Parameters

**other** (`const Slate&`)
    The numeric `Series` slate to correlate with.

**method** (`SlateCorrMethod`, default `Pearson`)
    `Pearson`, `Spearman`, or `KendallTau`.

## Returns

`double`
    The correlation coefficient in `[-1, 1]`, or `NaN` if there aren't enough
    valid pairs, or either side has zero variance.

## Examples

Example 1. Pearson correlation between two related numeric slates.

```cpp
using namespace pluss::table;

auto scores = Slate(pluss::make_series({10.0, 20.0, 30.0, 40.0}));
auto other = Slate(pluss::make_series({10.0, 10.0, 35.0, 45.0}));
auto out = scores.corr(other, SlateCorrMethod::Pearson);
std::cout << out;
```

### Output

```text
0.943119
```

Example 2. Spearman correlation is exactly `1.0` for a perfectly monotonic —
but nonlinear — relationship, where Pearson is not.

```cpp
auto x = Slate(pluss::make_series({1.0, 2.0, 3.0, 4.0, 5.0}));
auto y = Slate(pluss::make_series({1.0, 4.0, 9.0, 16.0, 25.0})); // y = x^2

auto pearson = x.corr(y, SlateCorrMethod::Pearson);
auto spearman = x.corr(y, SlateCorrMethod::Spearman);
std::cout << "pearson: " << pearson << ", spearman: " << spearman;
```

### Output

```text
pearson: 0.981105, spearman: 1
```

Pearson sees the curvature in `y = x^2` and reports slightly less than
perfect correlation; Spearman only cares that `y` strictly increases whenever
`x` does, so it reports a perfect `1`.

Example 3. Case-label alignment changes the result compared to raw positional
pairing.

```cpp
auto left = Slate(pluss::make_series({1.0, 2.0, 3.0}))
    .caselbl(Slate(pluss::make_note({"a", "b", "c"})));
auto right = Slate(pluss::make_series({30.0, 10.0, 20.0}))
    .caselbl(Slate(pluss::make_note({"c", "a", "b"})));

auto aligned = left.corr(right);
std::cout << aligned;
```

### Output

```text
1
```

Even though `right`'s raw values are in a different order than `left`'s,
matching them up by label (`"a"` with `"a"`, `"b"` with `"b"`, `"c"` with
`"c"`) reveals a perfect underlying relationship: label `"a"` pairs `1` with
`10`, `"b"` pairs `2` with `20`, `"c"` pairs `3` with `30`. Comparing the same
two slates' *raw positions* instead (`1` with `30`, `2` with `10`, `3` with
`20`) would give `-0.5` — label alignment is what makes this example correlate
perfectly.

Example 4. Fewer than 2 valid pairs, or zero variance in either slate,
returns `NaN` rather than throwing.

```cpp
auto constant = Slate(pluss::make_series({5.0, 5.0, 5.0}));
auto varying = Slate(pluss::make_series({1.0, 2.0, 3.0}));
auto out4 = constant.corr(varying);
std::cout << std::isnan(out4);
```

### Output

```text
true
```

## Notes

- If either slate carries explicit case labels, pairing is by matching label,
  not raw position — see the important callout above.
- Missing or non-finite entries are excluded pairwise; a position missing in
  either slate drops that pair from both.
- Returns `NaN` (never throws) for fewer than 2 valid pairs or zero variance
  in either side.
- Restricted to numeric `Series` payloads; anything else throws
  `"Slate::corr: both operands must be numeric Series"`.

## See Also

- [`Slate::corr_with`](corr_with.md): the same computation, returned as a
  `SlateValue` with a `min_count` validity gate instead of a raw `double`.
- [`Slate::covar`](covar.md): the unnormalized counterpart — covariance
  instead of correlation.
- [`Slate::rank`](rank.md): the ranking operation Spearman correlation is
  built on.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="compare.html">
    <span class="label">Previous</span>
    <span class="title">compare</span>
  </a>
  <a class="next-link" href="corr_with.html">
    <span class="label">Next</span>
    <span class="title">corr_with</span>
  </a>
</div>
