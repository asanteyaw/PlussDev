# DataFrame::observe

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::observe(std::int64_t row, bool render = false) -> DataFrameView

DataFrame::observe(std::int64_t start, std::int64_t stop, bool render = false) -> DataFrameView
```

## What It Does

Returns a `DataFrameView` over a positional row window so you can preview rows from the middle of a frame without materializing a new working copy.

## Parameters

**row**
    Zero-based row position to preview as a one-row `DataFrameView`.

**render**
    When `true`, the preview may auto-render in temporary terminal contexts. Pass `false` when you want to call `.show()` yourself.

**start**
    Inclusive zero-based starting row position of the preview window.

**stop**
    Exclusive zero-based ending row position of the preview window.

## Returns

`DataFrameView`
    Value produced by this call.

## Examples

Example 1. Preview one row without changing the underlying frame.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto preview = df.observe(1, false);
preview.show();
```

### Output

```text
id  sales  city
2   120.0  Tema
```
Example 2. Preview a contiguous row window without changing the underlying frame.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto preview = df.observe(0, 2, false);
preview.show();
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
```

## Practical Behavior

- `observe(...)` returns a `DataFrameView`, so it is the preview-oriented row window helper alongside `head(...)` and `tail(...)`.
- It uses positional row coordinates, not case-label lookup.
- Use `observe(start, stop).show()` when you want a middle slice without materializing a bespoke helper or mutating the source frame.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="notnull.html">
    <span class="label">Previous</span>
    <span class="title">notnull</span>
  </a>
  <a class="next-link" href="percentiles.html">
    <span class="label">Next</span>
    <span class="title">percentiles</span>
  </a>
</div>
