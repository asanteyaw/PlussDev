# DataFrame::shift

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::shift(pluss::RangeView<std::string_view> features, int periods, std::string_view freq) -> DataFrame

DataFrame::shift(pluss::RangeView<std::string_view> features, std::int64_t periods = 1) -> DataFrame

DataFrame::shift(std::int64_t periods = 1) -> DataFrame
```

## What It Does

This is part of the time-aware surface and expects timeline or time-index semantics.

## Parameters

**features**
    Feature name(s) that select the target DataFrame columns for the operation.

**periods**
    Number of rows to shift selected feature values. Positive values move values downward and introduce missing entries at the top; negative values move upward.

**freq**
    Bin or calendar frequency such as `"1h"`, `"D"`, `"W"`, or `"M"`.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Shift only selected columns by a fixed number of rows.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.shift({"sales"}, 2);
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 2. Shift only selected columns by a fixed number of rows.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.shift({"sales"}, 2);
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 3. Shift every compatible column by a fixed number of rows.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.shift(2);
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="shape.html">
    <span class="label">Previous</span>
    <span class="title">shape</span>
  </a>
  <a class="next-link" href="show.html">
    <span class="label">Next</span>
    <span class="title">show</span>
  </a>
</div>
