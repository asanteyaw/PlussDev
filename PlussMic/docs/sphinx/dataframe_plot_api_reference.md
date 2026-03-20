# DataFramePlotAccessor API Reference

# Practical DataFramePlotAccessor API Reference

## API Reference Style

This page documents the public plotting accessor returned by `DataFrame::plot()` and `Atlas::plot()`.

```{include} snippets/dataframe_plot_options_reference.md
```

### `act`

#### Signature

```cpp
pluss::pl::Studio act(self,  std::string_view feature, const ActPlotOptions& options =
```

#### Parameters

`feature` : `std::string_view`
    See the signature for the supported argument form.

`options` : `const ActPlotOptions&`
    See the signature for the supported argument form.

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

auto out = plot.act();
```

#### Output

```text
Figure(...)
```

### `andrews_curves`

#### Signature

```cpp
pluss::pl::Studio andrews_curves(self,  std::initializer_list<std::string_view> features =
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `andrews_curves` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.andrews_curves();
```

#### Output

```text
Figure(...)
```

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
- `pluss::pl::Studio area(self, std::string_view y_feature, std::optional<std::string_view> x_feature, const AreaPlotOptions& options) const`
- `pluss::pl::Studio area(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const AreaPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.area();
```

#### Output

```text
Figure(...)
```

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

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.area();
```

#### Output

```text
Figure(...)
```

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

- `pluss::pl::Studio area(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio area(self, std::string_view y_feature, std::optional<std::string_view> x_feature, const AreaPlotOptions& options) const`
- `pluss::pl::Studio area(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const AreaPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.area();
```

#### Output

```text
Figure(...)
```

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
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `area` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio area(self, std::string_view y_feature, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio area(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio area(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const AreaPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.area();
```

#### Output

```text
Figure(...)
```

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
Figure(...)
```

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
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `area_stacked` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio area_stacked(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`

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
Figure(...)
```

### `auto_plot`

#### Signature

```cpp
pluss::pl::Studio auto_plot(self, std::initializer_list<std::string_view> features =
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `auto_plot` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio auto_plot(self, std::initializer_list<std::string_view> features, const AutoPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.auto_plot();
```

#### Output

```text
Figure(...)
```

### `auto_plot`

#### Signature

```cpp
pluss::pl::Studio auto_plot(self, std::initializer_list<std::string_view> features, const AutoPlotOptions& options) const
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

`options` : `const AutoPlotOptions&`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `auto_plot` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio auto_plot(self, std::initializer_list<std::string_view> features =`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.auto_plot();
```

#### Output

```text
Figure(...)
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
- `pluss::pl::Studio bar(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const BarPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.bar("sales", std::nullopt);
```

#### Output

```text
Figure(...)
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

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.bar("sales", std::nullopt);
```

#### Output

```text
Figure(...)
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
- `pluss::pl::Studio bar(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio bar(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const BarPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.bar("sales", std::nullopt);
```

#### Output

```text
Figure(...)
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
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Builds a bar chart from one feature, optionally using another feature as the x-axis.

#### See also

- `pluss::pl::Studio bar(self, std::string_view y_feature, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio bar(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio bar(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const BarPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto fig = plot.bar("sales", std::nullopt);
```

#### Output

```text
Figure(...)
```

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

Applies `bar_stacked` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio bar_stacked(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const BarStackedPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.bar_stacked();
```

#### Output

```text
Figure(...)
```

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
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `bar_stacked` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio bar_stacked(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.bar_stacked();
```

#### Output

```text
Figure(...)
```

### `bootstrap`

#### Signature

```cpp
pluss::pl::Studio bootstrap(self,  std::string_view feature, const BootstrapPlotOptions& options =
```

#### Parameters

`feature` : `std::string_view`
    See the signature for the supported argument form.

`options` : `const BootstrapPlotOptions&`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `bootstrap` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.bootstrap();
```

#### Output

```text
Figure(...)
```

### `box`

#### Signature

```cpp
pluss::pl::Studio box(self, std::initializer_list<std::string_view> features =
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `box` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio box(self, std::initializer_list<std::string_view> features, const BoxPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.box();
```

#### Output

```text
Figure(...)
```

### `box`

#### Signature

```cpp
pluss::pl::Studio box(self, std::initializer_list<std::string_view> features, const BoxPlotOptions& options) const
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

`options` : `const BoxPlotOptions&`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `box` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio box(self, std::initializer_list<std::string_view> features =`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.box();
```

#### Output

```text
Figure(...)
```

### `brief`

#### Signature

```cpp
pluss::pl::Studio brief(self, std::initializer_list<std::string_view> features =
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `brief` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio& brief(self, pluss::pl::Studio& figure, std::initializer_list<std::string_view> features =`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.brief();
```

#### Output

```text
Figure(...)
```

### `brief`

#### Signature

```cpp
pluss::pl::Studio& brief(self, pluss::pl::Studio& figure, std::initializer_list<std::string_view> features =
```

#### Parameters

`figure` : `pluss::pl::Studio&`
    See the signature for the supported argument form.

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio&`
    Value produced by this call.

#### Notes

Applies `brief` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio brief(self, std::initializer_list<std::string_view> features =`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.brief();
```

#### Output

```text
Figure(...)
```

### `corr_heatmap`

#### Signature

```cpp
pluss::pl::Studio corr_heatmap(self, std::initializer_list<std::string_view> features =
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `corr_heatmap` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio corr_heatmap(self, std::initializer_list<std::string_view> features, const CorrHeatmapPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.corr_heatmap();
```

#### Output

```text
Figure(...)
```

### `corr_heatmap`

#### Signature

```cpp
pluss::pl::Studio corr_heatmap(self, std::initializer_list<std::string_view> features, const CorrHeatmapPlotOptions& options) const
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

`options` : `const CorrHeatmapPlotOptions&`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `corr_heatmap` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio corr_heatmap(self, std::initializer_list<std::string_view> features =`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.corr_heatmap();
```

#### Output

```text
Figure(...)
```

### `density`

#### Signature

```cpp
pluss::pl::Studio density(self, std::initializer_list<std::string_view> features, const DensityPlotOptions& options) const
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

`options` : `const DensityPlotOptions&`
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

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.density();
```

#### Output

```text
Figure(...)
```

### `density`

#### Signature

```cpp
pluss::pl::Studio density(self, std::initializer_list<std::string_view> features, std::int64_t points = 128) const
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

`points` : `std::int64_t`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `density` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio density(self, std::string_view feature, std::int64_t points = 128) const`
- `pluss::pl::Studio density(self, std::string_view feature, const DensityPlotOptions& options) const`
- `pluss::pl::Studio density(self, std::initializer_list<std::string_view> features, const DensityPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.density();
```

#### Output

```text
Figure(...)
```

### `density`

#### Signature

```cpp
pluss::pl::Studio density(self, std::string_view feature, const DensityPlotOptions& options) const
```

#### Parameters

`feature` : `std::string_view`
    See the signature for the supported argument form.

`options` : `const DensityPlotOptions&`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `density` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio density(self, std::string_view feature, std::int64_t points = 128) const`
- `pluss::pl::Studio density(self, std::initializer_list<std::string_view> features, std::int64_t points = 128) const`
- `pluss::pl::Studio density(self, std::initializer_list<std::string_view> features, const DensityPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.density();
```

#### Output

```text
Figure(...)
```

### `density`

#### Signature

```cpp
pluss::pl::Studio density(self, std::string_view feature, std::int64_t points = 128) const
```

#### Parameters

`feature` : `std::string_view`
    See the signature for the supported argument form.

`points` : `std::int64_t`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `density` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio density(self, std::initializer_list<std::string_view> features, std::int64_t points = 128) const`
- `pluss::pl::Studio density(self, std::string_view feature, const DensityPlotOptions& options) const`
- `pluss::pl::Studio density(self, std::initializer_list<std::string_view> features, const DensityPlotOptions& options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto plot = df.plot();

auto out = plot.density();
```

#### Output

```text
Figure(...)
```

### `heatmap`

#### Signature

```cpp
pluss::pl::Studio heatmap(self, std::initializer_list<std::string_view> features) const
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `heatmap` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio heatmap(self, std::initializer_list<std::string_view> features, const HeatmapPlotOptions& options) const`
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
pluss::pl::Studio heatmap(self, std::initializer_list<std::string_view> features, const HeatmapPlotOptions& options) const
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

`options` : `const HeatmapPlotOptions&`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `heatmap` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio heatmap(self, std::initializer_list<std::string_view> features) const`
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
    See the signature for the supported argument form.

`feature_feature` : `std::string_view`
    See the signature for the supported argument form.

`value_feature` : `std::string_view`
    See the signature for the supported argument form.

`op` : `DataFrame::GroupOp`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `heatmap` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio heatmap(self, std::initializer_list<std::string_view> features) const`
- `pluss::pl::Studio heatmap(self, std::initializer_list<std::string_view> features, const HeatmapPlotOptions& options) const`
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
    See the signature for the supported argument form.

`feature_feature` : `std::string_view`
    See the signature for the supported argument form.

`value_feature` : `std::string_view`
    See the signature for the supported argument form.

`op` : `DataFrame::GroupOp`
    See the signature for the supported argument form.

`options` : `const HeatmapPlotOptions&`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `heatmap` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio heatmap(self, std::initializer_list<std::string_view> features) const`
- `pluss::pl::Studio heatmap(self, std::initializer_list<std::string_view> features, const HeatmapPlotOptions& options) const`
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
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Builds a histogram for one feature using the requested number of bins.

#### See also

- `pluss::pl::Studio hist(self, std::int64_t bins = 20) const`
- `pluss::pl::Studio hist(self, std::string_view feature, std::int64_t bins = 20) const`
- `pluss::pl::Studio hist(self, std::initializer_list<std::string_view> features, std::int64_t bins = 20) const`
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
pluss::pl::Studio hist(self, std::initializer_list<std::string_view> features, const HistPlotOptions& options) const
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

`options` : `const HistPlotOptions&`
    See the signature for the supported argument form.

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
    See the signature for the supported argument form.

`bins` : `std::int64_t`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Builds a histogram for one feature using the requested number of bins.

#### See also

- `pluss::pl::Studio hist(self, std::int64_t bins = 20) const`
- `pluss::pl::Studio hist(self, std::string_view feature, std::int64_t bins = 20) const`
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
pluss::pl::Studio hist(self, std::int64_t bins = 20) const
```

#### Parameters

`bins` : `std::int64_t`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Builds a histogram for one feature using the requested number of bins.

#### See also

- `pluss::pl::Studio hist(self, std::string_view feature, std::int64_t bins = 20) const`
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
pluss::pl::Studio hist(self, std::string_view feature, const HistPlotOptions& options) const
```

#### Parameters

`feature` : `std::string_view`
    See the signature for the supported argument form.

`options` : `const HistPlotOptions&`
    See the signature for the supported argument form.

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
    See the signature for the supported argument form.

`bins` : `std::int64_t`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Builds a histogram for one feature using the requested number of bins.

#### See also

- `pluss::pl::Studio hist(self, std::int64_t bins = 20) const`
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

### `lag`

#### Signature

```cpp
pluss::pl::Studio lag(self, std::string_view feature, const LagPlotOptions& options =
```

#### Parameters

`feature` : `std::string_view`
    See the signature for the supported argument form.

`options` : `const LagPlotOptions&`
    See the signature for the supported argument form.

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
pluss::pl::Studio line(self, std::string_view y_feature, std::optional<std::string_view> x_feature, const LinePlotOptions& options) const
```

#### Parameters

`y_feature` : `std::string_view`
    See the signature for the supported argument form.

`x_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

`options` : `const LinePlotOptions&`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Builds a line chart for one or more y-features, optionally against a chosen x-feature.

#### See also

- `pluss::pl::Studio line(self, std::string_view y_feature, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio line(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
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
pluss::pl::Studio parallel(self,  std::initializer_list<std::string_view> features =
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

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
    See the signature for the supported argument form.

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
    See the signature for the supported argument form.

`label_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

`options` : `const PiePlotOptions&`
    See the signature for the supported argument form.

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
pluss::pl::Studio radviz(self,  std::initializer_list<std::string_view> features =
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

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
    See the signature for the supported argument form.

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
pluss::pl::Studio scatter_matrix(self, std::initializer_list<std::string_view> features =
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `scatter_matrix` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio scatter_matrix(self, std::initializer_list<std::string_view> features, ScatterMatrixTriangle triangle) const`
- `pluss::pl::Studio scatter_matrix(self, std::initializer_list<std::string_view> features, const ScatterMatrixPlotOptions& options) const`

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
pluss::pl::Studio scatter_matrix(self, std::initializer_list<std::string_view> features, const ScatterMatrixPlotOptions& options) const
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

`options` : `const ScatterMatrixPlotOptions&`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `scatter_matrix` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio scatter_matrix(self, std::initializer_list<std::string_view> features =`
- `pluss::pl::Studio scatter_matrix(self, std::initializer_list<std::string_view> features, ScatterMatrixTriangle triangle) const`

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
pluss::pl::Studio scatter_matrix(self, std::initializer_list<std::string_view> features, ScatterMatrixTriangle triangle) const
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

`triangle` : `ScatterMatrixTriangle`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `scatter_matrix` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio scatter_matrix(self, std::initializer_list<std::string_view> features =`
- `pluss::pl::Studio scatter_matrix(self, std::initializer_list<std::string_view> features, const ScatterMatrixPlotOptions& options) const`

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
pluss::pl::Studio step(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature, const StepPlotOptions& options) const
```

#### Parameters

`y_features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

`x_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

`options` : `const StepPlotOptions&`
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
pluss::pl::Studio step(self, std::string_view y_feature, std::optional<std::string_view> x_feature, const StepPlotOptions& options) const
```

#### Parameters

`y_feature` : `std::string_view`
    See the signature for the supported argument form.

`x_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

`options` : `const StepPlotOptions&`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `step` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio step(self, std::string_view y_feature, std::optional<std::string_view> x_feature = std::nullopt) const`
- `pluss::pl::Studio step(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`
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
pluss::pl::Studio violin(self, std::initializer_list<std::string_view> features =
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `violin` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

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
    See the signature for the supported argument form.

`options` : `const ViolinPlotOptions&`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `violin` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio violin(self, std::initializer_list<std::string_view> features =`
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
    See the signature for the supported argument form.

`group_feature` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `violin` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio violin(self, std::initializer_list<std::string_view> features =`
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
    See the signature for the supported argument form.

`group_feature` : `std::string_view`
    See the signature for the supported argument form.

`options` : `const ViolinPlotOptions&`
    See the signature for the supported argument form.

#### Returns

`pluss::pl::Studio`
    Value produced by this call.

#### Notes

Applies `violin` to the current `DataFramePlotAccessor` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `pluss::pl::Studio violin(self, std::initializer_list<std::string_view> features =`
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
