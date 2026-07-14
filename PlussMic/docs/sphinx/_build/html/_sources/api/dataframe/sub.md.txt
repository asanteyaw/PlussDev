# DataFrame::sub

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::sub(const DataFrame& other) -> DataFrame

DataFrame::sub(const DataFrame& other, const AlignOptions& options) -> DataFrame

DataFrame::sub(const Thread& other, Axis axis = Axis::Feature) -> DataFrame

DataFrame::sub(const Thread& other, Axis axis, const AlignOptions& options) -> DataFrame

DataFrame::sub(const Thread& other, const AlignOptions& options, Axis axis = Axis::Feature) -> DataFrame

DataFrame::sub(double other) -> DataFrame
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

Example 1. Subtract each compatible numeric cell against another aligned frame.

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({2, 2, 4}))},
    {"sales", pluss::table::Thread(pluss::make_series({110.0, 110.0, 100.0}))},
});

auto out = left.sub(right);
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 2. Subtract each compatible numeric cell against another aligned frame.

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({2, 2, 4}))},
    {"sales", pluss::table::Thread(pluss::make_series({110.0, 110.0, 100.0}))},
});

auto out = left.sub(right);
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 3. Subtract each compatible numeric cell against one broadcast scalar value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.sub(10);
```

### Output

```text
id  sales  city
-9  90.0   Accra
-8  110.0  Tema
-7  80.0   Kumasi
```
Example 4. Subtract each compatible numeric cell against one broadcast scalar value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.sub(10);
```

### Output

```text
id  sales  city
-9  90.0   Accra
-8  110.0  Tema
-7  80.0   Kumasi
```
Example 5. Subtract each compatible numeric cell against one broadcast scalar value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.sub(10);
```

### Output

```text
id  sales  city
-9  90.0   Accra
-8  110.0  Tema
-7  80.0   Kumasi
```
Example 6. Subtract each compatible numeric cell against one broadcast scalar value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.sub(10);
```

### Output

```text
id  sales  city
-9  90.0   Accra
-8  110.0  Tema
-7  80.0   Kumasi
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="std.html">
    <span class="label">Previous</span>
    <span class="title">std</span>
  </a>
  <a class="next-link" href="sum.html">
    <span class="label">Next</span>
    <span class="title">sum</span>
  </a>
</div>
