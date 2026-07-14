# DataFrame::caselbl

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::caselbl() -> const Thread&

DataFrame::caselbl(const Thread& labels) -> DataFrame
```

Return the current case axis exactly as it is stored on the frame.

Use `DataFrame::caselbl()` when you want the row labels themselves as a thread-like object. This is the axis accessor, not a row-selection method.

## Parameters

`None`

## Returns

`const Thread&`
    Borrowed reference to the current case axis.

    The payload family is preserved:
    - note-backed cases stay note-backed,
    - numeric cases stay series-backed,
    - temporal cases stay timeline/time-index backed.

## See Also

- <a href="case_study.html"><code>DataFrame::case_study</code></a>: select rows by position, label, mask, or structured case key.

## Notes

`caselbl()` does not copy or transform the axis. It exposes the current case labels in their native thread form.

If you need row selection, use `case_study(...)`. If you need numeric labels as numeric data, call `caselbl().as_series()` explicitly when the case axis is series-backed.

## Examples

Example 1. Inspect a labeled case axis directly.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"})},
    {"region", pluss::make_note({"south", "south", "middle", "north"})},
    {"units", pluss::make_series({10.0, 7.0, 12.0, 5.0})},
    {"price", pluss::make_series({3.0, 4.0, 2.5, 5.0})},
}).caselbl(pluss::make_note({"A100", "B200", "B200", "D400"}));

auto out = sales.caselbl();
```

### Output

```text
0  A100
1  B200
2  B200
3  D400
dtype: Note
```



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="case_study.html">
    <span class="label">Previous</span>
    <span class="title">case_study</span>
  </a>
  <a class="next-link" href="caselbl_.html">
    <span class="label">Next</span>
    <span class="title">caselbl_</span>
  </a>
</div>
