# DataFrame::floordiv

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::floordiv(const DataFrame& other) -> DataFrame

DataFrame::floordiv(const DataFrame& other, const AlignOptions& options) -> DataFrame

DataFrame::floordiv(const Thread& other, Axis axis = Axis::Feature) -> DataFrame

DataFrame::floordiv(const Thread& other, Axis axis, const AlignOptions& options) -> DataFrame

DataFrame::floordiv(const Thread& other, const AlignOptions& options, Axis axis = Axis::Feature) -> DataFrame

DataFrame::floordiv(double other) -> DataFrame
```

## What It Does

This applies arithmetic across the compatible numeric payloads in the current object.

## Parameters

**other**
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

**axis**
    Axis selector. On `DataFrame`, `0` usually means rows/cases and `1` means features/columns.

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

auto out = left.floordiv(right);
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 2. Divide each compatible numeric cell against another aligned frame.

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({2, 2, 4}))},
    {"sales", pluss::table::Thread(pluss::make_series({110.0, 110.0, 100.0}))},
});

auto out = left.floordiv(right);
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 3. Divide each compatible numeric cell against one broadcast scalar value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.floordiv(10);
```

### Output

```text
id  sales  city
0.0  10.0  Accra
0.0  12.0  Tema
0.0   9.0  Kumasi
```
Example 4. Divide each compatible numeric cell against one broadcast scalar value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.floordiv(10);
```

### Output

```text
id  sales  city
0.0  10.0  Accra
0.0  12.0  Tema
0.0   9.0  Kumasi
```
Example 5. Divide each compatible numeric cell against one broadcast scalar value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.floordiv(10);
```

### Output

```text
id  sales  city
0.0  10.0  Accra
0.0  12.0  Tema
0.0   9.0  Kumasi
```
Example 6. Divide each compatible numeric cell against one broadcast scalar value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.floordiv(10);
```

### Output

```text
id  sales  city
0.0  10.0  Accra
0.0  12.0  Tema
0.0   9.0  Kumasi
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="filter.html">
    <span class="label">Previous</span>
    <span class="title">filter</span>
  </a>
  <a class="next-link" href="flow.html">
    <span class="label">Next</span>
    <span class="title">flow</span>
  </a>
</div>
