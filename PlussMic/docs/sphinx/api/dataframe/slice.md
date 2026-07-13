# DataFrame::slice

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::slice(std::int64_t start, std::int64_t stop) -> DataFrame
```

## What It Does

Returns a contiguous row slice `[start, stop)` from the frame.

## Parameters

**start**
    Inclusive starting position of the slice.

**stop**
    Exclusive ending position of the slice.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Inspect one contiguous positional slice from the current object.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.slice(0, 2);
```

### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="skew.html">
    <span class="label">Previous</span>
    <span class="title">skew</span>
  </a>
  <a class="next-link" href="sort_cases.html">
    <span class="label">Next</span>
    <span class="title">sort_cases</span>
  </a>
</div>
