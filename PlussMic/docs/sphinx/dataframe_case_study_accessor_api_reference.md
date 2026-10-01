<!-- pluss-hand-authored -->

# DataFrameCaseStudyAccessor API Reference

`DataFrame::case_study()` returns a lightweight accessor for selecting cases without
mixing row selectors into the root `DataFrame` overload set. The accessor keeps a
cheap handle to the source frame; no rows are copied until a terminal method runs.

Use position-oriented methods (`at`, `slice`, `take`) when row location is the
contract, `labels` (and its single-key form) when the case axis is the contract,
`span` for a label-based interval, `where` for a boolean decision per row, and `pick`
for named hierarchical case levels. Every terminal returns a `DataFrame`, so feature
projection remains an ordinary chained `features(...)` call.

```cpp
auto west = sales.case_study().labels("West")
                 .features({"revenue", "cost"});
```

## Method Pages

```{toctree}
:hidden:
:maxdepth: 1

api/dataframe-case-study/at
api/dataframe-case-study/labels
api/dataframe-case-study/pick
api/dataframe-case-study/slice
api/dataframe-case-study/span
api/dataframe-case-study/take
api/dataframe-case-study/where
```
