# DataFrameGroupBuilder API Reference

This page documents the public grouped-workflow surface entered via
`DataFrame::group()` — the `DataFrameGroupBuilder` returned by `group()` itself,
and the `DataFrameGrouped` handle returned by its `.by(...)`.

## Using This Reference

- Use the left sidebar to open one method page at a time.
- Each method page contains its own signatures, parameter details, notes, verified outputs, and working previous/next navigation.
- This landing page stays intentionally short so the content panel is not one giant scroll of every method.

## Method Pages

The sidebar lists methods alphabetically. You can also use the search box to jump directly to a method name.

`by()` (on `DataFrameGroupBuilder`) fixes the grouping keys and returns
`DataFrameGrouped`. Every other method here (`agg`, `transform`, `filter`, and the
grouped convenience terminals `count`, `max`, `mean`, `median`, `min`, `mode`,
`ndistinct`, `std`, `sum`, `var`) is a `DataFrameGrouped` method that actually
executes the grouped work.

```{toctree}
:hidden:
:maxdepth: 1

api/dataframe-group/agg
api/dataframe-group/by
api/dataframe-group/count
api/dataframe-group/filter
api/dataframe-group/max
api/dataframe-group/mean
api/dataframe-group/median
api/dataframe-group/min
api/dataframe-group/mode
api/dataframe-group/ndistinct
api/dataframe-group/std
api/dataframe-group/sum
api/dataframe-group/transform
api/dataframe-group/var
```
