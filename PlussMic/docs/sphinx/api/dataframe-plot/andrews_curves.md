<!-- pluss-hand-authored -->

# DataFramePlotAccessor::andrews_curves

[Back to DataFramePlotAccessor API Reference](../../dataframe_plot_api_reference)

## Signature

```cpp
DataFramePlotAccessor::andrews_curves(
    pluss::msharp::RangeView::Of<std::string_view> features = {},
    std::optional<std::string_view> class_feature = std::nullopt,
    const AndrewsCurvesPlotOptions& options = {}) const -> pluss::pl::Studio
```

## What It Does

Draws Andrews curves: each row's values across the selected numeric features are combined
into a single periodic function (a truncated Fourier-series-style combination), and each
row's curve is plotted over a shared domain. Rows with similar values across the selected
features tend to produce visually similar curves, making this another way (alongside
[`parallel()`](parallel.md) and [`radviz()`](radviz.md)) to eyeball multivariate structure
across more than two features at once.

Shares its feature-selection rule (empty `features` infers every numeric feature),
`class_feature` behavior (accepts any feature kind, cast to text), and NaN-vs-drop handling
with [`parallel()`](parallel.md) — see that page for the full explanation; this page only
covers what's specific to `andrews_curves()`.

## Parameters

**features** (`RangeView<std::string_view>`, default `{}`)
    Numeric features combined into each row's curve. Empty infers every numeric feature —
    see [`box()`](box.md)'s important callout for the exact rule.

**class_feature** (`std::optional<std::string_view>`, default `std::nullopt`)
    Optional coloring/grouping feature, same behavior as [`parallel()`](parallel.md)'s.

**options** (`AndrewsCurvesPlotOptions`)
    ```cpp
    struct AndrewsCurvesPlotOptions {
      std::vector<std::string> labels{};
      bool normalize{true};
      std::int64_t points{220};
      std::int64_t max_curves{600};
      double alpha{0.28};
      float trace_width{0.9F};
      bool show_grid{true};
      bool show_legend{true};
    };
    ```
    `points` is how many points each row's curve is evaluated at over its domain (finer
    than a straight-line plot, since the curve is smooth). `max_curves` caps how many rows'
    curves are drawn — analogous to [`parallel()`](parallel.md)'s `max_points`. The rest
    (`labels`, `normalize`, `alpha`, `trace_width`, `show_grid`, `show_legend`) mirror
    [`ParallelPlotOptions`](parallel.md)'s fields of the same name.

## Returns

`pluss::pl::Studio`
    A figure with one pane containing one curve per row (or capped at `max_curves`), colored
    by class when `class_feature` is given.

## Examples

Example 1. No explicit features, no class: every numeric feature in the frame.

```cpp
using namespace pluss::table;

auto flowers = DataFrame({
    {"sepal_length", Thread(pluss::make_series({5.1, 6.7, 5.8, 6.3}))},
    {"sepal_width",  Thread(pluss::make_series({3.5, 3.0, 2.7, 3.3}))},
    {"petal_length", Thread(pluss::make_series({1.4, 5.2, 5.1, 6.0}))},
    {"species",      Thread(pluss::make_note({"setosa", "virginica", "virginica", "virginica"}))},
});

auto chart = flowers.plot().andrews_curves();
chart.save_svg("flowers_andrews.svg");
```

Example 2. An explicit feature subset, colored by class.

```cpp
auto chart = flowers.plot().andrews_curves(
    {"sepal_length", "sepal_width", "petal_length"}, "species");
chart.show();
```

Example 3. An explicit `AndrewsCurvesPlotOptions`, with a finer curve resolution and a cap
on how many rows are drawn.

```cpp
AndrewsCurvesPlotOptions options;
options.points = 400;
options.max_curves = 100;
options.alpha = 0.5;

auto chart = flowers.plot().andrews_curves(
    {"sepal_length", "sepal_width", "petal_length"}, "species", options);
chart.save_png("flowers_andrews.png");
```

Example 4. `class_feature` accepts a numeric feature too, exactly like
[`parallel()`](parallel.md)'s Example 4.

```cpp
auto numeric_class = DataFrame({
    {"x1",   Thread(pluss::make_series({1.0, 2.0, 3.0, 4.0}))},
    {"x2",   Thread(pluss::make_series({4.0, 3.0, 2.0, 1.0}))},
    {"code", Thread(pluss::make_series({1.0, 1.0, 2.0, 2.0}))},
});

auto chart = numeric_class.plot().andrews_curves({"x1", "x2"}, "code");
```

## Notes

- Shares its feature-selection, class-coloring, and NaN-handling rules with
  [`parallel()`](parallel.md) — see that page for the full explanation.
- `points` controls curve smoothness; `max_curves` caps how many rows are drawn.
- Returns a `pluss::pl::Studio` — see [`line()`](line.md)'s important callout for how to
  view or save the result.

## See Also

- [`DataFramePlotAccessor::parallel`](parallel.md): the canonical explanation of the
  feature-selection and class-coloring rules shared by this family of methods.
- [`DataFramePlotAccessor::radviz`](radviz.md): another multivariate view in the same
  family.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="act.html">
    <span class="label">Previous</span>
    <span class="title">act</span>
  </a>
  <a class="next-link" href="area.html">
    <span class="label">Next</span>
    <span class="title">area</span>
  </a>
</div>
