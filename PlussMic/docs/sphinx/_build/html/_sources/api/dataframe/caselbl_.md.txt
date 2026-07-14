# DataFrame::caselbl_

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::caselbl_(const Thread& labels) -> DataFrame&
```

Replace the DataFrame case-label axis and return the same frame object.

Use `DataFrame::caselbl_(...)` when the relabeling should happen in place instead of producing a detached copy.

## Parameters

**labels**

New case-label axis to attach to the current frame.

- The number of labels must match the current row count exactly.
- The payload kind can be note, numeric, timeline, or time-index based.

## Returns

`DataFrame&`
    Reference to the current frame after the case-label axis has been replaced.

## Notes

Only the case labels change. Feature values, feature order, and row order remain unchanged.

The supplied label thread must have the same case count as the frame.

## Examples

Example 1. Replace the case labels in place.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi"})},
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
}).title("sales");

auto labels = Thread(pluss::make_note({"A100", "B200", "C300"}));

sales.caselbl_(labels);
```

### Output

```text
sales.caselbl():
0  A100
1  B200
2  C300
dtype: Note
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="caselbl.html">
    <span class="label">Previous</span>
    <span class="title">caselbl</span>
  </a>
  <a class="next-link" href="cast.html">
    <span class="label">Next</span>
    <span class="title">cast</span>
  </a>
</div>
