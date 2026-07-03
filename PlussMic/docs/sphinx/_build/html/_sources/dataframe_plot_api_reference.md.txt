# DataFramePlotAccessor API Reference

# Practical DataFramePlotAccessor API Reference

## API Reference Style

This page documents the public plotting accessor returned by `DataFrame::plot()` and `Atlas::plot()`.

```{include} snippets/dataframe_plot_options_reference.md
```

## A

### `act`

#### API Signature

```cpp
DataFramePlotAccessor::act(feature: std::string_view, options: const ActPlotOptions& = {}) -> pluss::pl::Studio
```

#### Notes

Builds an autocorrelation-style lag plot for one numeric feature.

#### Inputs

`feature` : `std::string_view`
    Numeric feature whose autocorrelation-style lag structure should be plotted.

`options` : `const ActPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`pluss::pl::Studio`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

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

##### Expected output

```text
Figure(act)
```

#### Practical Behavior

- `act(feature, options)` requires one numeric feature.
- Use it when you want autocorrelation-style time-lag structure for one series.


### `andrews_curves`

#### API Signature

```cpp
DataFramePlotAccessor::andrews_curves(features: pluss::RangeView<std::string_view> = {}, class_feature: std::optional<std::string_view> = std::nullopt, options: const AndrewsCurvesPlotOptions& = {}) -> pluss::pl::Studio
```

#### Notes

Builds Andrews curves from several numeric features, optionally colored by a class feature.

#### Inputs

`features` : `pluss::RangeView<std::string_view>`
    Numeric features used to build each observation curve.

`class_feature` : `std::optional<std::string_view>`
    Optional grouping or class label feature used to color, partition, or annotate multivariate plots.

`options` : `const AndrewsCurvesPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`pluss::pl::Studio`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

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

##### Expected output

```text
Figure(andrews_curves)
```

#### Practical Behavior

- `andrews_curves(...)` works on numeric features and can optionally color/group by a class feature.
- If you do not supply labels in the options, the feature names are used.


### `area`

#### API Signatures

```cpp
DataFramePlotAccessor::area(y_feature: std::string_view, x_feature: std::optional<std::string_view> = std::nullopt) -> pluss::pl::Studio

DataFramePlotAccessor::area(y_feature: std::string_view, x_feature: std::optional<std::string_view>, options: const AreaPlotOptions&) -> pluss::pl::Studio

DataFramePlotAccessor::area(y_features: pluss::RangeView<std::string_view>, x_feature: std::optional<std::string_view> = std::nullopt) -> pluss::pl::Studio

DataFramePlotAccessor::area(y_features: pluss::RangeView<std::string_view>, x_feature: std::optional<std::string_view>, options: const AreaPlotOptions&) -> pluss::pl::Studio

DataFramePlotAccessor::area(y_features: std::initializer_list<std::string_view>, x_feature: std::optional<std::string_view> = std::nullopt) -> pluss::pl::Studio

DataFramePlotAccessor::area(y_features: std::initializer_list<std::string_view>, x_feature: std::optional<std::string_view>, options: const AreaPlotOptions&) -> pluss::pl::Studio
```

#### Notes

Builds a filled area plot from one or more numeric features.

#### Inputs

`y_feature` : `std::string_view`
    One numeric feature to draw as a filled area.

`x_feature` : `std::optional<std::string_view>`
    Feature used on the horizontal axis of a plot. Leave it empty to let the plot use the case axis or the accessor default when that overload allows it.

`options` : `const AreaPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

`y_features` : `pluss::RangeView<std::string_view>`
    Numeric features to draw as separate filled areas over the same axis positions.

#### Output

`pluss::pl::Studio`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Plot one or more y-features as an area chart.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.area("sales", "id");
```

##### Expected output

```text
Figure(area)
```
Example 2. Plot one or more y-features as an area chart with an explicit area-options object.

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

##### Expected output

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

##### Expected output

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

##### Expected output

```text
Figure(area)
```
Example 5. Plot one or more y-features as an area chart.

```cpp
auto df = pluss::table::DataFrame({
    {"month", pluss::make_note({"Jan", "Feb", "Mar"})},
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

auto fig = plot.area({"sales", "cost"}, "month");
```

##### Expected output

```text
Figure(area)
```
Example 6. Plot one or more y-features as an area chart with an explicit area-options object.

```cpp
auto df = pluss::table::DataFrame({
    {"month", pluss::make_note({"Jan", "Feb", "Mar"})},
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

auto fig = plot.area({"sales", "cost"}, "month");
```

##### Expected output

```text
Figure(area)
```

#### Practical Behavior

- One y-feature creates one filled area trace.
- Many y-features create one area trace per selected feature over the same x-axis.
- If `x_feature` is omitted, the case axis is used.


### `area_stacked`

#### API Signatures

