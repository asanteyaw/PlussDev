# DataFrame::transpose

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::transpose(bool render = false) -> DataFrameView
```

## What It Does

Swaps rows and features so former feature names become row labels.

## Parameters

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

auto out = df.transpose(false);
```

### Output

```text
feature  row0   row1   row2
id       1      2      3
sales    100.0  120.0  90.0
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="transform.html">
    <span class="label">Previous</span>
    <span class="title">transform</span>
  </a>
  <a class="next-link" href="truediv.html">
    <span class="label">Next</span>
    <span class="title">truediv</span>
  </a>
</div>
