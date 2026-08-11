# DataFrameSortBuilder API Reference

This page documents the value-ordering workflow entered via
`DataFrame::sort()` and continued through `DataFrameSortBuilder`.

## Using This Reference

- Use the left sidebar to open one method page at a time.
- Each method page contains its own signatures, parameter details, notes, verified outputs, and working previous/next navigation.
- This landing page stays intentionally short so the content panel is not one giant scroll of every method.

## Method Pages

`sort()` enters the planner. The builder then splits into three roles:

- plan keys with `by(...)`, `then(...)`, and `within(...)`
- refine the active key with `asc()`, `desc()`, `nulls_first()`, and `nulls_last()`
- execute or materialize with `order()`, `yield()`, `head(n)`, and `tail(n)`

Use `arrange(...)` when you want the direct execute-now sorter. Use the sort
builder when ordering itself is a plan you want to grow, reuse, or partially
materialize.

```{toctree}
:hidden:
:maxdepth: 1

api/dataframe-sort/asc
api/dataframe-sort/by
api/dataframe-sort/desc
api/dataframe-sort/head
api/dataframe-sort/nulls_first
api/dataframe-sort/nulls_last
api/dataframe-sort/order
api/dataframe-sort/tail
api/dataframe-sort/then
api/dataframe-sort/within
api/dataframe-sort/yield
```
