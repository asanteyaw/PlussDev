# DataFrame::mod

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::mod(const DataFrame& other) -> DataFrame

DataFrame::mod(double other) -> DataFrame
```

## What It Does

This applies arithmetic across the compatible numeric payloads in the current object.

## Parameters

**other**
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Compute the modulus of each compatible numeric cell against another aligned frame.

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({2, 2, 4}))},
    {"sales", pluss::table::Thread(pluss::make_series({110.0, 110.0, 100.0}))},
});

auto out = left.mod(right);
```

### Output

```text
id  sales
1   100.0
0    10.0
3    90.0
```
Example 2. Compute the modulus of each compatible numeric cell against one broadcast scalar value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.mod(10);
```

### Output

```text
id  sales  city
1   0.0    Accra
2   0.0    Tema
3   0.0    Kumasi
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="min.html">
    <span class="label">Previous</span>
    <span class="title">min</span>
  </a>
  <a class="next-link" href="mode.html">
    <span class="label">Next</span>
    <span class="title">mode</span>
  </a>
</div>
