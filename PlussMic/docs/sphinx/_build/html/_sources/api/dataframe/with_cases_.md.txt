# DataFrame::with_cases_

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::with_cases_(const Thread& cases) -> DataFrame&
```

## What It Does

Replaces the current case axis in place. The row values stay in the same order, but later case-aware operations now use the new labels.

## Parameters

**cases**
    Case-label thread to attach to the current object.

## Returns

`DataFrame&`
    Value produced by this call.

## Examples

Example 1. Mutate the current object in place, then inspect the updated state.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto cases = pluss::table::Thread(pluss::make_note({"cs0", "cs1", "cs2"}));
df.with_cases_(cases);
```

### Output

```text
df.cases():
["cs0", "cs1", "cs2"]

df.head():
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="with_cases.html">
    <span class="label">Previous</span>
    <span class="title">with_cases</span>
  </a>
</div>
