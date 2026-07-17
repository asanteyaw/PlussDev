# DataFrame::any

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::any(Axis axis, pluss::RangeView<std::string_view> features = {}, bool skipnull = true) -> Thread

DataFrame::any(pluss::RangeView<std::string_view> features = {}, bool skipnull = true) -> Thread
```

Return whether at least one value is truthy along the requested axis.

`DataFrame::any(...)` is the boolean reduction you use when you want an existence test rather than a count or a sum. The returned `Thread` changes shape with the axis you choose: one result per feature when you reduce feature-wise, or one result per case when you reduce row-wise.

## Parameters

**features**

Optional feature subset to reduce.

- Leave this empty to let PlussMic inspect every compatible feature in the frame.
- Pass a feature subset when you want the truth test to ignore identifier, label, or unrelated measure columns.
- The output order follows the exact left-to-right order of the selected features.

**skipnull**

Missing-value policy for the reduction.

- `true`: ignore missing values while evaluating truthiness.
- `false`: missing values can influence whether the reduced result is considered valid.

Most users will keep the default `true`.

**axis**

Reduction axis.

- `pluss::Axis::Feature`: reduce down each feature and return one boolean per feature.
- `pluss::Axis::Case`: reduce across the selected features inside each row and return one boolean per case.

Choose `pluss::Axis::Case` when you are building row-level policies such as “did this student pass any exam component?”.

## Returns

`Thread`
    Boolean reduction result.

    - With the default feature-wise reduction, the output is one boolean per participating feature.
    - With `pluss::Axis::Case`, the output is one boolean per row across the selected features.

## See Also

- <a href="all.html"><code>DataFrame::all</code></a>: require every participating value to be truthy instead of only one.
- <a href="../thread/any.html"><code>Thread::any</code></a>: reduce one thread to one boolean truth result.

## Notes

Only compatible boolean-like or numeric features participate in the reduction. Text and temporal features are not coerced into truth values for this method.

When you omit `axis`, PlussMic uses the feature-wise reduction. That is the common shape when you are checking whether each column has at least one truthy observation.

`skipnull=true` ignores missing values during the truth reduction. Use `skipnull=false` when null propagation matters to the policy you are implementing.

## Examples

Example 1. Reduce feature-wise using the default axis.

```cpp
using namespace pluss::table;

auto flags = DataFrame({
    {"quiz_ok", pluss::make_series(std::vector<bool>{true, true, false})},
    {"exam_ok", pluss::make_series(std::vector<bool>{true, false, false})},
    {"bonus_ok", pluss::make_series(std::vector<bool>{true, true, true})},
}).title("flags");

auto out = flags.any();
```

### Output

```text
quiz_ok  true
 exam_ok  true
bonus_ok  true
dtype: Series
```

Example 2. Reduce row-wise to ask whether each case has at least one truthy flag.

```cpp
using namespace pluss::table;

auto flags = DataFrame({
    {"quiz_ok", pluss::make_series(std::vector<bool>{true, true, false})},
    {"exam_ok", pluss::make_series(std::vector<bool>{true, false, false})},
    {"bonus_ok", pluss::make_series(std::vector<bool>{true, true, true})},
}).title("flags");

auto out = flags.any(pluss::Axis::Case);
```

### Output

```text
0  true
1  true
2  true
dtype: Series
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="all.html">
    <span class="label">Previous</span>
    <span class="title">all</span>
  </a>
  <a class="next-link" href="assign.html">
    <span class="label">Next</span>
    <span class="title">assign</span>
  </a>
</div>
