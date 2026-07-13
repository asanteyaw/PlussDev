# DataFramePlotAccessor API Reference

This page documents the public plotting accessor returned by `DataFrame::plot()` and `Atlas::plot()`.

## How To Use This Page

- Methods are grouped alphabetically so you can jump quickly from the left rail.
- Each method page section starts with the public signatures, then explains what the method does, how each parameter changes behavior, and what the call returns.
- Overloaded methods include several examples so you can see the supported call shapes instead of reverse-engineering them from signatures alone.
- Each example is followed immediately by verified output captured from the current compiled examples so the result shape is visible before you move to the next overload.

```{include} snippets/dataframe_plot_options_reference.md
```

## A

### `act`

#### Signature

```cpp
DataFramePlotAccessor::act(std::string_view feature, const ActPlotOptions& options = {}) -> pluss::pl::Studio
```

#### What It Does

Builds an autocorrelation-style lag plot for one numeric feature.

#### Parameters

**feature**
    Numeric feature whose autocorrelation-style lag structure should be plotted.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Examples

Example 1. Work with one named feature.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

pluss::table::ActPlotOptions options;
options.max_lag = 24;
auto fig = plot.act("sales", options);
```

##### Output

```text
Figure(act)
```

#### Practical Behavior

- `act(feature, options)` requires one numeric feature.
- Use it when you want autocorrelation-style time-lag structure for one series.

<div class="pluss-inline-prev-next">
  <a class="next-link" href="#andrews_curves">
    <span class="label">Next</span>
    <span class="title">andrews_curves</span>
  </a>
</div>

### `andrews_curves`

#### Signature

```cpp
DataFramePlotAccessor::andrews_curves(pluss::RangeView<std::string_view> features = {}, std::optional<std::string_view> class_feature = std::nullopt, const AndrewsCurvesPlotOptions& options = {}) -> pluss::pl::Studio
```

#### What It Does

Builds Andrews curves from several numeric features, optionally colored by a class feature.

#### Parameters

**features**
    Numeric features used to build each observation curve.

**class_feature**
    Optional grouping or class label feature used to color, partition, or annotate multivariate plots.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Examples

Example 1. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"x1", pluss::make_series({1.0, 2.0, 3.0})},
    {"x2", pluss::make_series({4.0, 3.0, 2.0})},
    {"x3", pluss::make_series({2.0, 5.0, 4.0})},
    {"group", pluss::make_note({"A", "B", "A"})},
});
auto plot = df.plot();

auto fig = plot.andrews_curves({"x1", "x2", "x3"}, "group");
```

##### Output

```text
Figure(andrews_curves)
```

#### Practical Behavior

- `andrews_curves(...)` works on numeric features and can optionally color/group by a class feature.
- If you do not supply labels in the options, the feature names are used.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#act">
    <span class="label">Previous</span>
    <span class="title">act</span>
  </a>
  <a class="next-link" href="#area">
    <span class="label">Next</span>
    <span class="title">area</span>
  </a>
</div>

### `area`

#### Signatures

```cpp
DataFramePlotAccessor::area(pluss::RangeView<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) -> pluss::pl::Studio

DataFramePlotAccessor::area(pluss::RangeView<std::string_view> y_features, std::optional<std::string_view> x_feature, const AreaPlotOptions& options) -> pluss::pl::Studio

DataFramePlotAccessor::area(std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) -> pluss::pl::Studio

DataFramePlotAccessor::area(std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const AreaPlotOptions& options) -> pluss::pl::Studio

DataFramePlotAccessor::area(std::string_view y_feature, std::optional<std::string_view> x_feature = std::nullopt) -> pluss::pl::Studio

DataFramePlotAccessor::area(std::string_view y_feature, std::optional<std::string_view> x_feature, const AreaPlotOptions& options) -> pluss::pl::Studio
```

#### What It Does

Builds a filled area plot from one or more numeric features.

#### Parameters

**y_features**
    Numeric features to draw as separate filled areas over the same axis positions.

**x_feature**
    Feature used on the horizontal axis of a plot. Leave it empty to let the plot use the case axis or the accessor default when that overload allows it.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

**y_feature**
    One numeric feature to draw as a filled area.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Examples

Example 1. Plot one or more y-features as an area chart.

```cpp
auto df = pluss::table::DataFrame({
    {"month", pluss::make_note({"Jan", "Feb", "Mar"})},
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

auto fig = plot.area({"sales", "cost"}, "month");
```

##### Output

```text
Figure(area)
```
Example 2. Plot one or more y-features as an area chart with an explicit area-options object.

```cpp
auto df = pluss::table::DataFrame({
    {"month", pluss::make_note({"Jan", "Feb", "Mar"})},
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

auto fig = plot.area({"sales", "cost"}, "month");
```

##### Output

```text
Figure(area)
```
Example 3. Plot one or more y-features as an area chart.

```cpp
auto df = pluss::table::DataFrame({
    {"month", pluss::make_note({"Jan", "Feb", "Mar"})},
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

auto fig = plot.area({"sales", "cost"}, "month");
```

##### Output

```text
Figure(area)
```
Example 4. Plot one or more y-features as an area chart with an explicit area-options object.

```cpp
auto df = pluss::table::DataFrame({
    {"month", pluss::make_note({"Jan", "Feb", "Mar"})},
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

auto fig = plot.area({"sales", "cost"}, "month");
```

##### Output

```text
Figure(area)
```
Example 5. Plot one or more y-features as an area chart.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.area("sales", "id");
```

##### Output

```text
Figure(area)
```
Example 6. Plot one or more y-features as an area chart with an explicit area-options object.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

pluss::table::AreaPlotOptions options;
options.alpha = 0.6;
auto fig = plot.area("sales", "id", options);
```

##### Output

```text
Figure(area)
```

#### Practical Behavior

- One y-feature creates one filled area trace.
- Many y-features create one area trace per selected feature over the same x-axis.
- If `x_feature` is omitted, the case axis is used.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#andrews_curves">
    <span class="label">Previous</span>
    <span class="title">andrews_curves</span>
  </a>
  <a class="next-link" href="#area_stacked">
    <span class="label">Next</span>
    <span class="title">area_stacked</span>
  </a>
</div>

### `area_stacked`

#### Signatures

```cpp
DataFramePlotAccessor::area_stacked(pluss::RangeView<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) -> pluss::pl::Studio

DataFramePlotAccessor::area_stacked(pluss::RangeView<std::string_view> y_features, std::optional<std::string_view> x_feature, const AreaStackedPlotOptions& options) -> pluss::pl::Studio

DataFramePlotAccessor::area_stacked(std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) -> pluss::pl::Studio

DataFramePlotAccessor::area_stacked(std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const AreaStackedPlotOptions& options) -> pluss::pl::Studio
```

#### What It Does

