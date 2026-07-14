# DataFrame::all

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::all(Axis axis, pluss::RangeView<std::string_view> features = {}, bool skipna = true) -> Thread

DataFrame::all(pluss::RangeView<std::string_view> features = {}, bool skipna = true) -> Thread
```

Return whether every participating value is truthy along the requested axis.

`DataFrame::all(...)` is the boolean reduction you use when one falsey value should invalidate the whole reduction result. It is useful for row-level eligibility checks, completeness gates, and feature-level assertions such as “is every observed value in this flag column true?”.

## Parameters

**features**

Optional feature subset to reduce.

- Leave this empty to reduce every compatible feature in the frame.
- Pass a subset when only specific policy columns should participate in the truth test.
- Output order follows the exact left-to-right order of the selected features.

**skipna**

Missing-value policy for the reduction.

- `true`: ignore missing values while reducing truthiness.
- `false`: missing values can affect whether the reduced result is valid.

Most workflows keep the default `true`.

**axis**

Reduction axis.

- `pluss::Axis::Feature`: reduce down each participating feature and return one boolean per feature.
- `pluss::Axis::Case`: reduce across the selected features inside each row and return one boolean per case.

## Returns

`Thread`
    Boolean reduction result.

    - With the default feature-wise reduction, the output contains one boolean per participating feature.
    - With `pluss::Axis::Case`, the output contains one boolean per row across the selected features.

## See Also

- <a href="any.html"><code>DataFrame::any</code></a>: require only one truthy value instead of all of them.
- <a href="../thread/all.html"><code>Thread::all</code></a>: reduce one boolean thread to one truth result.

## Notes

Only compatible boolean-like or numeric features participate in the reduction. Text and temporal features are not coerced into truth values for this method.

When you omit `axis`, PlussMic reduces feature-wise and returns one boolean per selected feature.

Use `pluss::Axis::Case` when you are building row-level policies such as “did this student satisfy every required condition?”.

## Examples

Example 1. Reduce feature-wise using the default axis.

```cpp
using namespace pluss::table;

auto flags = DataFrame({
    {"quiz_ok", pluss::make_series(std::vector<bool>{true, true, false})},
    {"exam_ok", pluss::make_series(std::vector<bool>{true, false, false})},
    {"bonus_ok", pluss::make_series(std::vector<bool>{true, true, true})},
}).title("flags");

auto out = flags.all();
```

### Output

```text
quiz_ok  false
 exam_ok  false
bonus_ok  true
dtype: Series
```

Example 2. Reduce row-wise to require every selected flag in each case to be true.

```cpp
using namespace pluss::table;

auto flags = DataFrame({
    {"quiz_ok", pluss::make_series(std::vector<bool>{true, true, false})},
    {"exam_ok", pluss::make_series(std::vector<bool>{true, false, false})},
    {"bonus_ok", pluss::make_series(std::vector<bool>{true, true, true})},
}).title("flags");

auto out = flags.all(pluss::Axis::Case);
```

### Output

```text
0  true
1  false
2  false
dtype: Series
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="align_caselbl.html">
    <span class="label">Previous</span>
    <span class="title">align_caselbl</span>
  </a>
  <a class="next-link" href="any.html">
    <span class="label">Next</span>
    <span class="title">any</span>
  </a>
</div>
