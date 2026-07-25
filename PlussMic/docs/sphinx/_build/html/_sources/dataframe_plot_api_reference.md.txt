# DataFramePlotAccessor API Reference

This page documents the public 2D plotting accessor returned by `DataFrame::plot()`.

## Using This Reference

- Use the left sidebar to open one method page at a time.
- Each method page contains its own signatures, parameter details, notes, and working previous/next navigation.
- This landing page stays intentionally short so the content panel is not one giant scroll of every method.

```{important} Every method returns a Studio, not a DataFrame
None of these methods print a table or draw anything to the console. Each one builds and
returns a `pluss::pl::Studio` figure object from the `PlussPl` plotting library — nothing is
rendered until you call one of `Studio`'s own methods (`.show()`, `.save_svg(path)`,
`.save_png(path)`, `.save_pdf(path)`, `.to_svg()`, `.to_pdf()`). `Studio` has no
`operator<<`, so `std::cout << figure` does not compile — see each page's examples for the
actual pattern.
```

## Method Pages

The sidebar lists methods alphabetically. You can also use the search box to jump directly to a method name.

```{toctree}
:hidden:
:maxdepth: 1

api/dataframe-plot/act
api/dataframe-plot/andrews_curves
api/dataframe-plot/area
api/dataframe-plot/area_stacked
api/dataframe-plot/auto_plot
api/dataframe-plot/bar
api/dataframe-plot/bar_stacked
api/dataframe-plot/bootstrap
api/dataframe-plot/box
api/dataframe-plot/brief
api/dataframe-plot/corr_heatmap
api/dataframe-plot/density
api/dataframe-plot/heatmap
api/dataframe-plot/hist
api/dataframe-plot/lag
api/dataframe-plot/line
api/dataframe-plot/parallel
api/dataframe-plot/pie
api/dataframe-plot/radviz
api/dataframe-plot/scatter
api/dataframe-plot/scatter_matrix
api/dataframe-plot/step
api/dataframe-plot/violin
```
