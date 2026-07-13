# DataFrame::truediv

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::truediv(const DataFrame& other) -> DataFrame

DataFrame::truediv(double other) -> DataFrame
```

## What It Does

Applies elementwise true division to compatible numeric features.

## Parameters

**other**
    Scalar divisor or right-hand frame used for elementwise true division.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Divide each compatible numeric cell against another aligned frame.

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({2, 2, 4}))},
    {"sales", pluss::table::Thread(pluss::make_series({110.0, 110.0, 100.0}))},
});

auto out = left.truediv(right);
```

### Output

```text
id    sales
0.5   0.909091
1.0   1.090909
0.75  0.9
```
Example 2. Divide each compatible numeric cell against one broadcast scalar value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.truediv(10);
```

### Output

```text
id  sales  city
0.1  10.0  Accra
0.2  12.0  Tema
0.3   9.0  Kumasi
```

## Practical Behavior

- Scalar `truediv(...)` divides every compatible numeric cell by the supplied scalar.
- Frame-vs-frame `truediv(...)` divides aligned numeric features row by row.
- Non-numeric features pass through unchanged in the scalar form and are excluded from numeric combination work.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="transpose.html">
    <span class="label">Previous</span>
    <span class="title">transpose</span>
  </a>
  <a class="next-link" href="ts.html">
    <span class="label">Next</span>
    <span class="title">ts</span>
  </a>
</div>
