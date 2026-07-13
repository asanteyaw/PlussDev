# DataFrame::view

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::view(std::int64_t start, std::int64_t stop, bool render = false) -> DataFrameView
```

## What It Does

Returns a lightweight row view of the requested slice.

## Parameters

**start**
    Slice start position. Negative values count backward from the end of the frame.

**stop**
    Slice stop position. Negative values count backward from the end, and `-1` includes the last row.

**render**
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

## Returns

`DataFrameView`
    Value produced by this call.

## Examples

Example 1. Choose whether the returned object should auto-render in display contexts.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.view(0, 2, false);
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
```

## Slice Semantics

- `view(start, stop)` behaves like a positional row slice `[start, stop)`.
- Negative `start` or `stop` values count backward from the end of the frame.
- Unlike `slice(...)`, `view(...)` returns a `DataFrameView` wrapper intended for lightweight inspection.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="var.html">
    <span class="label">Previous</span>
    <span class="title">var</span>
  </a>
  <a class="next-link" href="weigh.html">
    <span class="label">Next</span>
    <span class="title">weigh</span>
  </a>
</div>
