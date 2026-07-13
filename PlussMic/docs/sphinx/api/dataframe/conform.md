# DataFrame::conform

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::conform(const DataFrame& other) -> DataFrame
```

Reorder features and cases so the current frame conforms to another frame's structural layout.

Use this before arithmetic, updates, or pairwise comparisons when the other frame defines the structure you need to match.

## Parameters

**other**

Reference frame whose structure should be mirrored.

The result follows:
- `other.features()` for feature order, and
- `other.cases()` for case order.

## Returns

`DataFrame`
    New frame whose feature order and case order follow `other`.

## See Also

- <a href="align_cases.html"><code>DataFrame::align_cases</code></a>: case-axis-only alignment.
- <a href="combine.html"><code>DataFrame::combine</code></a>: combine two already-compatible frames.
- <a href="unify.html"><code>DataFrame::unify</code></a>: key-based merge rather than structure conformance.

## Notes

`conform(other)` first matches the feature axis and then aligns the case axis to `other`.

This is a structural preparation method. It does not perform arithmetic or merge values by itself.

## Examples

Example 1. Match another frame's feature order and case order before downstream work.

```cpp
using namespace pluss::table;

auto left = DataFrame({
    {"math", pluss::make_series({80.0, 90.0, 70.0})},
    {"science", pluss::make_series({75.0, 88.0, 92.0})},
}, pluss::make_note({"s1", "s2", "s3"}));

auto right = DataFrame({
    {"science", pluss::make_series({92.0, 75.0})},
    {"math", pluss::make_series({70.0, 80.0})},
}, pluss::make_note({"s3", "s1"}));

auto out = left.conform(right);
```

### Output

```text
====================
     | science  math
--------------------
s3   | 92       70  
s1   | 75       80  
--------------------
[2 cs x 2 feat]
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="compare.html">
    <span class="label">Previous</span>
    <span class="title">compare</span>
  </a>
  <a class="next-link" href="copy.html">
    <span class="label">Next</span>
    <span class="title">copy</span>
  </a>
</div>
