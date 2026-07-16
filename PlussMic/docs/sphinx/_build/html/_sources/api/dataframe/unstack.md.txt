# DataFrame::unstack

[Back to DataFrame API Reference](../../dataframe_api_reference)

## Signatures

```cpp
DataFrame::unstack(const UnstackOptions& options) -> DataFrame

DataFrame::unstack(std::string_view label) -> DataFrame
```

## What It Does

Pivots one structured case label out of a multi-label case axis so its distinct values become new output columns.

## Parameters

**options**
    Unstack behavior bundle.

    Supported values:
    - ``caselbl``: Structured case label whose distinct values are pivoted into output columns.
    - ``drop``: When `true`, the remaining case labels stay as case labels instead of also staying as ordinary columns.
    - ``sort``: When `true`, the pivoted column labels are sorted before the new columns are built.
    - ``separator``: Separator inserted between the source value-feature name and the pivoted case-label value when several value features are present.

**label**
    Label or labels used by this overload.

## Returns

`DataFrame`
    Value produced by this call.

## Examples

Example 1. Pivot one named case label into output columns while controlling drop, sort, and separator behavior.

```cpp
auto grades = pluss::table::DataFrame({
    {"student", pluss::table::Thread(pluss::make_note({"Ada", "Ada", "Kojo", "Kojo"}))},
    {"term", pluss::table::Thread(pluss::make_note({"mid", "final", "mid", "final"}))},
    {"score", pluss::table::Thread(pluss::make_series({88.0, 91.0, 84.0, 89.0}))},
});
auto indexed = grades.label().set({"student", "term"}, true, "|");

pluss::table::DataFrame::UnstackOptions options;
options.label = "term";
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
Example 2. Pivot one named case label into output columns while controlling drop, sort, and separator behavior.

```cpp
auto grades = pluss::table::DataFrame({
    {"student", pluss::table::Thread(pluss::make_note({"Ada", "Ada", "Kojo", "Kojo"}))},
    {"term", pluss::table::Thread(pluss::make_note({"mid", "final", "mid", "final"}))},
    {"score", pluss::table::Thread(pluss::make_series({88.0, 91.0, 84.0, 89.0}))},
});
auto indexed = grades.label().set({"student", "term"}, true, "|");

pluss::table::DataFrame::UnstackOptions options;
options.label = "term";
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
