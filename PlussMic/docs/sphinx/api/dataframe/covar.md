<!-- pluss-hand-authored -->

# DataFrame::covar

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::covar(const StatisticOptions& options = {}, bool render = false) const -> DataFrameView
```

## What It Does

Computes the pairwise covariance matrix across the frame's numeric features — the
same square, symmetric, one-row-and-column-per-feature layout as [`corr()`](corr.md),
but reporting raw covariance instead of a normalized `[-1, 1]` correlation coefficient.
The diagonal holds each feature's own variance (covariance with itself); off-diagonal
cell `(i, j)` holds the covariance between feature `i` and feature `j`. Non-numeric
features are silently excluded, exactly as in `corr()`.

Where `corr()` answers "how strongly (and in what direction) do these features move
together, on a fixed `[-1, 1]` scale?", `covar()` answers the same directional
question but on each feature's own original scale — useful when the actual magnitude
of joint variation matters (for example, feeding a covariance matrix into a downstream
statistical model), at the cost of values that aren't directly comparable across
feature pairs with different units or scales.

## Parameters

**options** (`const StatisticOptions&`, default `{}`)

- **`ddof`** (`std::int64_t`, default `1`): "delta degrees of freedom" — the
  covariance sum is divided by `(paired_count - ddof)`. The default `1` gives the
  usual sample covariance (Bessel's correction); `0` gives the population covariance,
  dividing by the raw paired observation count instead. Must be `>= 0`.
- **`min_count`** (`std::int64_t`, default `1`): minimum number of paired,
  non-null observations required before a cell is computed at all; cells that don't
  meet it come back `NULL`. Must be `>= 0`.

**render** (`bool`, default `false`)

Display toggle for the returned `DataFrameView`, following the same convention as
[`head()`](head.md).

## Returns

`DataFrameView`
    A square covariance matrix over the frame's numeric features, in the same feature
    order as [`corr()`](corr.md) would produce for the same frame.

## Examples

Example 1. Compute the sample covariance matrix (the default `ddof = 1`) across every
numeric feature, ignoring a non-numeric one.

```cpp
using namespace pluss::table;

auto sample = DataFrame({
    {"x", Thread(pluss::make_series({1.0, 2.0, 3.0, 4.0}))},
    {"y", Thread(pluss::make_series({2.0, 4.0, 6.0, 8.0}))},
    {"z", Thread(pluss::make_series({4.0, 1.0, 3.0, 2.0}))},
    {"city", Thread(pluss::make_note({"A", "B", "C", "D"}))},
});

auto out = sample.covar();
std::cout << out;
```

### Output

```text
=================================================
     | x              y             z            
-------------------------------------------------
x    | 1.666666667    3.333333333   -0.6666666667
y    | 3.333333333    6.666666667   -1.333333333 
z    | -0.6666666667  -1.333333333  1.666666667  
-------------------------------------------------
[3 cs x 3 feat]
```

`city` is silently excluded, the same non-numeric-skipping rule [`corr()`](corr.md)
follows. `x`'s own variance (`1.666...`, the top-left diagonal cell) is the sample
variance of `[1, 2, 3, 4]`; `y`, being exactly `2 * x`, has four times that variance
(`6.666...`), consistent with `Var(2X) = 4 * Var(X)`.

Example 2. Switch to population covariance (`ddof = 0`) and see every value shrink by
the same `(n-1)/n` ratio.

```cpp
StatisticOptions population_options;
population_options.ddof = 0;

auto out = sample.covar(population_options);
std::cout << out;
```

### Output

```text
=======================
     | x     y     z   
-----------------------
x    | 1.25  2.5   -0.5
y    | 2.5   5     -1  
z    | -0.5  -1    1.25
-----------------------
[3 cs x 3 feat]
```

With four paired observations, `ddof = 0` divides by `4` instead of `3`, so every
value in Example 1's matrix is scaled down by exactly `3/4` here (`1.666... * 0.75 =
1.25`, and so on) — the same relationship as between a sample variance and a
population variance anywhere else in the library.

Example 3. Pass `render = true` and let the returned view print itself, with no
explicit `std::cout` at all.

```cpp
sample.covar({}, true);
```

### Output

```text
=================================================
     | x              y             z            
-------------------------------------------------
x    | 1.666666667    3.333333333   -0.6666666667
y    | 3.333333333    6.666666667   -1.333333333 
z    | -0.6666666667  -1.333333333  1.666666667  
-------------------------------------------------
[3 cs x 3 feat]
```

Same matrix as Example 1's default `ddof = 1` result — this bare statement
differs only in `render`. The temporary `DataFrameView` is destroyed at the end
of the statement without ever being captured or streamed, and `render = true`
is what makes that destruction print automatically; with the default `render =
false`, the same unassigned call would produce no output.

## Notes

- Non-numeric features never participate and are never padded into the matrix as
  `NULL` rows/columns — they simply don't appear.
- `ddof = 0` (population) and `ddof = 1` (sample, the default) differ by a constant
  `(n-1)/n` scaling factor for a given pair, assuming the same paired-observation
  count for both.
- Use [`corr()`](corr.md) instead when you want values on a fixed, comparable
  `[-1, 1]` scale rather than each feature's own units.

## See Also

- [`DataFrame::corr`](corr.md): the normalized, unit-free counterpart to this matrix.
- [`DataFrame::corr_with`](corr_with.md): compare against a *different* frame's
  same-named features instead of this frame's own features against each other.
- [`DataFrame::var`](var.md) / [`DataFrame::std`](std.md): the single-feature variance
  and standard deviation this matrix's diagonal generalizes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="count_by.html">
    <span class="label">Previous</span>
    <span class="title">count_by</span>
  </a>
  <a class="next-link" href="csmax.html">
    <span class="label">Next</span>
    <span class="title">csmax</span>
  </a>
</div>
