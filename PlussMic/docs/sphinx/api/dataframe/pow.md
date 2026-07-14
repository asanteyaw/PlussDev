# DataFrame::pow

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::pow(const DataFrame& other) -> DataFrame

DataFrame::pow(const DataFrame& other, const AlignOptions& options) -> DataFrame

DataFrame::pow(const Thread& other, Axis axis = Axis::Feature) -> DataFrame

DataFrame::pow(const Thread& other, Axis axis, const AlignOptions& options) -> DataFrame

DataFrame::pow(const Thread& other, const AlignOptions& options, Axis axis = Axis::Feature) -> DataFrame

DataFrame::pow(double other) -> DataFrame
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

Example 1. Raise each compatible numeric cell against another aligned frame.

```cpp
auto left = pluss::table::DataFrame({
    {"a", pluss::table::Thread(pluss::make_series({2.0, 3.0, 4.0}))},
    {"b", pluss::table::Thread(pluss::make_series({3.0, 2.0, 5.0}))},
});
auto right = pluss::table::DataFrame({
    {"a", pluss::table::Thread(pluss::make_series({3.0, 2.0, 1.0}))},
    {"b", pluss::table::Thread(pluss::make_series({2.0, 3.0, 2.0}))},
});

auto out = left.pow(right);
```

### Output

```text
a    b
8.0  9.0
9.0  8.0
4.0  25.0
```
Example 2. Raise each compatible numeric cell against another aligned frame.

```cpp
auto left = pluss::table::DataFrame({
    {"a", pluss::table::Thread(pluss::make_series({2.0, 3.0, 4.0}))},
    {"b", pluss::table::Thread(pluss::make_series({3.0, 2.0, 5.0}))},
});
auto right = pluss::table::DataFrame({
    {"a", pluss::table::Thread(pluss::make_series({3.0, 2.0, 1.0}))},
    {"b", pluss::table::Thread(pluss::make_series({2.0, 3.0, 2.0}))},
});

auto out = left.pow(right);
```

### Output

```text
a    b
8.0  9.0
9.0  8.0
4.0  25.0
```
Example 3. Raise each compatible numeric cell against one broadcast scalar value.

```cpp
auto df = pluss::table::DataFrame({
    {"a", pluss::table::Thread(pluss::make_series({1.0, 2.0, 3.0}))},
    {"b", pluss::table::Thread(pluss::make_series({2.0, 4.0, 5.0}))},
});

auto out = df.pow(2);
```

### Output

```text
a    b
1.0  4.0
4.0  16.0
9.0  25.0
```
Example 4. Raise each compatible numeric cell against one broadcast scalar value.

```cpp
auto df = pluss::table::DataFrame({
    {"a", pluss::table::Thread(pluss::make_series({1.0, 2.0, 3.0}))},
    {"b", pluss::table::Thread(pluss::make_series({2.0, 4.0, 5.0}))},
});

auto out = df.pow(2);
```

### Output

```text
a    b
1.0  4.0
4.0  16.0
9.0  25.0
```
Example 5. Raise each compatible numeric cell against one broadcast scalar value.

```cpp
auto df = pluss::table::DataFrame({
    {"a", pluss::table::Thread(pluss::make_series({1.0, 2.0, 3.0}))},
    {"b", pluss::table::Thread(pluss::make_series({2.0, 4.0, 5.0}))},
});

auto out = df.pow(2);
```

### Output

```text
a    b
1.0  4.0
4.0  16.0
9.0  25.0
```
Example 6. Raise each compatible numeric cell against one broadcast scalar value.

```cpp
auto df = pluss::table::DataFrame({
    {"a", pluss::table::Thread(pluss::make_series({1.0, 2.0, 3.0}))},
    {"b", pluss::table::Thread(pluss::make_series({2.0, 4.0, 5.0}))},
});

auto out = df.pow(2);
```

### Output

```text
a    b
1.0  4.0
4.0  16.0
9.0  25.0
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="plot3d.html">
    <span class="label">Previous</span>
    <span class="title">plot3d</span>
  </a>
  <a class="next-link" href="prep_sql.html">
    <span class="label">Next</span>
    <span class="title">prep_sql</span>
  </a>
</div>