Builds a stacked area plot where several numeric features accumulate at shared x positions.

#### Parameters

**y_features**
    Numeric features whose values should accumulate vertically at shared x positions.

**x_feature**
    Feature used on the horizontal axis of a plot. Leave it empty to let the plot use the case axis or the accessor default when that overload allows it.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Examples

Example 1. Plot several y-features as a stacked area chart.

```cpp
auto df = pluss::table::DataFrame({
    {"month", pluss::make_note({"Jan", "Feb", "Mar"})},
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

auto fig = plot.area_stacked({"sales", "cost"}, "month");
```

##### Output

```text
Figure(area_stacked)
```
Example 2. Plot several y-features as a stacked area chart.

```cpp
auto df = pluss::table::DataFrame({
    {"month", pluss::make_note({"Jan", "Feb", "Mar"})},
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

pluss::table::AreaStackedPlotOptions options;
options.alpha = 0.6;
auto fig = plot.area_stacked({"sales", "cost"}, "month", options);
```

##### Output

```text
Figure(area_stacked)
```
Example 3. Plot several y-features as a stacked area chart.

```cpp
auto df = pluss::table::DataFrame({
    {"month", pluss::make_note({"Jan", "Feb", "Mar"})},
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

auto fig = plot.area_stacked({"sales", "cost"}, "month");
```

##### Output

```text
Figure(area_stacked)
```
Example 4. Plot several y-features as a stacked area chart.

```cpp
auto df = pluss::table::DataFrame({
    {"month", pluss::make_note({"Jan", "Feb", "Mar"})},
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

pluss::table::AreaStackedPlotOptions options;
options.alpha = 0.6;
auto fig = plot.area_stacked({"sales", "cost"}, "month", options);
```

##### Output

```text
Figure(area_stacked)
```

#### Practical Behavior

- `area_stacked(...)` requires at least one y-feature.
- Each later y-feature is stacked on top of the running baseline from the earlier ones.
- This is useful for contribution-over-total views across the same x positions.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#area">
    <span class="label">Previous</span>
    <span class="title">area</span>
  </a>
  <a class="next-link" href="#auto_plot">
    <span class="label">Next</span>
    <span class="title">auto_plot</span>
  </a>
</div>

### `auto_plot`

#### Signatures

```cpp
DataFramePlotAccessor::auto_plot(pluss::RangeView<std::string_view> features = {}) -> pluss::pl::Studio

DataFramePlotAccessor::auto_plot(pluss::RangeView<std::string_view> features, const AutoPlotOptions& options) -> pluss::pl::Studio
```

#### What It Does

Chooses a plot family automatically from the selected feature set and frame shape.

#### Parameters

**features**
    Feature subset to consider while the accessor chooses the plot family automatically.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Examples

Example 1. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"student", pluss::make_note({"Laurel", "Kojo", "Ama"})},
    {"midterm", pluss::make_series({71.0, 82.0, 93.0})},
    {"final", pluss::make_series({88.0, 79.0, 95.0})},
});
auto plot = df.plot();

auto fig = plot.auto_plot({"midterm", "final"});
```

##### Output

```text
Figure(auto_selected)
```
Example 2. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"student", pluss::make_note({"Laurel", "Kojo", "Ama"})},
    {"midterm", pluss::make_series({71.0, 82.0, 93.0})},
    {"final", pluss::make_series({88.0, 79.0, 95.0})},
});
auto plot = df.plot();

auto fig = plot.auto_plot({"midterm", "final"});
```

##### Output

```text
Figure(auto_selected)
```

#### Selection Rules

- `auto_plot(...)` chooses a plot family based on feature count, row count, case labels, and sign patterns.
- Examples include bar for one labeled feature, stacked bar/area for a few nonnegative features, scatter matrix for small numeric sets, and correlation heatmap for larger ones.
- Use it for quick inspection when you do not want to choose the plot form manually.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#area_stacked">
    <span class="label">Previous</span>
    <span class="title">area_stacked</span>
  </a>
  <a class="next-link" href="#bar">
    <span class="label">Next</span>
    <span class="title">bar</span>
  </a>
</div>

## B

### `bar`

#### Signatures

```cpp
DataFramePlotAccessor::bar(pluss::RangeView<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) -> pluss::pl::Studio

DataFramePlotAccessor::bar(pluss::RangeView<std::string_view> y_features, std::optional<std::string_view> x_feature, const BarPlotOptions& options) -> pluss::pl::Studio

DataFramePlotAccessor::bar(std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) -> pluss::pl::Studio

DataFramePlotAccessor::bar(std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const BarPlotOptions& options) -> pluss::pl::Studio

DataFramePlotAccessor::bar(std::string_view y_feature, std::optional<std::string_view> x_feature = std::nullopt) -> pluss::pl::Studio

DataFramePlotAccessor::bar(std::string_view y_feature, std::optional<std::string_view> x_feature, const BarPlotOptions& options) -> pluss::pl::Studio
```

#### What It Does

Builds a bar chart from one feature, optionally using another feature as the x-axis.

#### Parameters

**y_features**
    Several features whose values become grouped or layered bar series, depending on the method variant.

**x_feature**
    Feature used on the horizontal axis of a plot. Leave it empty to let the plot use the case axis or the accessor default when that overload allows it.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

**y_feature**
    Single feature whose values become one bar series.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Examples

Example 1. Plot several y-features from a reusable string view as a shared bar chart.

```cpp
auto df = pluss::table::DataFrame({
    {"student", pluss::make_note({"Laurel", "Kojo", "Ama"})},
    {"midterm", pluss::make_series({71.0, 82.0, 93.0})},
    {"final", pluss::make_series({88.0, 79.0, 95.0})},
});
auto plot = df.plot();

auto fig = plot.bar({"midterm", "final"}, "student");
```

##### Output

```text
Figure(bar)
```
Example 2. Plot several y-features from a reusable string view as a shared bar chart with explicit plot options.

```cpp
auto df = pluss::table::DataFrame({
    {"student", pluss::make_note({"Laurel", "Kojo", "Ama"})},
    {"midterm", pluss::make_series({71.0, 82.0, 93.0})},
    {"final", pluss::make_series({88.0, 79.0, 95.0})},
});
auto plot = df.plot();

auto fig = plot.bar({"midterm", "final"}, "student");
```

##### Output

```text
Figure(bar)
```
Example 3. Plot several y-features listed inline as a shared bar chart.

```cpp
auto df = pluss::table::DataFrame({
    {"student", pluss::make_note({"Laurel", "Kojo", "Ama"})},
    {"midterm", pluss::make_series({71.0, 82.0, 93.0})},
    {"final", pluss::make_series({88.0, 79.0, 95.0})},
});
auto plot = df.plot();

auto fig = plot.bar({"midterm", "final"}, "student");
```

##### Output

