# DataFrame::where

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::where(const pluss::Series& bool_mask) -> DataFrame

DataFrame::where(const Thread& bool_mask) -> DataFrame
```

## What It Does

Keeps only the rows where the supplied boolean mask is true.

## Parameters

**bool_mask**
    Boolean row mask. Rows where the mask is true are kept; rows where it is false are dropped.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Filter rows using an explicit boolean Series mask.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = pluss::make_series({true, false, true}, pluss::ScalarType::Bool);
auto out = df.where(mask);
```

### Output

```text
id  sales  city
1   100.0  Accra
3    90.0  Kumasi
```
Example 2. Filter rows using a boolean mask built from an existing frame expression.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = df.features("sales") >= 95.0;
auto out = df.where(mask);
```

### Output

```text
id  sales  city
1   100.0  Accra
3    90.0  Kumasi
```

## Practical Behavior

- `where(mask)` keeps rows where the boolean mask is true and drops the rest.
- The mask length must match the frame case count exactly.
- Use `put(...)` when you want masked replacement inside one feature instead of masked row filtering.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="weigh.html">
    <span class="label">Previous</span>
    <span class="title">weigh</span>
  </a>
  <a class="next-link" href="window.html">
    <span class="label">Next</span>
    <span class="title">window</span>
  </a>
</div>