```cpp
DataFramePlotAccessor::area_stacked(y_features: pluss::RangeView<std::string_view>, x_feature: std::optional<std::string_view> = std::nullopt) -> pluss::pl::Studio

DataFramePlotAccessor::area_stacked(y_features: pluss::RangeView<std::string_view>, x_feature: std::optional<std::string_view>, options: const AreaStackedPlotOptions&) -> pluss::pl::Studio

DataFramePlotAccessor::area_stacked(y_features: std::initializer_list<std::string_view>, x_feature: std::optional<std::string_view> = std::nullopt) -> pluss::pl::Studio

DataFramePlotAccessor::area_stacked(y_features: std::initializer_list<std::string_view>, x_feature: std::optional<std::string_view>, options: const AreaStackedPlotOptions&) -> pluss::pl::Studio
```

#### Notes

Builds a stacked area plot where several numeric features accumulate at shared x positions.

#### Inputs

`y_features` : `pluss::RangeView<std::string_view>`
    Numeric features whose values should accumulate vertically at shared x positions.

`x_feature` : `std::optional<std::string_view>`
    Feature used on the horizontal axis of a plot. Leave it empty to let the plot use the case axis or the accessor default when that overload allows it.

`options` : `const AreaStackedPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`pluss::pl::Studio`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

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

##### Expected output

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

##### Expected output

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

##### Expected output

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

##### Expected output

```text
Figure(area_stacked)
```

#### Practical Behavior

- `area_stacked(...)` requires at least one y-feature.
- Each later y-feature is stacked on top of the running baseline from the earlier ones.
- This is useful for contribution-over-total views across the same x positions.


### `auto_plot`

#### API Signatures

```cpp
DataFramePlotAccessor::auto_plot(features: pluss::RangeView<std::string_view> = {}) -> pluss::pl::Studio

DataFramePlotAccessor::auto_plot(features: pluss::RangeView<std::string_view>, options: const AutoPlotOptions&) -> pluss::pl::Studio
```

#### Notes

Chooses a plot family automatically from the selected feature set and frame shape.

#### Inputs

`features` : `pluss::RangeView<std::string_view>`
    Feature subset to consider while the accessor chooses the plot family automatically.

`options` : `const AutoPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`pluss::pl::Studio`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

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

##### Expected output

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

##### Expected output

```text
Figure(auto_selected)
```

#### Selection Rules

- `auto_plot(...)` chooses a plot family based on feature count, row count, case labels, and sign patterns.
- Examples include bar for one labeled feature, stacked bar/area for a few nonnegative features, scatter matrix for small numeric sets, and correlation heatmap for larger ones.
- Use it for quick inspection when you do not want to choose the plot form manually.


## B

### `bar`

#### API Signatures

```cpp
DataFramePlotAccessor::bar(y_feature: std::string_view, x_feature: std::optional<std::string_view> = std::nullopt) -> pluss::pl::Studio

DataFramePlotAccessor::bar(y_feature: std::string_view, x_feature: std::optional<std::string_view>, options: const BarPlotOptions&) -> pluss::pl::Studio

DataFramePlotAccessor::bar(y_features: pluss::RangeView<std::string_view>, x_feature: std::optional<std::string_view> = std::nullopt) -> pluss::pl::Studio

DataFramePlotAccessor::bar(y_features: pluss::RangeView<std::string_view>, x_feature: std::optional<std::string_view>, options: const BarPlotOptions&) -> pluss::pl::Studio

DataFramePlotAccessor::bar(y_features: std::initializer_list<std::string_view>, x_feature: std::optional<std::string_view> = std::nullopt) -> pluss::pl::Studio

DataFramePlotAccessor::bar(y_features: std::initializer_list<std::string_view>, x_feature: std::optional<std::string_view>, options: const BarPlotOptions&) -> pluss::pl::Studio
```

#### Notes

Builds a bar chart from one feature, optionally using another feature as the x-axis.

#### Inputs

`y_feature` : `std::string_view`
    Single feature whose values become one bar series.

`x_feature` : `std::optional<std::string_view>`
    Feature used on the horizontal axis of a plot. Leave it empty to let the plot use the case axis or the accessor default when that overload allows it.

`options` : `const BarPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

`y_features` : `pluss::RangeView<std::string_view>`
    Several features whose values become grouped or layered bar series, depending on the method variant.

#### Output

`pluss::pl::Studio`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Plot one y-feature as a bar chart.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.bar("sales", "city");
```

##### Expected output

```text
Figure(bar)
```
Example 2. Plot one y-feature as a bar chart with an explicit plot-options object.

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

##### Expected output

```text
Figure(bar)
```
Example 3. Plot several y-features from a reusable string view as a shared bar chart.

```cpp
auto df = pluss::table::DataFrame({
    {"student", pluss::make_note({"Laurel", "Kojo", "Ama"})},
    {"midterm", pluss::make_series({71.0, 82.0, 93.0})},
    {"final", pluss::make_series({88.0, 79.0, 95.0})},
});
auto plot = df.plot();

auto fig = plot.bar({"midterm", "final"}, "student");
```