```text
Figure(bar)
```
Example 4. Plot several y-features listed inline as a shared bar chart with explicit plot options.

```cpp
auto df = pluss::table::DataFrame({
    {"student", pluss::make_note({"Laurel", "Kojo", "Ama"})},
    {"midterm", pluss::make_series({71.0, 82.0, 93.0})},
    {"final", pluss::make_series({88.0, 79.0, 95.0})},
});
auto plot = df.plot();

auto fig = plot.bar({"midterm", "final"}, "student");
```

##### Output

```text
Figure(bar)
```
Example 5. Plot one y-feature as a bar chart.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.bar("sales", "city");
```

##### Output

```text
Figure(bar)
```
Example 6. Plot one y-feature as a bar chart with an explicit plot-options object.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

pluss::table::BarPlotOptions options;
options.horizontal = true;
auto fig = plot.bar("sales", "city", options);
```

##### Output

```text
Figure(bar)
```

#### Practical Behavior

- One `y_feature` creates one bar series.
- Many `y_features` create grouped bars over the same x-axis positions.
- If `x_feature` is omitted, the frame case axis is used as the bar axis.
- `BarPlotOptions::horizontal=true` swaps the plot orientation.
- `BarPlotOptions::ci_enabled=true` aggregates values by axis position and adds confidence-interval whiskers.

#### Common Patterns

```cpp
auto one_series = df.plot().bar("sales", "city");
auto grouped = df.plot().bar({"midterm", "final"}, "student");

pluss::table::BarPlotOptions options;
options.horizontal = true;
auto horizontal = df.plot().bar("sales", "city", options);
```

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#auto_plot">
    <span class="label">Previous</span>
    <span class="title">auto_plot</span>
  </a>
  <a class="next-link" href="#bar_stacked">
    <span class="label">Next</span>
    <span class="title">bar_stacked</span>
  </a>
</div>

### `bar_stacked`

#### Signatures

```cpp
DataFramePlotAccessor::bar_stacked(pluss::RangeView<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) -> pluss::pl::Studio

DataFramePlotAccessor::bar_stacked(pluss::RangeView<std::string_view> y_features, std::optional<std::string_view> x_feature, const BarStackedPlotOptions& options) -> pluss::pl::Studio

DataFramePlotAccessor::bar_stacked(std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) -> pluss::pl::Studio

DataFramePlotAccessor::bar_stacked(std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const BarStackedPlotOptions& options) -> pluss::pl::Studio
```

#### What It Does

Builds stacked bars so multiple y-features accumulate on the same axis positions.

#### Parameters

**y_features**
    Several features whose values stack on top of each other at each x position.

**x_feature**
    Feature used on the horizontal axis of a plot. Leave it empty to let the plot use the case axis or the accessor default when that overload allows it.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Examples

Example 1. Plot several y-features as a stacked bar chart.

```cpp
auto df = pluss::table::DataFrame({
    {"student", pluss::make_note({"Laurel", "Kojo", "Ama"})},
    {"midterm", pluss::make_series({71.0, 82.0, 93.0})},
    {"final", pluss::make_series({88.0, 79.0, 95.0})},
});
auto plot = df.plot();

auto fig = plot.bar_stacked({"midterm", "final"}, "student");
```

##### Output

```text
Figure(bar_stacked)
```
Example 2. Plot several y-features as a stacked bar chart.

```cpp
auto df = pluss::table::DataFrame({
    {"student", pluss::make_note({"Laurel", "Kojo", "Ama"})},
    {"midterm", pluss::make_series({71.0, 82.0, 93.0})},
    {"final", pluss::make_series({88.0, 79.0, 95.0})},
});
auto plot = df.plot();

pluss::table::BarStackedPlotOptions options;
options.alpha = 0.85;
auto fig = plot.bar_stacked({"midterm", "final"}, "student", options);
```

##### Output

```text
Figure(bar_stacked)
```
Example 3. Plot several y-features as a stacked bar chart.

```cpp
auto df = pluss::table::DataFrame({
    {"student", pluss::make_note({"Laurel", "Kojo", "Ama"})},
    {"midterm", pluss::make_series({71.0, 82.0, 93.0})},
    {"final", pluss::make_series({88.0, 79.0, 95.0})},
});
auto plot = df.plot();

auto fig = plot.bar_stacked({"midterm", "final"}, "student");
```

##### Output

```text
Figure(bar_stacked)
```
Example 4. Plot several y-features as a stacked bar chart.

```cpp
auto df = pluss::table::DataFrame({
    {"student", pluss::make_note({"Laurel", "Kojo", "Ama"})},
    {"midterm", pluss::make_series({71.0, 82.0, 93.0})},
    {"final", pluss::make_series({88.0, 79.0, 95.0})},
});
auto plot = df.plot();

pluss::table::BarStackedPlotOptions options;
options.alpha = 0.85;
auto fig = plot.bar_stacked({"midterm", "final"}, "student", options);
```

##### Output

```text
Figure(bar_stacked)
```

#### Practical Behavior

- `bar_stacked(...)` requires at least one y-feature.
- Each later y-feature is stacked on top of the earlier ones at the same x-axis positions.
- This is useful for composition views where the total height and each component both matter.
- If `x_feature` is omitted, the case axis is used.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#bar">
    <span class="label">Previous</span>
    <span class="title">bar</span>
  </a>
  <a class="next-link" href="#bootstrap">
    <span class="label">Next</span>
    <span class="title">bootstrap</span>
  </a>
</div>

### `bootstrap`

#### Signature

```cpp
DataFramePlotAccessor::bootstrap(std::string_view feature, const BootstrapPlotOptions& options = {}) -> pluss::pl::Studio
```

#### What It Does

Builds a bootstrap summary plot for one numeric feature using the supplied resampling options.

#### Parameters

**feature**
    Numeric feature to resample repeatedly for the bootstrap figure.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Examples

Example 1. Work with one named feature.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

pluss::table::BootstrapPlotOptions options;
options.samples = 1000;
auto fig = plot.bootstrap("sales", options);
```

##### Output

```text
Figure(bootstrap)
```

#### Practical Behavior

- `bootstrap(feature, options)` requires one numeric feature.
- The plot is driven by resampling options such as `samples`, `sample_size`, `seed`, and the chosen statistic.
- This is the method to use when you want sampling-based uncertainty or distribution shape around a summary statistic.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#bar_stacked">
    <span class="label">Previous</span>
    <span class="title">bar_stacked</span>
  </a>
  <a class="next-link" href="#box">
    <span class="label">Next</span>
    <span class="title">box</span>
  </a>
</div>

### `box`

#### Signatures

```cpp
DataFramePlotAccessor::box(pluss::RangeView<std::string_view> features = {}) -> pluss::pl::Studio

DataFramePlotAccessor::box(pluss::RangeView<std::string_view> features, const BoxPlotOptions& options) -> pluss::pl::Studio

