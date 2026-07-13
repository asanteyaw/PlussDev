# DataFrame::lt

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::lt(const DataFrame& other, bool strict = true) -> DataFrame

DataFrame::lt(double other, bool strict = true) -> DataFrame
```

## What It Does

This produces a comparison-style result you can inspect directly or reuse as a mask.

## Parameters

**other**
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

**strict**
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Compare each compatible cell using the `<` relation against another aligned frame.

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({2, 2, 4}))},
    {"sales", pluss::table::Thread(pluss::make_series({110.0, 110.0, 100.0}))},
});

auto out = left.lt(right);
```

### Output

```text
id  sales
1   true
2   false
3   true
```
Example 2. Compare each compatible cell using the `<` relation against one broadcast scalar value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.lt(110);
```

### Output

```text
id  sales
1   true
2   false
3   true
```

## Comparison Rules

- `lt(...)` returns a boolean-style frame with the same case axis and selected feature layout as the input.
- Scalar/native comparisons broadcast the right-hand value across every compatible numeric cell.
- Frame-vs-frame comparisons are row-wise and require compatible shape and feature alignment.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="load_txt.html">
    <span class="label">Previous</span>
    <span class="title">load_txt</span>
  </a>
  <a class="next-link" href="map.html">
    <span class="label">Next</span>
    <span class="title">map</span>
  </a>
</div>
