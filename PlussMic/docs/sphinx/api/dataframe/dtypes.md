# DataFrame::dtypes

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::dtypes() -> DTypes
```

Return the public dtype summary for every feature in the frame.

This is the schema-inspection method you use before casting, selecting numeric features, or debugging mixed-type input.

It answers “what kind of payload does each feature currently hold?” at the table layer.

## Parameters

`None`

## Returns

`DTypes`
    Feature-to-dtype summary object describing the current payload family for each feature.

## See Also

- <a href="cast.html"><code>DataFrame::cast</code></a>, <a href="astype.html"><code>DataFrame::astype</code></a>: change the dtypes after inspecting them.

## Notes

`dtypes()` reports the public table-level dtype categories such as `Series` and `Note`. It is intentionally higher-level than the scalar dtype returned by `Thread::dtype()`.

Use `dtypes()` before `cast(...)`, `astype(...)`, numeric reduction, or feature-selection logic when your frame was loaded from messy or mixed-source input.

## Examples

Example 1. Inspect the current feature families before downstream casting.

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
student_id: Series
passed: Series
raw_name: Note
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