DataFramePlotAccessor::box(std::initializer_list<std::string_view> features) -> pluss::pl::Studio

DataFramePlotAccessor::box(std::initializer_list<std::string_view> features, const BoxPlotOptions& options) -> pluss::pl::Studio
```

#### What It Does

Builds a box plot for one or more numeric features.

#### Parameters

**features**
    Numeric features to summarize as box plots. Leave empty to let the accessor infer the numeric feature set.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Examples

Example 1. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

auto fig = plot.box({"sales", "cost"});
```

##### Output

```text
Figure(box)
```
Example 2. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

pluss::table::BoxPlotOptions options;
options.line_width = 1.6;
auto fig = plot.box({"sales", "cost"}, options);
```

##### Output

```text
Figure(box)
```
Example 3. Pass several feature names inline.

```cpp
auto df = pluss::table::DataFrame({
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

auto fig = plot.box({"sales", "cost"});
```

##### Output

```text
Figure(box)
```
Example 4. Pass several feature names inline.

```cpp
auto df = pluss::table::DataFrame({
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

pluss::table::BoxPlotOptions options;
options.line_width = 1.6;
auto fig = plot.box({"sales", "cost"}, options);
```

##### Output

```text
Figure(box)
```

#### Practical Behavior

- `box(...)` accepts numeric features only.
- With no explicit feature list, the accessor resolves the available numeric features for you.
- Use this when you want a distribution summary instead of a per-row plot.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#bootstrap">
    <span class="label">Previous</span>
    <span class="title">bootstrap</span>
  </a>
  <a class="next-link" href="#brief">
    <span class="label">Next</span>
    <span class="title">brief</span>
  </a>
</div>

### `brief`

#### Signatures

```cpp
DataFramePlotAccessor::brief(pluss::pl::Studio& figure, pluss::RangeView<std::string_view> features = {}, const BriefPlotOptions& options = {}) -> pluss::pl::Studio&

DataFramePlotAccessor::brief(pluss::RangeView<std::string_view> features = {}, const BriefPlotOptions& options = {}) -> pluss::pl::Studio
```

#### What It Does

Builds a compact summary figure for one or more numeric features.

#### Parameters

**figure**
    Existing figure object to add summary content into instead of creating a new figure.

**features**
    Feature subset to summarize inside the compact figure. Leave empty to let the accessor choose a broad compatible set.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio&`
    Value produced by this call.

`pluss::pl::Studio`
    Value produced by this call.

#### Examples

Example 1. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

auto fig = plot.line("sales");
pluss::table::BriefPlotOptions options;
options.max_cases = 6;
plot.brief(fig, {"sales"}, options);
```

##### Output

```text
Figure(summary)
```
Example 2. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

pluss::table::BriefPlotOptions options;
options.max_cases = 6;
auto fig = plot.brief({"sales", "cost"}, options);
```

##### Output

```text
Figure(summary)
```

#### Practical Behavior

- This overload writes the summary into an existing figure instead of creating a new one.
- Use it when you want a compact numeric summary beside or under another plot.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#box">
    <span class="label">Previous</span>
    <span class="title">box</span>
  </a>
  <a class="next-link" href="#corr_heatmap">
    <span class="label">Next</span>
    <span class="title">corr_heatmap</span>
  </a>
</div>

## C

### `corr_heatmap`

#### Signatures

```cpp
DataFramePlotAccessor::corr_heatmap(pluss::RangeView<std::string_view> features = {}) -> pluss::pl::Studio

DataFramePlotAccessor::corr_heatmap(pluss::RangeView<std::string_view> features, const CorrHeatmapPlotOptions& options) -> pluss::pl::Studio
```

#### What It Does

Computes a correlation matrix across numeric features and renders it as a heatmap.

#### Parameters

**features**
    Numeric features to correlate. Leave empty to let the accessor infer the numeric feature set.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Examples

Example 1. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"x1", pluss::make_series({1.0, 2.0, 3.0})},
    {"x2", pluss::make_series({2.0, 4.0, 6.0})},
    {"x3", pluss::make_series({3.0, 1.0, 5.0})},
});
auto plot = df.plot();

auto fig = plot.corr_heatmap({"x1", "x2", "x3"});
```

##### Output

```text
Figure(corr_heatmap)
```
Example 2. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"x1", pluss::make_series({1.0, 2.0, 3.0})},
    {"x2", pluss::make_series({2.0, 4.0, 6.0})},
    {"x3", pluss::make_series({3.0, 1.0, 5.0})},
});
auto plot = df.plot();

pluss::table::CorrHeatmapPlotOptions options;
options.annotate_levels = true;
auto fig = plot.corr_heatmap({"x1", "x2", "x3"}, options);
```

##### Output

```text
Figure(corr_heatmap)
```

#### Practical Behavior

- `corr_heatmap(...)` selects numeric features, computes their correlation matrix, and renders it as a heatmap.
- If you omit the feature list, the accessor resolves the numeric feature set for you.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#brief">
    <span class="label">Previous</span>
    <span class="title">brief</span>
  </a>
  <a class="next-link" href="#density">
    <span class="label">Next</span>
    <span class="title">density</span>
  </a>
</div>

## D

### `density`

#### Signatures

```cpp
DataFramePlotAccessor::density(pluss::RangeView<std::string_view> features, const DensityPlotOptions& options) -> pluss::pl::Studio

DataFramePlotAccessor::density(pluss::RangeView<std::string_view> features, std::int64_t points = 128) -> pluss::pl::Studio

DataFramePlotAccessor::density(std::initializer_list<std::string_view> features, const DensityPlotOptions& options) -> pluss::pl::Studio

DataFramePlotAccessor::density(std::initializer_list<std::string_view> features, std::int64_t points = 128) -> pluss::pl::Studio

DataFramePlotAccessor::density(std::string_view feature, const DensityPlotOptions& options) -> pluss::pl::Studio

DataFramePlotAccessor::density(std::string_view feature, std::int64_t points = 128) -> pluss::pl::Studio
```

#### What It Does

Builds one or more kernel-density curves from numeric features.

#### Parameters

**features**
    Several numeric features whose kernel density estimates should be drawn together.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

**points**
    Requested evaluation-point count for density-style plots. Higher values make smoother curves at higher compute cost.

**feature**
    One numeric feature whose kernel density estimate should be drawn.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Examples

Example 1. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

pluss::table::DensityPlotOptions options;
options.points = 256;
auto fig = plot.density({"sales", "cost"}, options);
```

##### Output

```text
Figure(density)
```
Example 2. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

auto fig = plot.density({"sales", "cost"}, 128);
```

##### Output

```text
Figure(density)
```
Example 3. Pass several feature names inline.

```cpp
auto df = pluss::table::DataFrame({
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

pluss::table::DensityPlotOptions options;
options.points = 256;
auto fig = plot.density({"sales", "cost"}, options);
```

##### Output

```text
Figure(density)
```
Example 4. Pass several feature names inline.

```cpp
auto df = pluss::table::DataFrame({
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

auto fig = plot.density({"sales", "cost"}, 128);
```

##### Output

```text
Figure(density)
```
Example 5. Work with one named feature.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

pluss::table::DensityPlotOptions options;
options.points = 256;
auto fig = plot.density("sales", options);
```

##### Output

```text
Figure(density)
```
Example 6. Work with one named feature.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.density("sales", 128);
```

##### Output

```text
Figure(density)
```

#### Practical Behavior

- `density(...)` builds kernel-density traces for one or more numeric features.
- The requested point count is clamped to a minimum of 16.
- With many features, one density trace is drawn per feature on the same pane.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#corr_heatmap">
    <span class="label">Previous</span>
    <span class="title">corr_heatmap</span>
  </a>
  <a class="next-link" href="#heatmap">
    <span class="label">Next</span>
    <span class="title">heatmap</span>
  </a>
</div>

## H

### `heatmap`

#### Signatures

```cpp
DataFramePlotAccessor::heatmap(pluss::RangeView<std::string_view> features) -> pluss::pl::Studio

DataFramePlotAccessor::heatmap(pluss::RangeView<std::string_view> features, const HeatmapPlotOptions& options) -> pluss::pl::Studio

DataFramePlotAccessor::heatmap(std::string_view case_feature, std::string_view feature_feature, std::string_view value_feature, DataFrame::GroupOp op = DataFrame::GroupOp::Mean) -> pluss::pl::Studio

DataFramePlotAccessor::heatmap(std::string_view case_feature, std::string_view feature_feature, std::string_view value_feature, DataFrame::GroupOp op, const HeatmapPlotOptions& options) -> pluss::pl::Studio
```

#### What It Does

Builds a heatmap either directly from numeric feature values or from grouped feature/value aggregations.

#### Parameters

**features**
    Numeric features to render directly as a value heatmap. Each feature becomes one column in the heatmap matrix.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

**case_feature**
    Feature whose distinct values define the output heatmap rows in grouped heatmap mode.

**feature_feature**
    Feature whose distinct values define the output heatmap columns in grouped heatmap mode.

**value_feature**
    Feature whose values are aggregated into each grouped heatmap cell.

**op**
    Operation key or enum that chooses the aggregation, comparison, or fill behavior for the call.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Examples

Example 1. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

auto fig = plot.heatmap({"sales", "cost"});
```

##### Output

```text
Figure(heatmap)
```
Example 2. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

pluss::table::HeatmapPlotOptions options;
options.annotate_levels = true;
auto fig = plot.heatmap({"sales", "cost"}, options);
```

##### Output

```text
Figure(heatmap)
```
Example 3. Work with one named feature.

```cpp
auto df = pluss::table::DataFrame({
    {"campus", pluss::make_note({"North", "North", "West", "West"})},
    {"exam", pluss::make_note({"Midterm", "Final", "Midterm", "Final"})},
    {"score", pluss::make_series({71.0, 88.0, 79.0, 95.0})},
});
auto plot = df.plot();

auto fig = plot.heatmap("campus", "exam", "score", pluss::table::DataFrame::GroupOp::Mean);
```

##### Output

```text
Figure(heatmap)
```
Example 4. Work with one named feature.

```cpp
auto df = pluss::table::DataFrame({
    {"campus", pluss::make_note({"North", "North", "West", "West"})},
    {"exam", pluss::make_note({"Midterm", "Final", "Midterm", "Final"})},
    {"score", pluss::make_series({71.0, 88.0, 79.0, 95.0})},
});
auto plot = df.plot();

pluss::table::HeatmapPlotOptions options;
options.annotate_levels = true;
auto fig = plot.heatmap("campus", "exam", "score", pluss::table::DataFrame::GroupOp::Mean, options);
```

##### Output

```text
Figure(heatmap)
```

#### Practical Behavior

- The direct `heatmap(features)` form renders the selected numeric features as a matrix over the case axis.
- The grouped `heatmap(case_feature, feature_feature, value_feature, ...)` form first aggregates long-form rows into a two-dimensional summary grid.
- Use `annotate_levels=true` in the options when you want each cell to display its numeric value.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#density">
    <span class="label">Previous</span>
    <span class="title">density</span>
  </a>
  <a class="next-link" href="#hist">
    <span class="label">Next</span>
    <span class="title">hist</span>
  </a>
</div>

### `hist`

#### Signatures

```cpp
DataFramePlotAccessor::hist(const HistPlotOptions& options) -> pluss::pl::Studio

DataFramePlotAccessor::hist(pluss::RangeView<std::string_view> features, const HistPlotOptions& options) -> pluss::pl::Studio

DataFramePlotAccessor::hist(pluss::RangeView<std::string_view> features, std::int64_t bins = 20) -> pluss::pl::Studio

DataFramePlotAccessor::hist(std::initializer_list<std::string_view> features, const HistPlotOptions& options) -> pluss::pl::Studio

DataFramePlotAccessor::hist(std::initializer_list<std::string_view> features, std::int64_t bins = 20) -> pluss::pl::Studio

DataFramePlotAccessor::hist(std::int64_t bins = 20) -> pluss::pl::Studio

DataFramePlotAccessor::hist(std::string_view feature, const HistPlotOptions& options) -> pluss::pl::Studio

DataFramePlotAccessor::hist(std::string_view feature, std::int64_t bins = 20) -> pluss::pl::Studio
```

#### What It Does

Builds a histogram for one feature using the requested number of bins.

#### Parameters

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

**features**
    Several numeric features to histogram in the same figure.

**bins**
    Histogram bin count. Larger values create narrower bins; smaller values create coarser buckets.

**feature**
    One numeric feature to histogram.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Examples

Example 1. Plot every compatible numeric feature as a histogram with an explicit histogram-options object.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

pluss::table::HistPlotOptions options;
options.bins = 12;
auto fig = plot.hist("sales", options);
```

##### Output

```text
Figure(hist)
```
Example 2. Plot several numeric features from a reusable string view as histograms with explicit histogram options.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

pluss::table::HistPlotOptions options;
options.bins = 12;
auto fig = plot.hist("sales", options);
```

##### Output

```text
Figure(hist)
```
Example 3. Plot several numeric features from a reusable string view as histograms with an explicit shared bin count.

```cpp
auto df = pluss::table::DataFrame({
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

auto fig = plot.hist({"sales", "cost"}, 12);
```

##### Output

```text
Figure(hist)
```
Example 4. Plot several numeric features listed inline as histograms with explicit histogram options.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

pluss::table::HistPlotOptions options;
options.bins = 12;
auto fig = plot.hist("sales", options);
```

##### Output

```text
Figure(hist)
```
Example 5. Plot several numeric features listed inline as histograms with an explicit shared bin count.

```cpp
auto df = pluss::table::DataFrame({
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

auto fig = plot.hist({"sales", "cost"}, 12);
```

##### Output

```text
Figure(hist)
```
Example 6. Plot every compatible numeric feature as a histogram with a shared bin count.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.hist("sales", 12);
```

##### Output

```text
Figure(hist)
```
Example 7. Plot one numeric feature as a histogram with an explicit histogram-options object.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

pluss::table::HistPlotOptions options;
options.bins = 12;
auto fig = plot.hist("sales", options);
```

##### Output

```text
Figure(hist)
```
Example 8. Plot one numeric feature as a histogram with an explicit bin count.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.hist("sales", 12);
```

##### Output

```text
Figure(hist)
```

#### Practical Behavior

- `hist(...)` bins numeric values into frequency bars.
- With several features, the selected features are rendered together in one figure for quick distribution comparison.
- Use the options overload when you want explicit visual controls instead of just the bin count.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#heatmap">
    <span class="label">Previous</span>
    <span class="title">heatmap</span>
  </a>
  <a class="next-link" href="#lag">
    <span class="label">Next</span>
    <span class="title">lag</span>
  </a>
</div>

## L

### `lag`

#### Signature

```cpp
DataFramePlotAccessor::lag(std::string_view feature, const LagPlotOptions& options = {}) -> pluss::pl::Studio
```

#### What It Does

Builds a lag plot that compares one numeric feature against a shifted copy of itself.

#### Parameters

**feature**
    Numeric feature whose lag relationship should be plotted against itself.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Examples

Example 1. Work with one named feature.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

pluss::table::LagPlotOptions options;
options.lag = 2;
auto fig = plot.lag("sales", options);
```

##### Output

```text
Figure(lag)
```

#### Practical Behavior

- `lag(feature, options)` compares one numeric feature against a lagged copy of itself.
- Increase `LagPlotOptions::lag` when you want to inspect longer-gap autocorrelation structure.
- This is useful for quick time-dependence diagnostics before formal modeling.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#hist">
    <span class="label">Previous</span>
    <span class="title">hist</span>
  </a>
  <a class="next-link" href="#line">
    <span class="label">Next</span>
    <span class="title">line</span>
  </a>
</div>

### `line`

#### Signatures

```cpp
DataFramePlotAccessor::line(pluss::RangeView<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) -> pluss::pl::Studio

DataFramePlotAccessor::line(pluss::RangeView<std::string_view> y_features, std::optional<std::string_view> x_feature, const LinePlotOptions& options) -> pluss::pl::Studio

DataFramePlotAccessor::line(std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) -> pluss::pl::Studio

DataFramePlotAccessor::line(std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const LinePlotOptions& options) -> pluss::pl::Studio

DataFramePlotAccessor::line(std::string_view y_feature, std::optional<std::string_view> x_feature = std::nullopt) -> pluss::pl::Studio

DataFramePlotAccessor::line(std::string_view y_feature, std::optional<std::string_view> x_feature, const LinePlotOptions& options) -> pluss::pl::Studio
```

#### What It Does

Builds a line chart for one or more y-features, optionally against a chosen x-feature.

#### Parameters

**y_features**
    Numeric features to overlay as separate line traces on the same axes.

**x_feature**
    Feature used on the horizontal axis of a plot. Leave it empty to let the plot use the case axis or the accessor default when that overload allows it.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

**y_feature**
    One numeric feature to draw as a single line trace.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Examples

Example 1. Plot several y-features from a reusable string view as a shared line chart.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.line("sales");
```

##### Output

```text
Figure(line)
```
Example 2. Plot several y-features from a reusable string view as a shared line chart with explicit plot options.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.line("sales");
```

##### Output

```text
Figure(line)
```
Example 3. Plot several y-features listed inline as a shared line chart.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.line({"sales"}, std::nullopt);
```

##### Output

```text
Figure(line)
```
Example 4. Plot several y-features listed inline as a shared line chart with explicit plot options.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.line({"sales"}, std::nullopt);
```

##### Output

```text
Figure(line)
```
Example 5. Plot one y-feature as a line chart.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.line("sales");
```

##### Output

```text
Figure(line)
```
Example 6. Plot one y-feature as a line chart with an explicit plot-options object.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.line("sales");
```

##### Output

```text
Figure(line)
```

#### Practical Behavior

- `line(...)` draws one continuous trace per selected y-feature.
- Pass `x_feature` when the horizontal axis should come from a real feature instead of the case axis.
- Use the options overload when you want to tune line alpha, width, or y-limit policy.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#lag">
    <span class="label">Previous</span>
    <span class="title">lag</span>
  </a>
  <a class="next-link" href="#parallel">
    <span class="label">Next</span>
    <span class="title">parallel</span>
  </a>
</div>

## P

### `parallel`

#### Signature

```cpp
DataFramePlotAccessor::parallel(pluss::RangeView<std::string_view> features = {}, std::optional<std::string_view> class_feature = std::nullopt, const ParallelPlotOptions& options = {}) -> pluss::pl::Studio
```

#### What It Does

Builds a parallel-coordinates plot from several numeric features, optionally grouped by a class feature.

#### Parameters

**features**
    Numeric features to place on the parallel-coordinate axes. Leave empty to let the accessor infer them.

**class_feature**
    Optional grouping or class label feature used to color, partition, or annotate multivariate plots.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Examples

Example 1. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"x1", pluss::make_series({1.0, 2.0, 3.0})},
    {"x2", pluss::make_series({4.0, 3.0, 2.0})},
    {"x3", pluss::make_series({2.0, 5.0, 4.0})},
    {"group", pluss::make_note({"A", "B", "A"})},
});
auto plot = df.plot();

auto fig = plot.parallel({"x1", "x2", "x3"}, "group");
```

##### Output

```text
Figure(parallel)
```

#### Practical Behavior

- `parallel(...)` expects numeric features and draws one polyline per row across several aligned axes.
- Use `class_feature` when rows should be colored by a grouping label.
- This is useful for comparing multivariate profiles row by row.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#line">
    <span class="label">Previous</span>
    <span class="title">line</span>
  </a>
  <a class="next-link" href="#pie">
    <span class="label">Next</span>
    <span class="title">pie</span>
  </a>
</div>

### `pie`

#### Signatures

```cpp
DataFramePlotAccessor::pie(std::string_view value_feature, std::optional<std::string_view> label_feature = std::nullopt) -> pluss::pl::Studio

DataFramePlotAccessor::pie(std::string_view value_feature, std::optional<std::string_view> label_feature, const PiePlotOptions& options) -> pluss::pl::Studio
```

#### What It Does

Builds a pie chart from one value feature, optionally using another feature for labels.

#### Parameters

**value_feature**
    Feature whose values determine each slice magnitude.

**label_feature**
    Optional feature that supplies the text label for each slice.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Examples

Example 1. Plot one value feature as a pie chart.

```cpp
auto df = pluss::table::DataFrame({
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi"})},
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
});
auto plot = df.plot();

auto fig = plot.pie("sales", "city");
```

##### Output

```text
Figure(pie)
```
Example 2. Plot one value feature as a pie chart with an explicit pie-options object.

```cpp
auto df = pluss::table::DataFrame({
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi"})},
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
});
auto plot = df.plot();

pluss::table::PiePlotOptions options;
options.show_value = true;
auto fig = plot.pie("sales", "city", options);
```

##### Output

```text
Figure(pie)
```

#### Practical Behavior

- `pie(value_feature, label_feature, ...)` uses one row per slice.
- The value feature determines slice magnitude, and the label feature controls the text shown for each slice.
- This is best for small categorical summaries rather than large row counts.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#parallel">
    <span class="label">Previous</span>
    <span class="title">parallel</span>
  </a>
  <a class="next-link" href="#radviz">
    <span class="label">Next</span>
    <span class="title">radviz</span>
  </a>
</div>

## R

### `radviz`

#### Signature

```cpp
DataFramePlotAccessor::radviz(pluss::RangeView<std::string_view> features = {}, std::optional<std::string_view> class_feature = std::nullopt, const RadvizPlotOptions& options = {}) -> pluss::pl::Studio
```

#### What It Does

Builds a radial visualization from several numeric features, optionally grouped by a class feature.

#### Parameters

**features**
    Numeric features used as anchors in the radial visualization.

**class_feature**
    Optional grouping or class label feature used to color, partition, or annotate multivariate plots.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Examples

Example 1. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"x1", pluss::make_series({1.0, 2.0, 3.0})},
    {"x2", pluss::make_series({4.0, 3.0, 2.0})},
    {"x3", pluss::make_series({2.0, 5.0, 4.0})},
    {"group", pluss::make_note({"A", "B", "A"})},
});
auto plot = df.plot();