##### Expected output

```text
Figure(bar)
```
Example 4. Plot several y-features from a reusable string view as a shared bar chart with explicit plot options.

```cpp
auto df = pluss::table::DataFrame({
    {"student", pluss::make_note({"Laurel", "Kojo", "Ama"})},
    {"midterm", pluss::make_series({71.0, 82.0, 93.0})},
    {"final", pluss::make_series({88.0, 79.0, 95.0})},
});
auto plot = df.plot();

auto fig = plot.bar({"midterm", "final"}, "student");
```

##### Expected output

```text
Figure(bar)
```
Example 5. Plot several y-features listed inline as a shared bar chart.

```cpp
auto df = pluss::table::DataFrame({
    {"student", pluss::make_note({"Laurel", "Kojo", "Ama"})},
    {"midterm", pluss::make_series({71.0, 82.0, 93.0})},
    {"final", pluss::make_series({88.0, 79.0, 95.0})},
});
auto plot = df.plot();

auto fig = plot.bar({"midterm", "final"}, "student");
```

##### Expected output

```text
Figure(bar)
```
Example 6. Plot several y-features listed inline as a shared bar chart with explicit plot options.

```cpp
auto df = pluss::table::DataFrame({
    {"student", pluss::make_note({"Laurel", "Kojo", "Ama"})},
    {"midterm", pluss::make_series({71.0, 82.0, 93.0})},
    {"final", pluss::make_series({88.0, 79.0, 95.0})},
});
auto plot = df.plot();

auto fig = plot.bar({"midterm", "final"}, "student");
```

##### Expected output

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


### `bar_stacked`

#### API Signatures

```cpp
DataFramePlotAccessor::bar_stacked(y_features: pluss::RangeView<std::string_view>, x_feature: std::optional<std::string_view> = std::nullopt) -> pluss::pl::Studio

DataFramePlotAccessor::bar_stacked(y_features: pluss::RangeView<std::string_view>, x_feature: std::optional<std::string_view>, options: const BarStackedPlotOptions&) -> pluss::pl::Studio

DataFramePlotAccessor::bar_stacked(y_features: std::initializer_list<std::string_view>, x_feature: std::optional<std::string_view> = std::nullopt) -> pluss::pl::Studio

DataFramePlotAccessor::bar_stacked(y_features: std::initializer_list<std::string_view>, x_feature: std::optional<std::string_view>, options: const BarStackedPlotOptions&) -> pluss::pl::Studio
```

#### Notes

Builds stacked bars so multiple y-features accumulate on the same axis positions.

#### Inputs

`y_features` : `pluss::RangeView<std::string_view>`
    Several features whose values stack on top of each other at each x position.

`x_feature` : `std::optional<std::string_view>`
    Feature used on the horizontal axis of a plot. Leave it empty to let the plot use the case axis or the accessor default when that overload allows it.

`options` : `const BarStackedPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`pluss::pl::Studio`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

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

##### Expected output

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

##### Expected output

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

##### Expected output

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

##### Expected output

```text
Figure(bar_stacked)
```

#### Practical Behavior

- `bar_stacked(...)` requires at least one y-feature.
- Each later y-feature is stacked on top of the earlier ones at the same x-axis positions.
- This is useful for composition views where the total height and each component both matter.
- If `x_feature` is omitted, the case axis is used.


### `bootstrap`

#### API Signature

```cpp
DataFramePlotAccessor::bootstrap(feature: std::string_view, options: const BootstrapPlotOptions& = {}) -> pluss::pl::Studio
```

#### Notes

Builds a bootstrap summary plot for one numeric feature using the supplied resampling options.

#### Inputs

`feature` : `std::string_view`
    Numeric feature to resample repeatedly for the bootstrap figure.

`options` : `const BootstrapPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`pluss::pl::Studio`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

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

##### Expected output

```text
Figure(bootstrap)
```

#### Practical Behavior

- `bootstrap(feature, options)` requires one numeric feature.
- The plot is driven by resampling options such as `samples`, `sample_size`, `seed`, and the chosen statistic.
- This is the method to use when you want sampling-based uncertainty or distribution shape around a summary statistic.


### `box`

#### API Signatures

```cpp
DataFramePlotAccessor::box(features: pluss::RangeView<std::string_view> = {}) -> pluss::pl::Studio

DataFramePlotAccessor::box(features: pluss::RangeView<std::string_view>, options: const BoxPlotOptions&) -> pluss::pl::Studio

DataFramePlotAccessor::box(features: std::initializer_list<std::string_view>) -> pluss::pl::Studio

DataFramePlotAccessor::box(features: std::initializer_list<std::string_view>, options: const BoxPlotOptions&) -> pluss::pl::Studio
```

#### Notes

Builds a box plot for one or more numeric features.

#### Inputs

`features` : `pluss::RangeView<std::string_view>`
    Numeric features to summarize as box plots. Leave empty to let the accessor infer the numeric feature set.

