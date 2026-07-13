# DataFrame::extend_cases

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::extend_cases(const DataFrame& other, FrameMatchRule features = FrameMatchRule::Unite, bool fresh_cases = false, bool reject_repeated_cases = false) -> DataFrame
```

## What It Does

Appends another frame beneath this frame after matching the feature schemas and null-filling one-sided features.

## Parameters

**other**
    Frame whose cases should be appended beneath this frame.

**features**
    Feature matching rule for the non-extension axis. `Unite` keeps the stable union; `Common` keeps shared features.

**fresh_cases**
    When `true`, replace the appended case labels with contiguous integer cases starting at zero.

**reject_repeated_cases**
    When `true`, fail if preserving the two case axes would produce repeated case labels.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto left = pluss::table::DataFrame(
    {{"x", pluss::make_series({1.0, 2.0})},
     {"y", pluss::make_series({3.0, 4.0})}},
    pluss::table::Thread(pluss::make_note({"A", "C"})));
auto right = pluss::table::DataFrame(
    {{"y", pluss::make_series({5.0, 6.0})},
     {"z", pluss::make_series({7.0, 8.0})}},
    pluss::table::Thread(pluss::make_note({"B", "A"})));
auto out = left.extend_cases(right);
```

### Output

```text
case  x     y     z
A     1.0   3.0   null
C     2.0   4.0   null
B     null  5.0   7.0
A     null  6.0   8.0
```

## Extension Rules

- `extend_cases(other)` appends cases vertically and matches features by name.
- The default `Unite` feature rule preserves left feature order, appends right-only features, and null-fills one-sided payloads.
- Matching numeric features must have the same dtype and platform; other matching features must have the same Thread kind.
- Use `fresh_cases=true` for contiguous integer cases or `reject_repeated_cases=true` to require a unique preserved case axis.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="equals.html">
    <span class="label">Previous</span>
    <span class="title">equals</span>
  </a>
  <a class="next-link" href="factor.html">
    <span class="label">Next</span>
    <span class="title">factor</span>
  </a>
</div>