auto fig = plot.radviz({"x1", "x2", "x3"}, "group");
```

##### Output

```text
Figure(radviz)
```

#### Practical Behavior

- `radviz(...)` expects numeric features and projects each row toward feature anchors around a circle.
- Use `class_feature` when you want point colors to reflect group labels.
- This is useful for quick multivariate separation checks between classes.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#pie">
    <span class="label">Previous</span>
    <span class="title">pie</span>
  </a>
  <a class="next-link" href="#scatter">
    <span class="label">Next</span>
    <span class="title">scatter</span>
  </a>
</div>

## S

### `scatter`

#### Signatures

```cpp
DataFramePlotAccessor::scatter(std::string_view x_feature, std::string_view y_feature, std::optional<std::string_view> hue_feature = std::nullopt) -> pluss::pl::Studio

DataFramePlotAccessor::scatter(std::string_view x_feature, std::string_view y_feature, std::optional<std::string_view> hue_feature, const ScatterPlotOptions& options) -> pluss::pl::Studio
```

#### What It Does

Builds a scatter plot using one feature for x and another for y.

#### Parameters

**x_feature**
    Numeric or ordered feature placed on the horizontal axis.

**y_feature**
    Numeric feature placed on the vertical axis.

**hue_feature**
    Optional feature whose distinct values split one plot into color-coded groups.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Examples

Example 1. Plot one x/y feature pair as a scatter chart.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.scatter("id", "sales");
```

