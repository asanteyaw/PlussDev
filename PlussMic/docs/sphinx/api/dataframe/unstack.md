# DataFrame::unstack

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::unstack(const UnstackOptions& options) -> DataFrame

DataFrame::unstack(std::string_view level) -> DataFrame
```

## What It Does

Pivots one case level out of a multi-level case axis so its distinct values become new output columns.

## Parameters

**options**
    Unstack behavior bundle.

    Supported values:
    - ``level``: Case level whose distinct values are pivoted into output columns.
    - ``drop``: When `true`, the remaining case levels stay as case levels instead of also staying as ordinary columns.
    - ``sort``: When `true`, the pivoted column labels are sorted before the new columns are built.
    - ``separator``: Separator inserted between the source value-feature name and the pivoted level value when several value features are present.

**level**
    Case level whose distinct values should become new output columns in the reshaped frame.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Pivot one named case level into output columns while controlling drop, sort, and separator behavior.

```cpp
auto grades = pluss::table::DataFrame({
    {"student", pluss::table::Thread(pluss::make_note({"Ada", "Ada", "Kojo", "Kojo"}))},
    {"term", pluss::table::Thread(pluss::make_note({"mid", "final", "mid", "final"}))},
    {"score", pluss::table::Thread(pluss::make_series({88.0, 91.0, 84.0, 89.0}))},
});
auto indexed = grades.set_cases({"student", "term"}, true, "|");

pluss::table::DataFrame::UnstackOptions options;
options.level = "term";
options.drop = true;
options.sort = true;
options.separator = "_";
auto out = indexed.unstack(options);
```

### Output

```text
score_mid  score_final
88.0       91.0
84.0       89.0
```
Example 2. Pivot one named case level into output columns using the default unstack options.

```cpp
auto grades = pluss::table::DataFrame({
    {"student", pluss::table::Thread(pluss::make_note({"Ada", "Ada", "Kojo", "Kojo"}))},
    {"term", pluss::table::Thread(pluss::make_note({"mid", "final", "mid", "final"}))},
    {"score", pluss::table::Thread(pluss::make_series({88.0, 91.0, 84.0, 89.0}))},
});
auto indexed = grades.set_cases({"student", "term"}, true, "|");

auto out = indexed.unstack("term");
```

### Output

```text
score_mid  score_final
88.0       91.0
84.0       89.0
```


<div class="pluss-inline-prev-next">
  <a class="prev-link" href="unsqueeze.html">
    <span class="label">Previous</span>
    <span class="title">unsqueeze</span>
  </a>
  <a class="next-link" href="update.html">
    <span class="label">Next</span>
    <span class="title">update</span>
  </a>
</div>
