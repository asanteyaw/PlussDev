# DataFrame::select_like

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signature

```cpp
DataFrame::select_like(std::string_view needle, bool case_sensitive = true) -> DataFrame
```

## What It Does

Selects only the features whose names contain the requested substring.

## Parameters

**needle**
    Substring token used to match feature names.

**case_sensitive**
    When `true`, substring matching respects letter case. When `false`, feature names are matched case-insensitively.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Use the overload whose parameter combination matches the data you already have and the amount of control you want over the result.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({70.0, 80.0, 90.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({75.0, 85.0, 95.0}))},
    {"final_points", pluss::table::Thread(pluss::make_series({88.0, 91.0, 94.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.select_like("midterm");
```

### Output

```text
midterm_1  midterm_2
70.0       75.0
80.0       85.0
90.0       95.0
```

## Practical Behavior

- `select_like(...)` filters feature names, not row values.
- The output keeps the original row order and case axis unchanged.
- Use this when you want schema-driven feature selection without listing every matching feature manually.



<div class="pluss-inline-prev-next">
  <a class="prev-link" href="select.html">
    <span class="label">Previous</span>
    <span class="title">select</span>
  </a>
  <a class="next-link" href="select_regex.html">
    <span class="label">Next</span>
    <span class="title">select_regex</span>
  </a>
</div>
