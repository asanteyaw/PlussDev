# DataFrame::match

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::match(const DataFrame& other, const FrameMatchOptions& options = {}) -> FrameMatch
```

## What It Does

Returns a pair of frames whose cases and features have been matched according to explicit Pluss-native matching rules.

## Parameters

**other**
    Frame whose case labels and feature names should be matched with this frame.

**options**
    Matching policy for cases, features, and repeated case labels. Defaults to a stable union on both axes and occurrence-order pairing for repeats.

## Returns

`FrameMatch`
    Pair containing matched `left` and `right` frames with identical case and feature axes.

## Examples

Example 1. Pass an explicit options object to control the operation.

```cpp
auto left = pluss::table::DataFrame(
    {{"x", pluss::make_series({1.0, 2.0})},
     {"y", pluss::make_series({10.0, 20.0})}},
    pluss::table::Thread(pluss::make_note({"A", "C"})));
auto right = pluss::table::DataFrame(
    {{"y", pluss::make_series({100.0, 200.0})},
     {"z", pluss::make_series({7.0, 8.0})}},
    pluss::table::Thread(pluss::make_note({"B", "A"})));
auto matched = left.match(right);
```

### Output

```text
matched.left.caselbl():
["A", "C", "B"]

matched.left.features():
["x", "y", "z"]
```

## Matching Rules

- `match(other)` matches both case labels and feature names in one call.
- The default `Unite` rule preserves the left order and appends labels found only on the right.
- `Common`, `KeepLeft`, and `KeepRight` select the intersection or one side explicitly.
- Repeated cases pair by occurrence order by default; choose `RepeatedCaseRule::Reject` when repeated labels should be an error.
- Frame-to-frame arithmetic and comparisons use the same default matching behavior automatically.
- Only case-key matching is materialized on the host; feature gathers and arithmetic remain on the source Series platform.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="mask.html">
    <span class="label">Previous</span>
    <span class="title">mask</span>
  </a>
  <a class="next-link" href="matmul.html">
    <span class="label">Next</span>
    <span class="title">matmul</span>
  </a>
</div>