##### Output

```text
Figure(scatter)
```
Example 2. Plot one x/y feature pair as a scatter chart with an explicit scatter-options object.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.scatter("id", "sales");
```

##### Output

```text
Figure(scatter)
```

#### Practical Behavior

- `scatter(x_feature, y_feature, ...)` plots one point per row.
- Use `hue_feature` when rows should be color-grouped by a categorical label.
- This method is best when the relationship between two measures matters more than their order.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#radviz">
    <span class="label">Previous</span>
    <span class="title">radviz</span>
  </a>
  <a class="next-link" href="#scatter_matrix">
    <span class="label">Next</span>
    <span class="title">scatter_matrix</span>
  </a>
</div>

### `scatter_matrix`

#### Signatures

```cpp
DataFramePlotAccessor::scatter_matrix(pluss::RangeView<std::string_view> features = {}) -> pluss::pl::Studio

DataFramePlotAccessor::scatter_matrix(pluss::RangeView<std::string_view> features, const ScatterMatrixPlotOptions& options) -> pluss::pl::Studio

DataFramePlotAccessor::scatter_matrix(pluss::RangeView<std::string_view> features, ScatterMatrixTriangle triangle) -> pluss::pl::Studio
```

#### What It Does

Builds a scatter-matrix figure for several numeric features.

#### Parameters

**features**
    Numeric features to place on the pairwise matrix axes. Leave empty to let the accessor infer them.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

**triangle**
    Pair-plot triangle selection controlling whether the scatter matrix renders the upper, lower, or corner-only half.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Examples

Example 1. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"x1", pluss::make_series({1.0, 2.0, 3.0})},
    {"x2", pluss::make_series({4.0, 3.0, 2.0})},
    {"x3", pluss::make_series({2.0, 5.0, 4.0})},
});
auto plot = df.plot();

auto fig = plot.scatter_matrix({"x1", "x2", "x3"});
```

