# DataFrame::has_levels

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::has_levels() -> bool
```

## What It Does

Reports whether the current case axis carries named levels rather than only a flat label thread.

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

auto out = df.has_levels();
```

### Output

```text
The call completes and returns the contract described under Output.
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="has_feature.html">
    <span class="label">Previous</span>
    <span class="title">has_feature</span>
  </a>
  <a class="next-link" href="head.html">
    <span class="label">Next</span>
    <span class="title">head</span>
  </a>
</div>
