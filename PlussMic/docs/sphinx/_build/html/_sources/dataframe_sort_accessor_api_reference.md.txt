# DataFrameSortBuilder API Reference

This page documents the value-ordering workflow entered via `DataFrame::sort()` and continued through `DataFrameSortBuilder`.

## Using This Reference

- Use the left sidebar to open one method page at a time.
- Each method page contains its own signatures, parameter details, notes, verified outputs, and working previous/next navigation.
- This landing page stays intentionally short so the content panel is not one giant scroll of every method.

## Method Pages

The sidebar lists methods alphabetically. You can also use the search box to jump directly to a method name.

`sort()` enters a reusable sort plan, then the builder grows or executes it:

- Plan keys: [`by()`](api/dataframe-sort/by.md), [`then()`](api/dataframe-sort/then.md), [`within()`](api/dataframe-sort/within.md)
- Refine the active key: [`asc()`](api/dataframe-sort/asc.md), [`desc()`](api/dataframe-sort/desc.md), [`nulls_first()`](api/dataframe-sort/nulls_first.md), [`nulls_last()`](api/dataframe-sort/nulls_last.md)
- Materialize: [`order()`](api/dataframe-sort/order.md), [`yield()`](api/dataframe-sort/yield.md), [`head()`](api/dataframe-sort/head.md), [`tail()`](api/dataframe-sort/tail.md)

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
