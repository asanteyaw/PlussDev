# DataFrame::le

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::le(const char* other) -> DataFrame

DataFrame::le(const DataFrame& other) -> DataFrame

DataFrame::le(const DataFrame& other, const AlignOptions& options) -> DataFrame

DataFrame::le(const Thread& other, Axis axis = Axis::Feature) -> DataFrame

DataFrame::le(const Thread& other, Axis axis, const AlignOptions& options) -> DataFrame

DataFrame::le(const Thread& other, const AlignOptions& options, Axis axis = Axis::Feature) -> DataFrame

DataFrame::le(double other) -> DataFrame

DataFrame::le(std::string_view other) -> DataFrame
```

## What It Does

This produces a comparison-style result you can inspect directly or reuse as a mask.

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

Example 1. Compare each compatible cell using the `<=` relation against one broadcast scalar value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.le(10);
```

### Output

```text
id  sales
1   true
2   true
3   false
```
Example 2. Compare each compatible cell using the `<=` relation against another aligned frame.

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({2, 2, 4}))},
    {"sales", pluss::table::Thread(pluss::make_series({110.0, 110.0, 100.0}))},
});

auto out = left.le(right);
```

### Output

```text
id  sales
1   true
2   true
3   false
```
Example 3. Compare each compatible cell using the `<=` relation against another aligned frame.

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({2, 2, 4}))},
    {"sales", pluss::table::Thread(pluss::make_series({110.0, 110.0, 100.0}))},
});

auto out = left.le(right);
```

### Output

```text
id  sales
1   true
2   true
3   false
```
Example 4. Compare each compatible cell using the `<=` relation against one broadcast scalar value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.le(10);
```

### Output

```text
id  sales
1   true
2   true
3   false
```
Example 5. Compare each compatible cell using the `<=` relation against one broadcast scalar value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.le(10);
```

### Output

```text
id  sales
1   true
2   true
3   false
```
Example 6. Compare each compatible cell using the `<=` relation against one broadcast scalar value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.le(10);
```

### Output

```text
id  sales
1   true
2   true
3   false
```
Example 7. Compare each compatible cell using the `<=` relation against one broadcast scalar value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.le(10);
```

### Output

```text
id  sales
1   true
2   true
3   false
```
Example 8. Compare each compatible cell using the `<=` relation against one broadcast scalar value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.le(10);
```

### Output

```text
id  sales
1   true
2   true
3   false
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="land.html">
    <span class="label">Previous</span>
    <span class="title">land</span>
  </a>
  <a class="next-link" href="lengthen.html">
    <span class="label">Next</span>
    <span class="title">lengthen</span>
  </a>
</div>
