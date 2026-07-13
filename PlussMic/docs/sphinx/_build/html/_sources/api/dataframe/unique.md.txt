# DataFrame::unique

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::unique() -> bool
```

## What It Does

Reports whether the object satisfies the uniqueness constraint expected by its model, such as distinct labels or keys.

## Parameters

`None`

## Returns

`bool`
    Boolean status or membership result.

## Examples

Example 1. Call the simplest overload.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.unique();
```

### Output

```text
true
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="unify.html">
    <span class="label">Previous</span>
    <span class="title">unify</span>
  </a>
  <a class="next-link" href="unique_cases.html">
    <span class="label">Next</span>
    <span class="title">unique_cases</span>
  </a>
</div>
