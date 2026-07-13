# DataFrame::corr

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::corr(bool render = false) -> DataFrameView

DataFrame::corr(CorrMethod method, bool render = false) -> DataFrameView

DataFrame::corr(pluss::RangeView<std::string_view> features, bool render = false) -> DataFrameView

DataFrame::corr(pluss::RangeView<std::string_view> features, CorrMethod method, bool render = false) -> DataFrameView
```

Compute a correlation matrix across numeric features.

You can let PlussMic infer the numeric feature set or pass an explicit subset, and you can choose the correlation method explicitly when the default Pearson route is not the statistic you want.

## Parameters

**render**

Display toggle for the returned view.

- `false`: keep explicit control over when and how you render or print the result.
- `true`: allow the returned view to auto-render in temporary display contexts.

**method**

Correlation method to compute.

- `Pearson`: ordinary linear correlation.
- `Spearman`: rank correlation.
- `KendallTau`: rank-concordance correlation.

**features**

Optional feature subset.

Leave this empty to infer every numeric feature automatically. Pass a feature list when you want the matrix restricted to a known subset.

## Returns

`DataFrameView`
    Correlation matrix view whose rows and columns correspond to the participating numeric features.

## See Also

- <a href="corr_with.html"><code>DataFrame::corr_with</code></a>: correlate one frame against one or more peer frames.
- <a href="covar.html"><code>DataFrame::covar</code></a>: covariance matrix instead of correlation.

## Notes

Non-numeric features are ignored even when they exist on the source frame.

When you pass `features`, only that subset participates in the matrix.

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

Example 2. Switch to Spearman correlation without changing the selected feature set.

```cpp
using namespace pluss::table;

auto corr_base = DataFrame({
    {"x", pluss::make_series({1.0, 2.0, 3.0, 4.0})},
    {"y", pluss::make_series({2.0, 4.0, 6.0, 8.0})},
    {"z", pluss::make_series({4.0, 1.0, 3.0, 2.0})},
    {"city", pluss::make_note({"A", "B", "C", "D"})},
});

auto out = corr_base.corr(DataFrame::CorrMethod::Spearman, false);
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

Example 3. Restrict the correlation matrix to an explicit feature subset.

```cpp
using namespace pluss::table;

auto corr_base = DataFrame({
    {"x", pluss::make_series({1.0, 2.0, 3.0, 4.0})},
    {"y", pluss::make_series({2.0, 4.0, 6.0, 8.0})},
    {"z", pluss::make_series({4.0, 1.0, 3.0, 2.0})},
    {"city", pluss::make_note({"A", "B", "C", "D"})},
});

auto out = corr_base.corr({"x", "z"}, false);
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

Example 4. Use a non-default method on an explicit feature subset.

```cpp
using namespace pluss::table;

auto corr_base = DataFrame({
    {"x", pluss::make_series({1.0, 2.0, 3.0, 4.0})},
    {"y", pluss::make_series({2.0, 4.0, 6.0, 8.0})},
    {"z", pluss::make_series({4.0, 1.0, 3.0, 2.0})},
    {"city", pluss::make_note({"A", "B", "C", "D"})},
});

auto out = corr_base.corr({"x", "z"}, DataFrame::CorrMethod::KendallTau, false);
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
