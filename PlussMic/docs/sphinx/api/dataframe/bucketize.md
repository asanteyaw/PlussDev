# DataFrame::bucketize

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::bucketize(pluss::RangeView<double> bins, pluss::RangeView<std::int64_t> labels = {}, bool right = true, bool include_lowest = true) -> DataFrame

DataFrame::bucketize(pluss::RangeView<std::string_view> feature_names, pluss::RangeView<double> bins, pluss::RangeView<std::int64_t> labels = {}, bool right = true, bool include_lowest = true) -> DataFrame

DataFrame::bucketize(std::string_view feature_name, pluss::RangeView<double> bins, pluss::RangeView<std::int64_t> labels = {}, bool right = true, bool include_lowest = true) -> DataFrame
```

Convert selected numeric features into interval labels defined by explicit bin edges.

`DataFrame::bucketize(...)` delegates interval bucketing to the underlying numeric thread kernels while preserving the rest of the mixed-type frame. It is useful for grading bands, coarse feature engineering, histogram-style preprocessing, and rule-based segmentation.

## Parameters

**feature_name**

One feature to bucketize.

Use this overload when only one numeric column should be transformed and everything else should pass through unchanged.

**feature_names**

Explicit feature subset to bucketize.

Use this overload when several numeric columns should be bucketed together while the rest of the frame stays as-is.

**bins**

Strictly increasing interval edges.

For `N` bucket outputs you need `N + 1` edges. For example, edges `{0, 20, 40, 60}` define three intervals.

**labels**

Optional explicit labels for the resulting buckets.

- Leave this empty to let PlussMic emit default bucket labels `0, 1, 2, ...`.
- Pass an explicit integer label list when your downstream logic expects domain-specific codes such as `10, 20, 30`.

**right**

Right-closure policy for the intervals.

- `true`: intervals are right-closed.
- `false`: intervals are left-closed and right-open.

Changing this toggle changes which side owns boundary values such as exactly `20.0` or `40.0`.

**include_lowest**

Whether the very first interval includes its lower boundary.

- `true`: values exactly equal to the first bin edge can enter the first bucket.
- `false`: the first lower boundary stays open like the ordinary interval rule.

## Returns

`DataFrame`
    A new frame where the targeted numeric features have been replaced by integer bucket labels.

    Untargeted features are preserved unchanged.

## See Also

- <a href="../thread/bucketize.html"><code>Thread::bucketize</code></a>: bucketize one numeric column outside a frame.
- <a href="count_by.html"><code>DataFrame::count_by</code></a>: summarize how many rows fell into each engineered bucket after bucketing.
- <a href="bucketize.html"><code>DataFrame::bucketize</code></a>: use the feature-specific overloads when only one subset of columns should be bucketed.

## Notes

The no-feature overload auto-selects every numeric `Series` feature in the frame.

The one-feature and feature-subset overloads let you target only the columns you want while leaving every other feature untouched.

Out-of-range values remain missing. Bucket labels are returned as integer-like numeric outputs.

## Examples

Example 1. Bucketize every numeric feature in the frame and leave text columns alone.

```cpp
using namespace pluss::table;

auto metrics = DataFrame({
    {"sales", pluss::make_series({12.0, 27.0, 35.0, 49.0})},
    {"cost", pluss::make_series({5.0, 18.0, 33.0, 60.0})},
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"})},
}).title("metrics");

auto out = metrics.bucketize({0.0, 20.0, 40.0, 60.0});
```

### Output

```text
==========================
     | sales  cost  city
--------------------------
0    | 0      0     Accra
1    | 1      0     Tema
2    | 1      1     Kumasi
3    | 2      2     Tamale
--------------------------
[4 cs x 3 feat]
```

Example 2. Bucketize one feature with explicit output labels.

```cpp
using namespace pluss::table;

auto metrics = DataFrame({
    {"sales", pluss::make_series({12.0, 27.0, 35.0, 49.0})},
    {"cost", pluss::make_series({5.0, 18.0, 33.0, 60.0})},
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"})},
}).title("metrics");

auto out = metrics.bucketize("sales", {0.0, 20.0, 40.0, 60.0}, {10, 20, 30});
```

### Output

```text
==========================
     | sales  cost  city
--------------------------
0    | 10     5     Accra
1    | 20     18    Tema
2    | 20     33    Kumasi
3    | 30     60    Tamale
--------------------------
[4 cs x 3 feat]
```

Example 3. Bucketize an explicit subset of numeric features together.

```cpp
using namespace pluss::table;

auto metrics = DataFrame({
    {"sales", pluss::make_series({12.0, 27.0, 35.0, 49.0})},
    {"cost", pluss::make_series({5.0, 18.0, 33.0, 60.0})},
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"})},
}).title("metrics");

std::array<std::string_view, 2> pair_features{"sales", "cost"};

auto out = metrics.bucketize(pair_features, {0.0, 25.0, 50.0, 75.0}, {1, 2, 3});
```

### Output

```text
==========================
     | sales  cost  city
--------------------------
0    | 1      1     Accra
1    | 2      1     Tema
2    | 2      2     Kumasi
3    | 2      3     Tamale
--------------------------
[4 cs x 3 feat]
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="bottom.html">
    <span class="label">Previous</span>
    <span class="title">bottom</span>
  </a>
  <a class="next-link" href="case_study.html">
    <span class="label">Next</span>
    <span class="title">case_study</span>
  </a>
</div>