`options` : `const BoxPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`pluss::pl::Studio`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

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

##### Expected output

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

##### Expected output

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

##### Expected output

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

##### Expected output

```text
Figure(box)
```

#### Practical Behavior

- `box(...)` accepts numeric features only.
- With no explicit feature list, the accessor resolves the available numeric features for you.
- Use this when you want a distribution summary instead of a per-row plot.


### `brief`

#### API Signatures

```cpp
DataFramePlotAccessor::brief(features: pluss::RangeView<std::string_view> = {}, options: const BriefPlotOptions& = {}) -> pluss::pl::Studio

DataFramePlotAccessor::brief(figure: pluss::pl::Studio&, features: pluss::RangeView<std::string_view> = {}, options: const BriefPlotOptions& = {}) -> pluss::pl::Studio&
```

#### Notes

Builds a compact summary figure for one or more numeric features.

#### Inputs

`features` : `pluss::RangeView<std::string_view>`
    Feature subset to summarize inside the compact figure. Leave empty to let the accessor choose a broad compatible set.

`options` : `const BriefPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

`figure` : `pluss::pl::Studio&`
    Existing figure object to add summary content into instead of creating a new figure.

#### Output

`pluss::pl::Studio`
    Value produced by this call.

`pluss::pl::Studio&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Work with a selected feature subset.

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

##### Expected output

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

auto fig = plot.line("sales");
pluss::table::BriefPlotOptions options;
options.max_cases = 6;
plot.brief(fig, {"sales"}, options);
```

##### Expected output

```text
Figure(summary)
```

#### Practical Behavior

- `brief(...)` builds a compact summary figure for numeric features.
- `BriefPlotOptions` controls table-style presentation details such as `precision`, `max_cases`, and whether headers are shown.
- Use this when you want a readable figure-based summary rather than a chart trace per row.


## C

### `corr_heatmap`

#### API Signatures

```cpp
DataFramePlotAccessor::corr_heatmap(features: pluss::RangeView<std::string_view> = {}) -> pluss::pl::Studio

DataFramePlotAccessor::corr_heatmap(features: pluss::RangeView<std::string_view>, options: const CorrHeatmapPlotOptions&) -> pluss::pl::Studio
```

#### Notes

Computes a correlation matrix across numeric features and renders it as a heatmap.

#### Inputs

`features` : `pluss::RangeView<std::string_view>`
    Numeric features to correlate. Leave empty to let the accessor infer the numeric feature set.

`options` : `const CorrHeatmapPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`pluss::pl::Studio`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

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

##### Expected output

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

##### Expected output

```text
Figure(corr_heatmap)
```

#### Practical Behavior

- `corr_heatmap(...)` selects numeric features, computes their correlation matrix, and renders it as a heatmap.
- If you omit the feature list, the accessor resolves the numeric feature set for you.


## D

### `density`

#### API Signatures

```cpp
DataFramePlotAccessor::density(feature: std::string_view, options: const DensityPlotOptions&) -> pluss::pl::Studio

DataFramePlotAccessor::density(feature: std::string_view, points: std::int64_t = 128) -> pluss::pl::Studio

DataFramePlotAccessor::density(features: pluss::RangeView<std::string_view>, options: const DensityPlotOptions&) -> pluss::pl::Studio

DataFramePlotAccessor::density(features: pluss::RangeView<std::string_view>, points: std::int64_t = 128) -> pluss::pl::Studio

DataFramePlotAccessor::density(features: std::initializer_list<std::string_view>, options: const DensityPlotOptions&) -> pluss::pl::Studio

DataFramePlotAccessor::density(features: std::initializer_list<std::string_view>, points: std::int64_t = 128) -> pluss::pl::Studio
```

#### Notes

Builds one or more kernel-density curves from numeric features.

#### Inputs

`feature` : `std::string_view`
    One numeric feature whose kernel density estimate should be drawn.

`options` : `const DensityPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

`points` : `std::int64_t`
    Requested evaluation-point count for density-style plots. Higher values make smoother curves at higher compute cost.

`features` : `pluss::RangeView<std::string_view>`
    Several numeric features whose kernel density estimates should be drawn together.

#### Output

`pluss::pl::Studio`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Work with one named feature.

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

##### Expected output

```text
Figure(density)
```
Example 2. Work with one named feature.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.density("sales", 128);
```

##### Expected output

```text
Figure(density)
```
Example 3. Work with a selected feature subset.

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

##### Expected output

```text
Figure(density)
```
Example 4. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

auto fig = plot.density({"sales", "cost"}, 128);
```

##### Expected output

```text
Figure(density)
```
Example 5. Pass several feature names inline.

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

##### Expected output

```text
Figure(density)
```
Example 6. Pass several feature names inline.

```cpp
auto df = pluss::table::DataFrame({
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

auto fig = plot.density({"sales", "cost"}, 128);
```

