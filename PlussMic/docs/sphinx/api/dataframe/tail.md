# DataFrame::tail

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::tail(std::int64_t n = 5, bool render = false) -> DataFrameView
```

## What It Does

Returns the last `n` rows of the frame.

## Parameters

**n**
    Number of trailing rows to keep from the bottom of the frame.

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

auto out = df.tail(2, false);
```

### Output

```text
id  sales  city
2   120.0  Tema
3    90.0  Kumasi
```

## Practical Behavior

- `tail(n)` preserves the original feature order and returns the last `n` rows.
- If `n` is larger than the frame height, the whole frame is returned.
- The returned object is a `DataFrameView`, so it behaves like a lightweight preview slice rather than a deep copy.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="tabelize.html">
    <span class="label">Previous</span>
    <span class="title">tabelize</span>
  </a>
  <a class="next-link" href="take.html">
    <span class="label">Next</span>
    <span class="title">take</span>
  </a>
</div>
