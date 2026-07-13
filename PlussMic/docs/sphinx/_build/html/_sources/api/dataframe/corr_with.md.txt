# DataFrame::corr_with

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::corr_with(const DataFrame& other, CorrMethod method = CorrMethod::Pearson, std::int64_t min_count = 1) -> Thread

DataFrame::corr_with(pluss::RangeView<DataFrame> others, CorrMethod method = CorrMethod::Pearson, std::int64_t min_count = 1) -> DataFrame
```

Compute aligned feature-wise correlations between this frame and one or more peer frames.

This is the cross-frame correlation surface to use when you want one correlation value per shared numeric feature instead of a square correlation matrix.

## Parameters

**other**

One peer frame to compare against.

The result is a one-dimensional thread with one correlation value per shared numeric feature.

**others**

Several peer frames to compare against in one call.

Each peer frame becomes one output feature in the returned frame.

**method**

Correlation method.

- `Pearson`: ordinary linear correlation.
- `Spearman`: rank correlation.
- `KendallTau`: rank-concordance correlation.

**min_count**

Minimum number of pairwise-complete observations required before a correlation value is emitted.

## Returns

`Thread`
    Returned when you correlate against one other frame. Each value corresponds to one shared numeric feature.

`DataFrame`
    Returned when you correlate against several peer frames in one call. Each output feature corresponds to one peer frame.

## Notes

Only shared numeric features participate.

Rows are aligned before the correlation statistic is computed, so the comparison is case-aware rather than position-blind.

## Examples

Example 1. Correlate one frame against one peer and get one value per shared numeric feature.

```cpp
using namespace pluss::table;

auto corr_base = DataFrame({
    {"x", pluss::make_series({1.0, 2.0, 3.0, 4.0})},
    {"y", pluss::make_series({2.0, 4.0, 6.0, 8.0})},
    {"z", pluss::make_series({4.0, 1.0, 3.0, 2.0})},
});

auto corr_peer = DataFrame({
    {"x", pluss::make_series({2.0, 3.0, 4.0, 5.0})},
    {"y", pluss::make_series({4.0, 5.0, 7.0, 9.0})},
    {"z", pluss::make_series({3.0, 2.0, 4.0, 1.0})},
});

auto out = corr_base.corr_with(corr_peer, DataFrame::CorrMethod::Pearson, 1);
```

### Output

```text
x  1
y  0.9897782666
z  0.6
dtype: Series
```

Example 2. Correlate one frame against many peers and get one output feature per peer.

```cpp
using namespace pluss::table;

auto corr_base = DataFrame({
    {"x", pluss::make_series({1.0, 2.0, 3.0, 4.0})},
    {"y", pluss::make_series({2.0, 4.0, 6.0, 8.0})},
    {"z", pluss::make_series({4.0, 1.0, 3.0, 2.0})},
});

auto corr_peer = DataFrame({
    {"x", pluss::make_series({2.0, 3.0, 4.0, 5.0})},
    {"y", pluss::make_series({4.0, 5.0, 7.0, 9.0})},
    {"z", pluss::make_series({3.0, 2.0, 4.0, 1.0})},
});

auto corr_peer_2 = DataFrame({
    {"x", pluss::make_series({5.0, 4.0, 3.0, 2.0})},
    {"y", pluss::make_series({8.0, 6.0, 4.0, 2.0})},
    {"z", pluss::make_series({1.0, 4.0, 2.0, 3.0})},
});

auto out = corr_base.corr_with({corr_peer, corr_peer_2},
                               DataFrame::CorrMethod::Spearman,
                               1);
```

### Output

```text
=======================
     | frame_0  frame_1
-----------------------
0    | 1        -1     
1    | 1        -1     
2    | 0.6      -1     
-----------------------
[3 cs x 2 feat]
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="corr.html">
    <span class="label">Previous</span>
    <span class="title">corr</span>
  </a>
  <a class="next-link" href="count.html">
    <span class="label">Next</span>
    <span class="title">count</span>
  </a>
</div>
