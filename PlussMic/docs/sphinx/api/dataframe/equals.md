# DataFrame::equals

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::equals(const DataFrame& other) -> bool
```

Check whether two frames are exactly equal as whole DataFrame objects.

`equals(...)` is the whole-object identity check. Use it when you need one `bool` answer rather than a per-cell mask.

## Parameters

**other**

Frame to compare against the current frame.

The comparison is structural and value-wise at the same time. PlussMic does not attempt to realign `other` before deciding; it checks the two frames exactly as they are supplied.

## Returns

`bool`
    `true` only when both frames have the same case count, feature count, feature names, case-label type, case-label values, payload kinds, validity pattern, and visible cell values.

## See Also

- <a href="eq.html"><code>DataFrame::eq</code></a>: elementwise equality mask instead of one boolean answer.
- <a href="copy.html"><code>DataFrame::copy</code></a>: useful for producing a structurally identical frame before testing exact equality.

## Notes

`equals(...)` is stricter than visual similarity. Two frames that look similar in print can still return `false` if their feature order, case-label dtype, missingness pattern, or payload kinds differ.

Missingness is part of the equality contract. A valid cell and a null cell do not compare equal.

## Examples

Example 1. Two frames with identical structure and values compare equal.

```cpp
using namespace pluss::table;

auto official = DataFrame({
    {"quiz", pluss::make_series({84.0, 90.0, 84.0})},
    {"final", pluss::make_series({84.0, 88.0, 72.0})},
}, pluss::make_note({"s1", "s2", "s3"}));

auto out = official.equals(official.copy());
```

### Output

```text
true
```

Example 2. One changed value is enough for `equals(...)` to return false.

```cpp
using namespace pluss::table;

auto official = DataFrame({
    {"quiz", pluss::make_series({84.0, 90.0, 84.0})},
    {"final", pluss::make_series({84.0, 88.0, 72.0})},
}, pluss::make_note({"s1", "s2", "s3"}));

auto changed = DataFrame({
    {"quiz", pluss::make_series({84.0, 90.0, 83.0})},
    {"final", pluss::make_series({84.0, 88.0, 72.0})},
}, pluss::make_note({"s1", "s2", "s3"}));

auto out = official.equals(changed);
```

### Output

```text
false
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="eq.html">
    <span class="label">Previous</span>
    <span class="title">eq</span>
  </a>
  <a class="next-link" href="extend_cases.html">
    <span class="label">Next</span>
    <span class="title">extend_cases</span>
  </a>
</div>
