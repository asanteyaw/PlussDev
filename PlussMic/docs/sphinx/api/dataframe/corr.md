<!-- pluss-hand-authored -->

# DataFrame::corr

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::corr(bool render = false) const -> DataFrameView

DataFrame::corr(CorrMethod method, bool render = false) const -> DataFrameView
```

## What It Does

Computes a correlation matrix across every numeric feature currently on the frame and
returns it as a square `DataFrameView`: one row and one column per participating
feature, with cell `(i, j)` holding the correlation between feature `i` and feature
`j`. The diagonal is always `1` (every feature correlates perfectly with itself), and
the matrix is symmetric — `corr(i, j) == corr(j, i)`.

`corr()` carries no feature selector of its own: it always correlates every numeric
feature on the current frame. On a wide frame, where the full matrix would be large
and mostly uninteresting, narrow to the columns you actually care about first with
[`features()`](features.md) — `corr_base.features({"x", "z"}).corr()` — rather than
computing the full N×N matrix and slicing it down afterward.

`corr()` is naturally the next step after [`describe()`](describe.md): `describe()` tells
you what each feature looks like in isolation, `corr()` tells you how features move
together — useful for spotting redundant features before modeling, or for a quick check
that two columns that *should* be related (a price and a cost, a signal and its lagged
value) actually are.

## Parameters

**render** (`bool`, default `false`)

Display toggle for the returned view. `false` gives you explicit control over when and
how you render or print the result; `true` allows the returned view to auto-render in
temporary display contexts.

**method** (`CorrMethod`, default `Pearson`)

The correlation statistic to compute:

- `Pearson`: ordinary linear correlation — sensitive to the actual magnitude and shape
  of the relationship, and the right default when you expect roughly linear
  relationships between numeric features.
- `Spearman`: rank correlation — computed on the ranks of the values rather than the
  values themselves, so it captures monotonic (not necessarily linear) relationships and
  is less sensitive to outliers.
- `KendallTau`: rank-concordance correlation — another rank-based measure, generally
  more robust than Spearman on small samples at the cost of being more expensive to
  compute.

## Returns

`DataFrameView`
    A correlation matrix view whose rows and columns correspond, in the same order, to
    the participating numeric features (either every numeric feature on the frame, or
    the explicit `features` subset you passed).

## Examples

Example 1. Correlate every numeric feature using the default Pearson method.

```cpp
using namespace pluss::table;

auto corr_base = DataFrame({
    {"x", pluss::make_series({1.0, 2.0, 3.0, 4.0})},
    {"y", pluss::make_series({2.0, 4.0, 6.0, 8.0})},
    {"z", pluss::make_series({4.0, 1.0, 3.0, 2.0})},
    {"city", pluss::make_note({"A", "B", "C", "D"})},
});

auto out = corr_base.corr(false);
std::cout << out;
```

### Output

```text
=======================
     | x     y     z   
-----------------------
x    |  1     1    -0.4
y    |  1     1    -0.4
z    | -0.4  -0.4   1  
-----------------------
[3 cs x 3 feat]
```

`x` and `y` correlate perfectly (`1`) because `y` is exactly `2 * x` — a perfect linear
relationship. `z` correlates negatively with both (`-0.4`) because it tends to move in
the opposite direction. `city` is silently excluded, the same non-numeric-skipping rule
`describe()` and `info()`'s dtype handling follow throughout the table API.

Example 2. Switch to Spearman correlation without changing the selected feature set.

```cpp
auto out = corr_base.corr(DataFrame::CorrMethod::Spearman, false);
std::cout << out;
```

### Output

```text
=======================
     | x     y     z   
-----------------------
x    |  1     1    -0.4
y    |  1     1    -0.4
z    | -0.4  -0.4   1  
-----------------------
[3 cs x 3 feat]
```

The values are unchanged here because `x`, `y`, and `z` are all monotonically related to
each other in this tiny sample — Pearson and Spearman only tend to diverge once a
relationship is monotonic but not linear (or once outliers are present), which four
clean points does not exercise. On real data with heavier tails or a non-linear-but-
monotonic relationship, expect Pearson and Spearman to disagree, sometimes
substantially.

Example 3. Restrict the correlation matrix to an explicit feature subset by narrowing
with [`features()`](features.md) first.

```cpp
auto out = corr_base.features({"x", "z"}).corr(false);
std::cout << out;
```

### Output

```text
=================
     | x     z   
-----------------
x    |  1    -0.4
z    | -0.4   1  
-----------------
[2 cs x 2 feat]
```

Narrowing with `features()` first is not just a display convenience — on a frame with
many numeric columns, computing the full N×N matrix and then slicing it down is wasted
work compared to naming the handful of columns you actually want compared.

Example 4. Combine an explicit subset with a non-default method.

```cpp
auto out = corr_base.features({"x", "z"}).corr(DataFrame::CorrMethod::KendallTau, false);
std::cout << out;
```

### Output

```text
===================================
     | x              z            
-----------------------------------
x    |  1             -0.3333333333
z    | -0.3333333333   1           
-----------------------------------
[2 cs x 2 feat]
```

Kendall's tau reports a smaller magnitude (`-0.333` vs. Pearson's `-0.4`) for the same
pair of features — a reminder that "correlated" is method-dependent, and that the three
methods are not interchangeable numbers with the same scale, only the same `[-1, 1]`
range and sign convention.

Example 5. Pass `render = true` and let the returned view print itself, with no
explicit `std::cout` at all.

```cpp
corr_base.corr(true);
```

### Output

```text
=======================
     | x     y     z   
-----------------------
x    |  1     1    -0.4
y    |  1     1    -0.4
z    | -0.4  -0.4   1  
-----------------------
[3 cs x 3 feat]
```

Same matrix as Example 1 — `corr_base.corr(false)` versus this bare
`corr_base.corr(true);` statement differ only in `render`. Every other example
on this page assigns the result to a variable and streams it explicitly; here,
the temporary `DataFrameView` is destroyed at the end of the statement without
ever being captured, and `render = true` is what makes that destruction print
automatically. With the default `render = false`, this same bare statement
would produce no output.

## Notes

- Non-numeric features are ignored even when they exist on the current frame — there
  is no need to drop `Note` or `Timeline` columns manually before calling `corr()`.
- When you narrow with [`features()`](features.md) first, only that subset
  participates in the matrix, and only those columns need to be numeric.
- All three methods report values in `[-1, 1]`; `1` and `-1` indicate a perfect
  monotonic (Spearman/Kendall) or linear (Pearson) relationship, and `0` indicates none.

## See Also

- [`DataFrame::corr_with`](corr_with.md): correlate one frame against one or more peer
  frames instead of correlating a frame's features against each other.
- [`DataFrame::covar`](covar.md): covariance matrix instead of a normalized correlation
  matrix.
- [`DataFrame::describe`](describe.md): per-feature statistics, typically checked before
  moving on to cross-feature correlation.
- [`DataFrame::features`](features.md): narrow the frame to the feature subset
  `corr()` should compute over.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="copy.html">
    <span class="label">Previous</span>
    <span class="title">copy</span>
  </a>
  <a class="next-link" href="corr_with.html">
    <span class="label">Next</span>
    <span class="title">corr_with</span>
  </a>
</div>
