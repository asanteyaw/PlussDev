# DataFramePlotAccessor API Reference

# Practical DataFramePlotAccessor API Reference

## API Reference Style

This page documents the public plotting accessor returned by `DataFrame::plot()` and `Atlas::plot()`.

```{include} snippets/dataframe_plot_options_reference.md
```

### `act`

#### Signature

```cpp
pluss::pl::Studio act(self,  std::string_view feature, const ActPlotOptions& options = {}) const
```

#### Parameters

`feature` : `std::string_view`
    Name of the source feature used by the operation.

`options` : `const ActPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `act` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

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

#### Output

```text
Figure(act)
```

#### Practical Behavior

- `act(feature, options)` requires one numeric feature.
- Use it when you want autocorrelation-style time-lag structure for one series.


### `andrews_curves`

#### Signature

```cpp
pluss::pl::Studio andrews_curves(self,  pluss::msharp::RangeView::Of<std::string_view> features = {}, std::optional<std::string_view> class_feature = std::nullopt, const AndrewsCurvesPlotOptions& options = {}) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`class_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

`options` : `const AndrewsCurvesPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `andrews_curves` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

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

#### Output

```text
Figure(andrews_curves)
```

#### Practical Behavior

- `andrews_curves(...)` works on numeric features and can optionally color/group by a class feature.
- If you do not supply labels in the options, the feature names are used.


### `area`

#### Signature

```cpp
pluss::pl::Studio area(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const
```

#### Parameters

`y_features` : `pluss::msharp::RangeView::Of<std::string_view>`
    See the signature for the supported argument form.

`x_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `area` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio area(self, std::string_view y_feature, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio area(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio area(self, std::string_view y_feature, std::optional<std::string_view> x_feature, const AreaPlotOptions& options) const`
- `pluss::pl::Studio area(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature, const AreaPlotOptions& options) const`
- `pluss::pl::Studio area(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const AreaPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"month", pluss::make_note({"Jan", "Feb", "Mar"})},
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

auto fig = plot.area({"sales", "cost"}, "month");
```

#### Output

```text
Figure(area)
```

#### Practical Behavior

- One y-feature creates one filled area trace.
- Many y-features create one area trace per selected feature over the same x-axis.
- If `x_feature` is omitted, the case axis is used.


### `area`

#### Signature

```cpp
pluss::pl::Studio area(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature, const AreaPlotOptions& options) const
```

#### Parameters

`y_features` : `pluss::msharp::RangeView::Of<std::string_view>`
    See the signature for the supported argument form.

