# DataFrame::head

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::head(std::int64_t n = 5, bool render = false) -> DataFrameView
```

## What It Does

Returns the first `n` rows of the frame.

## Parameters

**n**
    Number of rows, entries, or depth items to keep in the preview.

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

auto out = df.head(2, false);
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="has_levels.html">
    <span class="label">Previous</span>
    <span class="title">has_levels</span>
  </a>
  <a class="next-link" href="height.html">
    <span class="label">Next</span>
    <span class="title">height</span>
  </a>
</div>
