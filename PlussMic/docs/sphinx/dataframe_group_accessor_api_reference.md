# DataFrameGroupBuilder API Reference

This page documents the public grouped-workflow surface entered via `DataFrame::group()` -- the `DataFrameGroupBuilder` returned by `group()` itself, and the `DataFrameGrouped` handle returned by its `.by(...)`.

## Using This Reference

- Use the left sidebar to open one method page at a time.
- Each method page contains its own signatures, parameter details, notes, verified outputs, and working previous/next navigation.
- This landing page stays intentionally short so the content panel is not one giant scroll of every method.

## Method Pages

The sidebar lists methods alphabetically. You can also use the search box to jump directly to a method name.

`by()` fixes the grouping keys, then `DataFrameGrouped` executes the grouped work:

- Plan grouping keys: [`by()`](api/dataframe-group/by.md)
- Explicit grouped reductions: [`agg()`](api/dataframe-group/agg.md), [`transform()`](api/dataframe-group/transform.md), [`filter()`](api/dataframe-group/filter.md), [`yield()`](api/dataframe-group/yield.md)
- Uniform grouped reducers: [`count()`](api/dataframe-group/count.md), [`ndistinct()`](api/dataframe-group/ndistinct.md), [`sum()`](api/dataframe-group/sum.md), [`mean()`](api/dataframe-group/mean.md), [`median()`](api/dataframe-group/median.md), [`min()`](api/dataframe-group/min.md), [`max()`](api/dataframe-group/max.md), [`std()`](api/dataframe-group/std.md), [`var()`](api/dataframe-group/var.md), [`mode()`](api/dataframe-group/mode.md), [`mode_ties()`](api/dataframe-group/mode_ties.md)

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
api/dataframe-group/mode_ties
api/dataframe-group/ndistinct
api/dataframe-group/std
api/dataframe-group/sum
api/dataframe-group/transform
api/dataframe-group/var
api/dataframe-group/yield
```