##### Expected output

```text
Figure(density)
```

#### Practical Behavior

- `density(...)` builds kernel-density traces for one or more numeric features.
- The requested point count is clamped to a minimum of 16.
- With many features, one density trace is drawn per feature on the same pane.


## H

### `heatmap`

#### API Signatures

```cpp
DataFramePlotAccessor::heatmap(case_feature: std::string_view, feature_feature: std::string_view, value_feature: std::string_view, op: DataFrame::GroupOp = DataFrame::GroupOp::Mean) -> pluss::pl::Studio

DataFramePlotAccessor::heatmap(case_feature: std::string_view, feature_feature: std::string_view, value_feature: std::string_view, op: DataFrame::GroupOp, options: const HeatmapPlotOptions&) -> pluss::pl::Studio

DataFramePlotAccessor::heatmap(features: pluss::RangeView<std::string_view>) -> pluss::pl::Studio

DataFramePlotAccessor::heatmap(features: pluss::RangeView<std::string_view>, options: const HeatmapPlotOptions&) -> pluss::pl::Studio
```

#### Notes

Builds a heatmap either directly from numeric feature values or from grouped feature/value aggregations.

#### Inputs

`case_feature` : `std::string_view`
    Feature whose distinct values define the output heatmap rows in grouped heatmap mode.

`feature_feature` : `std::string_view`
    Feature whose distinct values define the output heatmap columns in grouped heatmap mode.

`value_feature` : `std::string_view`
    Feature whose values are aggregated into each grouped heatmap cell.

`op` : `DataFrame::GroupOp`
    Operation key or enum that chooses the aggregation, comparison, or fill behavior for the call.

`options` : `const HeatmapPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

`features` : `pluss::RangeView<std::string_view>`
    Numeric features to render directly as a value heatmap. Each feature becomes one column in the heatmap matrix.

#### Output

`pluss::pl::Studio`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Work with one named feature.

```cpp
auto df = pluss::table::DataFrame({
    {"campus", pluss::make_note({"North", "North", "West", "West"})},
    {"exam", pluss::make_note({"Midterm", "Final", "Midterm", "Final"})},
    {"score", pluss::make_series({71.0, 88.0, 79.0, 95.0})},
});
auto plot = df.plot();

auto fig = plot.heatmap("campus", "exam", "score", pluss::table::DataFrame::GroupOp::Mean);
```

##### Expected output

```text
Figure(heatmap)
```
Example 2. Work with one named feature.

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

##### Expected output

```text
Figure(heatmap)
```
Example 3. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

auto fig = plot.heatmap({"sales", "cost"});
```

##### Expected output

```text
Figure(heatmap)
```
Example 4. Work with a selected feature subset.

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

##### Expected output

```text
Figure(heatmap)
```

#### Practical Behavior

- The direct `heatmap(features)` form renders the selected numeric features as a matrix over the case axis.
- The grouped `heatmap(case_feature, feature_feature, value_feature, ...)` form first aggregates long-form rows into a two-dimensional summary grid.
- Use `annotate_levels=true` in the options when you want each cell to display its numeric value.


### `hist`

#### API Signatures

```cpp
DataFramePlotAccessor::hist(bins: std::int64_t = 20) -> pluss::pl::Studio

DataFramePlotAccessor::hist(feature: std::string_view, bins: std::int64_t = 20) -> pluss::pl::Studio

DataFramePlotAccessor::hist(feature: std::string_view, options: const HistPlotOptions&) -> pluss::pl::Studio

DataFramePlotAccessor::hist(features: pluss::RangeView<std::string_view>, bins: std::int64_t = 20) -> pluss::pl::Studio

DataFramePlotAccessor::hist(features: pluss::RangeView<std::string_view>, options: const HistPlotOptions&) -> pluss::pl::Studio

DataFramePlotAccessor::hist(features: std::initializer_list<std::string_view>, bins: std::int64_t = 20) -> pluss::pl::Studio

DataFramePlotAccessor::hist(features: std::initializer_list<std::string_view>, options: const HistPlotOptions&) -> pluss::pl::Studio

DataFramePlotAccessor::hist(options: const HistPlotOptions&) -> pluss::pl::Studio
```

#### Notes

Builds a histogram for one feature using the requested number of bins.

#### Inputs

`bins` : `std::int64_t`
    Histogram bin count. Larger values create narrower bins; smaller values create coarser buckets.

`feature` : `std::string_view`
    One numeric feature to histogram.

`options` : `const HistPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

`features` : `pluss::RangeView<std::string_view>`
    Several numeric features to histogram in the same figure.

#### Output

`pluss::pl::Studio`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Plot every compatible numeric feature as a histogram with a shared bin count.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.hist("sales", 12);
```

##### Expected output

