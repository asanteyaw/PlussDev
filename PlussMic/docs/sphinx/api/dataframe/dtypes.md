# DataFrame::dtypes

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::dtypes() -> DTypes
```

Return the concrete dtype summary for every feature in the frame.

This is the schema-inspection method you use when you need both the payload family and the payload's concrete storage/readout type.

Use `ptypes()` when you only need the broad payload family (`Series`, `Note`, `Timeline`, or `TimeIndex`).

## Parameters

`None`

## Returns

`DTypes`
    Feature-to-dtype summary object describing the current concrete dtype for each feature.

## See Also

- <a href="ptypes.html"><code>DataFrame::ptypes</code></a>: inspect broad payload families.
- <a href="cast.html"><code>DataFrame::cast</code></a>, <a href="astype.html"><code>DataFrame::astype</code></a>: change the dtypes after inspecting them.

## Notes

`dtypes()` reports concrete values such as `Series<float64>`, `Note<utf8>`, `Timeline<datetime64[ns, UTC]>`, and `TimeIndex<datetime64[ns, UTC]>`.

`TimeIndex` is intentionally separate from `Timeline`: a `Timeline` stores temporal values, while a `TimeIndex` is the temporal axis/index payload used for time-label alignment and resampling.

## Examples

Example 1. Inspect concrete dtypes before downstream casting.

```cpp
using namespace pluss::table;

auto raw = DataFrame({
    {"student_id", pluss::make_series({101.0, 102.0, 103.0})},
    {"passed", pluss::make_series({1.0, 0.0, 1.0})},
    {"raw_name", pluss::make_note({"Ada", "Bola", "Chen"})},
});

auto out = raw.dtypes();
```

### Output

```text
student_id: Series<float64>
passed: Series<float64>
raw_name: Note<utf8>
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="dropnull.html">
    <span class="label">Previous</span>
    <span class="title">dropnull</span>
  </a>
  <a class="next-link" href="duplicated.html">
    <span class="label">Next</span>
    <span class="title">duplicated</span>
  </a>
</div>