##### Output

```text
Figure(scatter_matrix)
```
Example 2. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"x1", pluss::make_series({1.0, 2.0, 3.0})},
    {"x2", pluss::make_series({4.0, 3.0, 2.0})},
    {"x3", pluss::make_series({2.0, 5.0, 4.0})},
});
auto plot = df.plot();

pluss::table::ScatterMatrixPlotOptions options;
options.corner = true;
auto fig = plot.scatter_matrix({"x1", "x2", "x3"}, options);
```

##### Output

```text
Figure(scatter_matrix)
```
Example 3. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"x1", pluss::make_series({1.0, 2.0, 3.0})},
    {"x2", pluss::make_series({4.0, 3.0, 2.0})},
    {"x3", pluss::make_series({2.0, 5.0, 4.0})},
});
auto plot = df.plot();

auto fig = plot.scatter_matrix({"x1", "x2", "x3"}, pluss::table::ScatterMatrixTriangle::Lower);
```

##### Output

```text
Figure(scatter_matrix)
```

#### Practical Behavior

- `scatter_matrix(...)` creates pairwise scatter panels for the selected numeric features.
- Use the `triangle` overload when you want only the upper, lower, or corner half of the matrix.
- Use the options overload when you want finer pair-plot rendering control.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#scatter">
    <span class="label">Previous</span>
    <span class="title">scatter</span>
  </a>
  <a class="next-link" href="#step">
    <span class="label">Next</span>
    <span class="title">step</span>
  </a>
