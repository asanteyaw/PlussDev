# DataFrame::extend_cases

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::extend_cases(const DataFrame& other, FrameMatchRule features = FrameMatchRule::Unite, bool fresh_cases = false, bool reject_repeated_cases = false) -> DataFrame
```

Append the cases of another frame under the current frame while reconciling the feature set.

`extend_cases(...)` is the vertical extension surface for table-shaped data. Use it when you want more rows from another frame and you need a policy for how the two feature sets should be combined.

## Parameters

**other**

Frame whose cases should be appended after the current frame's cases.

**features**

Feature reconciliation rule used before the append.

- `FrameMatchRule::Unite`: keep all features seen on either side.
- `FrameMatchRule::Common`: keep only shared features.
- `FrameMatchRule::KeepLeft`: keep the left/current frame's feature set.
- `FrameMatchRule::KeepRight`: keep the right/other frame's feature set.

**fresh_cases**

Case-axis policy after the append.

- `false`: preserve and concatenate the existing case labels.
- `true`: replace the final case axis with a fresh integer-like sequence.

**reject_repeated_cases**

Whether repeated case labels should be rejected after concatenation when the final case axis is being preserved.

## Returns

`DataFrame`
    New frame whose cases are the current frame's cases followed by the other frame's cases.

    Feature handling depends on `features`:

    - `FrameMatchRule::Unite`: keep the union of feature names.
    - `FrameMatchRule::Common`: keep only shared feature names.
    - other feature-match rules follow the same structural interpretation used across frame matching.

## See Also

- <a href="combine.html"><code>DataFrame::combine</code></a>: horizontal or policy-driven composition instead of row appending.
- <a href="align.html"><code>DataFrame::align</code></a>: conform the current frame before or after extending cases.

## Notes

When one side does not have a participating feature, PlussMic creates a null column segment of the correct length so the appended result remains rectangular.

`fresh_cases = true` discards the original case labels and emits a fresh integer-like case axis for the appended result.

This method appends rows. It does not try to match rows by label.

## Examples

Example 1. Append two frames while keeping the union of features.

```cpp
using namespace pluss::table;

auto left = DataFrame({
    {"x", pluss::make_series({1.0, 2.0})},
    {"y", pluss::make_series({10.0, 20.0})},
}, pluss::make_note({"A", "B"}));

auto right = DataFrame({
    {"y", pluss::make_series({30.0, 40.0})},
    {"z", pluss::make_series({300.0, 400.0})},
}, pluss::make_note({"C", "D"}));

auto out = left.extend_cases(right);
```

### Output

```text
=======================
     | x     y     z
-----------------------
A    | 1     10    NA
B    | 2     20    NA
C    | NA    30    300
D    | NA    40    400
-----------------------
[4 cs x 3 feat]
```

Example 2. Keep only shared features while appending cases.

```cpp
using namespace pluss::table;

auto left = DataFrame({
    {"x", pluss::make_series({1.0, 2.0})},
    {"y", pluss::make_series({10.0, 20.0})},
}, pluss::make_note({"A", "B"}));

auto right = DataFrame({
    {"y", pluss::make_series({30.0, 40.0})},
    {"z", pluss::make_series({300.0, 400.0})},
}, pluss::make_note({"C", "D"}));

auto out = left.extend_cases(right, FrameMatchRule::Common);
```

### Output

```text
===========
     | y
-----------
A    | 10
B    | 20
C    | 30
D    | 40
-----------
[4 cs x 1 feat]
```

Example 3. Request fresh case labels after the append.

```cpp
using namespace pluss::table;

auto left = DataFrame({
    {"x", pluss::make_series({1.0, 2.0})},
    {"y", pluss::make_series({10.0, 20.0})},
}, pluss::make_note({"A", "B"}));

auto right = DataFrame({
    {"y", pluss::make_series({30.0, 40.0})},
    {"z", pluss::make_series({300.0, 400.0})},
}, pluss::make_note({"C", "D"}));

auto out = left.extend_cases(right, FrameMatchRule::Unite, true);
```

### Output

```text
=======================
     | x     y     z
-----------------------
0    | 1     10    NA
1    | 2     20    NA
2    | NA    30    300
3    | NA    40    400
-----------------------
[4 cs x 3 feat]
```



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
