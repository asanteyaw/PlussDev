# DataFrame::copy

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::copy() -> DataFrame
```

Return a detached copy of the current frame.

Use this when you want to make it explicit that later edits should happen on an independent value rather than the current object.

## Parameters

`None`

## Returns

`DataFrame`
    Independent copy of the current frame.

## Notes

`copy()` preserves the current case axis, feature order, payloads, and values.

## Examples

Example 1. Copy a frame before downstream mutation or experimentation.

```cpp
using namespace pluss::table;

auto scores = DataFrame({
    {"math", pluss::make_series({80.0, 90.0, 70.0})},
    {"science", pluss::make_series({75.0, 88.0, 92.0})},
}, pluss::make_note({"s1", "s2", "s3"}));

auto out = scores.copy();
```

### Output

```text
====================
     | math  science
--------------------
s1   | 80    75     
s2   | 90    88     
s3   | 70    92     
--------------------
[3 cs x 2 feat]
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="conform.html">
    <span class="label">Previous</span>
    <span class="title">conform</span>
  </a>
  <a class="next-link" href="corr.html">
    <span class="label">Next</span>
    <span class="title">corr</span>
  </a>
</div>
