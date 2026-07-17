# DataFrame::ptypes

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::ptypes() -> DTypes
```

Return the payload-family summary for every feature in the frame.

`ptypes()` answers “which Pluss payload does each feature hold?” without exposing scalar storage detail.

## Parameters

`None`

## Returns

`DTypes`
    Feature-to-payload summary object mapping each feature name to `Series`, `Note`, `Timeline`, or `TimeIndex`.

## See Also

- <a href="dtypes.html"><code>DataFrame::dtypes</code></a>: inspect concrete feature dtypes.
- <a href="feataxis.html"><code>DataFrame::feataxis</code></a>: select features by payload family.

## Notes

Use `ptypes()` for high-level routing decisions: numeric work usually starts from `Series`, text work from `Note`, datetime work from `Timeline`, and time-axis work from `TimeIndex`.

Use `cast(...)` when you need to make a `Timeline` column into a `TimeIndex` column explicitly.

## Examples

Example 1. Inspect payload families.

```cpp
using namespace pluss::table;

auto raw = DataFrame({
    {"student_id", pluss::make_series({101.0, 102.0, 103.0})},
    {"raw_name", pluss::make_note({"Ada", "Bola", "Chen"})},
});

auto out = raw.ptypes();
```

### Output

```text
student_id: Series
raw_name: Note
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="prod.html">
    <span class="label">Previous</span>
    <span class="title">prod</span>
  </a>
  <a class="next-link" href="put.html">
    <span class="label">Next</span>
    <span class="title">put</span>
  </a>
</div>
