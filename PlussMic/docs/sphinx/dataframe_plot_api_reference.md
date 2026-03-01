# DataFramePlotAccessor API Reference

# Practical DataFramePlotAccessor API Reference

## API Reference Style

This page documents the public plotting accessor returned by `DataFrame::plot()` and `Atlas::plot()`.

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

This call is part of the public `DataFramePlotAccessor` API and is documented here using a concrete usage example.

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

This call is part of the public `DataFramePlotAccessor` API and is documented here using a concrete usage example.

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

This call is part of the public `DataFramePlotAccessor` API and is documented here using a concrete usage example.

#### See also

- `pluss::plotism::Figure line(self, std::string_view y_feature, std::optional<std::string_view> x_feature = std::nullopt) const`

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

This call is part of the public `DataFramePlotAccessor` API and is documented here using a concrete usage example.

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

This call is part of the public `DataFramePlotAccessor` API and is documented here using a concrete usage example.

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
