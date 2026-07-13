# DataFrame::covar

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::covar(const StatisticOptions& options = {}, bool render = false) -> DataFrameView
```

Compute the pairwise covariance matrix across numeric features.

Use this when you want covariance scale rather than unit-free correlation.

## Parameters

**options**

Statistic policy options.

Use `StatisticOptions` to control details such as:
- denominator correction (`ddof`),
- minimum pairwise-complete count before a value is reported.

**render**

Display toggle for the returned view.

## Returns

`DataFrameView`
    Covariance matrix over the participating numeric features.

## See Also

- <a href="corr.html"><code>DataFrame::corr</code></a>: correlation matrix instead of covariance.

## Examples

Example 1. Compute the covariance matrix across all numeric features.

```cpp
using namespace pluss::table;

auto corr_base = DataFrame({
    {"x", pluss::make_series({1.0, 2.0, 3.0, 4.0})},
    {"y", pluss::make_series({2.0, 4.0, 6.0, 8.0})},
    {"z", pluss::make_series({4.0, 1.0, 3.0, 2.0})},
    {"city", pluss::make_note({"A", "B", "C", "D"})},
});

auto out = corr_base.covar({}, false);
```

### Output

```text
=================================================
     | x              y             z            
-------------------------------------------------
x    |  1.666666667    3.333333333  -0.6666666667
y    |  3.333333333    6.666666667  -1.333333333 
z    | -0.6666666667  -1.333333333   1.666666667 
-------------------------------------------------
[3 cs x 3 feat]
```



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
