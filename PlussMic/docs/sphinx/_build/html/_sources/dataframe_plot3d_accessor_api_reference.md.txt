# DataFramePlot3DAccessor API Reference

This page documents the public 3D plotting accessor returned by `DataFrame::plot3d()`.

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

api/dataframe-plot3d/hist
api/dataframe-plot3d/line
api/dataframe-plot3d/scatter
api/dataframe-plot3d/surface
api/dataframe-plot3d/wireframe
```
