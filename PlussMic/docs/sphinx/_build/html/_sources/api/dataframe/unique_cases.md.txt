# DataFrame::unique_cases

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::unique_cases(pluss::RangeView<std::string_view> features = {}) -> DataFrame
```

## What It Does

Keeps only the first occurrence of each case label and drops later duplicates.

## Parameters

**features**
    Optional feature subset used to decide which rows count as duplicates. Leave empty to compare the full row payload.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"campus", pluss::table::Thread(pluss::make_note({"North", "North", "West"}))},
    {"student", pluss::table::Thread(pluss::make_note({"Ada", "Kojo", "Esi"}))},
    {"score", pluss::table::Thread(pluss::make_series({88.0, 91.0, 84.0}))},
});

auto out = df.unique_cases({"campus"});
```

### Output

```text
campus  student  score
North   Ada      88.0
West    Esi      84.0
```

## Practical Behavior

- `unique_cases(features)` keeps only the first row for each distinct key combination defined by `features`.
- With no feature list, the full row payload is used as the duplicate key.
- This is the row-filtering companion to `repeated_cases(...)`.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="unique.html">
    <span class="label">Previous</span>
    <span class="title">unique</span>
  </a>
  <a class="next-link" href="unsqueeze.html">
    <span class="label">Next</span>
    <span class="title">unsqueeze</span>
  </a>
</div>