```text
Figure(hist)
```
Example 2. Plot one numeric feature as a histogram with an explicit bin count.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.hist("sales", 12);
```

##### Expected output

```text
Figure(hist)
```
Example 3. Plot one numeric feature as a histogram with an explicit histogram-options object.

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

##### Expected output

```text
Figure(hist)
```
Example 4. Plot several numeric features from a reusable string view as histograms with an explicit shared bin count.

```cpp
auto df = pluss::table::DataFrame({
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

auto fig = plot.hist({"sales", "cost"}, 12);
```

##### Expected output

```text
Figure(hist)
```
Example 5. Plot several numeric features from a reusable string view as histograms with explicit histogram options.

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

##### Expected output

```text
Figure(hist)
```
Example 6. Plot several numeric features listed inline as histograms with an explicit shared bin count.

```cpp
auto df = pluss::table::DataFrame({
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

auto fig = plot.hist({"sales", "cost"}, 12);
```

##### Expected output

```text
Figure(hist)
```
Example 7. Plot several numeric features listed inline as histograms with explicit histogram options.

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

##### Expected output

```text
Figure(hist)
```
Example 8. Plot every compatible numeric feature as a histogram with an explicit histogram-options object.

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

##### Expected output

```text
Figure(hist)
```

#### Practical Behavior

- `hist(...)` bins numeric values into frequency bars.
- With several features, the selected features are rendered together in one figure for quick distribution comparison.
- Use the options overload when you want explicit visual controls instead of just the bin count.


## L

### `lag`

#### API Signature

```cpp
DataFramePlotAccessor::lag(feature: std::string_view, options: const LagPlotOptions& = {}) -> pluss::pl::Studio
```

#### Notes

Builds a lag plot that compares one numeric feature against a shifted copy of itself.

#### Inputs

`feature` : `std::string_view`
    Numeric feature whose lag relationship should be plotted against itself.

`options` : `const LagPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`pluss::pl::Studio`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

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

##### Expected output

```text
Figure(lag)
```

#### Practical Behavior

- `lag(feature, options)` compares one numeric feature against a lagged copy of itself.
- Increase `LagPlotOptions::lag` when you want to inspect longer-gap autocorrelation structure.
- This is useful for quick time-dependence diagnostics before formal modeling.


### `line`

#### API Signatures

```cpp
DataFramePlotAccessor::line(y_feature: std::string_view, x_feature: std::optional<std::string_view> = std::nullopt) -> pluss::pl::Studio

DataFramePlotAccessor::line(y_feature: std::string_view, x_feature: std::optional<std::string_view>, options: const LinePlotOptions&) -> pluss::pl::Studio

DataFramePlotAccessor::line(y_features: pluss::RangeView<std::string_view>, x_feature: std::optional<std::string_view> = std::nullopt) -> pluss::pl::Studio

DataFramePlotAccessor::line(y_features: pluss::RangeView<std::string_view>, x_feature: std::optional<std::string_view>, options: const LinePlotOptions&) -> pluss::pl::Studio

DataFramePlotAccessor::line(y_features: std::initializer_list<std::string_view>, x_feature: std::optional<std::string_view> = std::nullopt) -> pluss::pl::Studio

DataFramePlotAccessor::line(y_features: std::initializer_list<std::string_view>, x_feature: std::optional<std::string_view>, options: const LinePlotOptions&) -> pluss::pl::Studio
```

#### Notes

Builds a line chart for one or more y-features, optionally against a chosen x-feature.

#### Inputs

`y_feature` : `std::string_view`
    One numeric feature to draw as a single line trace.

`x_feature` : `std::optional<std::string_view>`
    Feature used on the horizontal axis of a plot. Leave it empty to let the plot use the case axis or the accessor default when that overload allows it.

`options` : `const LinePlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

`y_features` : `pluss::RangeView<std::string_view>`
    Numeric features to overlay as separate line traces on the same axes.

#### Output

`pluss::pl::Studio`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Plot one y-feature as a line chart.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.line("sales");
```

##### Expected output

```text
Figure(line)
```
Example 2. Plot one y-feature as a line chart with an explicit plot-options object.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.line("sales");
```

##### Expected output

```text
Figure(line)
```
Example 3. Plot several y-features from a reusable string view as a shared line chart.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.line("sales");
```

##### Expected output

```text
Figure(line)
```
Example 4. Plot several y-features from a reusable string view as a shared line chart with explicit plot options.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.line("sales");
```

##### Expected output

```text
Figure(line)
```
Example 5. Plot several y-features listed inline as a shared line chart.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.line({"sales"}, std::nullopt);
```

##### Expected output

```text
Figure(line)
```
Example 6. Plot several y-features listed inline as a shared line chart with explicit plot options.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.line({"sales"}, std::nullopt);
```

##### Expected output

```text
Figure(line)
```

#### Practical Behavior

- `line(...)` draws one continuous trace per selected y-feature.
- Pass `x_feature` when the horizontal axis should come from a real feature instead of the case axis.
- Use the options overload when you want to tune line alpha, width, or y-limit policy.


## P

### `parallel`

#### API Signature

```cpp
DataFramePlotAccessor::parallel(features: pluss::RangeView<std::string_view> = {}, class_feature: std::optional<std::string_view> = std::nullopt, options: const ParallelPlotOptions& = {}) -> pluss::pl::Studio
```

#### Notes

Builds a parallel-coordinates plot from several numeric features, optionally grouped by a class feature.

#### Inputs

`features` : `pluss::RangeView<std::string_view>`
    Numeric features to place on the parallel-coordinate axes. Leave empty to let the accessor infer them.

`class_feature` : `std::optional<std::string_view>`
    Optional grouping or class label feature used to color, partition, or annotate multivariate plots.

`options` : `const ParallelPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`pluss::pl::Studio`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

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

##### Expected output

```text
Figure(parallel)
```

#### Practical Behavior

- `parallel(...)` expects numeric features and draws one polyline per row across several aligned axes.
- Use `class_feature` when rows should be colored by a grouping label.
- This is useful for comparing multivariate profiles row by row.


### `pie`

#### API Signatures

```cpp
DataFramePlotAccessor::pie(value_feature: std::string_view, label_feature: std::optional<std::string_view> = std::nullopt) -> pluss::pl::Studio

DataFramePlotAccessor::pie(value_feature: std::string_view, label_feature: std::optional<std::string_view>, options: const PiePlotOptions&) -> pluss::pl::Studio
```

#### Notes

Builds a pie chart from one value feature, optionally using another feature for labels.

#### Inputs

`value_feature` : `std::string_view`
    Feature whose values determine each slice magnitude.

`label_feature` : `std::optional<std::string_view>`
    Optional feature that supplies the text label for each slice.

`options` : `const PiePlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`pluss::pl::Studio`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

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

##### Expected output

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

##### Expected output

```text
Figure(pie)
```

#### Practical Behavior

- `pie(value_feature, label_feature, ...)` uses one row per slice.
- The value feature determines slice magnitude, and the label feature controls the text shown for each slice.
- This is best for small categorical summaries rather than large row counts.


## R

### `radviz`

#### API Signature

```cpp
DataFramePlotAccessor::radviz(features: pluss::RangeView<std::string_view> = {}, class_feature: std::optional<std::string_view> = std::nullopt, options: const RadvizPlotOptions& = {}) -> pluss::pl::Studio
```

#### Notes

Builds a radial visualization from several numeric features, optionally grouped by a class feature.

#### Inputs

`features` : `pluss::RangeView<std::string_view>`
    Numeric features used as anchors in the radial visualization.

`class_feature` : `std::optional<std::string_view>`
    Optional grouping or class label feature used to color, partition, or annotate multivariate plots.

`options` : `const RadvizPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`pluss::pl::Studio`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

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

##### Expected output

```text
Figure(radviz)
```

#### Practical Behavior

- `radviz(...)` expects numeric features and projects each row toward feature anchors around a circle.
- Use `class_feature` when you want point colors to reflect group labels.
- This is useful for quick multivariate separation checks between classes.


## S

### `scatter`

#### API Signatures

```cpp
DataFramePlotAccessor::scatter(x_feature: std::string_view, y_feature: std::string_view, hue_feature: std::optional<std::string_view> = std::nullopt) -> pluss::pl::Studio

DataFramePlotAccessor::scatter(x_feature: std::string_view, y_feature: std::string_view, hue_feature: std::optional<std::string_view>, options: const ScatterPlotOptions&) -> pluss::pl::Studio
```

#### Notes

Builds a scatter plot using one feature for x and another for y.

#### Inputs

`x_feature` : `std::string_view`
    Numeric or ordered feature placed on the horizontal axis.

`y_feature` : `std::string_view`
    Numeric feature placed on the vertical axis.

`hue_feature` : `std::optional<std::string_view>`
    Optional feature whose distinct values split one plot into color-coded groups.

`options` : `const ScatterPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`pluss::pl::Studio`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

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

##### Expected output

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

##### Expected output

```text
Figure(scatter)
```

#### Practical Behavior

- `scatter(x_feature, y_feature, ...)` plots one point per row.
- Use `hue_feature` when rows should be color-grouped by a categorical label.
- This method is best when the relationship between two measures matters more than their order.


### `scatter_matrix`

#### API Signatures

```cpp
DataFramePlotAccessor::scatter_matrix(features: pluss::RangeView<std::string_view> = {}) -> pluss::pl::Studio

DataFramePlotAccessor::scatter_matrix(features: pluss::RangeView<std::string_view>, options: const ScatterMatrixPlotOptions&) -> pluss::pl::Studio

DataFramePlotAccessor::scatter_matrix(features: pluss::RangeView<std::string_view>, triangle: ScatterMatrixTriangle) -> pluss::pl::Studio
```

#### Notes

Builds a scatter-matrix figure for several numeric features.

#### Inputs

`features` : `pluss::RangeView<std::string_view>`
    Numeric features to place on the pairwise matrix axes. Leave empty to let the accessor infer them.

`options` : `const ScatterMatrixPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

`triangle` : `ScatterMatrixTriangle`
    Pair-plot triangle selection controlling whether the scatter matrix renders the upper, lower, or corner-only half.

#### Output

`pluss::pl::Studio`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

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

##### Expected output

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

##### Expected output

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

##### Expected output

```text
Figure(scatter_matrix)
```

#### Practical Behavior

- `scatter_matrix(...)` creates pairwise scatter panels for the selected numeric features.
- Use the `triangle` overload when you want only the upper, lower, or corner half of the matrix.
- Use the options overload when you want finer pair-plot rendering control.


### `step`

#### API Signatures

```cpp
DataFramePlotAccessor::step(y_feature: std::string_view, x_feature: std::optional<std::string_view> = std::nullopt) -> pluss::pl::Studio

DataFramePlotAccessor::step(y_feature: std::string_view, x_feature: std::optional<std::string_view>, options: const StepPlotOptions&) -> pluss::pl::Studio

DataFramePlotAccessor::step(y_features: pluss::RangeView<std::string_view>, x_feature: std::optional<std::string_view> = std::nullopt) -> pluss::pl::Studio

DataFramePlotAccessor::step(y_features: pluss::RangeView<std::string_view>, x_feature: std::optional<std::string_view>, options: const StepPlotOptions&) -> pluss::pl::Studio

DataFramePlotAccessor::step(y_features: std::initializer_list<std::string_view>, x_feature: std::optional<std::string_view> = std::nullopt) -> pluss::pl::Studio

DataFramePlotAccessor::step(y_features: std::initializer_list<std::string_view>, x_feature: std::optional<std::string_view>, options: const StepPlotOptions&) -> pluss::pl::Studio
```

#### Notes

Builds a step chart for one or more y-features, optionally against a chosen x-feature.

#### Inputs

`y_feature` : `std::string_view`
    One numeric feature to draw as a step trace.

`x_feature` : `std::optional<std::string_view>`
    Feature used on the horizontal axis of a plot. Leave it empty to let the plot use the case axis or the accessor default when that overload allows it.

`options` : `const StepPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

`y_features` : `pluss::RangeView<std::string_view>`
    Numeric features to overlay as separate step traces on the same axes.

#### Output

`pluss::pl::Studio`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Plot one or more y-features as a step chart.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.step("sales", "id");
```

##### Expected output

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

##### Expected output

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

##### Expected output

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

##### Expected output

```text
Figure(step)
```
Example 5. Plot one or more y-features as a step chart.

```cpp
auto df = pluss::table::DataFrame({
    {"month", pluss::make_note({"Jan", "Feb", "Mar"})},
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

auto fig = plot.step({"sales", "cost"}, "month");
```

##### Expected output

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

##### Expected output

```text
Figure(step)
```

#### Practical Behavior

- `step(...)` uses the same feature selection pattern as `line(...)`, but the trace changes by steps instead of straight segments.
- This is useful when the y-value should be interpreted as holding until the next x position.
- If `x_feature` is omitted, the case axis is used.


## V

### `violin`

#### API Signatures

```cpp
DataFramePlotAccessor::violin(features: pluss::RangeView<std::string_view> = {}) -> pluss::pl::Studio

DataFramePlotAccessor::violin(features: pluss::RangeView<std::string_view>, options: const ViolinPlotOptions&) -> pluss::pl::Studio

DataFramePlotAccessor::violin(features: std::initializer_list<std::string_view>) -> pluss::pl::Studio

DataFramePlotAccessor::violin(features: std::initializer_list<std::string_view>, options: const ViolinPlotOptions&) -> pluss::pl::Studio

DataFramePlotAccessor::violin(value_feature: std::string_view, group_feature: std::string_view) -> pluss::pl::Studio

DataFramePlotAccessor::violin(value_feature: std::string_view, group_feature: std::string_view, options: const ViolinPlotOptions&) -> pluss::pl::Studio
```

#### Notes

Builds violin plots for one or more numeric features or grouped distributions.

#### Inputs

`features` : `pluss::RangeView<std::string_view>`
    Numeric features to summarize as violin plots. Leave empty to let the accessor infer the numeric feature set.

`options` : `const ViolinPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

`value_feature` : `std::string_view`
    Numeric feature whose distribution is shown inside each violin.

`group_feature` : `std::string_view`
    Categorical feature that splits the violin plot into separate groups.

#### Output

`pluss::pl::Studio`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

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

##### Expected output

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

##### Expected output

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

##### Expected output

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

##### Expected output

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

##### Expected output

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

##### Expected output

```text
Figure(violin)
```

#### Practical Behavior

- The feature-list overload renders one violin per numeric feature.
- The `(value_feature, group_feature)` overload renders one violin per group in the grouping feature.
- Use this when you want full distribution shape rather than only quartiles or whiskers.

