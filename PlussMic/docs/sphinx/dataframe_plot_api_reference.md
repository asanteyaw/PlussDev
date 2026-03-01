# DataFramePlotAccessor API Reference

# Practical DataFramePlotAccessor API Reference

## API Reference Style

This page documents the public plotting accessor returned by `DataFrame::plot()` and `Atlas::plot()`.

`DataFrame::plot()` is intended to be a smart front end over `PlussPl`, not a raw vector escape hatch.
When `x_feature` is omitted, plots now use the DataFrame case axis automatically.
If that axis is note-like or time-like, the accessor delegates a labeled categorical axis to `PlussPl`.
Figures also inherit stronger `PlussPl` defaults, including the `paper` theme, `IBM Plex Sans`, and outside legends for multi-series line plots.

### `bar`

#### Signature

```cpp
pluss::plotism::Figure bar(self, std::string_view y_feature, std::optional<std::string_view> x_feature = std::nullopt) const
```

#### Parameters

`y_feature` : `std::string_view`
    See the signature for the supported argument form.

`x_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`pluss::plotism::Figure`
    Value produced by this call.

#### Notes

Builds a bar chart from one feature, optionally using another feature as the x-axis.
If `x_feature` is omitted, the DataFrame case axis is used.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto indexed = df.set_cases("city", false);
auto plot = indexed.plot();

auto fig = plot.bar("sales");
```

#### Output

```text
Figure(...)
```

### `hist`

#### Signature

```cpp
pluss::plotism::Figure hist(self, std::string_view feature, std::int64_t bins = 20) const
```

#### Parameters

`feature` : `std::string_view`
    See the signature for the supported argument form.

`bins` : `std::int64_t`
    See the signature for the supported argument form.

#### Returns

`pluss::plotism::Figure`
    Value produced by this call.

#### Notes

Builds a histogram for one feature using the requested number of bins.

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

### `line`

#### Signature

```cpp
pluss::plotism::Figure line(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const
```

#### Parameters

`y_features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

`x_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`pluss::plotism::Figure`
    Value produced by this call.

#### Notes

Builds a line chart for one or more y-features, optionally against a chosen x-feature.
If `x_feature` is omitted, the DataFrame case axis is used and multi-series plots get an outside legend by default.

#### See also

- `pluss::plotism::Figure line(self, std::string_view y_feature, std::optional<std::string_view> x_feature = std::nullopt) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});
auto indexed = df.set_cases("city", false);
auto plot = indexed.plot();

auto fig = plot.line({"sales"}, std::nullopt);
```

#### Output

```text
Figure(...)
```

### `line`

#### Signature

```cpp
pluss::plotism::Figure line(self, std::string_view y_feature, std::optional<std::string_view> x_feature = std::nullopt) const
```

#### Parameters

`y_feature` : `std::string_view`
    See the signature for the supported argument form.

`x_feature` : `std::optional<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`pluss::plotism::Figure`
    Value produced by this call.

#### Notes

Builds a line chart for one or more y-features, optionally against a chosen x-feature.
If `x_feature` is omitted, the DataFrame case axis is used.

#### See also

- `pluss::plotism::Figure line(self, std::initializer_list<std::string_view> y_features, std::optional<std::string_view> x_feature = std::nullopt) const`

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

### `scatter`

#### Signature

```cpp
pluss::plotism::Figure scatter(self, std::string_view x_feature, std::string_view y_feature) const
```

#### Parameters

`x_feature` : `std::string_view`
    See the signature for the supported argument form.

`y_feature` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`pluss::plotism::Figure`
    Value produced by this call.

#### Notes

Builds a scatter plot using one feature for x and another for y.
If the selected x feature is note-like or time-like, the accessor delegates labeled positions to `PlussPl` automatically.

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