</div>

### `step`

#### Signatures

```cpp
DataFramePlotAccessor::step(pluss::RangeView<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) -> pluss::pl::Studio

DataFramePlotAccessor::step(pluss::RangeView<std::string_view> y_features, std::optional<std::string_view> x_feature, const StepPlotOptions& options) -> pluss::pl::Studio

DataFramePlotAccessor::step(std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) -> pluss::pl::Studio

DataFramePlotAccessor::step(std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const StepPlotOptions& options) -> pluss::pl::Studio

DataFramePlotAccessor::step(std::string_view y_feature, std::optional<std::string_view> x_feature = std::nullopt) -> pluss::pl::Studio

DataFramePlotAccessor::step(std::string_view y_feature, std::optional<std::string_view> x_feature, const StepPlotOptions& options) -> pluss::pl::Studio
```

#### What It Does

Builds a step chart for one or more y-features, optionally against a chosen x-feature.

#### Parameters

**y_features**
    Numeric features to overlay as separate step traces on the same axes.

**x_feature**
    Feature used on the horizontal axis of a plot. Leave it empty to let the plot use the case axis or the accessor default when that overload allows it.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

**y_feature**
    One numeric feature to draw as a step trace.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Examples

Example 1. Plot one or more y-features as a step chart.

```cpp
auto df = pluss::table::DataFrame({
    {"month", pluss::make_note({"Jan", "Feb", "Mar"})},
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

auto fig = plot.step({"sales", "cost"}, "month");
```

##### Output

```text
Figure(step)
```
Example 2. Plot one or more y-features as a step chart with an explicit step-options object.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

pluss::table::StepPlotOptions options;
options.line_width = 2.0;
auto fig = plot.step("sales", "id", options);
```

##### Output

```text
Figure(step)
```
Example 3. Plot one or more y-features as a step chart.

```cpp
auto df = pluss::table::DataFrame({
    {"month", pluss::make_note({"Jan", "Feb", "Mar"})},
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

auto fig = plot.step({"sales", "cost"}, "month");
```

##### Output

```text
Figure(step)
```
Example 4. Plot one or more y-features as a step chart with an explicit step-options object.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

pluss::table::StepPlotOptions options;
options.line_width = 2.0;
auto fig = plot.step("sales", "id", options);
```

##### Output

```text
Figure(step)
```
Example 5. Plot one or more y-features as a step chart.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.step("sales", "id");
```

##### Output

```text
Figure(step)
```
Example 6. Plot one or more y-features as a step chart with an explicit step-options object.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

pluss::table::StepPlotOptions options;
options.line_width = 2.0;
auto fig = plot.step("sales", "id", options);
```

##### Output

```text
Figure(step)
```

#### Practical Behavior

- `step(...)` uses the same feature selection pattern as `line(...)`, but the trace changes by steps instead of straight segments.
- This is useful when the y-value should be interpreted as holding until the next x position.
- If `x_feature` is omitted, the case axis is used.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#scatter_matrix">
    <span class="label">Previous</span>
    <span class="title">scatter_matrix</span>
  </a>
  <a class="next-link" href="#violin">
    <span class="label">Next</span>
    <span class="title">violin</span>
  </a>
</div>

## V

### `violin`

#### Signatures

```cpp
DataFramePlotAccessor::violin(pluss::RangeView<std::string_view> features = {}) -> pluss::pl::Studio

DataFramePlotAccessor::violin(pluss::RangeView<std::string_view> features, const ViolinPlotOptions& options) -> pluss::pl::Studio

DataFramePlotAccessor::violin(std::initializer_list<std::string_view> features) -> pluss::pl::Studio

DataFramePlotAccessor::violin(std::initializer_list<std::string_view> features, const ViolinPlotOptions& options) -> pluss::pl::Studio

DataFramePlotAccessor::violin(std::string_view value_feature, std::string_view group_feature) -> pluss::pl::Studio

DataFramePlotAccessor::violin(std::string_view value_feature, std::string_view group_feature, const ViolinPlotOptions& options) -> pluss::pl::Studio
```

#### What It Does

Builds violin plots for one or more numeric features or grouped distributions.

#### Parameters

**features**
    Numeric features to summarize as violin plots. Leave empty to let the accessor infer the numeric feature set.

**options**
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

**value_feature**
    Numeric feature whose distribution is shown inside each violin.

**group_feature**
    Categorical feature that splits the violin plot into separate groups.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Examples

Example 1. Plot one or more numeric features as violin distributions.

```cpp
auto df = pluss::table::DataFrame({
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

auto fig = plot.violin({"sales", "cost"});
```

##### Output

```text
Figure(violin)
```
Example 2. Plot one or more numeric features as violin distributions with an explicit violin-options object.

```cpp
auto df = pluss::table::DataFrame({
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

pluss::table::ViolinPlotOptions options;
options.width = 0.65;
auto fig = plot.violin({"sales", "cost"}, options);
```

##### Output

```text
Figure(violin)
```
Example 3. Plot one or more numeric features as violin distributions.

```cpp
auto df = pluss::table::DataFrame({
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

auto fig = plot.violin({"sales", "cost"});
```

##### Output

```text
Figure(violin)
```
Example 4. Plot one or more numeric features as violin distributions with an explicit violin-options object.

```cpp
auto df = pluss::table::DataFrame({
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

pluss::table::ViolinPlotOptions options;
options.width = 0.65;
auto fig = plot.violin({"sales", "cost"}, options);
```

##### Output

```text
Figure(violin)
```
Example 5. Plot one or more numeric features as violin distributions.

```cpp
auto df = pluss::table::DataFrame({
    {"section", pluss::make_note({"A", "A", "B", "B"})},
    {"score", pluss::make_series({71.0, 88.0, 79.0, 95.0})},
});
auto plot = df.plot();

auto fig = plot.violin("score", "section");
```

##### Output

```text
Figure(violin)
```
Example 6. Plot one or more numeric features as violin distributions with an explicit violin-options object.

```cpp
auto df = pluss::table::DataFrame({
    {"section", pluss::make_note({"A", "A", "B", "B"})},
    {"score", pluss::make_series({71.0, 88.0, 79.0, 95.0})},
});
auto plot = df.plot();

pluss::table::ViolinPlotOptions options;
options.inner = pluss::pl::ViolinInner::Quartile;
auto fig = plot.violin("score", "section", options);
```

##### Output

```text
Figure(violin)
```

#### Practical Behavior

- The feature-list overload renders one violin per numeric feature.
- The `(value_feature, group_feature)` overload renders one violin per group in the grouping feature.
- Use this when you want full distribution shape rather than only quartiles or whiskers.

<div class="pluss-inline-prev-next">
  <a class="prev-link" href="#step">
    <span class="label">Previous</span>
    <span class="title">step</span>
  </a>
</div>