`x_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

`options` : `const AreaPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `area` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio area(self, std::string_view y_feature, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio area(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio area(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio area(self, std::string_view y_feature, std::optional<std::string_view> x_feature, const AreaPlotOptions& options) const`
- `pluss::pl::Studio area(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const AreaPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"month", pluss::make_note({"Jan", "Feb", "Mar"})},
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

auto fig = plot.area({"sales", "cost"}, "month");
```

#### Output

```text
Figure(area)
```

#### Practical Behavior

- One y-feature creates one filled area trace.
- Many y-features create one area trace per selected feature over the same x-axis.
- If `x_feature` is omitted, the case axis is used.


### `area`

#### Signature

```cpp
pluss::pl::Studio area(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const
```

#### Parameters

`y_features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

`x_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `area` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio area(self, std::string_view y_feature, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio area(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio area(self, std::string_view y_feature, std::optional<std::string_view> x_feature, const AreaPlotOptions& options) const`
- `pluss::pl::Studio area(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature, const AreaPlotOptions& options) const`
- `pluss::pl::Studio area(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const AreaPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"month", pluss::make_note({"Jan", "Feb", "Mar"})},
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

auto fig = plot.area({"sales", "cost"}, "month");
```

#### Output

```text
Figure(area)
```

#### Practical Behavior

- One y-feature creates one filled area trace.
- Many y-features create one area trace per selected feature over the same x-axis.
- If `x_feature` is omitted, the case axis is used.


### `area`

#### Signature

```cpp
pluss::pl::Studio area(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const AreaPlotOptions& options) const
```

#### Parameters

`y_features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

`x_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

`options` : `const AreaPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `area` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio area(self, std::string_view y_feature, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio area(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio area(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio area(self, std::string_view y_feature, std::optional<std::string_view> x_feature, const AreaPlotOptions& options) const`
- `pluss::pl::Studio area(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature, const AreaPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"month", pluss::make_note({"Jan", "Feb", "Mar"})},
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

auto fig = plot.area({"sales", "cost"}, "month");
```

#### Output

```text
Figure(area)
```

#### Practical Behavior

- One y-feature creates one filled area trace.
- Many y-features create one area trace per selected feature over the same x-axis.
- If `x_feature` is omitted, the case axis is used.


### `area`

#### Signature

```cpp
pluss::pl::Studio area(self, std::string_view y_feature, std::optional<std::string_view> x_feature = std::nullopt) const
```

#### Parameters

`y_feature` : `std::string_view`
    See the signature for the supported argument form.

`x_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `area` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio area(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio area(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio area(self, std::string_view y_feature, std::optional<std::string_view> x_feature, const AreaPlotOptions& options) const`
- `pluss::pl::Studio area(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature, const AreaPlotOptions& options) const`
- `pluss::pl::Studio area(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const AreaPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.area("sales", "id");
```

#### Output

```text
Figure(area)
```

#### Practical Behavior

- One y-feature creates one filled area trace.
- Many y-features create one area trace per selected feature over the same x-axis.
- If `x_feature` is omitted, the case axis is used.


### `area`

#### Signature

```cpp
pluss::pl::Studio area(self, std::string_view y_feature, std::optional<std::string_view> x_feature, const AreaPlotOptions& options) const
```

#### Parameters

`y_feature` : `std::string_view`
    See the signature for the supported argument form.

`x_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

`options` : `const AreaPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `area` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio area(self, std::string_view y_feature, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio area(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio area(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio area(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature, const AreaPlotOptions& options) const`
- `pluss::pl::Studio area(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const AreaPlotOptions& options) const`

#### Examples

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

#### Output

```text
Figure(area)
```

#### Practical Behavior

- One y-feature creates one filled area trace.
- Many y-features create one area trace per selected feature over the same x-axis.
- If `x_feature` is omitted, the case axis is used.


### `area_stacked`

#### Signature

```cpp
pluss::pl::Studio area_stacked(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const
```

#### Parameters

`y_features` : `pluss::msharp::RangeView::Of<std::string_view>`
    See the signature for the supported argument form.

`x_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `area_stacked` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio area_stacked(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio area_stacked(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature, const AreaStackedPlotOptions& options) const`
- `pluss::pl::Studio area_stacked(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const AreaStackedPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.area_stacked();
```

#### Output

```text
Figure(area_stacked)
```

#### Practical Behavior

- `area_stacked(...)` requires at least one y-feature.
- Each later y-feature is stacked on top of the running baseline from the earlier ones.
- This is useful for contribution-over-total views across the same x positions.


### `area_stacked`

#### Signature

```cpp
pluss::pl::Studio area_stacked(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature, const AreaStackedPlotOptions& options) const
```

#### Parameters

`y_features` : `pluss::msharp::RangeView::Of<std::string_view>`
    See the signature for the supported argument form.

`x_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

`options` : `const AreaStackedPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `area_stacked` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio area_stacked(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio area_stacked(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio area_stacked(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const AreaStackedPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.area_stacked();
```

#### Output

```text
Figure(area_stacked)
```

#### Practical Behavior

- `area_stacked(...)` requires at least one y-feature.
- Each later y-feature is stacked on top of the running baseline from the earlier ones.
- This is useful for contribution-over-total views across the same x positions.


### `area_stacked`

#### Signature

```cpp
pluss::pl::Studio area_stacked(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const
```

#### Parameters

`y_features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

`x_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `area_stacked` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio area_stacked(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio area_stacked(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature, const AreaStackedPlotOptions& options) const`
- `pluss::pl::Studio area_stacked(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const AreaStackedPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.area_stacked();
```

#### Output

```text
Figure(area_stacked)
```

#### Practical Behavior

- `area_stacked(...)` requires at least one y-feature.
- Each later y-feature is stacked on top of the running baseline from the earlier ones.
- This is useful for contribution-over-total views across the same x positions.


### `area_stacked`

#### Signature

```cpp
pluss::pl::Studio area_stacked(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const AreaStackedPlotOptions& options) const
```

#### Parameters

`y_features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

`x_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

`options` : `const AreaStackedPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `area_stacked` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio area_stacked(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio area_stacked(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio area_stacked(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature, const AreaStackedPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.area_stacked();
```

#### Output

```text
Figure(area_stacked)
```

#### Practical Behavior

- `area_stacked(...)` requires at least one y-feature.
- Each later y-feature is stacked on top of the running baseline from the earlier ones.
- This is useful for contribution-over-total views across the same x positions.


### `auto_plot`

#### Signature

```cpp
pluss::pl::Studio auto_plot(self, pluss::msharp::RangeView::Of<std::string_view> features = {}) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `auto_plot` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio auto_plot(self, pluss::msharp::RangeView::Of<std::string_view> features, const AutoPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"student", pluss::make_note({"Laurel", "Kojo", "Ama"})},
    {"midterm", pluss::make_series({71.0, 82.0, 93.0})},
    {"final", pluss::make_series({88.0, 79.0, 95.0})},
});
auto plot = df.plot();

auto fig = plot.auto_plot({"midterm", "final"});
```

#### Output

```text
Figure(auto_selected)
```

#### Selection Rules

- `auto_plot(...)` chooses a plot family based on feature count, row count, case labels, and sign patterns.
- Examples include bar for one labeled feature, stacked bar/area for a few nonnegative features, scatter matrix for small numeric sets, and correlation heatmap for larger ones.
- Use it for quick inspection when you do not want to choose the plot form manually.


### `auto_plot`

#### Signature

```cpp
pluss::pl::Studio auto_plot(self, pluss::msharp::RangeView::Of<std::string_view> features, const AutoPlotOptions& options) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`options` : `const AutoPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `auto_plot` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio auto_plot(self, pluss::msharp::RangeView::Of<std::string_view> features = {}) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"student", pluss::make_note({"Laurel", "Kojo", "Ama"})},
    {"midterm", pluss::make_series({71.0, 82.0, 93.0})},
    {"final", pluss::make_series({88.0, 79.0, 95.0})},
});
auto plot = df.plot();

auto fig = plot.auto_plot({"midterm", "final"});
```

#### Output

```text
Figure(auto_selected)
```

#### Selection Rules

- `auto_plot(...)` chooses a plot family based on feature count, row count, case labels, and sign patterns.
- Examples include bar for one labeled feature, stacked bar/area for a few nonnegative features, scatter matrix for small numeric sets, and correlation heatmap for larger ones.
- Use it for quick inspection when you do not want to choose the plot form manually.


### `bar`

#### Signature

```cpp
pluss::pl::Studio bar(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const
```

#### Parameters

`y_features` : `pluss::msharp::RangeView::Of<std::string_view>`
    See the signature for the supported argument form.

`x_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Builds a bar chart from one feature, optionally using another feature as the x-axis.

#### See also

- `pluss::pl::Studio bar(self, std::string_view y_feature, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio bar(self, std::string_view y_feature, std::optional<std::string_view> x_feature, const BarPlotOptions& options) const`
- `pluss::pl::Studio bar(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio bar(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature, const BarPlotOptions& options) const`
- `pluss::pl::Studio bar(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const BarPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"student", pluss::make_note({"Laurel", "Kojo", "Ama"})},
    {"midterm", pluss::make_series({71.0, 82.0, 93.0})},
    {"final", pluss::make_series({88.0, 79.0, 95.0})},
});
auto plot = df.plot();

auto fig = plot.bar({"midterm", "final"}, "student");
```

#### Output

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


### `bar`

#### Signature

```cpp
pluss::pl::Studio bar(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature, const BarPlotOptions& options) const
```

#### Parameters

`y_features` : `pluss::msharp::RangeView::Of<std::string_view>`
    See the signature for the supported argument form.

`x_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

`options` : `const BarPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Builds a bar chart from one feature, optionally using another feature as the x-axis.

#### See also

- `pluss::pl::Studio bar(self, std::string_view y_feature, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio bar(self, std::string_view y_feature, std::optional<std::string_view> x_feature, const BarPlotOptions& options) const`
- `pluss::pl::Studio bar(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio bar(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio bar(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const BarPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"student", pluss::make_note({"Laurel", "Kojo", "Ama"})},
    {"midterm", pluss::make_series({71.0, 82.0, 93.0})},
    {"final", pluss::make_series({88.0, 79.0, 95.0})},
});
auto plot = df.plot();

auto fig = plot.bar({"midterm", "final"}, "student");
```

#### Output

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


### `bar`

#### Signature

```cpp
pluss::pl::Studio bar(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const
```

#### Parameters

`y_features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

`x_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Builds a bar chart from one feature, optionally using another feature as the x-axis.

#### See also

- `pluss::pl::Studio bar(self, std::string_view y_feature, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio bar(self, std::string_view y_feature, std::optional<std::string_view> x_feature, const BarPlotOptions& options) const`
- `pluss::pl::Studio bar(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio bar(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature, const BarPlotOptions& options) const`
- `pluss::pl::Studio bar(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const BarPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"student", pluss::make_note({"Laurel", "Kojo", "Ama"})},
    {"midterm", pluss::make_series({71.0, 82.0, 93.0})},
    {"final", pluss::make_series({88.0, 79.0, 95.0})},
});
auto plot = df.plot();

auto fig = plot.bar({"midterm", "final"}, "student");
```

#### Output

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


### `bar`

#### Signature

```cpp
pluss::pl::Studio bar(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const BarPlotOptions& options) const
```

#### Parameters

`y_features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

`x_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

`options` : `const BarPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Builds a bar chart from one feature, optionally using another feature as the x-axis.

#### See also

- `pluss::pl::Studio bar(self, std::string_view y_feature, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio bar(self, std::string_view y_feature, std::optional<std::string_view> x_feature, const BarPlotOptions& options) const`
- `pluss::pl::Studio bar(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio bar(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio bar(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature, const BarPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"student", pluss::make_note({"Laurel", "Kojo", "Ama"})},
    {"midterm", pluss::make_series({71.0, 82.0, 93.0})},
    {"final", pluss::make_series({88.0, 79.0, 95.0})},
});
auto plot = df.plot();

auto fig = plot.bar({"midterm", "final"}, "student");
```

#### Output

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


### `bar`

#### Signature

```cpp
pluss::pl::Studio bar(self, std::string_view y_feature, std::optional<std::string_view> x_feature = std::nullopt) const
```

#### Parameters

`y_feature` : `std::string_view`
    See the signature for the supported argument form.

`x_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Builds a bar chart from one feature, optionally using another feature as the x-axis.

#### See also

- `pluss::pl::Studio bar(self, std::string_view y_feature, std::optional<std::string_view> x_feature, const BarPlotOptions& options) const`
- `pluss::pl::Studio bar(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio bar(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio bar(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature, const BarPlotOptions& options) const`
- `pluss::pl::Studio bar(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const BarPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.bar("sales", "city");
```

#### Output

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


### `bar`

#### Signature

```cpp
pluss::pl::Studio bar(self, std::string_view y_feature, std::optional<std::string_view> x_feature, const BarPlotOptions& options) const
```

#### Parameters

`y_feature` : `std::string_view`
    See the signature for the supported argument form.

`x_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

`options` : `const BarPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Builds a bar chart from one feature, optionally using another feature as the x-axis.

#### See also

- `pluss::pl::Studio bar(self, std::string_view y_feature, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio bar(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio bar(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio bar(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature, const BarPlotOptions& options) const`
- `pluss::pl::Studio bar(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const BarPlotOptions& options) const`

#### Examples

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

#### Output

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

#### Signature

```cpp
pluss::pl::Studio bar_stacked(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const
```

#### Parameters

`y_features` : `pluss::msharp::RangeView::Of<std::string_view>`
    See the signature for the supported argument form.

`x_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Builds stacked bars so multiple y-features accumulate on the same axis positions.

#### See also

- `pluss::pl::Studio bar_stacked(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio bar_stacked(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature, const BarStackedPlotOptions& options) const`
- `pluss::pl::Studio bar_stacked(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const BarStackedPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"student", pluss::make_note({"Laurel", "Kojo", "Ama"})},
    {"midterm", pluss::make_series({71.0, 82.0, 93.0})},
    {"final", pluss::make_series({88.0, 79.0, 95.0})},
});
auto plot = df.plot();

auto fig = plot.bar_stacked({"midterm", "final"}, "student");
```

#### Output

```text
Figure(bar_stacked)
```

#### Practical Behavior

- `bar_stacked(...)` requires at least one y-feature.
- Each later y-feature is stacked on top of the earlier ones at the same x-axis positions.
- This is useful for composition views where the total height and each component both matter.
- If `x_feature` is omitted, the case axis is used.


### `bar_stacked`

#### Signature

```cpp
pluss::pl::Studio bar_stacked(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature, const BarStackedPlotOptions& options) const
```

#### Parameters

`y_features` : `pluss::msharp::RangeView::Of<std::string_view>`
    See the signature for the supported argument form.

`x_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

`options` : `const BarStackedPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Builds stacked bars so multiple y-features accumulate on the same axis positions.

#### See also

- `pluss::pl::Studio bar_stacked(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio bar_stacked(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio bar_stacked(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const BarStackedPlotOptions& options) const`

#### Examples

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

#### Output

```text
Figure(bar_stacked)
```

#### Practical Behavior

- `bar_stacked(...)` requires at least one y-feature.
- Each later y-feature is stacked on top of the earlier ones at the same x-axis positions.
- This is useful for composition views where the total height and each component both matter.
- If `x_feature` is omitted, the case axis is used.


### `bar_stacked`

#### Signature

```cpp
pluss::pl::Studio bar_stacked(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const
```

#### Parameters

`y_features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

`x_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Builds stacked bars so multiple y-features accumulate on the same axis positions.

#### See also

- `pluss::pl::Studio bar_stacked(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio bar_stacked(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature, const BarStackedPlotOptions& options) const`
- `pluss::pl::Studio bar_stacked(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const BarStackedPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"student", pluss::make_note({"Laurel", "Kojo", "Ama"})},
    {"midterm", pluss::make_series({71.0, 82.0, 93.0})},
    {"final", pluss::make_series({88.0, 79.0, 95.0})},
});
auto plot = df.plot();

auto fig = plot.bar_stacked({"midterm", "final"}, "student");
```

#### Output

```text
Figure(bar_stacked)
```

#### Practical Behavior

- `bar_stacked(...)` requires at least one y-feature.
- Each later y-feature is stacked on top of the earlier ones at the same x-axis positions.
- This is useful for composition views where the total height and each component both matter.
- If `x_feature` is omitted, the case axis is used.


### `bar_stacked`

#### Signature

```cpp
pluss::pl::Studio bar_stacked(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const BarStackedPlotOptions& options) const
```

#### Parameters

`y_features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

`x_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

`options` : `const BarStackedPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Builds stacked bars so multiple y-features accumulate on the same axis positions.

#### See also

- `pluss::pl::Studio bar_stacked(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio bar_stacked(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio bar_stacked(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature, const BarStackedPlotOptions& options) const`

#### Examples

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

#### Output

```text
Figure(bar_stacked)
```

#### Practical Behavior

- `bar_stacked(...)` requires at least one y-feature.
- Each later y-feature is stacked on top of the earlier ones at the same x-axis positions.
- This is useful for composition views where the total height and each component both matter.
- If `x_feature` is omitted, the case axis is used.


### `bootstrap`

#### Signature

```cpp
pluss::pl::Studio bootstrap(self,  std::string_view feature, const BootstrapPlotOptions& options = {}) const
```

#### Parameters

`feature` : `std::string_view`
    Name of the source feature used by the operation.

`options` : `const BootstrapPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Builds a bootstrap summary plot for one numeric feature using the supplied resampling options.

#### Examples

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

#### Output

```text
Figure(bootstrap)
```

#### Practical Behavior

- `bootstrap(feature, options)` requires one numeric feature.
- The plot is driven by resampling options such as `samples`, `sample_size`, `seed`, and the chosen statistic.
- This is the method to use when you want sampling-based uncertainty or distribution shape around a summary statistic.


### `box`

#### Signature

```cpp
pluss::pl::Studio box(self, pluss::msharp::RangeView::Of<std::string_view> features = {}) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Builds a box plot for one or more numeric features.

#### See also

- `pluss::pl::Studio box(self, std::initializer_list<std::string_view> features) const`
- `pluss::pl::Studio box(self, pluss::msharp::RangeView::Of<std::string_view> features, const BoxPlotOptions& options) const`
- `pluss::pl::Studio box(self, std::initializer_list<std::string_view> features, const BoxPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

auto fig = plot.box({"sales", "cost"});
```

#### Output

```text
Figure(box)
```

#### Practical Behavior

- `box(...)` accepts numeric features only.
- With no explicit feature list, the accessor resolves the available numeric features for you.
- Use this when you want a distribution summary instead of a per-row plot.


### `box`

#### Signature

```cpp
pluss::pl::Studio box(self, pluss::msharp::RangeView::Of<std::string_view> features, const BoxPlotOptions& options) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`options` : `const BoxPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Builds a box plot for one or more numeric features.

#### See also

- `pluss::pl::Studio box(self, pluss::msharp::RangeView::Of<std::string_view> features = {}) const`
- `pluss::pl::Studio box(self, std::initializer_list<std::string_view> features) const`
- `pluss::pl::Studio box(self, std::initializer_list<std::string_view> features, const BoxPlotOptions& options) const`

#### Examples

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

#### Output

```text
Figure(box)
```

#### Practical Behavior

- `box(...)` accepts numeric features only.
- With no explicit feature list, the accessor resolves the available numeric features for you.
- Use this when you want a distribution summary instead of a per-row plot.


### `box`

#### Signature

```cpp
pluss::pl::Studio box(self, std::initializer_list<std::string_view> features) const
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Builds a box plot for one or more numeric features.

#### See also

- `pluss::pl::Studio box(self, pluss::msharp::RangeView::Of<std::string_view> features = {}) const`
- `pluss::pl::Studio box(self, pluss::msharp::RangeView::Of<std::string_view> features, const BoxPlotOptions& options) const`
- `pluss::pl::Studio box(self, std::initializer_list<std::string_view> features, const BoxPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

auto fig = plot.box({"sales", "cost"});
```

#### Output

```text
Figure(box)
```

#### Practical Behavior

- `box(...)` accepts numeric features only.
- With no explicit feature list, the accessor resolves the available numeric features for you.
- Use this when you want a distribution summary instead of a per-row plot.


### `box`

#### Signature

```cpp
pluss::pl::Studio box(self, std::initializer_list<std::string_view> features, const BoxPlotOptions& options) const
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`options` : `const BoxPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Builds a box plot for one or more numeric features.

#### See also

- `pluss::pl::Studio box(self, pluss::msharp::RangeView::Of<std::string_view> features = {}) const`
- `pluss::pl::Studio box(self, std::initializer_list<std::string_view> features) const`
- `pluss::pl::Studio box(self, pluss::msharp::RangeView::Of<std::string_view> features, const BoxPlotOptions& options) const`

#### Examples

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

#### Output

```text
Figure(box)
```

#### Practical Behavior

- `box(...)` accepts numeric features only.
- With no explicit feature list, the accessor resolves the available numeric features for you.
- Use this when you want a distribution summary instead of a per-row plot.


### `brief`

#### Signature

```cpp
pluss::pl::Studio brief(self, pluss::msharp::RangeView::Of<std::string_view> features = {}, const BriefPlotOptions& options = {}) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`options` : `const BriefPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Builds a compact summary figure for one or more numeric features.

#### See also

- `pluss::pl::Studio& brief(self, pluss::pl::Studio& figure, pluss::msharp::RangeView::Of<std::string_view> features = {}, const BriefPlotOptions& options = {}) const`

#### Examples

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

#### Output

```text
Figure(summary)
```

#### Practical Behavior

- `brief(...)` builds a compact summary figure for numeric features.
- `BriefPlotOptions` controls table-style presentation details such as `precision`, `max_cases`, and whether headers are shown.
- Use this when you want a readable figure-based summary rather than a chart trace per row.


### `brief`

#### Signature

```cpp
pluss::pl::Studio& brief(self, pluss::pl::Studio& figure, pluss::msharp::RangeView::Of<std::string_view> features = {}, const BriefPlotOptions& options = {}) const
```

#### Parameters

`figure` : `pluss::pl::Studio&`
    See the signature for the supported argument form.

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`options` : `const BriefPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio&`
    Value produced by this call.

#### Notes

Builds a compact summary figure for one or more numeric features.

#### See also

- `pluss::pl::Studio brief(self, pluss::msharp::RangeView::Of<std::string_view> features = {}, const BriefPlotOptions& options = {}) const`

#### Examples

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

#### Output

```text
Figure(summary)
```

#### Practical Behavior

- This overload writes the summary into an existing figure instead of creating a new one.
- Use it when you want a compact numeric summary beside or under another plot.


### `corr_heatmap`

#### Signature

```cpp
pluss::pl::Studio corr_heatmap(self, pluss::msharp::RangeView::Of<std::string_view> features = {}) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `corr_heatmap` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio corr_heatmap(self, pluss::msharp::RangeView::Of<std::string_view> features, const CorrHeatmapPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"x1", pluss::make_series({1.0, 2.0, 3.0})},
    {"x2", pluss::make_series({2.0, 4.0, 6.0})},
    {"x3", pluss::make_series({3.0, 1.0, 5.0})},
});
auto plot = df.plot();

auto fig = plot.corr_heatmap({"x1", "x2", "x3"});
```

#### Output

```text
Figure(corr_heatmap)
```

#### Practical Behavior

- `corr_heatmap(...)` selects numeric features, computes their correlation matrix, and renders it as a heatmap.
- If you omit the feature list, the accessor resolves the numeric feature set for you.


### `corr_heatmap`

#### Signature

```cpp
pluss::pl::Studio corr_heatmap(self, pluss::msharp::RangeView::Of<std::string_view> features, const CorrHeatmapPlotOptions& options) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`options` : `const CorrHeatmapPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `corr_heatmap` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio corr_heatmap(self, pluss::msharp::RangeView::Of<std::string_view> features = {}) const`

#### Examples

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

#### Output

```text
Figure(corr_heatmap)
```

#### Practical Behavior

- `corr_heatmap(...)` selects numeric features, computes their correlation matrix, and renders it as a heatmap.
- If you omit the feature list, the accessor resolves the numeric feature set for you.


### `density`

#### Signature

```cpp
pluss::pl::Studio density(self, pluss::msharp::RangeView::Of<std::string_view> features, const DensityPlotOptions& options) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`options` : `const DensityPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `density` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio density(self, std::string_view feature, std::int64_t points = 128) const`
- `pluss::pl::Studio density(self, pluss::msharp::RangeView::Of<std::string_view> features, std::int64_t points = 128) const`
- `pluss::pl::Studio density(self, std::initializer_list<std::string_view> features, std::int64_t points = 128) const`
- `pluss::pl::Studio density(self, std::string_view feature, const DensityPlotOptions& options) const`
- `pluss::pl::Studio density(self, std::initializer_list<std::string_view> features, const DensityPlotOptions& options) const`

#### Examples

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

#### Output

```text
Figure(density)
```

#### Practical Behavior

- `density(...)` builds kernel-density traces for one or more numeric features.
- The requested point count is clamped to a minimum of 16.
- With many features, one density trace is drawn per feature on the same pane.


### `density`

#### Signature

```cpp
pluss::pl::Studio density(self, pluss::msharp::RangeView::Of<std::string_view> features, std::int64_t points = 128) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`points` : `std::int64_t`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `density` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio density(self, std::string_view feature, std::int64_t points = 128) const`
- `pluss::pl::Studio density(self, std::initializer_list<std::string_view> features, std::int64_t points = 128) const`
- `pluss::pl::Studio density(self, std::string_view feature, const DensityPlotOptions& options) const`
- `pluss::pl::Studio density(self, pluss::msharp::RangeView::Of<std::string_view> features, const DensityPlotOptions& options) const`
- `pluss::pl::Studio density(self, std::initializer_list<std::string_view> features, const DensityPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

auto fig = plot.density({"sales", "cost"}, 128);
```

#### Output

```text
Figure(density)
```

#### Practical Behavior

- `density(...)` builds kernel-density traces for one or more numeric features.
- The requested point count is clamped to a minimum of 16.
- With many features, one density trace is drawn per feature on the same pane.


### `density`

#### Signature

```cpp
pluss::pl::Studio density(self, std::initializer_list<std::string_view> features, const DensityPlotOptions& options) const
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`options` : `const DensityPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `density` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio density(self, std::string_view feature, std::int64_t points = 128) const`
- `pluss::pl::Studio density(self, pluss::msharp::RangeView::Of<std::string_view> features, std::int64_t points = 128) const`
- `pluss::pl::Studio density(self, std::initializer_list<std::string_view> features, std::int64_t points = 128) const`
- `pluss::pl::Studio density(self, std::string_view feature, const DensityPlotOptions& options) const`
- `pluss::pl::Studio density(self, pluss::msharp::RangeView::Of<std::string_view> features, const DensityPlotOptions& options) const`

#### Examples

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

#### Output

```text
Figure(density)
```

#### Practical Behavior

- `density(...)` builds kernel-density traces for one or more numeric features.
- The requested point count is clamped to a minimum of 16.
- With many features, one density trace is drawn per feature on the same pane.


### `density`

#### Signature

```cpp
pluss::pl::Studio density(self, std::initializer_list<std::string_view> features, std::int64_t points = 128) const
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`points` : `std::int64_t`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `density` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio density(self, std::string_view feature, std::int64_t points = 128) const`
- `pluss::pl::Studio density(self, pluss::msharp::RangeView::Of<std::string_view> features, std::int64_t points = 128) const`
- `pluss::pl::Studio density(self, std::string_view feature, const DensityPlotOptions& options) const`
- `pluss::pl::Studio density(self, pluss::msharp::RangeView::Of<std::string_view> features, const DensityPlotOptions& options) const`
- `pluss::pl::Studio density(self, std::initializer_list<std::string_view> features, const DensityPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"sales", pluss::make_series({100.0, 120.0, 90.0})},
    {"cost", pluss::make_series({60.0, 80.0, 55.0})},
});
auto plot = df.plot();

auto fig = plot.density({"sales", "cost"}, 128);
```

#### Output

```text
Figure(density)
```

#### Practical Behavior

- `density(...)` builds kernel-density traces for one or more numeric features.
- The requested point count is clamped to a minimum of 16.
- With many features, one density trace is drawn per feature on the same pane.


### `density`

#### Signature

```cpp
pluss::pl::Studio density(self, std::string_view feature, const DensityPlotOptions& options) const
```

#### Parameters

`feature` : `std::string_view`
    Name of the source feature used by the operation.

`options` : `const DensityPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `density` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio density(self, std::string_view feature, std::int64_t points = 128) const`
- `pluss::pl::Studio density(self, pluss::msharp::RangeView::Of<std::string_view> features, std::int64_t points = 128) const`
- `pluss::pl::Studio density(self, std::initializer_list<std::string_view> features, std::int64_t points = 128) const`
- `pluss::pl::Studio density(self, pluss::msharp::RangeView::Of<std::string_view> features, const DensityPlotOptions& options) const`
- `pluss::pl::Studio density(self, std::initializer_list<std::string_view> features, const DensityPlotOptions& options) const`

#### Examples

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

#### Output

```text
Figure(density)
```

#### Practical Behavior

- `density(...)` builds kernel-density traces for one or more numeric features.
- The requested point count is clamped to a minimum of 16.
- With many features, one density trace is drawn per feature on the same pane.


### `density`

#### Signature

```cpp
pluss::pl::Studio density(self, std::string_view feature, std::int64_t points = 128) const
```

#### Parameters

`feature` : `std::string_view`
    Name of the source feature used by the operation.

`points` : `std::int64_t`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `density` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio density(self, pluss::msharp::RangeView::Of<std::string_view> features, std::int64_t points = 128) const`
- `pluss::pl::Studio density(self, std::initializer_list<std::string_view> features, std::int64_t points = 128) const`
- `pluss::pl::Studio density(self, std::string_view feature, const DensityPlotOptions& options) const`
- `pluss::pl::Studio density(self, pluss::msharp::RangeView::Of<std::string_view> features, const DensityPlotOptions& options) const`
- `pluss::pl::Studio density(self, std::initializer_list<std::string_view> features, const DensityPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.density("sales", 128);
```

#### Output

```text
Figure(density)
```

#### Practical Behavior

- `density(...)` builds kernel-density traces for one or more numeric features.
- The requested point count is clamped to a minimum of 16.
- With many features, one density trace is drawn per feature on the same pane.


### `heatmap`

#### Signature

```cpp
pluss::pl::Studio heatmap(self, pluss::msharp::RangeView::Of<std::string_view> features) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `heatmap` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio heatmap(self, pluss::msharp::RangeView::Of<std::string_view> features, const HeatmapPlotOptions& options) const`
- `pluss::pl::Studio heatmap(self, std::string_view case_feature, std::string_view feature_feature, std::string_view value_feature, DataFrame::GroupOp op = DataFrame::GroupOp::Mean) const`
- `pluss::pl::Studio heatmap(self, std::string_view case_feature, std::string_view feature_feature, std::string_view value_feature, DataFrame::GroupOp op, const HeatmapPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.heatmap();
```

#### Output

```text
Figure(...)
```

### `heatmap`

#### Signature

```cpp
pluss::pl::Studio heatmap(self, pluss::msharp::RangeView::Of<std::string_view> features, const HeatmapPlotOptions& options) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`options` : `const HeatmapPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `heatmap` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio heatmap(self, pluss::msharp::RangeView::Of<std::string_view> features) const`
- `pluss::pl::Studio heatmap(self, std::string_view case_feature, std::string_view feature_feature, std::string_view value_feature, DataFrame::GroupOp op = DataFrame::GroupOp::Mean) const`
- `pluss::pl::Studio heatmap(self, std::string_view case_feature, std::string_view feature_feature, std::string_view value_feature, DataFrame::GroupOp op, const HeatmapPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.heatmap();
```

#### Output

```text
Figure(...)
```

### `heatmap`

#### Signature

```cpp
pluss::pl::Studio heatmap(self, std::string_view case_feature, std::string_view feature_feature, std::string_view value_feature, DataFrame::GroupOp op = DataFrame::GroupOp::Mean) const
```

#### Parameters

`case_feature` : `std::string_view`
    Name to use for the restored case-axis feature when cases are materialized back into ordinary columns.

`feature_feature` : `std::string_view`
    Feature whose distinct values become new output columns in pivot-style reshaping.

`value_feature` : `std::string_view`
    Feature whose values populate pivot cells or grouped summaries.

`op` : `DataFrame::GroupOp`
    Operation key or enum that chooses the aggregation, comparison, or fill behavior for the call.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `heatmap` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio heatmap(self, pluss::msharp::RangeView::Of<std::string_view> features) const`
- `pluss::pl::Studio heatmap(self, pluss::msharp::RangeView::Of<std::string_view> features, const HeatmapPlotOptions& options) const`
- `pluss::pl::Studio heatmap(self, std::string_view case_feature, std::string_view feature_feature, std::string_view value_feature, DataFrame::GroupOp op, const HeatmapPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.heatmap();
```

#### Output

```text
Figure(...)
```

### `heatmap`

#### Signature

```cpp
pluss::pl::Studio heatmap(self, std::string_view case_feature, std::string_view feature_feature, std::string_view value_feature, DataFrame::GroupOp op, const HeatmapPlotOptions& options) const
```

#### Parameters

`case_feature` : `std::string_view`
    Name to use for the restored case-axis feature when cases are materialized back into ordinary columns.

`feature_feature` : `std::string_view`
    Feature whose distinct values become new output columns in pivot-style reshaping.

`value_feature` : `std::string_view`
    Feature whose values populate pivot cells or grouped summaries.

`op` : `DataFrame::GroupOp`
    Operation key or enum that chooses the aggregation, comparison, or fill behavior for the call.

`options` : `const HeatmapPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `heatmap` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio heatmap(self, pluss::msharp::RangeView::Of<std::string_view> features) const`
- `pluss::pl::Studio heatmap(self, pluss::msharp::RangeView::Of<std::string_view> features, const HeatmapPlotOptions& options) const`
- `pluss::pl::Studio heatmap(self, std::string_view case_feature, std::string_view feature_feature, std::string_view value_feature, DataFrame::GroupOp op = DataFrame::GroupOp::Mean) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.heatmap();
```

#### Output

```text
Figure(...)
```

### `hist`

#### Signature

```cpp
pluss::pl::Studio hist(self, const HistPlotOptions& options) const
```

#### Parameters

`options` : `const HistPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Builds a histogram for one feature using the requested number of bins.

#### See also

- `pluss::pl::Studio hist(self, std::int64_t bins = 20) const`
- `pluss::pl::Studio hist(self, std::string_view feature, std::int64_t bins = 20) const`
- `pluss::pl::Studio hist(self, pluss::msharp::RangeView::Of<std::string_view> features, std::int64_t bins = 20) const`
- `pluss::pl::Studio hist(self, std::initializer_list<std::string_view> features, std::int64_t bins = 20) const`
- `pluss::pl::Studio hist(self, std::string_view feature, const HistPlotOptions& options) const`
- `pluss::pl::Studio hist(self, pluss::msharp::RangeView::Of<std::string_view> features, const HistPlotOptions& options) const`
- `pluss::pl::Studio hist(self, std::initializer_list<std::string_view> features, const HistPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.hist("sales", 12);
```

#### Output

```text
Figure(...)
```

### `hist`

#### Signature

```cpp
pluss::pl::Studio hist(self, pluss::msharp::RangeView::Of<std::string_view> features, const HistPlotOptions& options) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`options` : `const HistPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Builds a histogram for one feature using the requested number of bins.

#### See also

- `pluss::pl::Studio hist(self, std::int64_t bins = 20) const`
- `pluss::pl::Studio hist(self, std::string_view feature, std::int64_t bins = 20) const`
- `pluss::pl::Studio hist(self, pluss::msharp::RangeView::Of<std::string_view> features, std::int64_t bins = 20) const`
- `pluss::pl::Studio hist(self, std::initializer_list<std::string_view> features, std::int64_t bins = 20) const`
- `pluss::pl::Studio hist(self, const HistPlotOptions& options) const`
- `pluss::pl::Studio hist(self, std::string_view feature, const HistPlotOptions& options) const`
- `pluss::pl::Studio hist(self, std::initializer_list<std::string_view> features, const HistPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.hist("sales", 12);
```

#### Output

```text
Figure(...)
```

### `hist`

#### Signature

```cpp
pluss::pl::Studio hist(self, pluss::msharp::RangeView::Of<std::string_view> features, std::int64_t bins = 20) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`bins` : `std::int64_t`
    Strictly increasing interval edges used to define bucket boundaries.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Builds a histogram for one feature using the requested number of bins.

#### See also

- `pluss::pl::Studio hist(self, std::int64_t bins = 20) const`
- `pluss::pl::Studio hist(self, std::string_view feature, std::int64_t bins = 20) const`
- `pluss::pl::Studio hist(self, std::initializer_list<std::string_view> features, std::int64_t bins = 20) const`
- `pluss::pl::Studio hist(self, const HistPlotOptions& options) const`
- `pluss::pl::Studio hist(self, std::string_view feature, const HistPlotOptions& options) const`
- `pluss::pl::Studio hist(self, pluss::msharp::RangeView::Of<std::string_view> features, const HistPlotOptions& options) const`
- `pluss::pl::Studio hist(self, std::initializer_list<std::string_view> features, const HistPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.hist("sales", 12);
```

#### Output

```text
Figure(...)
```

### `hist`

#### Signature

```cpp
pluss::pl::Studio hist(self, std::initializer_list<std::string_view> features, const HistPlotOptions& options) const
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`options` : `const HistPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Builds a histogram for one feature using the requested number of bins.

#### See also

- `pluss::pl::Studio hist(self, std::int64_t bins = 20) const`
- `pluss::pl::Studio hist(self, std::string_view feature, std::int64_t bins = 20) const`
- `pluss::pl::Studio hist(self, pluss::msharp::RangeView::Of<std::string_view> features, std::int64_t bins = 20) const`
- `pluss::pl::Studio hist(self, std::initializer_list<std::string_view> features, std::int64_t bins = 20) const`
- `pluss::pl::Studio hist(self, const HistPlotOptions& options) const`
- `pluss::pl::Studio hist(self, std::string_view feature, const HistPlotOptions& options) const`
- `pluss::pl::Studio hist(self, pluss::msharp::RangeView::Of<std::string_view> features, const HistPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.hist("sales", 12);
```

#### Output

```text
Figure(...)
```

### `hist`

#### Signature

```cpp
pluss::pl::Studio hist(self, std::initializer_list<std::string_view> features, std::int64_t bins = 20) const
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`bins` : `std::int64_t`
    Strictly increasing interval edges used to define bucket boundaries.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Builds a histogram for one feature using the requested number of bins.

#### See also

- `pluss::pl::Studio hist(self, std::int64_t bins = 20) const`
- `pluss::pl::Studio hist(self, std::string_view feature, std::int64_t bins = 20) const`
- `pluss::pl::Studio hist(self, pluss::msharp::RangeView::Of<std::string_view> features, std::int64_t bins = 20) const`
- `pluss::pl::Studio hist(self, const HistPlotOptions& options) const`
- `pluss::pl::Studio hist(self, std::string_view feature, const HistPlotOptions& options) const`
- `pluss::pl::Studio hist(self, pluss::msharp::RangeView::Of<std::string_view> features, const HistPlotOptions& options) const`
- `pluss::pl::Studio hist(self, std::initializer_list<std::string_view> features, const HistPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.hist("sales", 12);
```

#### Output

```text
Figure(...)
```

### `hist`

#### Signature

```cpp
pluss::pl::Studio hist(self, std::int64_t bins = 20) const
```

#### Parameters

`bins` : `std::int64_t`
    Strictly increasing interval edges used to define bucket boundaries.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Builds a histogram for one feature using the requested number of bins.

#### See also

- `pluss::pl::Studio hist(self, std::string_view feature, std::int64_t bins = 20) const`
- `pluss::pl::Studio hist(self, pluss::msharp::RangeView::Of<std::string_view> features, std::int64_t bins = 20) const`
- `pluss::pl::Studio hist(self, std::initializer_list<std::string_view> features, std::int64_t bins = 20) const`
- `pluss::pl::Studio hist(self, const HistPlotOptions& options) const`
- `pluss::pl::Studio hist(self, std::string_view feature, const HistPlotOptions& options) const`
- `pluss::pl::Studio hist(self, pluss::msharp::RangeView::Of<std::string_view> features, const HistPlotOptions& options) const`
- `pluss::pl::Studio hist(self, std::initializer_list<std::string_view> features, const HistPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.hist("sales", 12);
```

#### Output

```text
Figure(...)
```

### `hist`

#### Signature

```cpp
pluss::pl::Studio hist(self, std::string_view feature, const HistPlotOptions& options) const
```

#### Parameters

`feature` : `std::string_view`
    Name of the source feature used by the operation.

`options` : `const HistPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Builds a histogram for one feature using the requested number of bins.

#### See also

- `pluss::pl::Studio hist(self, std::int64_t bins = 20) const`
- `pluss::pl::Studio hist(self, std::string_view feature, std::int64_t bins = 20) const`
- `pluss::pl::Studio hist(self, pluss::msharp::RangeView::Of<std::string_view> features, std::int64_t bins = 20) const`
- `pluss::pl::Studio hist(self, std::initializer_list<std::string_view> features, std::int64_t bins = 20) const`
- `pluss::pl::Studio hist(self, const HistPlotOptions& options) const`
- `pluss::pl::Studio hist(self, pluss::msharp::RangeView::Of<std::string_view> features, const HistPlotOptions& options) const`
- `pluss::pl::Studio hist(self, std::initializer_list<std::string_view> features, const HistPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.hist("sales", 12);
```

#### Output

```text
Figure(...)
```

### `hist`

#### Signature

```cpp
pluss::pl::Studio hist(self, std::string_view feature, std::int64_t bins = 20) const
```

#### Parameters

`feature` : `std::string_view`
    Name of the source feature used by the operation.

`bins` : `std::int64_t`
    Strictly increasing interval edges used to define bucket boundaries.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Builds a histogram for one feature using the requested number of bins.

#### See also

- `pluss::pl::Studio hist(self, std::int64_t bins = 20) const`
- `pluss::pl::Studio hist(self, pluss::msharp::RangeView::Of<std::string_view> features, std::int64_t bins = 20) const`
- `pluss::pl::Studio hist(self, std::initializer_list<std::string_view> features, std::int64_t bins = 20) const`
- `pluss::pl::Studio hist(self, const HistPlotOptions& options) const`
- `pluss::pl::Studio hist(self, std::string_view feature, const HistPlotOptions& options) const`
- `pluss::pl::Studio hist(self, pluss::msharp::RangeView::Of<std::string_view> features, const HistPlotOptions& options) const`
- `pluss::pl::Studio hist(self, std::initializer_list<std::string_view> features, const HistPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.hist("sales", 12);
```

#### Output

```text
Figure(...)
```

### `lag`

#### Signature

```cpp
pluss::pl::Studio lag(self, std::string_view feature, const LagPlotOptions& options = {}) const
```

#### Parameters

`feature` : `std::string_view`
    Name of the source feature used by the operation.

`options` : `const LagPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `lag` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.lag();
```

#### Output

```text
Figure(...)
```

### `line`

#### Signature

```cpp
pluss::pl::Studio line(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const
```

#### Parameters

`y_features` : `pluss::msharp::RangeView::Of<std::string_view>`
    See the signature for the supported argument form.

`x_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Builds a line chart for one or more y-features, optionally against a chosen x-feature.

#### See also

- `pluss::pl::Studio line(self, std::string_view y_feature, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio line(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio line(self, std::string_view y_feature, std::optional<std::string_view> x_feature, const LinePlotOptions& options) const`
- `pluss::pl::Studio line(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature, const LinePlotOptions& options) const`
- `pluss::pl::Studio line(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const LinePlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.line("sales");
```

#### Output

```text
Figure(...)
```

### `line`

#### Signature

```cpp
pluss::pl::Studio line(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature, const LinePlotOptions& options) const
```

#### Parameters

`y_features` : `pluss::msharp::RangeView::Of<std::string_view>`
    See the signature for the supported argument form.

`x_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

`options` : `const LinePlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Builds a line chart for one or more y-features, optionally against a chosen x-feature.

#### See also

- `pluss::pl::Studio line(self, std::string_view y_feature, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio line(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio line(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio line(self, std::string_view y_feature, std::optional<std::string_view> x_feature, const LinePlotOptions& options) const`
- `pluss::pl::Studio line(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const LinePlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.line("sales");
```

#### Output

```text
Figure(...)
```

### `line`

#### Signature

```cpp
pluss::pl::Studio line(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const
```

#### Parameters

`y_features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

`x_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Builds a line chart for one or more y-features, optionally against a chosen x-feature.

#### See also

- `pluss::pl::Studio line(self, std::string_view y_feature, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio line(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio line(self, std::string_view y_feature, std::optional<std::string_view> x_feature, const LinePlotOptions& options) const`
- `pluss::pl::Studio line(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature, const LinePlotOptions& options) const`
- `pluss::pl::Studio line(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const LinePlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.line({"sales"}, std::nullopt);
```

#### Output

```text
Figure(...)
```

### `line`

#### Signature

```cpp
pluss::pl::Studio line(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const LinePlotOptions& options) const
```

#### Parameters

`y_features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

`x_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

`options` : `const LinePlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Builds a line chart for one or more y-features, optionally against a chosen x-feature.

#### See also

- `pluss::pl::Studio line(self, std::string_view y_feature, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio line(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio line(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio line(self, std::string_view y_feature, std::optional<std::string_view> x_feature, const LinePlotOptions& options) const`
- `pluss::pl::Studio line(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature, const LinePlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.line({"sales"}, std::nullopt);
```

#### Output

```text
Figure(...)
```

### `line`

#### Signature

```cpp
pluss::pl::Studio line(self, std::string_view y_feature, std::optional<std::string_view> x_feature = std::nullopt) const
```

#### Parameters

`y_feature` : `std::string_view`
    See the signature for the supported argument form.

`x_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Builds a line chart for one or more y-features, optionally against a chosen x-feature.

#### See also

- `pluss::pl::Studio line(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio line(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio line(self, std::string_view y_feature, std::optional<std::string_view> x_feature, const LinePlotOptions& options) const`
- `pluss::pl::Studio line(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature, const LinePlotOptions& options) const`
- `pluss::pl::Studio line(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const LinePlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.line("sales");
```

#### Output

```text
Figure(...)
```

### `line`

#### Signature

```cpp
pluss::pl::Studio line(self, std::string_view y_feature, std::optional<std::string_view> x_feature, const LinePlotOptions& options) const
```

#### Parameters

`y_feature` : `std::string_view`
    See the signature for the supported argument form.

`x_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

`options` : `const LinePlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Builds a line chart for one or more y-features, optionally against a chosen x-feature.

#### See also

- `pluss::pl::Studio line(self, std::string_view y_feature, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio line(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio line(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio line(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature, const LinePlotOptions& options) const`
- `pluss::pl::Studio line(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const LinePlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.line("sales");
```

#### Output

```text
Figure(...)
```

### `parallel`

#### Signature

```cpp
pluss::pl::Studio parallel(self,  pluss::msharp::RangeView::Of<std::string_view> features = {}, std::optional<std::string_view> class_feature = std::nullopt, const ParallelPlotOptions& options = {}) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`class_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

`options` : `const ParallelPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `parallel` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.parallel();
```

#### Output

```text
Figure(...)
```

### `pie`

#### Signature

```cpp
pluss::pl::Studio pie(self, std::string_view value_feature, std::optional<std::string_view> label_feature = std::nullopt) const
```

#### Parameters

`value_feature` : `std::string_view`
    Feature whose values populate pivot cells or grouped summaries.

`label_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `pie` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio pie(self, std::string_view value_feature, std::optional<std::string_view> label_feature, const PiePlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.pie();
```

#### Output

```text
Figure(...)
```

### `pie`

#### Signature

```cpp
pluss::pl::Studio pie(self, std::string_view value_feature, std::optional<std::string_view> label_feature, const PiePlotOptions& options) const
```

#### Parameters

`value_feature` : `std::string_view`
    Feature whose values populate pivot cells or grouped summaries.

`label_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

`options` : `const PiePlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `pie` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio pie(self, std::string_view value_feature, std::optional<std::string_view> label_feature = std::nullopt) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.pie();
```

#### Output

```text
Figure(...)
```

### `radviz`

#### Signature

```cpp
pluss::pl::Studio radviz(self,  pluss::msharp::RangeView::Of<std::string_view> features = {}, std::optional<std::string_view> class_feature = std::nullopt, const RadvizPlotOptions& options = {}) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`class_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

`options` : `const RadvizPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `radviz` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.radviz();
```

#### Output

```text
Figure(...)
```

### `scatter`

#### Signature

```cpp
pluss::pl::Studio scatter(self, std::string_view x_feature, std::string_view y_feature, std::optional<std::string_view> hue_feature = std::nullopt) const
```

#### Parameters

`x_feature` : `std::string_view`
    See the signature for the supported argument form.

`y_feature` : `std::string_view`
    See the signature for the supported argument form.

`hue_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Builds a scatter plot using one feature for x and another for y.

#### See also

- `pluss::pl::Studio scatter(self, std::string_view x_feature, std::string_view y_feature, std::optional<std::string_view> hue_feature, const ScatterPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.scatter("id", "sales");
```

#### Output

```text
Figure(...)
```

### `scatter`

#### Signature

```cpp
pluss::pl::Studio scatter(self, std::string_view x_feature, std::string_view y_feature, std::optional<std::string_view> hue_feature, const ScatterPlotOptions& options) const
```

#### Parameters

`x_feature` : `std::string_view`
    See the signature for the supported argument form.

`y_feature` : `std::string_view`
    See the signature for the supported argument form.

`hue_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

`options` : `const ScatterPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Builds a scatter plot using one feature for x and another for y.

#### See also

- `pluss::pl::Studio scatter(self, std::string_view x_feature, std::string_view y_feature, std::optional<std::string_view> hue_feature = std::nullopt) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.scatter("id", "sales");
```

#### Output

```text
Figure(...)
```

### `scatter_matrix`

#### Signature

```cpp
pluss::pl::Studio scatter_matrix(self, pluss::msharp::RangeView::Of<std::string_view> features = {}) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `scatter_matrix` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio scatter_matrix(self, pluss::msharp::RangeView::Of<std::string_view> features, ScatterMatrixTriangle triangle) const`
- `pluss::pl::Studio scatter_matrix(self, pluss::msharp::RangeView::Of<std::string_view> features, const ScatterMatrixPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.scatter_matrix();
```

#### Output

```text
Figure(...)
```

### `scatter_matrix`

#### Signature

```cpp
pluss::pl::Studio scatter_matrix(self, pluss::msharp::RangeView::Of<std::string_view> features, const ScatterMatrixPlotOptions& options) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`options` : `const ScatterMatrixPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `scatter_matrix` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio scatter_matrix(self, pluss::msharp::RangeView::Of<std::string_view> features = {}) const`
- `pluss::pl::Studio scatter_matrix(self, pluss::msharp::RangeView::Of<std::string_view> features, ScatterMatrixTriangle triangle) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.scatter_matrix();
```

#### Output

```text
Figure(...)
```

### `scatter_matrix`

#### Signature

```cpp
pluss::pl::Studio scatter_matrix(self, pluss::msharp::RangeView::Of<std::string_view> features, ScatterMatrixTriangle triangle) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`triangle` : `ScatterMatrixTriangle`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `scatter_matrix` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio scatter_matrix(self, pluss::msharp::RangeView::Of<std::string_view> features = {}) const`
- `pluss::pl::Studio scatter_matrix(self, pluss::msharp::RangeView::Of<std::string_view> features, const ScatterMatrixPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.scatter_matrix();
```

#### Output

```text
Figure(...)
```

### `step`

#### Signature

```cpp
pluss::pl::Studio step(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const
```

#### Parameters

`y_features` : `pluss::msharp::RangeView::Of<std::string_view>`
    See the signature for the supported argument form.

`x_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `step` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio step(self, std::string_view y_feature, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio step(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio step(self, std::string_view y_feature, std::optional<std::string_view> x_feature, const StepPlotOptions& options) const`
- `pluss::pl::Studio step(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature, const StepPlotOptions& options) const`
- `pluss::pl::Studio step(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const StepPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.step();
```

#### Output

```text
Figure(...)
```

### `step`

#### Signature

```cpp
pluss::pl::Studio step(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature, const StepPlotOptions& options) const
```

#### Parameters

`y_features` : `pluss::msharp::RangeView::Of<std::string_view>`
    See the signature for the supported argument form.

`x_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

`options` : `const StepPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `step` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio step(self, std::string_view y_feature, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio step(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio step(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio step(self, std::string_view y_feature, std::optional<std::string_view> x_feature, const StepPlotOptions& options) const`
- `pluss::pl::Studio step(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const StepPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.step();
```

#### Output

```text
Figure(...)
```

### `step`

#### Signature

```cpp
pluss::pl::Studio step(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const
```

#### Parameters

`y_features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

`x_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `step` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio step(self, std::string_view y_feature, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio step(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio step(self, std::string_view y_feature, std::optional<std::string_view> x_feature, const StepPlotOptions& options) const`
- `pluss::pl::Studio step(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature, const StepPlotOptions& options) const`
- `pluss::pl::Studio step(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const StepPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.step();
```

#### Output

```text
Figure(...)
```

### `step`

#### Signature

```cpp
pluss::pl::Studio step(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const StepPlotOptions& options) const
```

#### Parameters

`y_features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

`x_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

`options` : `const StepPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `step` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio step(self, std::string_view y_feature, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio step(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio step(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio step(self, std::string_view y_feature, std::optional<std::string_view> x_feature, const StepPlotOptions& options) const`
- `pluss::pl::Studio step(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature, const StepPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.step();
```

#### Output

```text
Figure(...)
```

### `step`

#### Signature

```cpp
pluss::pl::Studio step(self, std::string_view y_feature, std::optional<std::string_view> x_feature = std::nullopt) const
```

#### Parameters

`y_feature` : `std::string_view`
    See the signature for the supported argument form.

`x_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `step` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio step(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio step(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio step(self, std::string_view y_feature, std::optional<std::string_view> x_feature, const StepPlotOptions& options) const`
- `pluss::pl::Studio step(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature, const StepPlotOptions& options) const`
- `pluss::pl::Studio step(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const StepPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.step();
```

#### Output

```text
Figure(...)
```

### `step`

#### Signature

```cpp
pluss::pl::Studio step(self, std::string_view y_feature, std::optional<std::string_view> x_feature, const StepPlotOptions& options) const
```

#### Parameters

`y_feature` : `std::string_view`
    See the signature for the supported argument form.

`x_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

`options` : `const StepPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `step` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio step(self, std::string_view y_feature, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio step(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio step(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio step(self, pluss::msharp::RangeView::Of<std::string_view> y_features, std::optional<std::string_view> x_feature, const StepPlotOptions& options) const`
- `pluss::pl::Studio step(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const StepPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.step();
```

#### Output

```text
Figure(...)
```

### `violin`

#### Signature

```cpp
pluss::pl::Studio violin(self, pluss::msharp::RangeView::Of<std::string_view> features = {}) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `violin` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio violin(self, std::initializer_list<std::string_view> features) const`
- `pluss::pl::Studio violin(self, pluss::msharp::RangeView::Of<std::string_view> features, const ViolinPlotOptions& options) const`
- `pluss::pl::Studio violin(self, std::initializer_list<std::string_view> features, const ViolinPlotOptions& options) const`
- `pluss::pl::Studio violin(self, std::string_view value_feature, std::string_view group_feature) const`
- `pluss::pl::Studio violin(self, std::string_view value_feature, std::string_view group_feature, const ViolinPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.violin();
```

#### Output

```text
Figure(...)
```

### `violin`

#### Signature

```cpp
pluss::pl::Studio violin(self, pluss::msharp::RangeView::Of<std::string_view> features, const ViolinPlotOptions& options) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`options` : `const ViolinPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `violin` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio violin(self, pluss::msharp::RangeView::Of<std::string_view> features = {}) const`
- `pluss::pl::Studio violin(self, std::initializer_list<std::string_view> features) const`
- `pluss::pl::Studio violin(self, std::initializer_list<std::string_view> features, const ViolinPlotOptions& options) const`
- `pluss::pl::Studio violin(self, std::string_view value_feature, std::string_view group_feature) const`
- `pluss::pl::Studio violin(self, std::string_view value_feature, std::string_view group_feature, const ViolinPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.violin();
```

#### Output

```text
Figure(...)
```

### `violin`

#### Signature

```cpp
pluss::pl::Studio violin(self, std::initializer_list<std::string_view> features) const
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `violin` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio violin(self, pluss::msharp::RangeView::Of<std::string_view> features = {}) const`
- `pluss::pl::Studio violin(self, pluss::msharp::RangeView::Of<std::string_view> features, const ViolinPlotOptions& options) const`
- `pluss::pl::Studio violin(self, std::initializer_list<std::string_view> features, const ViolinPlotOptions& options) const`
- `pluss::pl::Studio violin(self, std::string_view value_feature, std::string_view group_feature) const`
- `pluss::pl::Studio violin(self, std::string_view value_feature, std::string_view group_feature, const ViolinPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.violin();
```

#### Output

```text
Figure(...)
```

### `violin`

#### Signature

```cpp
pluss::pl::Studio violin(self, std::initializer_list<std::string_view> features, const ViolinPlotOptions& options) const
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`options` : `const ViolinPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `violin` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio violin(self, pluss::msharp::RangeView::Of<std::string_view> features = {}) const`
- `pluss::pl::Studio violin(self, std::initializer_list<std::string_view> features) const`
- `pluss::pl::Studio violin(self, pluss::msharp::RangeView::Of<std::string_view> features, const ViolinPlotOptions& options) const`
- `pluss::pl::Studio violin(self, std::string_view value_feature, std::string_view group_feature) const`
- `pluss::pl::Studio violin(self, std::string_view value_feature, std::string_view group_feature, const ViolinPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.violin();
```

#### Output

```text
Figure(...)
```

### `violin`

#### Signature

```cpp
pluss::pl::Studio violin(self, std::string_view value_feature, std::string_view group_feature) const
```

#### Parameters

`value_feature` : `std::string_view`
    Feature whose values populate pivot cells or grouped summaries.

`group_feature` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `violin` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio violin(self, pluss::msharp::RangeView::Of<std::string_view> features = {}) const`
- `pluss::pl::Studio violin(self, std::initializer_list<std::string_view> features) const`
- `pluss::pl::Studio violin(self, pluss::msharp::RangeView::Of<std::string_view> features, const ViolinPlotOptions& options) const`
- `pluss::pl::Studio violin(self, std::initializer_list<std::string_view> features, const ViolinPlotOptions& options) const`
- `pluss::pl::Studio violin(self, std::string_view value_feature, std::string_view group_feature, const ViolinPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.violin();
```

#### Output

```text
Figure(...)
```

### `violin`

#### Signature

```cpp
pluss::pl::Studio violin(self, std::string_view value_feature, std::string_view group_feature, const ViolinPlotOptions& options) const
```

#### Parameters

`value_feature` : `std::string_view`
    Feature whose values populate pivot cells or grouped summaries.

`group_feature` : `std::string_view`
    See the signature for the supported argument form.

`options` : `const ViolinPlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `violin` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio violin(self, pluss::msharp::RangeView::Of<std::string_view> features = {}) const`
- `pluss::pl::Studio violin(self, std::initializer_list<std::string_view> features) const`
- `pluss::pl::Studio violin(self, pluss::msharp::RangeView::Of<std::string_view> features, const ViolinPlotOptions& options) const`
- `pluss::pl::Studio violin(self, std::initializer_list<std::string_view> features, const ViolinPlotOptions& options) const`
- `pluss::pl::Studio violin(self, std::string_view value_feature, std::string_view group_feature) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.violin();
```

#### Output

```text
Figure(...)
```
