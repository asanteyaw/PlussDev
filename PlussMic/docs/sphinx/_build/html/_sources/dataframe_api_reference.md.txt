# DataFrame API Reference

# Practical DataFrame API Reference

## API Reference Style

This page documents the full public `DataFrame` surface with usage-first examples generated from the current table API.

### `add`

#### Signature

```cpp
DataFrame add(self, const DataFrame& other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### See also

- `DataFrame add(self, scalar other) const`

#### Examples

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"cost", pluss::table::Thread(pluss::make_series({60.0, 80.0, 55.0}))},
});

auto out = left.add(right);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `add`

#### Signature

```cpp
DataFrame add(self, scalar other) const
```

#### Parameters

`other` : `scalar`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### See also

- `DataFrame add(self, const DataFrame& other) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.add(10);
```

#### Output

```text
id  sales  city
11  110.0  Accra
12  130.0  Tema
13  100.0  Kumasi
```

### `add_prefix`

#### Signature

```cpp
DataFrame add_prefix(self, std::string_view prefix) const
```

#### Parameters

`prefix` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Prepends the supplied text to every feature name. Row values, row order, and case labels stay unchanged.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.add_prefix("feat_");
```

#### Output

```text
df.head():
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi

out.head():
feat_id  feat_sales  feat_city
1        100.0       Accra
2        120.0       Tema
3         90.0       Kumasi
```

### `add_suffix`

#### Signature

```cpp
DataFrame add_suffix(self, std::string_view suffix) const
```

#### Parameters

`suffix` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Appends the supplied text to every feature name. Row values, row order, and case labels stay unchanged.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.add_suffix("_2024");
```

#### Output

```text
df.head():
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi

out.head():
id_2024  sales_2024  city_2024
1        100.0       Accra
2        120.0       Tema
3         90.0       Kumasi
```

### `aggregate`

#### Signature

```cpp
DataFrame aggregate(self, std::initializer_list<SummarySpec> specs, bool render = true) const
```

#### Parameters

`specs` : `std::initializer_list<SummarySpec>`
    See the signature for the supported argument form.

`render` : `bool`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Builds a compact summary frame from one or more aggregation specs, so you can inspect computed metrics instead of the full row set.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema"}))},
    {"product", pluss::table::Thread(pluss::make_note({"A", "B", "A", "B"}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0, 150.0}))},
});

auto out = df.aggregate({{"sales_mean", "sales", pluss::table::DataFrame::SummaryOp::Mean}}, false);
```

#### Output

```text
metric       value
sales_mean    115.0
```

### `align_cases`

#### Signature

```cpp
DataFrame align_cases(self, const Thread& new_cases) const
```

#### Parameters

`new_cases` : `const Thread&`
    Replacement case labels in the exact order you want the rows to follow.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Reorders rows so their case labels match the supplied `new_cases` order. Every requested case must already exist, and the output case axis is rewritten to that order.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df = df.with_cases(pluss::table::Thread(pluss::make_note({"cs0", "cs1", "cs2"})));
auto new_cases = pluss::table::Thread(pluss::make_note({"cs1", "cs0", "cs2"}));
auto out = df.align_cases(new_cases);
```

#### Output

```text
df.cases():
["cs0", "cs1", "cs2"]

out.cases():
["cs1", "cs0", "cs2"]

out.head():
id  sales  city
2   120.0  Tema
1   100.0  Accra
3    90.0  Kumasi
```

### `assign`

#### Signature

```cpp
DataFrame assign(self, std::initializer_list<DeriveSpec> specs) const
```

#### Parameters

`specs` : `std::initializer_list<DeriveSpec>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Adds or replaces derived features computed from the current frame. Use this when the new column depends on one or more existing columns.

#### See also

- `DataFrame assign(self, std::initializer_list<std::pair<std::string, Thread>> assignments) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.assign({{.feature = "profit", .expr = [](const pluss::table::DataFrame& frame) { return pluss::table::Thread(frame.features("sales").as_series().sub(60)); }}});
```

#### Output

```text
id  sales  city    profit
1   100.0  Accra   40.0
2   120.0  Tema    35.0
3    90.0  Kumasi  30.0
```

### `assign`

#### Signature

```cpp
DataFrame assign(self, std::initializer_list<std::pair<std::string, Thread>> assignments) const
```

#### Parameters

`assignments` : `std::initializer_list<std::pair<std::string, Thread>>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Adds or replaces derived features computed from the current frame. Use this when the new column depends on one or more existing columns.

#### See also

- `DataFrame assign(self, std::initializer_list<DeriveSpec> specs) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.assign({{"profit", pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0}))}});
```

#### Output

```text
id  sales  city    profit
1   100.0  Accra   40.0
2   120.0  Tema    35.0
3    90.0  Kumasi  30.0
```

### `astype`

#### Signature

```cpp
DataFrame astype(self,  std::initializer_list<std::pair<std::string_view, pluss::codem::ScalarType>> feature_dtypes) const
```

#### Parameters

`feature_dtypes` : `std::initializer_list<std::pair<std::string_view, pluss::codem::ScalarType>>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This changes the semantic or numeric type so later operations route through the correct implementation path.

#### See also

- `DataFrame astype(self, std::initializer_list<std::string_view> features, pluss::codem::ScalarType dtype) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({{"sales", pluss::table::Thread(pluss::make_note({"100.0", "120.0", "90.0"}))}});

auto out = df.astype({{"sales", pluss::codem::ScalarType::Float64}});
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `astype`

#### Signature

```cpp
DataFrame astype(self, std::initializer_list<std::string_view> features, pluss::codem::ScalarType dtype) const
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

`dtype` : `pluss::codem::ScalarType`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This changes the semantic or numeric type so later operations route through the correct implementation path.

#### See also

- `DataFrame astype(self,  std::initializer_list<std::pair<std::string_view, pluss::codem::ScalarType>> feature_dtypes) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.astype();
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `at`

#### Signature

```cpp
std::string at(self, std::int64_t row, std::string_view feature_name) const
```

#### Parameters

`row` : `std::int64_t`
    See the signature for the supported argument form.

`feature_name` : `std::string_view`
    Name of the target feature to read, rewrite, sort by, or promote to cases.

#### Returns

`std::string`
    Rendered string result.

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.at(0, "city");
```

#### Output

```text
1
```

### `bfill`

#### Signature

```cpp
DataFrame bfill(self) const
```

#### Parameters

`None`

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.bfill();
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `bottom`

#### Signature

```cpp
DataFrame bottom(self, std::string_view feature, std::int64_t n) const
```

#### Parameters

`feature` : `std::string_view`
    See the signature for the supported argument form.

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Returns the rows with the smallest values in the selected feature.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.bottom("sales", 2);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `cases`

#### Signature

```cpp
const Thread& cases(self) const
```

#### Parameters

`None`

#### Returns

`const Thread&`
    Value produced by this call.

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### See also

- `DataFrame cases(self, std::int64_t row) const`
- `DataFrame cases(self, std::pair<std::int64_t, std::int64_t> row_span) const`
- `std::string cases(self, std::int64_t row, std::size_t feature_idx) const`
- `std::string cases(self, std::int64_t row, std::string_view feature_name) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.cases(0);
```

### `cases`

#### Signature

```cpp
DataFrame cases(self, std::int64_t row) const
```

#### Parameters

`row` : `std::int64_t`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### See also

- `const Thread& cases(self) const`
- `DataFrame cases(self, std::pair<std::int64_t, std::int64_t> row_span) const`
- `std::string cases(self, std::int64_t row, std::size_t feature_idx) const`
- `std::string cases(self, std::int64_t row, std::string_view feature_name) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.cases(0);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `cases`

#### Signature

```cpp
DataFrame cases(self, std::pair<std::int64_t, std::int64_t> row_span) const
```

#### Parameters

`row_span` : `std::pair<std::int64_t, std::int64_t>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### See also

- `const Thread& cases(self) const`
- `DataFrame cases(self, std::int64_t row) const`
- `std::string cases(self, std::int64_t row, std::size_t feature_idx) const`
- `std::string cases(self, std::int64_t row, std::string_view feature_name) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.cases({0, 2});
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `cases`

#### Signature

```cpp
std::string cases(self, std::int64_t row, std::size_t feature_idx) const
```

#### Parameters

`row` : `std::int64_t`
    See the signature for the supported argument form.

`feature_idx` : `std::size_t`
    See the signature for the supported argument form.

#### Returns

`std::string`
    Rendered string result.

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### See also

- `const Thread& cases(self) const`
- `DataFrame cases(self, std::int64_t row) const`
- `DataFrame cases(self, std::pair<std::int64_t, std::int64_t> row_span) const`
- `std::string cases(self, std::int64_t row, std::string_view feature_name) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.cases(0, 0);
```

#### Output

```text
1
```

### `cases`

#### Signature

```cpp
std::string cases(self, std::int64_t row, std::string_view feature_name) const
```

#### Parameters

`row` : `std::int64_t`
    See the signature for the supported argument form.

`feature_name` : `std::string_view`
    Name of the target feature to read, rewrite, sort by, or promote to cases.

#### Returns

`std::string`
    Rendered string result.

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### See also

- `const Thread& cases(self) const`
- `DataFrame cases(self, std::int64_t row) const`
- `DataFrame cases(self, std::pair<std::int64_t, std::int64_t> row_span) const`
- `std::string cases(self, std::int64_t row, std::size_t feature_idx) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.cases(0, "city");
```

#### Output

```text
1
```

### `cases_series`

#### Signature

```cpp
pluss::Series cases_series(self) const
```

#### Parameters

`None`

#### Returns

`pluss::Series`
    Value produced by this call.

#### Notes

Returns the current case axis as a `Series`, which is useful when you need the row labels as data.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.cases_series();
```

#### Output

```text
[0, 1, 2]
```

### `cast`

#### Signature

```cpp
DataFrame cast(self,  std::initializer_list<std::pair<std::string_view, pluss::codem::ScalarType>> feature_dtypes) const
```

#### Parameters

`feature_dtypes` : `std::initializer_list<std::pair<std::string_view, pluss::codem::ScalarType>>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This changes the semantic or numeric type so later operations route through the correct implementation path.

#### See also

- `DataFrame cast(self, std::initializer_list<CastSpec> specs) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({{"sales", pluss::table::Thread(pluss::make_note({"100.0", "120.0", "90.0"}))}});

auto out = df.astype({{"sales", pluss::codem::ScalarType::Float64}});
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `cast`

#### Signature

```cpp
DataFrame cast(self, std::initializer_list<CastSpec> specs) const
```

#### Parameters

`specs` : `std::initializer_list<CastSpec>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This changes the semantic or numeric type so later operations route through the correct implementation path.

#### See also

- `DataFrame cast(self,  std::initializer_list<std::pair<std::string_view, pluss::codem::ScalarType>> feature_dtypes) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({{"sales", pluss::table::Thread(pluss::make_note({"100.0", "120.0", "90.0"}))}});

auto out = df.cast({{.feature = "sales", .target = pluss::table::ThreadCastTarget::Series, .series_dtype = pluss::codem::ScalarType::Float64}});
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `cast_note`

#### Signature

```cpp
DataFrame cast_note(self, std::initializer_list<std::string_view> features) const
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This changes the semantic or numeric type so later operations route through the correct implementation path.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.cast_note({"sales"});
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `cast_series`

#### Signature

```cpp
DataFrame cast_series(self, std::initializer_list<std::string_view> features, std::optional<pluss::codem::ScalarType> dtype = std::nullopt) const
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

`dtype` : `std::optional<pluss::codem::ScalarType>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This changes the semantic or numeric type so later operations route through the correct implementation path.

#### Examples

```cpp
auto df = pluss::table::DataFrame({{"sales", pluss::table::Thread(pluss::make_note({"100.0", "120.0", "90.0"}))}});

auto out = df.cast_series({"sales"}, pluss::codem::ScalarType::Float64);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `cast_timeindex`

#### Signature

```cpp
DataFrame cast_timeindex(self, std::initializer_list<std::string_view> features) const
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This changes the semantic or numeric type so later operations route through the correct implementation path.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.cast_timeindex({"sales"});
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `cast_timeline`

#### Signature

```cpp
DataFrame cast_timeline(self, std::initializer_list<std::string_view> features) const
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This changes the semantic or numeric type so later operations route through the correct implementation path.

#### Examples

```cpp
auto df = pluss::table::DataFrame({{"ts", pluss::table::Thread(pluss::make_note({"2024-01-01T09:10:00Z", "2024-01-01T09:40:00Z"}))}});

auto out = df.cast_timeline({"ts"});
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `ceil_time`

#### Signature

```cpp
DataFrame ceil_time(self, std::initializer_list<std::string_view> features, std::string_view unit) const
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

`unit` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This is part of the time-aware surface and expects timeline or time-index semantics.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"ts", pluss::table::Thread(pluss::make_timeline({
        "2024-01-01T09:10:00Z",
        "2024-01-01T09:40:00Z",
        "2024-01-01T10:05:00Z",
    }))},
    {"load", pluss::table::Thread(pluss::make_series({10.0, 20.0, 15.0}))},
});

auto out = df.ceil_time({"ts"}, "hour");
```

#### Output

```text
ts
2024-01-01T09:00:00Z
2024-01-01T10:00:00Z
```

### `column`

#### Signature

```cpp
Thread column(self, std::string_view name) const
```

#### Parameters

`name` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.column("sales");
```

#### Output

```text
Thread([100.0, 120.0, 90.0])
```

### `column_names`

#### Signature

```cpp
const std::vector<std::string>& column_names(self) const
```

#### Parameters

`None`

#### Returns

`const std::vector<std::string>&`
    Borrowed list of names.

#### Notes

Returns the current labels used to address fields, slots, rows, or depth items.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.column_names();
```

#### Output

```text
["id", "sales", "city"]
```

### `column_ref`

#### Signature

```cpp
ColumnRef column_ref(self, std::string_view name) const
```

#### Parameters

`name` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`ColumnRef`
    Value produced by this call.

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.column_ref("sales");
```

### `columns`

#### Signature

```cpp
std::vector<Thread> columns(self) const
```

#### Parameters

`None`

#### Returns

`std::vector<Thread>`
    Materialized collection of thread objects.

#### Notes

Materializes the current members as a collection you can inspect or reuse elsewhere.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.columns();
```

#### Output

```text
[Thread(id), Thread(sales), Thread(city)]
```

### `combine`

#### Signature

```cpp
DataFrame combine(self, const DataFrame& other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Combines this frame with another frame according to the supplied combine policy, such as feature-wise update or join-like alignment.

#### See also

- `DataFrame combine(self, const DataFrame& other, const CombineSpec& spec) const`

#### Examples

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"cost", pluss::table::Thread(pluss::make_series({60.0, 80.0, 55.0}))},
});

auto out = left.combine(right);
```

#### Output

```text
id  sales  cost
1   100.0  60.0
2   120.0  80.0
3    90.0  55.0
```

### `combine`

#### Signature

```cpp
DataFrame combine(self, const DataFrame& other, const CombineSpec& spec) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

`spec` : `const CombineSpec&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Combines this frame with another frame according to the supplied combine policy, such as feature-wise update or join-like alignment.

#### See also

- `DataFrame combine(self, const DataFrame& other) const`

#### Examples

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"cost", pluss::table::Thread(pluss::make_series({60.0, 80.0, 55.0}))},
});

pluss::table::DataFrame::CombineSpec spec;
spec.op = pluss::table::DataFrame::CombineOp::Update;
auto out = left.combine(right, spec);
```

#### Output

```text
id  sales  cost
1   100.0  60.0
2   120.0  80.0
3    90.0  55.0
```

### `compare`

#### Signature

```cpp
DataFrame compare(self, const DataFrame& other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This produces a comparison-style result you can inspect directly or reuse as a mask.

#### See also

- `DataFrame compare(self, scalar other) const`

#### Examples

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"cost", pluss::table::Thread(pluss::make_series({60.0, 80.0, 55.0}))},
});

auto out = left.compare(right);
```

#### Output

```text
id  sales
1   true
2   true
3   false
```

### `compare`

#### Signature

```cpp
DataFrame compare(self, scalar other) const
```

#### Parameters

`other` : `scalar`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This produces a comparison-style result you can inspect directly or reuse as a mask.

#### See also

- `DataFrame compare(self, const DataFrame& other) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.compare(10);
```

#### Output

```text
id  sales
1   true
2   true
3   false
```

### `conform_to`

#### Signature

```cpp
DataFrame conform_to(self, const DataFrame& other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Reorders and aligns this frame so it is compatible with another frame for downstream joins or arithmetic.

#### Examples

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"cost", pluss::table::Thread(pluss::make_series({60.0, 80.0, 55.0}))},
});

auto out = left.conform_to(right);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `copy`

#### Signature

```cpp
DataFrame copy(self) const
```

#### Parameters

`None`

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Returns a detached copy of the frame so later edits to the result do not affect the source object.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.copy();
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `count`

#### Signature

```cpp
DataFrame count(self, bool render = true) const
```

#### Parameters

`render` : `bool`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Returns a size/count summary for the current object without changing it.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.count(false);
```

#### Output

```text
feature  value
sales    310.0
```

### `count_by`

#### Signature

```cpp
DataFrame count_by(self, std::string_view feature, std::int64_t top_n = 0, bool dropna = true, bool render = true) const
```

#### Parameters

`feature` : `std::string_view`
    See the signature for the supported argument form.

`top_n` : `std::int64_t`
    See the signature for the supported argument form.

`dropna` : `bool`
    See the signature for the supported argument form.

`render` : `bool`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Groups by one feature and returns per-group row counts.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema"}))},
    {"product", pluss::table::Thread(pluss::make_note({"A", "B", "A", "B"}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0, 150.0}))},
});

auto out = df.count_by("city", 3, true, false);
```

#### Output

```text
city    count
Accra   2
Tema    2
```

### `count_null_rows`

#### Signature

```cpp
std::int64_t count_null_rows(self) const
```

#### Parameters

`None`

#### Returns

`std::int64_t`
    Integer count, size, or positional result.

#### Notes

Counts how many rows contain at least one null value.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.count_null_rows();
```

#### Output

```text
3
```

### `defined`

#### Signature

```cpp
bool defined(self) const
```

#### Parameters

`None`

#### Returns

`bool`
    Boolean status or membership result.

#### Notes

Reports whether the current object owns a concrete payload that can be used safely in later operations.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.defined();
```

#### Output

```text
true
```

### `describe`

#### Signature

```cpp
DataFrame describe(self, bool render = true) const
```

#### Parameters

`render` : `bool`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Returns descriptive summary statistics for the selected numeric features.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.describe(false);
```

#### Output

```text
feature  count  mean   min   max
sales    3      103.3  90.0  120.0
```

### `div`

#### Signature

```cpp
DataFrame div(self, const DataFrame& other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### See also

- `DataFrame div(self, scalar other) const`

#### Examples

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"cost", pluss::table::Thread(pluss::make_series({60.0, 80.0, 55.0}))},
});

auto out = left.div(right);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `div`

#### Signature

```cpp
DataFrame div(self, scalar other) const
```

#### Parameters

`other` : `scalar`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### See also

- `DataFrame div(self, const DataFrame& other) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.div(10);
```

#### Output

```text
id  sales  city
11  110.0  Accra
12  130.0  Tema
13  100.0  Kumasi
```

### `divide`

#### Signature

```cpp
DataFrame divide(self, const DataFrame& other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### See also

- `DataFrame divide(self, scalar other) const`

#### Examples

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"cost", pluss::table::Thread(pluss::make_series({60.0, 80.0, 55.0}))},
});

auto out = left.divide(right);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `divide`

#### Signature

```cpp
DataFrame divide(self, scalar other) const
```

#### Parameters

`other` : `scalar`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### See also

- `DataFrame divide(self, const DataFrame& other) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.divide(10);
```

#### Output

```text
id  sales  city
11  110.0  Accra
12  130.0  Tema
13  100.0  Kumasi
```

### `drop`

#### Signature

```cpp
DataFrame drop(self, std::initializer_list<std::string_view> names) const
```

#### Parameters

`names` : `std::initializer_list<std::string_view>`
    Collection of feature, slot, or stream names supplied in positional order.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Removes one or more features and returns the remaining frame.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.drop({"city"});
```

#### Output

```text
id  sales
1   100.0
2   120.0
3    90.0
```

### `dropna`

#### Signature

```cpp
DataFrame dropna(self) const
```

#### Parameters

`None`

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.dropna();
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `dropna_all`

#### Signature

```cpp
DataFrame dropna_all(self, std::initializer_list<std::string_view> features =
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.dropna_all({"sales"});
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `eq`

#### Signature

```cpp
DataFrame eq(self, const DataFrame& other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This produces a comparison-style result you can inspect directly or reuse as a mask.

#### See also

- `DataFrame eq(self, scalar other) const`

#### Examples

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"cost", pluss::table::Thread(pluss::make_series({60.0, 80.0, 55.0}))},
});

auto out = left.eq(right);
```

#### Output

```text
id  sales
1   true
2   true
3   false
```

### `eq`

#### Signature

```cpp
DataFrame eq(self, scalar other) const
```

#### Parameters

`other` : `scalar`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This produces a comparison-style result you can inspect directly or reuse as a mask.

#### See also

- `DataFrame eq(self, const DataFrame& other) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.eq(10);
```

#### Output

```text
id  sales
1   true
2   true
3   false
```

### `equals`

#### Signature

```cpp
bool equals(self, const DataFrame& other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`bool`
    Boolean status or membership result.

#### Notes

Checks whether two frames have the same structure, labels, and values.

#### Examples

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"cost", pluss::table::Thread(pluss::make_series({60.0, 80.0, 55.0}))},
});

auto out = left.equals(right);
```

#### Output

```text
true
```

### `feature_names`

#### Signature

```cpp
const std::vector<std::string>& feature_names(self) const
```

#### Parameters

`None`

#### Returns

`const std::vector<std::string>&`
    Borrowed list of names.

#### Notes

Returns the current labels used to address fields, slots, rows, or depth items.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.feature_names();
```

#### Output

```text
["id", "sales", "city"]
```

### `feature_ref`

#### Signature

```cpp
ColumnRef feature_ref(self, std::string_view name) const
```

#### Parameters

`name` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`ColumnRef`
    Value produced by this call.

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.feature_ref("sales");
```

### `features`

#### Signature

```cpp
DataFrame features(self, std::initializer_list<std::string_view> names)
```

#### Parameters

`names` : `std::initializer_list<std::string_view>`
    Collection of feature, slot, or stream names supplied in positional order.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Materializes the current members as a collection you can inspect or reuse elsewhere.

#### See also

- `std::vector<Thread> features(self) const`
- `Thread features(self, std::string_view name)`
- `Thread features(self, std::string_view name) const`
- `DataFrame features(self, std::initializer_list<std::string_view> names) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.features({"id", "sales"});
```

#### Output

```text
Thread([100.0, 120.0, 90.0])
```

### `features`

#### Signature

```cpp
DataFrame features(self, std::initializer_list<std::string_view> names) const
```

#### Parameters

`names` : `std::initializer_list<std::string_view>`
    Collection of feature, slot, or stream names supplied in positional order.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Materializes the current members as a collection you can inspect or reuse elsewhere.

#### See also

- `std::vector<Thread> features(self) const`
- `Thread features(self, std::string_view name)`
- `Thread features(self, std::string_view name) const`
- `DataFrame features(self, std::initializer_list<std::string_view> names)`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.features({"id", "sales"});
```

#### Output

```text
Thread([100.0, 120.0, 90.0])
```

### `features`

#### Signature

```cpp
std::vector<Thread> features(self) const
```

#### Parameters

`None`

#### Returns

`std::vector<Thread>`
    Materialized collection of thread objects.

#### Notes

Materializes the current members as a collection you can inspect or reuse elsewhere.

#### See also

- `Thread features(self, std::string_view name)`
- `Thread features(self, std::string_view name) const`
- `DataFrame features(self, std::initializer_list<std::string_view> names)`
- `DataFrame features(self, std::initializer_list<std::string_view> names) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.features();
```

#### Output

```text
[Thread(id), Thread(sales), Thread(city)]
```

### `features`

#### Signature

```cpp
Thread features(self, std::string_view name)
```

#### Parameters

`name` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

Materializes the current members as a collection you can inspect or reuse elsewhere.

#### See also

- `std::vector<Thread> features(self) const`
- `Thread features(self, std::string_view name) const`
- `DataFrame features(self, std::initializer_list<std::string_view> names)`
- `DataFrame features(self, std::initializer_list<std::string_view> names) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.features("sales");
```

#### Output

```text
Thread([100.0, 120.0, 90.0])
```

### `features`

#### Signature

```cpp
Thread features(self, std::string_view name) const
```

#### Parameters

`name` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Thread`
    Value produced by this call.

#### Notes

Materializes the current members as a collection you can inspect or reuse elsewhere.

#### See also

- `std::vector<Thread> features(self) const`
- `Thread features(self, std::string_view name)`
- `DataFrame features(self, std::initializer_list<std::string_view> names)`
- `DataFrame features(self, std::initializer_list<std::string_view> names) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.features("sales");
```

#### Output

```text
Thread([100.0, 120.0, 90.0])
```

### `ffill`

#### Signature

```cpp
DataFrame ffill(self) const
```

#### Parameters

`None`

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.ffill();
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `fillna`

#### Signature

```cpp
DataFrame fillna(self, const char* value) const
```

#### Parameters

`value` : `const char*`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### See also

- `DataFrame fillna(self, scalar value) const`
- `DataFrame fillna(self, scalar value, std::initializer_list<std::string_view> features) const`
- `DataFrame fillna(self, std::string_view value) const`
- `DataFrame fillna(self, std::string_view value, std::initializer_list<std::string_view> features) const`
- `DataFrame fillna(self, const char* value, std::initializer_list<std::string_view> features) const`
- `DataFrame fillna(self, const pluss::Timeline& value) const`
- `DataFrame fillna(self, const pluss::Timeline& value, std::initializer_list<std::string_view> features) const`
- `DataFrame fillna(self, const pluss::TimeIndex& value) const`
- `DataFrame fillna(self, const pluss::TimeIndex& value, std::initializer_list<std::string_view> features) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({{"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))}});

auto out = df.fillna("Unknown");
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `fillna`

#### Signature

```cpp
DataFrame fillna(self, const char* value, std::initializer_list<std::string_view> features) const
```

#### Parameters

`value` : `const char*`
    See the signature for the supported argument form.

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### See also

- `DataFrame fillna(self, scalar value) const`
- `DataFrame fillna(self, scalar value, std::initializer_list<std::string_view> features) const`
- `DataFrame fillna(self, std::string_view value) const`
- `DataFrame fillna(self, std::string_view value, std::initializer_list<std::string_view> features) const`
- `DataFrame fillna(self, const char* value) const`
- `DataFrame fillna(self, const pluss::Timeline& value) const`
- `DataFrame fillna(self, const pluss::Timeline& value, std::initializer_list<std::string_view> features) const`
- `DataFrame fillna(self, const pluss::TimeIndex& value) const`
- `DataFrame fillna(self, const pluss::TimeIndex& value, std::initializer_list<std::string_view> features) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({{"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))}});

auto out = df.fillna("Unknown");
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `fillna`

#### Signature

```cpp
DataFrame fillna(self, const pluss::TimeIndex& value) const
```

#### Parameters

`value` : `const pluss::TimeIndex&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### See also

- `DataFrame fillna(self, scalar value) const`
- `DataFrame fillna(self, scalar value, std::initializer_list<std::string_view> features) const`
- `DataFrame fillna(self, std::string_view value) const`
- `DataFrame fillna(self, std::string_view value, std::initializer_list<std::string_view> features) const`
- `DataFrame fillna(self, const char* value) const`
- `DataFrame fillna(self, const char* value, std::initializer_list<std::string_view> features) const`
- `DataFrame fillna(self, const pluss::Timeline& value) const`
- `DataFrame fillna(self, const pluss::Timeline& value, std::initializer_list<std::string_view> features) const`
- `DataFrame fillna(self, const pluss::TimeIndex& value, std::initializer_list<std::string_view> features) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({{"idx", pluss::table::Thread(pluss::make_timeindex({"2024-01-01T09:10:00Z", "2024-01-01T09:40:00Z"}))}});

auto out = df.fillna(pluss::make_timeindex("2024-01-01T00:00:00Z"));
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `fillna`

#### Signature

```cpp
DataFrame fillna(self, const pluss::TimeIndex& value, std::initializer_list<std::string_view> features) const
```

#### Parameters

`value` : `const pluss::TimeIndex&`
    See the signature for the supported argument form.

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### See also

- `DataFrame fillna(self, scalar value) const`
- `DataFrame fillna(self, scalar value, std::initializer_list<std::string_view> features) const`
- `DataFrame fillna(self, std::string_view value) const`
- `DataFrame fillna(self, std::string_view value, std::initializer_list<std::string_view> features) const`
- `DataFrame fillna(self, const char* value) const`
- `DataFrame fillna(self, const char* value, std::initializer_list<std::string_view> features) const`
- `DataFrame fillna(self, const pluss::Timeline& value) const`
- `DataFrame fillna(self, const pluss::Timeline& value, std::initializer_list<std::string_view> features) const`
- `DataFrame fillna(self, const pluss::TimeIndex& value) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({{"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))}});

auto out = df.fillna("Unknown");
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `fillna`

#### Signature

```cpp
DataFrame fillna(self, const pluss::Timeline& value) const
```

#### Parameters

`value` : `const pluss::Timeline&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### See also

- `DataFrame fillna(self, scalar value) const`
- `DataFrame fillna(self, scalar value, std::initializer_list<std::string_view> features) const`
- `DataFrame fillna(self, std::string_view value) const`
- `DataFrame fillna(self, std::string_view value, std::initializer_list<std::string_view> features) const`
- `DataFrame fillna(self, const char* value) const`
- `DataFrame fillna(self, const char* value, std::initializer_list<std::string_view> features) const`
- `DataFrame fillna(self, const pluss::Timeline& value, std::initializer_list<std::string_view> features) const`
- `DataFrame fillna(self, const pluss::TimeIndex& value) const`
- `DataFrame fillna(self, const pluss::TimeIndex& value, std::initializer_list<std::string_view> features) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({{"ts", pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-01-01T09:40:00Z"}))}});

auto out = df.fillna(pluss::make_timeline("2024-01-01T00:00:00Z"));
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `fillna`

#### Signature

```cpp
DataFrame fillna(self, const pluss::Timeline& value, std::initializer_list<std::string_view> features) const
```

#### Parameters

`value` : `const pluss::Timeline&`
    See the signature for the supported argument form.

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### See also

- `DataFrame fillna(self, scalar value) const`
- `DataFrame fillna(self, scalar value, std::initializer_list<std::string_view> features) const`
- `DataFrame fillna(self, std::string_view value) const`
- `DataFrame fillna(self, std::string_view value, std::initializer_list<std::string_view> features) const`
- `DataFrame fillna(self, const char* value) const`
- `DataFrame fillna(self, const char* value, std::initializer_list<std::string_view> features) const`
- `DataFrame fillna(self, const pluss::Timeline& value) const`
- `DataFrame fillna(self, const pluss::TimeIndex& value) const`
- `DataFrame fillna(self, const pluss::TimeIndex& value, std::initializer_list<std::string_view> features) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({{"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))}});

auto out = df.fillna("Unknown");
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `fillna`

#### Signature

```cpp
DataFrame fillna(self, scalar value) const
```

#### Parameters

`value` : `scalar`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### See also

- `DataFrame fillna(self, scalar value, std::initializer_list<std::string_view> features) const`
- `DataFrame fillna(self, std::string_view value) const`
- `DataFrame fillna(self, std::string_view value, std::initializer_list<std::string_view> features) const`
- `DataFrame fillna(self, const char* value) const`
- `DataFrame fillna(self, const char* value, std::initializer_list<std::string_view> features) const`
- `DataFrame fillna(self, const pluss::Timeline& value) const`
- `DataFrame fillna(self, const pluss::Timeline& value, std::initializer_list<std::string_view> features) const`
- `DataFrame fillna(self, const pluss::TimeIndex& value) const`
- `DataFrame fillna(self, const pluss::TimeIndex& value, std::initializer_list<std::string_view> features) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.fillna(0.0, {"sales"});
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `fillna`

#### Signature

```cpp
DataFrame fillna(self, scalar value, std::initializer_list<std::string_view> features) const
```

#### Parameters

`value` : `scalar`
    See the signature for the supported argument form.

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### See also

- `DataFrame fillna(self, scalar value) const`
- `DataFrame fillna(self, std::string_view value) const`
- `DataFrame fillna(self, std::string_view value, std::initializer_list<std::string_view> features) const`
- `DataFrame fillna(self, const char* value) const`
- `DataFrame fillna(self, const char* value, std::initializer_list<std::string_view> features) const`
- `DataFrame fillna(self, const pluss::Timeline& value) const`
- `DataFrame fillna(self, const pluss::Timeline& value, std::initializer_list<std::string_view> features) const`
- `DataFrame fillna(self, const pluss::TimeIndex& value) const`
- `DataFrame fillna(self, const pluss::TimeIndex& value, std::initializer_list<std::string_view> features) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({{"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))}});

auto out = df.fillna("Unknown");
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `fillna`

#### Signature

```cpp
DataFrame fillna(self, std::string_view value) const
```

#### Parameters

`value` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### See also

- `DataFrame fillna(self, scalar value) const`
- `DataFrame fillna(self, scalar value, std::initializer_list<std::string_view> features) const`
- `DataFrame fillna(self, std::string_view value, std::initializer_list<std::string_view> features) const`
- `DataFrame fillna(self, const char* value) const`
- `DataFrame fillna(self, const char* value, std::initializer_list<std::string_view> features) const`
- `DataFrame fillna(self, const pluss::Timeline& value) const`
- `DataFrame fillna(self, const pluss::Timeline& value, std::initializer_list<std::string_view> features) const`
- `DataFrame fillna(self, const pluss::TimeIndex& value) const`
- `DataFrame fillna(self, const pluss::TimeIndex& value, std::initializer_list<std::string_view> features) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({{"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))}});

auto out = df.fillna("Unknown");
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `fillna`

#### Signature

```cpp
DataFrame fillna(self, std::string_view value, std::initializer_list<std::string_view> features) const
```

#### Parameters

`value` : `std::string_view`
    See the signature for the supported argument form.

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### See also

- `DataFrame fillna(self, scalar value) const`
- `DataFrame fillna(self, scalar value, std::initializer_list<std::string_view> features) const`
- `DataFrame fillna(self, std::string_view value) const`
- `DataFrame fillna(self, const char* value) const`
- `DataFrame fillna(self, const char* value, std::initializer_list<std::string_view> features) const`
- `DataFrame fillna(self, const pluss::Timeline& value) const`
- `DataFrame fillna(self, const pluss::Timeline& value, std::initializer_list<std::string_view> features) const`
- `DataFrame fillna(self, const pluss::TimeIndex& value) const`
- `DataFrame fillna(self, const pluss::TimeIndex& value, std::initializer_list<std::string_view> features) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({{"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))}});

auto out = df.fillna("Unknown");
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `fillna_`

#### Signature

```cpp
DataFrame& fillna_(self, const char* value)
```

#### Parameters

`value` : `const char*`
    See the signature for the supported argument form.

#### Returns

`DataFrame&`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### See also

- `DataFrame& fillna_(self, scalar value)`
- `DataFrame& fillna_(self, std::string_view value)`
- `DataFrame& fillna_(self, const pluss::Timeline& value)`
- `DataFrame& fillna_(self, const pluss::TimeIndex& value)`

#### Examples

```cpp
auto df = pluss::table::DataFrame({{"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))}});

df.fillna_("Unknown");
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `fillna_`

#### Signature

```cpp
DataFrame& fillna_(self, const pluss::TimeIndex& value)
```

#### Parameters

`value` : `const pluss::TimeIndex&`
    See the signature for the supported argument form.

#### Returns

`DataFrame&`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### See also

- `DataFrame& fillna_(self, scalar value)`
- `DataFrame& fillna_(self, std::string_view value)`
- `DataFrame& fillna_(self, const char* value)`
- `DataFrame& fillna_(self, const pluss::Timeline& value)`

#### Examples

```cpp
auto df = pluss::table::DataFrame({{"idx", pluss::table::Thread(pluss::make_timeindex({"2024-01-01T09:10:00Z", "2024-01-01T09:40:00Z"}))}});

df.fillna_(pluss::make_timeindex("2024-01-01T00:00:00Z"));
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `fillna_`

#### Signature

```cpp
DataFrame& fillna_(self, const pluss::Timeline& value)
```

#### Parameters

`value` : `const pluss::Timeline&`
    See the signature for the supported argument form.

#### Returns

`DataFrame&`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### See also

- `DataFrame& fillna_(self, scalar value)`
- `DataFrame& fillna_(self, std::string_view value)`
- `DataFrame& fillna_(self, const char* value)`
- `DataFrame& fillna_(self, const pluss::TimeIndex& value)`

#### Examples

```cpp
auto df = pluss::table::DataFrame({{"ts", pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-01-01T09:40:00Z"}))}});

df.fillna_(pluss::make_timeline("2024-01-01T00:00:00Z"));
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `fillna_`

#### Signature

```cpp
DataFrame& fillna_(self, scalar value)
```

#### Parameters

`value` : `scalar`
    See the signature for the supported argument form.

#### Returns

`DataFrame&`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### See also

- `DataFrame& fillna_(self, std::string_view value)`
- `DataFrame& fillna_(self, const char* value)`
- `DataFrame& fillna_(self, const pluss::Timeline& value)`
- `DataFrame& fillna_(self, const pluss::TimeIndex& value)`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.fillna_(0.0);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `fillna_`

#### Signature

```cpp
DataFrame& fillna_(self, std::string_view value)
```

#### Parameters

`value` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`DataFrame&`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### See also

- `DataFrame& fillna_(self, scalar value)`
- `DataFrame& fillna_(self, const char* value)`
- `DataFrame& fillna_(self, const pluss::Timeline& value)`
- `DataFrame& fillna_(self, const pluss::TimeIndex& value)`

#### Examples

```cpp
auto df = pluss::table::DataFrame({{"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))}});

df.fillna_("Unknown");
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `filter`

#### Signature

```cpp
DataFrame filter(self, const pluss::Series& bool_mask) const
```

#### Parameters

`bool_mask` : `const pluss::Series&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Keeps only the rows where the supplied boolean mask is true.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = pluss::make_series({true, false, true}, pluss::codem::ScalarType::Bool);
auto out = df.filter(mask);
```

#### Output

```text
id  sales  city
1   100.0  Accra
3    90.0  Kumasi
```

### `floor_time`

#### Signature

```cpp
DataFrame floor_time(self, std::initializer_list<std::string_view> features, std::string_view unit) const
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

`unit` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This is part of the time-aware surface and expects timeline or time-index semantics.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"ts", pluss::table::Thread(pluss::make_timeline({
        "2024-01-01T09:10:00Z",
        "2024-01-01T09:40:00Z",
        "2024-01-01T10:05:00Z",
    }))},
    {"load", pluss::table::Thread(pluss::make_series({10.0, 20.0, 15.0}))},
});

auto out = df.floor_time({"ts"}, "hour");
```

#### Output

```text
ts
2024-01-01T09:00:00Z
2024-01-01T10:00:00Z
```

### `floordiv`

#### Signature

```cpp
DataFrame floordiv(self, const DataFrame& other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### See also

- `DataFrame floordiv(self, scalar other) const`

#### Examples

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"cost", pluss::table::Thread(pluss::make_series({60.0, 80.0, 55.0}))},
});

auto out = left.floordiv(right);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `floordiv`

#### Signature

```cpp
DataFrame floordiv(self, scalar other) const
```

#### Parameters

`other` : `scalar`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### See also

- `DataFrame floordiv(self, const DataFrame& other) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.floordiv(10);
```

#### Output

```text
id  sales  city
11  110.0  Accra
12  130.0  Tema
13  100.0  Kumasi
```

### `ge`

#### Signature

```cpp
DataFrame ge(self, const DataFrame& other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This produces a comparison-style result you can inspect directly or reuse as a mask.

#### See also

- `DataFrame ge(self, scalar other) const`

#### Examples

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"cost", pluss::table::Thread(pluss::make_series({60.0, 80.0, 55.0}))},
});

auto out = left.ge(right);
```

#### Output

```text
id  sales
1   true
2   true
3   false
```

### `ge`

#### Signature

```cpp
DataFrame ge(self, scalar other) const
```

#### Parameters

`other` : `scalar`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This produces a comparison-style result you can inspect directly or reuse as a mask.

#### See also

- `DataFrame ge(self, const DataFrame& other) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.ge(10);
```

#### Output

```text
id  sales
1   true
2   true
3   false
```

### `get`

#### Signature

```cpp
std::optional<Thread> get(self, std::string_view feature_name) const
```

#### Parameters

`feature_name` : `std::string_view`
    Name of the target feature to read, rewrite, sort by, or promote to cases.

#### Returns

`std::optional<Thread>`
    Value produced by this call.

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.get("sales");
```

#### Output

```text
Thread([100.0, 120.0, 90.0])
```

### `group_by`

#### Signature

```cpp
DataFrame group_by(self, std::initializer_list<std::string_view> keys, std::initializer_list<GroupSpec> specs, bool dropna_keys = true, bool render = true) const
```

#### Parameters

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

`specs` : `std::initializer_list<GroupSpec>`
    See the signature for the supported argument form.

`dropna_keys` : `bool`
    See the signature for the supported argument form.

`render` : `bool`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Groups rows by one or more key features and computes the requested aggregations for each group.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema"}))},
    {"product", pluss::table::Thread(pluss::make_note({"A", "B", "A", "B"}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0, 150.0}))},
});

auto out = df.group_by({"city"}, {{"sales_sum", "sales", pluss::table::DataFrame::GroupOp::Sum}}, true, false);
```

#### Output

```text
city    sales_sum
Accra   220.0
Tema    240.0
```

### `gt`

#### Signature

```cpp
DataFrame gt(self, const DataFrame& other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This produces a comparison-style result you can inspect directly or reuse as a mask.

#### See also

- `DataFrame gt(self, scalar other) const`

#### Examples

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"cost", pluss::table::Thread(pluss::make_series({60.0, 80.0, 55.0}))},
});

auto out = left.gt(right);
```

#### Output

```text
id  sales
1   true
2   true
3   false
```

### `gt`

#### Signature

```cpp
DataFrame gt(self, scalar other) const
```

#### Parameters

`other` : `scalar`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This produces a comparison-style result you can inspect directly or reuse as a mask.

#### See also

- `DataFrame gt(self, const DataFrame& other) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.gt(10);
```

#### Output

```text
id  sales
1   true
2   true
3   false
```

### `has_column`

#### Signature

```cpp
bool has_column(self, std::string_view name) const
```

#### Parameters

`name` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`bool`
    Boolean status or membership result.

#### Notes

Checks whether a feature name exists in the frame.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.has_column("sales");
```

#### Output

```text
true
```

### `has_feature`

#### Signature

```cpp
bool has_feature(self, std::string_view name) const
```

#### Parameters

`name` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`bool`
    Boolean status or membership result.

#### Notes

Checks whether a feature name exists in the frame.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.has_feature("sales");
```

#### Output

```text
true
```

### `head`

#### Signature

```cpp
DataFrame head(self, std::int64_t n = 5, bool render = true) const
```

#### Parameters

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

`render` : `bool`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Returns the first `n` rows of the frame.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.head(2, false);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `iat`

#### Signature

```cpp
std::string iat(self, std::int64_t row, std::size_t feature_idx) const
```

#### Parameters

`row` : `std::int64_t`
    See the signature for the supported argument form.

`feature_idx` : `std::size_t`
    See the signature for the supported argument form.

#### Returns

`std::string`
    Rendered string result.

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.iat(0, 0);
```

#### Output

```text
1
```

### `iloc`

#### Signature

```cpp
DataFrame iloc(self, std::int64_t row) const
```

#### Parameters

`row` : `std::int64_t`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### See also

- `DataFrame iloc(self, std::int64_t start, std::int64_t stop) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.iloc(1);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `iloc`

#### Signature

```cpp
DataFrame iloc(self, std::int64_t start, std::int64_t stop) const
```

#### Parameters

`start` : `std::int64_t`
    Inclusive starting position of the slice.

`stop` : `std::int64_t`
    Exclusive ending position of the slice.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### See also

- `DataFrame iloc(self, std::int64_t row) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.iloc(0, 2);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `info`

#### Signature

```cpp
void info(self, const display::DisplayOptions& options =
```

#### Parameters

`options` : `const display::DisplayOptions&`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

This renders a human-readable view for inspection instead of creating a new analytic object.

#### See also

- `void info(self, std::ostream& os, const display::DisplayOptions& options =`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.info();
```

#### Output

```text
rows: 3
features: 3
numeric: id, sales
note: city
```

### `info`

#### Signature

```cpp
void info(self, std::ostream& os, const display::DisplayOptions& options =
```

#### Parameters

`os` : `std::ostream&`
    See the signature for the supported argument form.

`options` : `const display::DisplayOptions&`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

This renders a human-readable view for inspection instead of creating a new analytic object.

#### See also

- `void info(self, const display::DisplayOptions& options =`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.info();
```

#### Output

```text
rows: 3
features: 3
numeric: id, sales
note: city
```

### `info_string`

#### Signature

```cpp
std::string info_string(self, const display::DisplayOptions& options =
```

#### Parameters

`options` : `const display::DisplayOptions&`
    See the signature for the supported argument form.

#### Returns

`std::string`
    Rendered string result.

#### Notes

Serializes or renders the current object into a text form you can inspect, log, or export.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.info_string();
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `insert`

#### Signature

```cpp
DataFrame insert(self, std::int64_t feature_index, std::string name, const Thread& feature) const
```

#### Parameters

`feature_index` : `std::int64_t`
    See the signature for the supported argument form.

`name` : `std::string`
    See the signature for the supported argument form.

`feature` : `const Thread&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Inserts a new feature at the requested position and shifts the later features to the right.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.insert(1, "profit", pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0})));
```

#### Output

```text
id  sales  city    profit
1   100.0  Accra   40.0
2   120.0  Tema    35.0
3    90.0  Kumasi  30.0
```

### `isna`

#### Signature

```cpp
DataFrame isna(self, std::initializer_list<std::string_view> features =
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.isna({"sales"});
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `isnull`

#### Signature

```cpp
DataFrame isnull(self, std::initializer_list<std::string_view> features =
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.isnull({"sales"});
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `join`

#### Signature

```cpp
DataFrame join(self, const DataFrame& other, std::string collision_suffix = "_right") const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

`collision_suffix` : `std::string`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Adds compatible features from another frame, aligning rows by the shared case axis.

#### Examples

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"cost", pluss::table::Thread(pluss::make_series({60.0, 80.0, 55.0}))},
});

auto out = left.join(right);
```

#### Output

```text
id  sales  cost
1   100.0  60.0
2   120.0  80.0
3    90.0  55.0
```

### `kindelize`

#### Signature

```cpp
pluss::kindle::Catalog kindelize(self) const
```

#### Parameters

`None`

#### Returns

`pluss::kindle::Catalog`
    Value produced by this call.

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### See also

- `pluss::kindle::Catalog kindelize(self, const pluss::kindle::KindelizePolicy& policy) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.kindelize();
```

### `kindelize`

#### Signature

```cpp
pluss::kindle::Catalog kindelize(self, const pluss::kindle::KindelizePolicy& policy) const
```

#### Parameters

`policy` : `const pluss::kindle::KindelizePolicy&`
    See the signature for the supported argument form.

#### Returns

`pluss::kindle::Catalog`
    Value produced by this call.

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### See also

- `pluss::kindle::Catalog kindelize(self) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.kindelize();
```

### `le`

#### Signature

```cpp
DataFrame le(self, const DataFrame& other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This produces a comparison-style result you can inspect directly or reuse as a mask.

#### See also

- `DataFrame le(self, scalar other) const`

#### Examples

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"cost", pluss::table::Thread(pluss::make_series({60.0, 80.0, 55.0}))},
});

auto out = left.le(right);
```

#### Output

```text
id  sales
1   true
2   true
3   false
```

### `le`

#### Signature

```cpp
DataFrame le(self, scalar other) const
```

#### Parameters

`other` : `scalar`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This produces a comparison-style result you can inspect directly or reuse as a mask.

#### See also

- `DataFrame le(self, const DataFrame& other) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.le(10);
```

#### Output

```text
id  sales
1   true
2   true
3   false
```

### `load`

#### Signature

```cpp
static DataFrame load(std::string_view source_type, std::string_view filepath)
```

#### Parameters

`source_type` : `std::string_view`
    Storage format selector such as `"csv"`, `"xlsx"`, `"json"`, or `"parquet"`.

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static DataFrame load(std::string_view source_type, std::string_view filepath, const pluss::loader::Config& config)`
- `static DataFrame load(std::string_view source_type, std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)`
- `static DataFrame load(std::string_view source_type, std::string_view filepath, const pluss::loader::ReadCsvOptions& options)`
- `static DataFrame load(std::string_view source_type, std::string_view filepath, char sep, std::optional<std::size_t> nrows = std::nullopt, std::vector<std::string> usecols =`
- `static DataFrame load(std::string_view source_type, std::string_view filepath, char sep, std::optional<std::size_t> nrows, std::initializer_list<std::string_view> usecols, std::optional<std::string> case_col = std::nullopt, pluss::loader::BadLinePolicy on_bad_lines = pluss::loader::BadLinePolicy::Error)`

#### Examples

```cpp
auto out = pluss::table::DataFrame::load("csv", "path/to/data.csv");
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `load`

#### Signature

```cpp
static DataFrame load(std::string_view source_type, std::string_view filepath, char sep, std::optional<std::size_t> nrows = std::nullopt, std::vector<std::string> usecols =
```

#### Parameters

`source_type` : `std::string_view`
    Storage format selector such as `"csv"`, `"xlsx"`, `"json"`, or `"parquet"`.

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`sep` : `char`
    Column separator used for delimited text files such as CSV.

`nrows` : `std::optional<std::size_t>`
    See the signature for the supported argument form.

`usecols` : `std::vector<std::string>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static DataFrame load(std::string_view source_type, std::string_view filepath)`
- `static DataFrame load(std::string_view source_type, std::string_view filepath, const pluss::loader::Config& config)`
- `static DataFrame load(std::string_view source_type, std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)`
- `static DataFrame load(std::string_view source_type, std::string_view filepath, const pluss::loader::ReadCsvOptions& options)`
- `static DataFrame load(std::string_view source_type, std::string_view filepath, char sep, std::optional<std::size_t> nrows, std::initializer_list<std::string_view> usecols, std::optional<std::string> case_col = std::nullopt, pluss::loader::BadLinePolicy on_bad_lines = pluss::loader::BadLinePolicy::Error)`

#### Examples

```cpp
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv", ',', 16, std::vector<std::string>{"Agency Name"}, std::nullopt);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `load`

#### Signature

```cpp
static DataFrame load(std::string_view source_type, std::string_view filepath, char sep, std::optional<std::size_t> nrows, std::initializer_list<std::string_view> usecols, std::optional<std::string> case_col = std::nullopt, pluss::loader::BadLinePolicy on_bad_lines = pluss::loader::BadLinePolicy::Error)
```

#### Parameters

`source_type` : `std::string_view`
    Storage format selector such as `"csv"`, `"xlsx"`, `"json"`, or `"parquet"`.

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`sep` : `char`
    Column separator used for delimited text files such as CSV.

`nrows` : `std::optional<std::size_t>`
    See the signature for the supported argument form.

`usecols` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

`case_col` : `std::optional<std::string>`
    See the signature for the supported argument form.

`on_bad_lines` : `pluss::loader::BadLinePolicy`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static DataFrame load(std::string_view source_type, std::string_view filepath)`
- `static DataFrame load(std::string_view source_type, std::string_view filepath, const pluss::loader::Config& config)`
- `static DataFrame load(std::string_view source_type, std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)`
- `static DataFrame load(std::string_view source_type, std::string_view filepath, const pluss::loader::ReadCsvOptions& options)`
- `static DataFrame load(std::string_view source_type, std::string_view filepath, char sep, std::optional<std::size_t> nrows = std::nullopt, std::vector<std::string> usecols =`

#### Examples

```cpp
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv", ',', 16, std::vector<std::string>{"Agency Name"}, std::nullopt);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `load`

#### Signature

```cpp
static DataFrame load(std::string_view source_type, std::string_view filepath, const pluss::loader::Config& config)
```

#### Parameters

`source_type` : `std::string_view`
    Storage format selector such as `"csv"`, `"xlsx"`, `"json"`, or `"parquet"`.

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static DataFrame load(std::string_view source_type, std::string_view filepath)`
- `static DataFrame load(std::string_view source_type, std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)`
- `static DataFrame load(std::string_view source_type, std::string_view filepath, const pluss::loader::ReadCsvOptions& options)`
- `static DataFrame load(std::string_view source_type, std::string_view filepath, char sep, std::optional<std::size_t> nrows = std::nullopt, std::vector<std::string> usecols =`
- `static DataFrame load(std::string_view source_type, std::string_view filepath, char sep, std::optional<std::size_t> nrows, std::initializer_list<std::string_view> usecols, std::optional<std::string> case_col = std::nullopt, pluss::loader::BadLinePolicy on_bad_lines = pluss::loader::BadLinePolicy::Error)`

#### Examples

```cpp
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv", {"nrows", 16});
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `load`

#### Signature

```cpp
static DataFrame load(std::string_view source_type, std::string_view filepath, const pluss::loader::ReadCsvOptions& options)
```

#### Parameters

`source_type` : `std::string_view`
    Storage format selector such as `"csv"`, `"xlsx"`, `"json"`, or `"parquet"`.

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`options` : `const pluss::loader::ReadCsvOptions&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static DataFrame load(std::string_view source_type, std::string_view filepath)`
- `static DataFrame load(std::string_view source_type, std::string_view filepath, const pluss::loader::Config& config)`
- `static DataFrame load(std::string_view source_type, std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)`
- `static DataFrame load(std::string_view source_type, std::string_view filepath, char sep, std::optional<std::size_t> nrows = std::nullopt, std::vector<std::string> usecols =`
- `static DataFrame load(std::string_view source_type, std::string_view filepath, char sep, std::optional<std::size_t> nrows, std::initializer_list<std::string_view> usecols, std::optional<std::string> case_col = std::nullopt, pluss::loader::BadLinePolicy on_bad_lines = pluss::loader::BadLinePolicy::Error)`

#### Examples

```cpp
pluss::loader::ReadCsvOptions options;
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv", options);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `load`

#### Signature

```cpp
static DataFrame load(std::string_view source_type, std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)
```

#### Parameters

`source_type` : `std::string_view`
    Storage format selector such as `"csv"`, `"xlsx"`, `"json"`, or `"parquet"`.

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config_entries` : `std::initializer_list<pluss::loader::ConfigEntry>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static DataFrame load(std::string_view source_type, std::string_view filepath)`
- `static DataFrame load(std::string_view source_type, std::string_view filepath, const pluss::loader::Config& config)`
- `static DataFrame load(std::string_view source_type, std::string_view filepath, const pluss::loader::ReadCsvOptions& options)`
- `static DataFrame load(std::string_view source_type, std::string_view filepath, char sep, std::optional<std::size_t> nrows = std::nullopt, std::vector<std::string> usecols =`
- `static DataFrame load(std::string_view source_type, std::string_view filepath, char sep, std::optional<std::size_t> nrows, std::initializer_list<std::string_view> usecols, std::optional<std::string> case_col = std::nullopt, pluss::loader::BadLinePolicy on_bad_lines = pluss::loader::BadLinePolicy::Error)`

#### Examples

```cpp
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv", {"nrows", 16});
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `load_csv`

#### Signature

```cpp
static DataFrame load_csv(std::string_view filepath)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static DataFrame load_csv(std::string_view filepath, const pluss::loader::ReadCsvOptions& options)`
- `static DataFrame load_csv(std::string_view filepath, const pluss::loader::Config& config)`
- `static DataFrame load_csv(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)`
- `static DataFrame load_csv(std::string_view filepath, std::string_view header, std::string_view case_feature = "none", std::string_view engine = "auto", std::optional<std::size_t> ncases = std::nullopt)`
- `static DataFrame load_csv(std::string_view filepath, std::string_view header, std::size_t ncases)`
- `static DataFrame load_csv(std::string_view filepath, char sep, std::optional<std::size_t> nrows = std::nullopt, std::vector<std::string> usecols =`
- `static DataFrame load_csv(std::string_view filepath, char sep, std::optional<std::size_t> nrows, std::initializer_list<std::string_view> usecols, std::optional<std::string> case_col = std::nullopt, pluss::loader::BadLinePolicy on_bad_lines = pluss::loader::BadLinePolicy::Error)`

#### Examples

```cpp
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv");
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `load_csv`

#### Signature

```cpp
static DataFrame load_csv(std::string_view filepath, char sep, std::optional<std::size_t> nrows = std::nullopt, std::vector<std::string> usecols =
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`sep` : `char`
    Column separator used for delimited text files such as CSV.

`nrows` : `std::optional<std::size_t>`
    See the signature for the supported argument form.

`usecols` : `std::vector<std::string>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static DataFrame load_csv(std::string_view filepath)`
- `static DataFrame load_csv(std::string_view filepath, const pluss::loader::ReadCsvOptions& options)`
- `static DataFrame load_csv(std::string_view filepath, const pluss::loader::Config& config)`
- `static DataFrame load_csv(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)`
- `static DataFrame load_csv(std::string_view filepath, std::string_view header, std::string_view case_feature = "none", std::string_view engine = "auto", std::optional<std::size_t> ncases = std::nullopt)`
- `static DataFrame load_csv(std::string_view filepath, std::string_view header, std::size_t ncases)`
- `static DataFrame load_csv(std::string_view filepath, char sep, std::optional<std::size_t> nrows, std::initializer_list<std::string_view> usecols, std::optional<std::string> case_col = std::nullopt, pluss::loader::BadLinePolicy on_bad_lines = pluss::loader::BadLinePolicy::Error)`

#### Examples

```cpp
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv", ',', 16, std::vector<std::string>{"Agency Name"}, std::nullopt);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `load_csv`

#### Signature

```cpp
static DataFrame load_csv(std::string_view filepath, char sep, std::optional<std::size_t> nrows, std::initializer_list<std::string_view> usecols, std::optional<std::string> case_col = std::nullopt, pluss::loader::BadLinePolicy on_bad_lines = pluss::loader::BadLinePolicy::Error)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`sep` : `char`
    Column separator used for delimited text files such as CSV.

`nrows` : `std::optional<std::size_t>`
    See the signature for the supported argument form.

`usecols` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

`case_col` : `std::optional<std::string>`
    See the signature for the supported argument form.

`on_bad_lines` : `pluss::loader::BadLinePolicy`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static DataFrame load_csv(std::string_view filepath)`
- `static DataFrame load_csv(std::string_view filepath, const pluss::loader::ReadCsvOptions& options)`
- `static DataFrame load_csv(std::string_view filepath, const pluss::loader::Config& config)`
- `static DataFrame load_csv(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)`
- `static DataFrame load_csv(std::string_view filepath, std::string_view header, std::string_view case_feature = "none", std::string_view engine = "auto", std::optional<std::size_t> ncases = std::nullopt)`
- `static DataFrame load_csv(std::string_view filepath, std::string_view header, std::size_t ncases)`
- `static DataFrame load_csv(std::string_view filepath, char sep, std::optional<std::size_t> nrows = std::nullopt, std::vector<std::string> usecols =`

#### Examples

```cpp
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv", ',', 16, std::vector<std::string>{"Agency Name"}, std::nullopt);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `load_csv`

#### Signature

```cpp
static DataFrame load_csv(std::string_view filepath, const pluss::loader::Config& config)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static DataFrame load_csv(std::string_view filepath)`
- `static DataFrame load_csv(std::string_view filepath, const pluss::loader::ReadCsvOptions& options)`
- `static DataFrame load_csv(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)`
- `static DataFrame load_csv(std::string_view filepath, std::string_view header, std::string_view case_feature = "none", std::string_view engine = "auto", std::optional<std::size_t> ncases = std::nullopt)`
- `static DataFrame load_csv(std::string_view filepath, std::string_view header, std::size_t ncases)`
- `static DataFrame load_csv(std::string_view filepath, char sep, std::optional<std::size_t> nrows = std::nullopt, std::vector<std::string> usecols =`
- `static DataFrame load_csv(std::string_view filepath, char sep, std::optional<std::size_t> nrows, std::initializer_list<std::string_view> usecols, std::optional<std::string> case_col = std::nullopt, pluss::loader::BadLinePolicy on_bad_lines = pluss::loader::BadLinePolicy::Error)`

#### Examples

```cpp
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv", {"nrows", 16});
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `load_csv`

#### Signature

```cpp
static DataFrame load_csv(std::string_view filepath, const pluss::loader::ReadCsvOptions& options)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`options` : `const pluss::loader::ReadCsvOptions&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static DataFrame load_csv(std::string_view filepath)`
- `static DataFrame load_csv(std::string_view filepath, const pluss::loader::Config& config)`
- `static DataFrame load_csv(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)`
- `static DataFrame load_csv(std::string_view filepath, std::string_view header, std::string_view case_feature = "none", std::string_view engine = "auto", std::optional<std::size_t> ncases = std::nullopt)`
- `static DataFrame load_csv(std::string_view filepath, std::string_view header, std::size_t ncases)`
- `static DataFrame load_csv(std::string_view filepath, char sep, std::optional<std::size_t> nrows = std::nullopt, std::vector<std::string> usecols =`
- `static DataFrame load_csv(std::string_view filepath, char sep, std::optional<std::size_t> nrows, std::initializer_list<std::string_view> usecols, std::optional<std::string> case_col = std::nullopt, pluss::loader::BadLinePolicy on_bad_lines = pluss::loader::BadLinePolicy::Error)`

#### Examples

```cpp
pluss::loader::ReadCsvOptions options;
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv", options);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `load_csv`

#### Signature

```cpp
static DataFrame load_csv(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config_entries` : `std::initializer_list<pluss::loader::ConfigEntry>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static DataFrame load_csv(std::string_view filepath)`
- `static DataFrame load_csv(std::string_view filepath, const pluss::loader::ReadCsvOptions& options)`
- `static DataFrame load_csv(std::string_view filepath, const pluss::loader::Config& config)`
- `static DataFrame load_csv(std::string_view filepath, std::string_view header, std::string_view case_feature = "none", std::string_view engine = "auto", std::optional<std::size_t> ncases = std::nullopt)`
- `static DataFrame load_csv(std::string_view filepath, std::string_view header, std::size_t ncases)`
- `static DataFrame load_csv(std::string_view filepath, char sep, std::optional<std::size_t> nrows = std::nullopt, std::vector<std::string> usecols =`
- `static DataFrame load_csv(std::string_view filepath, char sep, std::optional<std::size_t> nrows, std::initializer_list<std::string_view> usecols, std::optional<std::string> case_col = std::nullopt, pluss::loader::BadLinePolicy on_bad_lines = pluss::loader::BadLinePolicy::Error)`

#### Examples

```cpp
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv", {"nrows", 16});
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `load_csv`

#### Signature

```cpp
static DataFrame load_csv(std::string_view filepath, std::string_view header, std::size_t ncases)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`header` : `std::string_view`
    Controls whether feature names are written to, or expected from, the file header row.

`ncases` : `std::size_t`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static DataFrame load_csv(std::string_view filepath)`
- `static DataFrame load_csv(std::string_view filepath, const pluss::loader::ReadCsvOptions& options)`
- `static DataFrame load_csv(std::string_view filepath, const pluss::loader::Config& config)`
- `static DataFrame load_csv(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)`
- `static DataFrame load_csv(std::string_view filepath, std::string_view header, std::string_view case_feature = "none", std::string_view engine = "auto", std::optional<std::size_t> ncases = std::nullopt)`
- `static DataFrame load_csv(std::string_view filepath, char sep, std::optional<std::size_t> nrows = std::nullopt, std::vector<std::string> usecols =`
- `static DataFrame load_csv(std::string_view filepath, char sep, std::optional<std::size_t> nrows, std::initializer_list<std::string_view> usecols, std::optional<std::string> case_col = std::nullopt, pluss::loader::BadLinePolicy on_bad_lines = pluss::loader::BadLinePolicy::Error)`

#### Examples

```cpp
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv");
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `load_csv`

#### Signature

```cpp
static DataFrame load_csv(std::string_view filepath, std::string_view header, std::string_view case_feature = "none", std::string_view engine = "auto", std::optional<std::size_t> ncases = std::nullopt)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`header` : `std::string_view`
    Controls whether feature names are written to, or expected from, the file header row.

`case_feature` : `std::string_view`
    See the signature for the supported argument form.

`engine` : `std::string_view`
    See the signature for the supported argument form.

`ncases` : `std::optional<std::size_t>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static DataFrame load_csv(std::string_view filepath)`
- `static DataFrame load_csv(std::string_view filepath, const pluss::loader::ReadCsvOptions& options)`
- `static DataFrame load_csv(std::string_view filepath, const pluss::loader::Config& config)`
- `static DataFrame load_csv(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)`
- `static DataFrame load_csv(std::string_view filepath, std::string_view header, std::size_t ncases)`
- `static DataFrame load_csv(std::string_view filepath, char sep, std::optional<std::size_t> nrows = std::nullopt, std::vector<std::string> usecols =`
- `static DataFrame load_csv(std::string_view filepath, char sep, std::optional<std::size_t> nrows, std::initializer_list<std::string_view> usecols, std::optional<std::string> case_col = std::nullopt, pluss::loader::BadLinePolicy on_bad_lines = pluss::loader::BadLinePolicy::Error)`

#### Examples

```cpp
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv");
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `load_excel`

#### Signature

```cpp
static DataFrame load_excel(std::string_view filepath)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static DataFrame load_excel(std::string_view filepath, const pluss::loader::ReadExcelOptions& options)`
- `static DataFrame load_excel(std::string_view filepath, const pluss::loader::Config& config)`
- `static DataFrame load_excel(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)`
- `static DataFrame load_excel(std::string_view filepath, std::string_view sheet_name, bool header = true)`
- `static DataFrame load_excel(std::string_view filepath, std::int64_t sheet_index, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, std::nullptr_t, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, const std::vector<std::string>& sheet_names, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, std::initializer_list<std::string_view> sheet_names, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, const std::vector<std::int64_t>& sheet_indices, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, std::initializer_list<std::int64_t> sheet_indices, bool header = true)`

#### Examples

```cpp
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx");
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `load_excel`

#### Signature

```cpp
static DataFrame load_excel(std::string_view filepath, const pluss::loader::Config& config)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static DataFrame load_excel(std::string_view filepath)`
- `static DataFrame load_excel(std::string_view filepath, const pluss::loader::ReadExcelOptions& options)`
- `static DataFrame load_excel(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)`
- `static DataFrame load_excel(std::string_view filepath, std::string_view sheet_name, bool header = true)`
- `static DataFrame load_excel(std::string_view filepath, std::int64_t sheet_index, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, std::nullptr_t, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, const std::vector<std::string>& sheet_names, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, std::initializer_list<std::string_view> sheet_names, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, const std::vector<std::int64_t>& sheet_indices, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, std::initializer_list<std::int64_t> sheet_indices, bool header = true)`

#### Examples

```cpp
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv", {"nrows", 16});
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `load_excel`

#### Signature

```cpp
static DataFrame load_excel(std::string_view filepath, const pluss::loader::ReadExcelOptions& options)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`options` : `const pluss::loader::ReadExcelOptions&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static DataFrame load_excel(std::string_view filepath)`
- `static DataFrame load_excel(std::string_view filepath, const pluss::loader::Config& config)`
- `static DataFrame load_excel(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)`
- `static DataFrame load_excel(std::string_view filepath, std::string_view sheet_name, bool header = true)`
- `static DataFrame load_excel(std::string_view filepath, std::int64_t sheet_index, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, std::nullptr_t, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, const std::vector<std::string>& sheet_names, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, std::initializer_list<std::string_view> sheet_names, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, const std::vector<std::int64_t>& sheet_indices, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, std::initializer_list<std::int64_t> sheet_indices, bool header = true)`

#### Examples

```cpp
pluss::loader::ReadExcelOptions options;
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx", options);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `load_excel`

#### Signature

```cpp
static DataFrame load_excel(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config_entries` : `std::initializer_list<pluss::loader::ConfigEntry>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static DataFrame load_excel(std::string_view filepath)`
- `static DataFrame load_excel(std::string_view filepath, const pluss::loader::ReadExcelOptions& options)`
- `static DataFrame load_excel(std::string_view filepath, const pluss::loader::Config& config)`
- `static DataFrame load_excel(std::string_view filepath, std::string_view sheet_name, bool header = true)`
- `static DataFrame load_excel(std::string_view filepath, std::int64_t sheet_index, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, std::nullptr_t, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, const std::vector<std::string>& sheet_names, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, std::initializer_list<std::string_view> sheet_names, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, const std::vector<std::int64_t>& sheet_indices, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, std::initializer_list<std::int64_t> sheet_indices, bool header = true)`

#### Examples

```cpp
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv", {"nrows", 16});
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `load_excel`

#### Signature

```cpp
static DataFrame load_excel(std::string_view filepath, std::int64_t sheet_index, bool header = true)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`sheet_index` : `std::int64_t`
    Zero-based worksheet position inside an Excel workbook.

`header` : `bool`
    Controls whether feature names are written to, or expected from, the file header row.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static DataFrame load_excel(std::string_view filepath)`
- `static DataFrame load_excel(std::string_view filepath, const pluss::loader::ReadExcelOptions& options)`
- `static DataFrame load_excel(std::string_view filepath, const pluss::loader::Config& config)`
- `static DataFrame load_excel(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)`
- `static DataFrame load_excel(std::string_view filepath, std::string_view sheet_name, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, std::nullptr_t, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, const std::vector<std::string>& sheet_names, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, std::initializer_list<std::string_view> sheet_names, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, const std::vector<std::int64_t>& sheet_indices, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, std::initializer_list<std::int64_t> sheet_indices, bool header = true)`

#### Examples

```cpp
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx", 0, true);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `load_excel`

#### Signature

```cpp
static DataFrame load_excel(std::string_view filepath, std::string_view sheet_name, bool header = true)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`sheet_name` : `std::string_view`
    Worksheet name to read from or write to inside an Excel workbook.

`header` : `bool`
    Controls whether feature names are written to, or expected from, the file header row.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static DataFrame load_excel(std::string_view filepath)`
- `static DataFrame load_excel(std::string_view filepath, const pluss::loader::ReadExcelOptions& options)`
- `static DataFrame load_excel(std::string_view filepath, const pluss::loader::Config& config)`
- `static DataFrame load_excel(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)`
- `static DataFrame load_excel(std::string_view filepath, std::int64_t sheet_index, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, std::nullptr_t, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, const std::vector<std::string>& sheet_names, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, std::initializer_list<std::string_view> sheet_names, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, const std::vector<std::int64_t>& sheet_indices, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, std::initializer_list<std::int64_t> sheet_indices, bool header = true)`

#### Examples

```cpp
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx", "Sheet1", true);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `load_excel`

#### Signature

```cpp
static pluss::loader::LoadResult load_excel(std::string_view filepath, const std::vector<std::int64_t>& sheet_indices, bool header = true)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`sheet_indices` : `const std::vector<std::int64_t>&`
    See the signature for the supported argument form.

`header` : `bool`
    Controls whether feature names are written to, or expected from, the file header row.

#### Returns

`pluss::loader::LoadResult`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static DataFrame load_excel(std::string_view filepath)`
- `static DataFrame load_excel(std::string_view filepath, const pluss::loader::ReadExcelOptions& options)`
- `static DataFrame load_excel(std::string_view filepath, const pluss::loader::Config& config)`
- `static DataFrame load_excel(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)`
- `static DataFrame load_excel(std::string_view filepath, std::string_view sheet_name, bool header = true)`
- `static DataFrame load_excel(std::string_view filepath, std::int64_t sheet_index, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, std::nullptr_t, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, const std::vector<std::string>& sheet_names, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, std::initializer_list<std::string_view> sheet_names, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, std::initializer_list<std::int64_t> sheet_indices, bool header = true)`

#### Examples

```cpp
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx");
```

### `load_excel`

#### Signature

```cpp
static pluss::loader::LoadResult load_excel(std::string_view filepath, const std::vector<std::string>& sheet_names, bool header = true)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`sheet_names` : `const std::vector<std::string>&`
    See the signature for the supported argument form.

`header` : `bool`
    Controls whether feature names are written to, or expected from, the file header row.

#### Returns

`pluss::loader::LoadResult`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static DataFrame load_excel(std::string_view filepath)`
- `static DataFrame load_excel(std::string_view filepath, const pluss::loader::ReadExcelOptions& options)`
- `static DataFrame load_excel(std::string_view filepath, const pluss::loader::Config& config)`
- `static DataFrame load_excel(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)`
- `static DataFrame load_excel(std::string_view filepath, std::string_view sheet_name, bool header = true)`
- `static DataFrame load_excel(std::string_view filepath, std::int64_t sheet_index, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, std::nullptr_t, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, std::initializer_list<std::string_view> sheet_names, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, const std::vector<std::int64_t>& sheet_indices, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, std::initializer_list<std::int64_t> sheet_indices, bool header = true)`

#### Examples

```cpp
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx", "Sheet1", true);
```

### `load_excel`

#### Signature

```cpp
static pluss::loader::LoadResult load_excel(std::string_view filepath, std::initializer_list<std::int64_t> sheet_indices, bool header = true)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`sheet_indices` : `std::initializer_list<std::int64_t>`
    See the signature for the supported argument form.

`header` : `bool`
    Controls whether feature names are written to, or expected from, the file header row.

#### Returns

`pluss::loader::LoadResult`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static DataFrame load_excel(std::string_view filepath)`
- `static DataFrame load_excel(std::string_view filepath, const pluss::loader::ReadExcelOptions& options)`
- `static DataFrame load_excel(std::string_view filepath, const pluss::loader::Config& config)`
- `static DataFrame load_excel(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)`
- `static DataFrame load_excel(std::string_view filepath, std::string_view sheet_name, bool header = true)`
- `static DataFrame load_excel(std::string_view filepath, std::int64_t sheet_index, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, std::nullptr_t, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, const std::vector<std::string>& sheet_names, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, std::initializer_list<std::string_view> sheet_names, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, const std::vector<std::int64_t>& sheet_indices, bool header = true)`

#### Examples

```cpp
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx");
```

### `load_excel`

#### Signature

```cpp
static pluss::loader::LoadResult load_excel(std::string_view filepath, std::initializer_list<std::string_view> sheet_names, bool header = true)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`sheet_names` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

`header` : `bool`
    Controls whether feature names are written to, or expected from, the file header row.

#### Returns

`pluss::loader::LoadResult`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static DataFrame load_excel(std::string_view filepath)`
- `static DataFrame load_excel(std::string_view filepath, const pluss::loader::ReadExcelOptions& options)`
- `static DataFrame load_excel(std::string_view filepath, const pluss::loader::Config& config)`
- `static DataFrame load_excel(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)`
- `static DataFrame load_excel(std::string_view filepath, std::string_view sheet_name, bool header = true)`
- `static DataFrame load_excel(std::string_view filepath, std::int64_t sheet_index, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, std::nullptr_t, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, const std::vector<std::string>& sheet_names, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, const std::vector<std::int64_t>& sheet_indices, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, std::initializer_list<std::int64_t> sheet_indices, bool header = true)`

#### Examples

```cpp
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx", "Sheet1", true);
```

### `load_excel`

#### Signature

```cpp
static pluss::loader::LoadResult load_excel(std::string_view filepath, std::nullptr_t, bool header = true)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`arg2` : `std::nullptr_t`
    See the signature for the supported argument form.

`header` : `bool`
    Controls whether feature names are written to, or expected from, the file header row.

#### Returns

`pluss::loader::LoadResult`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static DataFrame load_excel(std::string_view filepath)`
- `static DataFrame load_excel(std::string_view filepath, const pluss::loader::ReadExcelOptions& options)`
- `static DataFrame load_excel(std::string_view filepath, const pluss::loader::Config& config)`
- `static DataFrame load_excel(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)`
- `static DataFrame load_excel(std::string_view filepath, std::string_view sheet_name, bool header = true)`
- `static DataFrame load_excel(std::string_view filepath, std::int64_t sheet_index, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, const std::vector<std::string>& sheet_names, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, std::initializer_list<std::string_view> sheet_names, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, const std::vector<std::int64_t>& sheet_indices, bool header = true)`
- `static pluss::loader::LoadResult load_excel(std::string_view filepath, std::initializer_list<std::int64_t> sheet_indices, bool header = true)`

#### Examples

```cpp
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx", nullptr, true);
```

### `load_json`

#### Signature

```cpp
static DataFrame load_json(std::string_view filepath)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static DataFrame load_json(std::string_view filepath, const pluss::loader::Config& config)`
- `static DataFrame load_json(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)`

#### Examples

```cpp
auto out = pluss::table::DataFrame::load_json("path/to/data.json");
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `load_json`

#### Signature

```cpp
static DataFrame load_json(std::string_view filepath, const pluss::loader::Config& config)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static DataFrame load_json(std::string_view filepath)`
- `static DataFrame load_json(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)`

#### Examples

```cpp
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv", {"nrows", 16});
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `load_json`

#### Signature

```cpp
static DataFrame load_json(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config_entries` : `std::initializer_list<pluss::loader::ConfigEntry>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static DataFrame load_json(std::string_view filepath)`
- `static DataFrame load_json(std::string_view filepath, const pluss::loader::Config& config)`

#### Examples

```cpp
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv", {"nrows", 16});
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `load_parquet`

#### Signature

```cpp
static DataFrame load_parquet(std::string_view filepath)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static DataFrame load_parquet(std::string_view filepath, const pluss::loader::Config& config)`
- `static DataFrame load_parquet(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)`

#### Examples

```cpp
auto out = pluss::table::DataFrame::load_parquet("path/to/data.parquet");
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `load_parquet`

#### Signature

```cpp
static DataFrame load_parquet(std::string_view filepath, const pluss::loader::Config& config)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static DataFrame load_parquet(std::string_view filepath)`
- `static DataFrame load_parquet(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)`

#### Examples

```cpp
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv", {"nrows", 16});
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `load_parquet`

#### Signature

```cpp
static DataFrame load_parquet(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config_entries` : `std::initializer_list<pluss::loader::ConfigEntry>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static DataFrame load_parquet(std::string_view filepath)`
- `static DataFrame load_parquet(std::string_view filepath, const pluss::loader::Config& config)`

#### Examples

```cpp
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv", {"nrows", 16});
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `load_txt`

#### Signature

```cpp
static DataFrame load_txt(std::string_view filepath)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static DataFrame load_txt(std::string_view filepath, const pluss::loader::Config& config)`
- `static DataFrame load_txt(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)`

#### Examples

```cpp
auto out = pluss::table::DataFrame::load_txt("path/to/data.txt");
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `load_txt`

#### Signature

```cpp
static DataFrame load_txt(std::string_view filepath, const pluss::loader::Config& config)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static DataFrame load_txt(std::string_view filepath)`
- `static DataFrame load_txt(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)`

#### Examples

```cpp
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv", {"nrows", 16});
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `load_txt`

#### Signature

```cpp
static DataFrame load_txt(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config_entries` : `std::initializer_list<pluss::loader::ConfigEntry>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static DataFrame load_txt(std::string_view filepath)`
- `static DataFrame load_txt(std::string_view filepath, const pluss::loader::Config& config)`

#### Examples

```cpp
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv", {"nrows", 16});
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `lt`

#### Signature

```cpp
DataFrame lt(self, const DataFrame& other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This produces a comparison-style result you can inspect directly or reuse as a mask.

#### See also

- `DataFrame lt(self, scalar other) const`

#### Examples

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"cost", pluss::table::Thread(pluss::make_series({60.0, 80.0, 55.0}))},
});

auto out = left.lt(right);
```

#### Output

```text
id  sales
1   true
2   true
3   false
```

### `lt`

#### Signature

```cpp
DataFrame lt(self, scalar other) const
```

#### Parameters

`other` : `scalar`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This produces a comparison-style result you can inspect directly or reuse as a mask.

#### See also

- `DataFrame lt(self, const DataFrame& other) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.lt(10);
```

#### Output

```text
id  sales
1   true
2   true
3   false
```

### `max`

#### Signature

```cpp
DataFrame max(self, bool render = true) const
```

#### Parameters

`render` : `bool`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Computes per-feature maxima for the selected numeric features.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.max(false);
```

#### Output

```text
feature  value
sales    310.0
```

### `mean`

#### Signature

```cpp
DataFrame mean(self, bool render = true) const
```

#### Parameters

`render` : `bool`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Computes per-feature means for the selected numeric features.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.mean(false);
```

#### Output

```text
feature  value
sales    310.0
```

### `mean_by`

#### Signature

```cpp
DataFrame mean_by(self, std::string_view group_feature, std::string_view value_feature, std::int64_t top_n = 0, std::int64_t min_count = 1, bool dropna_group = true, bool render = true) const
```

#### Parameters

`group_feature` : `std::string_view`
    See the signature for the supported argument form.

`value_feature` : `std::string_view`
    See the signature for the supported argument form.

`top_n` : `std::int64_t`
    See the signature for the supported argument form.

`min_count` : `std::int64_t`
    See the signature for the supported argument form.

`dropna_group` : `bool`
    See the signature for the supported argument form.

`render` : `bool`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Groups by one feature and returns the mean of another feature within each group.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema"}))},
    {"product", pluss::table::Thread(pluss::make_note({"A", "B", "A", "B"}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0, 150.0}))},
});

auto out = df.mean_by("city", "sales", 3, 1, true, false);
```

#### Output

```text
city    sales_mean
Accra   110.0
Tema    120.0
```

### `min`

#### Signature

```cpp
DataFrame min(self, bool render = true) const
```

#### Parameters

`render` : `bool`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Computes per-feature minima for the selected numeric features.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.min(false);
```

#### Output

```text
feature  value
sales    310.0
```

### `mod`

#### Signature

```cpp
DataFrame mod(self, const DataFrame& other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### See also

- `DataFrame mod(self, scalar other) const`

#### Examples

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"cost", pluss::table::Thread(pluss::make_series({60.0, 80.0, 55.0}))},
});

auto out = left.mod(right);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `mod`

#### Signature

```cpp
DataFrame mod(self, scalar other) const
```

#### Parameters

`other` : `scalar`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### See also

- `DataFrame mod(self, const DataFrame& other) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.mod(10);
```

#### Output

```text
id  sales  city
11  110.0  Accra
12  130.0  Tema
13  100.0  Kumasi
```

### `mul`

#### Signature

```cpp
DataFrame mul(self, const DataFrame& other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### See also

- `DataFrame mul(self, scalar other) const`

#### Examples

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"cost", pluss::table::Thread(pluss::make_series({60.0, 80.0, 55.0}))},
});

auto out = left.mul(right);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `mul`

#### Signature

```cpp
DataFrame mul(self, scalar other) const
```

#### Parameters

`other` : `scalar`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### See also

- `DataFrame mul(self, const DataFrame& other) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.mul(10);
```

#### Output

```text
id  sales  city
11  110.0  Accra
12  130.0  Tema
13  100.0  Kumasi
```

### `multiply`

#### Signature

```cpp
DataFrame multiply(self, const DataFrame& other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### See also

- `DataFrame multiply(self, scalar other) const`

#### Examples

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"cost", pluss::table::Thread(pluss::make_series({60.0, 80.0, 55.0}))},
});

auto out = left.multiply(right);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `multiply`

#### Signature

```cpp
DataFrame multiply(self, scalar other) const
```

#### Parameters

`other` : `scalar`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### See also

- `DataFrame multiply(self, const DataFrame& other) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.multiply(10);
```

#### Output

```text
id  sales  city
11  110.0  Accra
12  130.0  Tema
13  100.0  Kumasi
```

### `ncols`

#### Signature

```cpp
std::int64_t ncols(self) const
```

#### Parameters

`None`

#### Returns

`std::int64_t`
    Integer count, size, or positional result.

#### Notes

Returns a size/count summary for the current object without changing it.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.ncols();
```

#### Output

```text
3
```

### `ne`

#### Signature

```cpp
DataFrame ne(self, const DataFrame& other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This produces a comparison-style result you can inspect directly or reuse as a mask.

#### See also

- `DataFrame ne(self, scalar other) const`

#### Examples

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"cost", pluss::table::Thread(pluss::make_series({60.0, 80.0, 55.0}))},
});

auto out = left.ne(right);
```

#### Output

```text
id  sales
1   true
2   true
3   false
```

### `ne`

#### Signature

```cpp
DataFrame ne(self, scalar other) const
```

#### Parameters

`other` : `scalar`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This produces a comparison-style result you can inspect directly or reuse as a mask.

#### See also

- `DataFrame ne(self, const DataFrame& other) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.ne(10);
```

#### Output

```text
id  sales
1   true
2   true
3   false
```

### `nfeatures`

#### Signature

```cpp
std::int64_t nfeatures(self) const
```

#### Parameters

`None`

#### Returns

`std::int64_t`
    Integer count, size, or positional result.

#### Notes

Returns a size/count summary for the current object without changing it.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.nfeatures();
```

#### Output

```text
3
```

### `note_features`

#### Signature

```cpp
DataFrame note_features(self) const
```

#### Parameters

`None`

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Returns only the note-backed features.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.note_features();
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `notna`

#### Signature

```cpp
DataFrame notna(self, std::initializer_list<std::string_view> features =
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.notna({"sales"});
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `notnull`

#### Signature

```cpp
DataFrame notnull(self, std::initializer_list<std::string_view> features =
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.notnull({"sales"});
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `nrows`

#### Signature

```cpp
std::int64_t nrows(self) const
```

#### Parameters

`None`

#### Returns

`std::int64_t`
    Integer count, size, or positional result.

#### Notes

Returns a size/count summary for the current object without changing it.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.nrows();
```

#### Output

```text
3
```

### `nunique`

#### Signature

```cpp
DataFrame nunique(self, bool render = true) const
```

#### Parameters

`render` : `bool`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Counts distinct values for each selected feature.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.nunique(false);
```

#### Output

```text
feature  value
sales    310.0
```

### `pivot`

#### Signature

```cpp
DataFrame pivot(self, std::string_view case_feature, std::string_view feature_feature, std::string_view value_feature, bool dropna_keys = true, bool render = true) const
```

#### Parameters

`case_feature` : `std::string_view`
    See the signature for the supported argument form.

`feature_feature` : `std::string_view`
    See the signature for the supported argument form.

`value_feature` : `std::string_view`
    See the signature for the supported argument form.

`dropna_keys` : `bool`
    See the signature for the supported argument form.

`render` : `bool`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Reshapes long-form rows into a wider table indexed by one feature and expanded by another.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema"}))},
    {"product", pluss::table::Thread(pluss::make_note({"A", "B", "A", "B"}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0, 150.0}))},
});

auto out = df.pivot("city", "product", "sales", true, false);
```

#### Output

```text
city   A      B
Accra  100.0  120.0
Tema    90.0  150.0
```

### `pivot_table`

#### Signature

```cpp
DataFrame pivot_table(self, std::string_view case_feature, std::string_view feature_feature, std::string_view value_feature, GroupOp op = GroupOp::Mean, bool dropna_keys = true, bool render = true) const
```

#### Parameters

`case_feature` : `std::string_view`
    See the signature for the supported argument form.

`feature_feature` : `std::string_view`
    See the signature for the supported argument form.

`value_feature` : `std::string_view`
    See the signature for the supported argument form.

`op` : `GroupOp`
    See the signature for the supported argument form.

`dropna_keys` : `bool`
    See the signature for the supported argument form.

`render` : `bool`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Builds a pivoted summary table using an aggregation function when multiple rows map to the same cell.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema"}))},
    {"product", pluss::table::Thread(pluss::make_note({"A", "B", "A", "B"}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0, 150.0}))},
});

auto out = df.pivot_table("city", "product", "sales", pluss::table::DataFrame::GroupOp::Mean, true, false);
```

#### Output

```text
city   A      B
Accra  100.0  120.0
Tema    90.0  150.0
```

### `plot`

#### Signature

```cpp
DataFramePlotAccessor plot(self) const
```

#### Parameters

`None`

#### Returns

`DataFramePlotAccessor`
    Value produced by this call.

#### Notes

This opens the plotting accessor for the current data object.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto fig = df.plot().line("sales");
```

#### Output

```text
Figure(line/scatter/bar/hist)
```

### `pow`

#### Signature

```cpp
DataFrame pow(self, const DataFrame& other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### See also

- `DataFrame pow(self, scalar other) const`

#### Examples

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"cost", pluss::table::Thread(pluss::make_series({60.0, 80.0, 55.0}))},
});

auto out = left.pow(right);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `pow`

#### Signature

```cpp
DataFrame pow(self, scalar other) const
```

#### Parameters

`other` : `scalar`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### See also

- `DataFrame pow(self, const DataFrame& other) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.pow(10);
```

#### Output

```text
id  sales  city
11  110.0  Accra
12  130.0  Tema
13  100.0  Kumasi
```

### `prepare_sql`

#### Signature

```cpp
DataFrameSqlPrepared prepare_sql(self, std::string_view statement) const
```

#### Parameters

`statement` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`DataFrameSqlPrepared`
    Value produced by this call.

#### Notes

Use this when the transformation is easier to express as SQL than as a chained sequence of table verbs.

#### See also

- `DataFrameSqlPrepared prepare_sql(self, std::string_view statement, SqlOptions options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto stmt = df.prepare_sql("SELECT id, sales FROM self WHERE sales > {min_sales}");
```

#### Output

```text
SQL accessor / prepared statement created
```

### `prepare_sql`

#### Signature

```cpp
DataFrameSqlPrepared prepare_sql(self, std::string_view statement, SqlOptions options) const
```

#### Parameters

`statement` : `std::string_view`
    See the signature for the supported argument form.

`options` : `SqlOptions`
    See the signature for the supported argument form.

#### Returns

`DataFrameSqlPrepared`
    Value produced by this call.

#### Notes

Use this when the transformation is easier to express as SQL than as a chained sequence of table verbs.

#### See also

- `DataFrameSqlPrepared prepare_sql(self, std::string_view statement) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto stmt = df.prepare_sql("SELECT id, sales FROM self WHERE sales > {min_sales}");
```

#### Output

```text
SQL accessor / prepared statement created
```

### `query`

#### Signature

```cpp
DataFrame query(self, std::string_view expression) const
```

#### Parameters

`expression` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Keeps only the rows that satisfy the supplied filter expression or mask-producing query.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.query("sales > 95");
```

#### Output

```text
id  sales  city
1   100.0  Accra
3    90.0  Kumasi
```

### `radd`

#### Signature

```cpp
DataFrame radd(self, scalar other) const
```

#### Parameters

`other` : `scalar`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.radd(10);
```

#### Output

```text
id  sales  city
11  110.0  Accra
12  130.0  Tema
13  100.0  Kumasi
```

### `rdiv`

#### Signature

```cpp
DataFrame rdiv(self, scalar other) const
```

#### Parameters

`other` : `scalar`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.rdiv(10);
```

#### Output

```text
id  sales  city
11  110.0  Accra
12  130.0  Tema
13  100.0  Kumasi
```

### `reindex_cases`

#### Signature

```cpp
DataFrame reindex_cases(self, const Thread& new_cases) const
```

#### Parameters

`new_cases` : `const Thread&`
    Replacement case labels in the exact order you want the rows to follow.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Builds a new case axis from the supplied labels and aligns rows onto that case order.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto cases = pluss::table::Thread(pluss::make_note({"cs1", "cs0", "cs2"}));
auto out = df.reindex_cases(cases);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `reindex_like`

#### Signature

```cpp
DataFrame reindex_like(self, const DataFrame& other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Realigns this frame so its cases match another frame.

#### Examples

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"cost", pluss::table::Thread(pluss::make_series({60.0, 80.0, 55.0}))},
});

auto out = left.reindex_like(right);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `relabel_features`

#### Signature

```cpp
DataFrame relabel_features(self, std::initializer_list<std::string_view> feature_names) const
```

#### Parameters

`feature_names` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Replaces the full feature label set with the supplied names in positional order.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.relabel_features({"revenue", "location", "id_new"});
```

#### Output

```text
id  revenue  city
1   100.0    Accra
2   120.0    Tema
3    90.0    Kumasi
```

### `rename`

#### Signature

```cpp
DataFrame rename(self, std::string_view old_name, std::string new_name) const
```

#### Parameters

`old_name` : `std::string_view`
    See the signature for the supported argument form.

`new_name` : `std::string`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Renames one feature without changing its values.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.rename("sales", "revenue");
```

#### Output

```text
id  revenue  city
1   100.0    Accra
2   120.0    Tema
3    90.0    Kumasi
```

### `rename_`

#### Signature

```cpp
DataFrame& rename_(self, std::string_view old_name, std::string new_name)
```

#### Parameters

`old_name` : `std::string_view`
    See the signature for the supported argument form.

`new_name` : `std::string`
    See the signature for the supported argument form.

#### Returns

`DataFrame&`
    Value produced by this call.

#### Notes

Renames one feature in place without changing its values.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.rename_("sales", "revenue");
```

#### Output

```text
id  revenue  city
1   100.0    Accra
2   120.0    Tema
3    90.0    Kumasi
```

### `rename_features`

#### Signature

```cpp
DataFrame rename_features(self,  std::initializer_list<std::pair<std::string_view, std::string_view>> renames) const
```

#### Parameters

`renames` : `std::initializer_list<std::pair<std::string_view, std::string_view>>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Renames several features in one call.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.rename_features({{"sales", "revenue"}, {"city", "location"}});
```

#### Output

```text
id  revenue  city
1   100.0    Accra
2   120.0    Tema
3    90.0    Kumasi
```

### `reorder`

#### Signature

```cpp
DataFrame reorder(self, std::initializer_list<std::string_view> names) const
```

#### Parameters

`names` : `std::initializer_list<std::string_view>`
    Collection of feature, slot, or stream names supplied in positional order.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Reorders the feature columns without changing any row values.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.reorder({"sales", "id", "city"});
```

#### Output

```text
sales  id  city
100.0  1  Accra
120.0  2  Tema
90.0   3  Kumasi
```

### `repeated_cases`

#### Signature

```cpp
pluss::Series repeated_cases(self, std::initializer_list<std::string_view> features =
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`pluss::Series`
    Value produced by this call.

#### Notes

Returns the rows whose current case labels are duplicates of an earlier row.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.repeated_cases();
```

### `resample`

#### Signature

```cpp
DataFrame resample(self, std::string_view time_feature, std::initializer_list<std::string_view> value_features, std::string_view freq, std::string_view op = "mean", std::int64_t step = 1, bool render = true) const
```

#### Parameters

`time_feature` : `std::string_view`
    See the signature for the supported argument form.

`value_features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

`freq` : `std::string_view`
    See the signature for the supported argument form.

`op` : `std::string_view`
    See the signature for the supported argument form.

`step` : `std::int64_t`
    See the signature for the supported argument form.

`render` : `bool`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This is part of the time-aware surface and expects timeline or time-index semantics.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"ts", pluss::table::Thread(pluss::make_timeline({
        "2024-01-01T09:10:00Z",
        "2024-01-01T09:40:00Z",
        "2024-01-01T10:05:00Z",
    }))},
    {"load", pluss::table::Thread(pluss::make_series({10.0, 20.0, 15.0}))},
});

auto out = df.resample("ts", {"load"}, "1h", "mean", 1, false);
```

#### Output

```text
ts                   load
2024-01-01T09:00:00Z  15.0
2024-01-01T10:00:00Z  15.0
```

### `reset_cases`

#### Signature

```cpp
DataFrame reset_cases(self, std::string case_feature = "case") const
```

#### Parameters

`case_feature` : `std::string`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Materializes the current case axis as a normal feature, then restores the default positional cases `0..n-1`.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto with_cases = df.set_cases("city", true);
auto out = with_cases.reset_cases("case");
```

#### Output

```text
with_cases.cases():
["Accra", "Tema", "Kumasi"]

out.cases():
[0, 1, 2]

out.head():
id  sales  case
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `rfloordiv`

#### Signature

```cpp
DataFrame rfloordiv(self, scalar other) const
```

#### Parameters

`other` : `scalar`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.rfloordiv(10);
```

#### Output

```text
id  sales  city
11  110.0  Accra
12  130.0  Tema
13  100.0  Kumasi
```

### `rmod`

#### Signature

```cpp
DataFrame rmod(self, scalar other) const
```

#### Parameters

`other` : `scalar`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.rmod(10);
```

#### Output

```text
id  sales  city
11  110.0  Accra
12  130.0  Tema
13  100.0  Kumasi
```

### `rmul`

#### Signature

```cpp
DataFrame rmul(self, scalar other) const
```

#### Parameters

`other` : `scalar`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.rmul(10);
```

#### Output

```text
id  sales  city
11  110.0  Accra
12  130.0  Tema
13  100.0  Kumasi
```

### `rolling`

#### Signature

```cpp
DataFrame rolling(self, std::initializer_list<WindowSpec> specs, bool keep_input = true) const
```

#### Parameters

`specs` : `std::initializer_list<WindowSpec>`
    See the signature for the supported argument form.

`keep_input` : `bool`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Computes rolling-window features and returns them as a derived frame.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.rolling({{.output = "sales_roll", .feature = "sales", .op = pluss::table::ThreadWindowOp::RollingMean, .window = 2, .min_periods = 1}}, true);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `round_time`

#### Signature

```cpp
DataFrame round_time(self, std::initializer_list<std::string_view> features, std::string_view unit) const
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

`unit` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This is part of the time-aware surface and expects timeline or time-index semantics.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"ts", pluss::table::Thread(pluss::make_timeline({
        "2024-01-01T09:10:00Z",
        "2024-01-01T09:40:00Z",
        "2024-01-01T10:05:00Z",
    }))},
    {"load", pluss::table::Thread(pluss::make_series({10.0, 20.0, 15.0}))},
});

auto out = df.round_time({"ts"}, "hour");
```

#### Output

```text
ts
2024-01-01T09:00:00Z
2024-01-01T10:00:00Z
```

### `rpow`

#### Signature

```cpp
DataFrame rpow(self, scalar other) const
```

#### Parameters

`other` : `scalar`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.rpow(10);
```

#### Output

```text
id  sales  city
11  110.0  Accra
12  130.0  Tema
13  100.0  Kumasi
```

### `rsub`

#### Signature

```cpp
DataFrame rsub(self, scalar other) const
```

#### Parameters

`other` : `scalar`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.rsub(10);
```

#### Output

```text
id  sales  city
11  110.0  Accra
12  130.0  Tema
13  100.0  Kumasi
```

### `rtruediv`

#### Signature

```cpp
DataFrame rtruediv(self, scalar other) const
```

#### Parameters

`other` : `scalar`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.rtruediv(10);
```

#### Output

```text
id  sales  city
11  110.0  Accra
12  130.0  Tema
13  100.0  Kumasi
```

### `sample`

#### Signature

```cpp
DataFrame sample(self, std::int64_t n, std::optional<std::uint64_t> random_state = std::nullopt) const
```

#### Parameters

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

`random_state` : `std::optional<std::uint64_t>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Returns a random sample of rows.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.sample(2, 7);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `save_as`

#### Signature

```cpp
void save_as(self, std::string_view target_type, std::string_view filepath, const pluss::loader::Config& config =
```

#### Parameters

`target_type` : `std::string_view`
    See the signature for the supported argument form.

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the frame using the explicitly supplied output format selector rather than a dedicated saver overload.

#### See also

- `void save_as(self, std::string_view target_type, std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_as("json", "path/to/output.json");
```

#### Output

```text
saved: out.*
```

### `save_as`

#### Signature

```cpp
void save_as(self, std::string_view target_type, std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) const
```

#### Parameters

`target_type` : `std::string_view`
    See the signature for the supported argument form.

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config_entries` : `std::initializer_list<pluss::loader::ConfigEntry>`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the frame using the explicitly supplied output format selector rather than a dedicated saver overload.

#### See also

- `void save_as(self, std::string_view target_type, std::string_view filepath, const pluss::loader::Config& config =`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_as("json", "path/to/output.json");
```

#### Output

```text
saved: out.*
```

### `save_csv`

#### Signature

```cpp
void save_csv(self, std::string_view filepath) const
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the frame to CSV. Set `add_cases=true` when you want the current case axis exported as a regular column so row labels survive the round trip.

#### See also

- `void save_csv(self, std::string_view filepath, bool add_cases, bool header = true, char sep = ',', std::string na_rep = "NA", std::optional<std::string> case_label = std::nullopt, std::optional<std::string> float_format = std::nullopt, char quotechar = '"', bool scalarquote = true, std::string line_terminator = "\n", char decimal = '.', std::string encoding = "utf-8") const`
- `void save_csv(self, std::string_view filepath, const pluss::loader::WriteCsvOptions& options) const`
- `void save_csv(self, std::string_view filepath, const pluss::loader::Config& config) const`
- `void save_csv(self, std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_csv("path/to/output.csv");
```

#### Output

```text
saved: out.*
```

### `save_csv`

#### Signature

```cpp
void save_csv(self, std::string_view filepath, bool add_cases, bool header = true, char sep = ',', std::string na_rep = "NA", std::optional<std::string> case_label = std::nullopt, std::optional<std::string> float_format = std::nullopt, char quotechar = '"', bool scalarquote = true, std::string line_terminator = "\n", char decimal = '.', std::string encoding = "utf-8") const
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`add_cases` : `bool`
    When `true`, the current case axis is exported as its own column so row labels can be reconstructed when the file is loaded again.

`header` : `bool`
    Controls whether feature names are written to, or expected from, the file header row.

`sep` : `char`
    Column separator used for delimited text files such as CSV.

`arg5` : `'`
    See the signature for the supported argument form.

`na_rep` : `std::string`
    String written anywhere a null or missing value must be serialized.

`case_label` : `std::optional<std::string>`
    Optional column label to use for the exported case axis when `add_cases=true`.

`float_format` : `std::optional<std::string>`
    Optional numeric formatting string used when floating-point values are written to text output.

`quotechar` : `char`
    Quote character used when writing delimited text output.

`doublequote` : `bool`
    When `true`, embedded quote characters are escaped by doubling them in CSV-style output.

`line_terminator` : `std::string`
    Line ending written between serialized rows in text-based export.

`decimal` : `char`
    Decimal marker used when formatting floating-point output.

`encoding` : `std::string`
    Text encoding used when reading or writing text-based files.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the frame to CSV. Set `add_cases=true` when you want the current case axis exported as a regular column so row labels survive the round trip.

#### See also

- `void save_csv(self, std::string_view filepath) const`
- `void save_csv(self, std::string_view filepath, const pluss::loader::WriteCsvOptions& options) const`
- `void save_csv(self, std::string_view filepath, const pluss::loader::Config& config) const`
- `void save_csv(self, std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_csv("path/to/output.csv");
```

#### Output

```text
saved: out.*
```

### `save_csv`

#### Signature

```cpp
void save_csv(self, std::string_view filepath, const pluss::loader::Config& config) const
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the frame to CSV. Set `add_cases=true` when you want the current case axis exported as a regular column so row labels survive the round trip.

#### See also

- `void save_csv(self, std::string_view filepath) const`
- `void save_csv(self, std::string_view filepath, bool add_cases, bool header = true, char sep = ',', std::string na_rep = "NA", std::optional<std::string> case_label = std::nullopt, std::optional<std::string> float_format = std::nullopt, char quotechar = '"', bool scalarquote = true, std::string line_terminator = "\n", char decimal = '.', std::string encoding = "utf-8") const`
- `void save_csv(self, std::string_view filepath, const pluss::loader::WriteCsvOptions& options) const`
- `void save_csv(self, std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_csv("path/to/output.csv");
```

#### Output

```text
saved: out.*
```

### `save_csv`

#### Signature

```cpp
void save_csv(self, std::string_view filepath, const pluss::loader::WriteCsvOptions& options) const
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`options` : `const pluss::loader::WriteCsvOptions&`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the frame to CSV. Set `add_cases=true` when you want the current case axis exported as a regular column so row labels survive the round trip.

#### See also

- `void save_csv(self, std::string_view filepath) const`
- `void save_csv(self, std::string_view filepath, bool add_cases, bool header = true, char sep = ',', std::string na_rep = "NA", std::optional<std::string> case_label = std::nullopt, std::optional<std::string> float_format = std::nullopt, char quotechar = '"', bool scalarquote = true, std::string line_terminator = "\n", char decimal = '.', std::string encoding = "utf-8") const`
- `void save_csv(self, std::string_view filepath, const pluss::loader::Config& config) const`
- `void save_csv(self, std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_csv("path/to/output.csv");
```

#### Output

```text
saved: out.*
```

### `save_csv`

#### Signature

```cpp
void save_csv(self, std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) const
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config_entries` : `std::initializer_list<pluss::loader::ConfigEntry>`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the frame to CSV. Set `add_cases=true` when you want the current case axis exported as a regular column so row labels survive the round trip.

#### See also

- `void save_csv(self, std::string_view filepath) const`
- `void save_csv(self, std::string_view filepath, bool add_cases, bool header = true, char sep = ',', std::string na_rep = "NA", std::optional<std::string> case_label = std::nullopt, std::optional<std::string> float_format = std::nullopt, char quotechar = '"', bool scalarquote = true, std::string line_terminator = "\n", char decimal = '.', std::string encoding = "utf-8") const`
- `void save_csv(self, std::string_view filepath, const pluss::loader::WriteCsvOptions& options) const`
- `void save_csv(self, std::string_view filepath, const pluss::loader::Config& config) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_csv("path/to/output.csv");
```

#### Output

```text
saved: out.*
```

### `save_excel`

#### Signature

```cpp
void save_excel(self, std::string_view filepath) const
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the frame to an Excel sheet. Set `add_cases=true` when you want the current case axis exported as a regular column.

#### See also

- `void save_excel(self, std::string_view filepath, std::string sheet_name, bool add_cases = false, bool header = true, std::string na_rep = "NA", std::optional<std::string> case_label = std::nullopt) const`
- `void save_excel(self, std::string_view filepath, const pluss::loader::WriteExcelOptions& options) const`
- `void save_excel(self, std::string_view filepath, const pluss::loader::Config& config) const`
- `void save_excel(self, std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_excel("path/to/output.xlsx");
```

#### Output

```text
saved: out.*
```

### `save_excel`

#### Signature

```cpp
void save_excel(self, std::string_view filepath, const pluss::loader::Config& config) const
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the frame to an Excel sheet. Set `add_cases=true` when you want the current case axis exported as a regular column.

#### See also

- `void save_excel(self, std::string_view filepath) const`
- `void save_excel(self, std::string_view filepath, std::string sheet_name, bool add_cases = false, bool header = true, std::string na_rep = "NA", std::optional<std::string> case_label = std::nullopt) const`
- `void save_excel(self, std::string_view filepath, const pluss::loader::WriteExcelOptions& options) const`
- `void save_excel(self, std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_excel("path/to/output.xlsx");
```

#### Output

```text
saved: out.*
```

### `save_excel`

#### Signature

```cpp
void save_excel(self, std::string_view filepath, const pluss::loader::WriteExcelOptions& options) const
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`options` : `const pluss::loader::WriteExcelOptions&`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the frame to an Excel sheet. Set `add_cases=true` when you want the current case axis exported as a regular column.

#### See also

- `void save_excel(self, std::string_view filepath) const`
- `void save_excel(self, std::string_view filepath, std::string sheet_name, bool add_cases = false, bool header = true, std::string na_rep = "NA", std::optional<std::string> case_label = std::nullopt) const`
- `void save_excel(self, std::string_view filepath, const pluss::loader::Config& config) const`
- `void save_excel(self, std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_excel("path/to/output.xlsx");
```

#### Output

```text
saved: out.*
```

### `save_excel`

#### Signature

```cpp
void save_excel(self, std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) const
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config_entries` : `std::initializer_list<pluss::loader::ConfigEntry>`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the frame to an Excel sheet. Set `add_cases=true` when you want the current case axis exported as a regular column.

#### See also

- `void save_excel(self, std::string_view filepath) const`
- `void save_excel(self, std::string_view filepath, std::string sheet_name, bool add_cases = false, bool header = true, std::string na_rep = "NA", std::optional<std::string> case_label = std::nullopt) const`
- `void save_excel(self, std::string_view filepath, const pluss::loader::WriteExcelOptions& options) const`
- `void save_excel(self, std::string_view filepath, const pluss::loader::Config& config) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_excel("path/to/output.xlsx");
```

#### Output

```text
saved: out.*
```

### `save_excel`

#### Signature

```cpp
void save_excel(self, std::string_view filepath, std::string sheet_name, bool add_cases = false, bool header = true, std::string na_rep = "NA", std::optional<std::string> case_label = std::nullopt) const
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`sheet_name` : `std::string`
    Worksheet name to read from or write to inside an Excel workbook.

`add_cases` : `bool`
    When `true`, the current case axis is exported as its own column so row labels can be reconstructed when the file is loaded again.

`header` : `bool`
    Controls whether feature names are written to, or expected from, the file header row.

`na_rep` : `std::string`
    String written anywhere a null or missing value must be serialized.

`case_label` : `std::optional<std::string>`
    Optional column label to use for the exported case axis when `add_cases=true`.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the frame to an Excel sheet. Set `add_cases=true` when you want the current case axis exported as a regular column.

#### See also

- `void save_excel(self, std::string_view filepath) const`
- `void save_excel(self, std::string_view filepath, const pluss::loader::WriteExcelOptions& options) const`
- `void save_excel(self, std::string_view filepath, const pluss::loader::Config& config) const`
- `void save_excel(self, std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_excel("path/to/output.xlsx");
```

#### Output

```text
saved: out.*
```

### `save_json`

#### Signature

```cpp
void save_json(self, std::string_view filepath) const
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the frame to JSON using the table export route for later reload or interchange.

#### See also

- `void save_json(self, std::string_view filepath, const pluss::loader::Config& config) const`
- `void save_json(self, std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_json("path/to/output.json");
```

#### Output

```text
saved: out.*
```

### `save_json`

#### Signature

```cpp
void save_json(self, std::string_view filepath, const pluss::loader::Config& config) const
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the frame to JSON using the table export route for later reload or interchange.

#### See also

- `void save_json(self, std::string_view filepath) const`
- `void save_json(self, std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_json("path/to/output.json");
```

#### Output

```text
saved: out.*
```

### `save_json`

#### Signature

```cpp
void save_json(self, std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) const
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config_entries` : `std::initializer_list<pluss::loader::ConfigEntry>`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the frame to JSON using the table export route for later reload or interchange.

#### See also

- `void save_json(self, std::string_view filepath) const`
- `void save_json(self, std::string_view filepath, const pluss::loader::Config& config) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_json("path/to/output.json");
```

#### Output

```text
saved: out.*
```

### `save_parquet`

#### Signature

```cpp
void save_parquet(self, std::string_view filepath) const
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the frame to Parquet for compact typed storage.

#### See also

- `void save_parquet(self, std::string_view filepath, const pluss::loader::Config& config) const`
- `void save_parquet(self, std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_parquet("path/to/output.parquet");
```

#### Output

```text
saved: out.*
```

### `save_parquet`

#### Signature

```cpp
void save_parquet(self, std::string_view filepath, const pluss::loader::Config& config) const
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the frame to Parquet for compact typed storage.

#### See also

- `void save_parquet(self, std::string_view filepath) const`
- `void save_parquet(self, std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_parquet("path/to/output.parquet");
```

#### Output

```text
saved: out.*
```

### `save_parquet`

#### Signature

```cpp
void save_parquet(self, std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) const
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config_entries` : `std::initializer_list<pluss::loader::ConfigEntry>`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the frame to Parquet for compact typed storage.

#### See also

- `void save_parquet(self, std::string_view filepath) const`
- `void save_parquet(self, std::string_view filepath, const pluss::loader::Config& config) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_parquet("path/to/output.parquet");
```

#### Output

```text
saved: out.*
```

### `save_txt`

#### Signature

```cpp
void save_txt(self, std::string_view filepath) const
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the frame to a plain-text table preview suitable for inspection or lightweight logging.

#### See also

- `void save_txt(self, std::string_view filepath, const pluss::loader::Config& config) const`
- `void save_txt(self, std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_txt("path/to/output.txt");
```

#### Output

```text
saved: out.*
```

### `save_txt`

#### Signature

```cpp
void save_txt(self, std::string_view filepath, const pluss::loader::Config& config) const
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the frame to a plain-text table preview suitable for inspection or lightweight logging.

#### See also

- `void save_txt(self, std::string_view filepath) const`
- `void save_txt(self, std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_txt("path/to/output.txt");
```

#### Output

```text
saved: out.*
```

### `save_txt`

#### Signature

```cpp
void save_txt(self, std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) const
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config_entries` : `std::initializer_list<pluss::loader::ConfigEntry>`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the frame to a plain-text table preview suitable for inspection or lightweight logging.

#### See also

- `void save_txt(self, std::string_view filepath) const`
- `void save_txt(self, std::string_view filepath, const pluss::loader::Config& config) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_txt("path/to/output.txt");
```

#### Output

```text
saved: out.*
```

### `series_features`

#### Signature

```cpp
DataFrame series_features(self) const
```

#### Parameters

`None`

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Returns only the numeric series-backed features.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.series_features();
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `set`

#### Signature

```cpp
LetSlot set(self, std::string_view name)
```

#### Parameters

`name` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`LetSlot`
    Value produced by this call.

#### Notes

Opens an assignment helper so you can write `df.set(...) = ...` and replace or create features declaratively.

#### See also

- `LetSlots set(self, std::initializer_list<std::string_view> names)`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.set("profit") = pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0}));
```

### `set`

#### Signature

```cpp
LetSlots set(self, std::initializer_list<std::string_view> names)
```

#### Parameters

`names` : `std::initializer_list<std::string_view>`
    Collection of feature, slot, or stream names supplied in positional order.

#### Returns

`LetSlots`
    Value produced by this call.

#### Notes

Opens an assignment helper so you can write `df.set(...) = ...` and replace or create features declaratively.

#### See also

- `LetSlot set(self, std::string_view name)`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.set({"profit", "cost"}) = {pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0})), pluss::table::Thread(pluss::make_series({60.0, 85.0, 60.0}))};
```

### `set_axis`

#### Signature

```cpp
DataFrame set_axis(self, std::initializer_list<std::string_view> feature_names) const
```

#### Parameters

`feature_names` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Replaces the full feature label set with the supplied names in positional order.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.set_axis({"revenue", "location", "id_new"});
```

#### Output

```text
id  revenue  city
1   100.0    Accra
2   120.0    Tema
3    90.0    Kumasi
```

### `set_cases`

#### Signature

```cpp
DataFrame set_cases(self, std::string_view feature_name, bool drop_feature = true) const
```

#### Parameters

`feature_name` : `std::string_view`
    Name of the target feature to read, rewrite, sort by, or promote to cases.

`drop_feature` : `bool`
    When `true`, the feature promoted to cases is removed from the ordinary feature columns.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Promotes one feature to become the case axis. With the default `drop_feature=true`, that feature stops being a normal column and becomes the row labels instead.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.set_cases("city");
```

#### Output

```text
df.head():
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi

out.cases():
["Accra", "Tema", "Kumasi"]

out.head():
id  sales
1   100.0
2   120.0
3    90.0
```

### `set_feature_axis`

#### Signature

```cpp
DataFrame set_feature_axis(self, std::initializer_list<std::string_view> feature_names) const
```

#### Parameters

`feature_names` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Replaces the full feature label set with the supplied names in positional order.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.set_feature_axis({"revenue", "location", "id_new"});
```

#### Output

```text
id  revenue  city
1   100.0    Accra
2   120.0    Tema
3    90.0    Kumasi
```

### `set_features`

#### Signature

```cpp
DataFrame set_features(self, std::initializer_list<std::string_view> names, std::initializer_list<Thread> values) const
```

#### Parameters

`names` : `std::initializer_list<std::string_view>`
    Collection of feature, slot, or stream names supplied in positional order.

`values` : `std::initializer_list<Thread>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Adds or replaces one or more features and returns the updated frame.

#### See also

- `DataFrame set_features(self, std::string_view name, const Thread& value) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.set_features(
    {"profit", "cost"},
    {
        pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0})),
        pluss::table::Thread(pluss::make_series({60.0, 85.0, 60.0})),
    });
```

#### Output

```text
id  sales  city    profit
1   100.0  Accra   40.0
2   120.0  Tema    35.0
3    90.0  Kumasi  30.0
```

### `set_features`

#### Signature

```cpp
DataFrame set_features(self, std::string_view name, const Thread& value) const
```

#### Parameters

`name` : `std::string_view`
    See the signature for the supported argument form.

`value` : `const Thread&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Adds or replaces one or more features and returns the updated frame.

#### See also

- `DataFrame set_features(self, std::initializer_list<std::string_view> names, std::initializer_list<Thread> values) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.set_features("profit", pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0})));
```

#### Output

```text
id  sales  city    profit
1   100.0  Accra   40.0
2   120.0  Tema    35.0
3    90.0  Kumasi  30.0
```

### `set_features_`

#### Signature

```cpp
DataFrame& set_features_(self, std::initializer_list<std::string_view> names, std::initializer_list<Thread> values)
```

#### Parameters

`names` : `std::initializer_list<std::string_view>`
    Collection of feature, slot, or stream names supplied in positional order.

`values` : `std::initializer_list<Thread>`
    See the signature for the supported argument form.

#### Returns

`DataFrame&`
    Value produced by this call.

#### Notes

Adds or replaces one or more features in place.

#### See also

- `DataFrame& set_features_(self, std::string_view name, const Thread& value)`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.set_features_(
    {"profit", "cost"},
    {
        pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0})),
        pluss::table::Thread(pluss::make_series({60.0, 85.0, 60.0})),
    });
```

#### Output

```text
id  sales  city    profit
1   100.0  Accra   40.0
2   120.0  Tema    35.0
3    90.0  Kumasi  30.0
```

### `set_features_`

#### Signature

```cpp
DataFrame& set_features_(self, std::string_view name, const Thread& value)
```

#### Parameters

`name` : `std::string_view`
    See the signature for the supported argument form.

`value` : `const Thread&`
    See the signature for the supported argument form.

#### Returns

`DataFrame&`
    Value produced by this call.

#### Notes

Adds or replaces one or more features in place.

#### See also

- `DataFrame& set_features_(self, std::initializer_list<std::string_view> names, std::initializer_list<Thread> values)`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.set_features_("profit", pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0})));
```

#### Output

```text
id  sales  city    profit
1   100.0  Accra   40.0
2   120.0  Tema    35.0
3    90.0  Kumasi  30.0
```

### `shift_time`

#### Signature

```cpp
DataFrame shift_time(self, std::initializer_list<std::string_view> features, int periods, std::string_view freq) const
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

`periods` : `int`
    See the signature for the supported argument form.

`freq` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This is part of the time-aware surface and expects timeline or time-index semantics.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"ts", pluss::table::Thread(pluss::make_timeline({
        "2024-01-01T09:10:00Z",
        "2024-01-01T09:40:00Z",
        "2024-01-01T10:05:00Z",
    }))},
    {"load", pluss::table::Thread(pluss::make_series({10.0, 20.0, 15.0}))},
});

auto out = df.shift_time({"ts"}, 1, "hour");
```

#### Output

```text
ts
2024-01-01T09:00:00Z
2024-01-01T10:00:00Z
```

### `slice`

#### Signature

```cpp
DataFrame slice(self, std::int64_t start, std::int64_t stop) const
```

#### Parameters

`start` : `std::int64_t`
    Inclusive starting position of the slice.

`stop` : `std::int64_t`
    Exclusive ending position of the slice.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Returns a contiguous row slice `[start, stop)` from the frame.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.slice(0, 2);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `sort_cases`

#### Signature

```cpp
DataFrame sort_cases(self, bool ascending = true, bool nulls_last = true) const
```

#### Parameters

`ascending` : `bool`
    See the signature for the supported argument form.

`nulls_last` : `bool`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Sorts rows by the current case axis rather than by a data feature.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto with_cases = df.with_cases(pluss::table::Thread(pluss::make_note({"cs2", "cs0", "cs1"})));
auto out = with_cases.sort_cases(true, true);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `sort_values`

#### Signature

```cpp
DataFrame sort_values(self, std::initializer_list<ArrangeSpec> specs) const
```

#### Parameters

`specs` : `std::initializer_list<ArrangeSpec>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Reorders rows using one or more feature values as sort keys.

#### See also

- `DataFrame sort_values(self, std::initializer_list<std::string_view> features, bool ascending = true, bool nulls_last = true) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.sort_values({{.feature = "sales", .ascending = false}});
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `sort_values`

#### Signature

```cpp
DataFrame sort_values(self, std::initializer_list<std::string_view> features, bool ascending = true, bool nulls_last = true) const
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

`ascending` : `bool`
    See the signature for the supported argument form.

`nulls_last` : `bool`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Reorders rows using one or more feature values as sort keys.

#### See also

- `DataFrame sort_values(self, std::initializer_list<ArrangeSpec> specs) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.sort_values({"sales"}, false, true);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `sql`

#### Signature

```cpp
DataFrameSqlAccessor sql(self) const
```

#### Parameters

`None`

#### Returns

`DataFrameSqlAccessor`
    Value produced by this call.

#### Notes

Use this when the transformation is easier to express as SQL than as a chained sequence of table verbs.

#### See also

- `DataFrameSqlAccessor sql(self, SqlOptions options) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.sql().execute("SELECT id, sales FROM self WHERE sales > 95");
```

#### Output

```text
SQL accessor / prepared statement created
```

### `sql`

#### Signature

```cpp
DataFrameSqlAccessor sql(self, SqlOptions options) const
```

#### Parameters

`options` : `SqlOptions`
    See the signature for the supported argument form.

#### Returns

`DataFrameSqlAccessor`
    Value produced by this call.

#### Notes

Use this when the transformation is easier to express as SQL than as a chained sequence of table verbs.

#### See also

- `DataFrameSqlAccessor sql(self) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.sql().execute("SELECT id, sales FROM self WHERE sales > 95");
```

#### Output

```text
SQL accessor / prepared statement created
```

### `std`

#### Signature

```cpp
DataFrame std(self, bool render = true) const
```

#### Parameters

`render` : `bool`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Computes per-feature standard deviations for the selected numeric features.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.std(false);
```

#### Output

```text
feature  value
sales    310.0
```

### `stitch_on`

#### Signature

```cpp
DataFrame stitch_on(self, std::initializer_list<std::string_view> left_features, const DataFrame& right, std::initializer_list<std::string_view> right_features =
```

#### Parameters

`left_features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

`right` : `const DataFrame&`
    See the signature for the supported argument form.

`right_features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Matches rows from two frames using key features and appends the matched features from the right-hand frame.

#### See also

- `DataFrame stitch_on(self, std::string_view left_feature, const DataFrame& right, std::string_view right_feature = "", std::string right_suffix = "_right") const`

#### Examples

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"cost", pluss::table::Thread(pluss::make_series({60.0, 80.0, 55.0}))},
});

auto out = left.stitch_on({"id"}, right, {"id"}, "_right");
```

#### Output

```text
id  sales  cost
1   100.0  60.0
2   120.0  80.0
3    90.0  55.0
```

### `stitch_on`

#### Signature

```cpp
DataFrame stitch_on(self, std::string_view left_feature, const DataFrame& right, std::string_view right_feature = "", std::string right_suffix = "_right") const
```

#### Parameters

`left_feature` : `std::string_view`
    See the signature for the supported argument form.

`right` : `const DataFrame&`
    See the signature for the supported argument form.

`right_feature` : `std::string_view`
    See the signature for the supported argument form.

`right_suffix` : `std::string`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Matches rows from two frames using key features and appends the matched features from the right-hand frame.

#### See also

- `DataFrame stitch_on(self, std::initializer_list<std::string_view> left_features, const DataFrame& right, std::initializer_list<std::string_view> right_features =`

#### Examples

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"cost", pluss::table::Thread(pluss::make_series({60.0, 80.0, 55.0}))},
});

auto out = left.stitch_on("id", right, "id", "_right");
```

#### Output

```text
id  sales  cost
1   100.0  60.0
2   120.0  80.0
3    90.0  55.0
```

### `sub`

#### Signature

```cpp
DataFrame sub(self, const DataFrame& other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### See also

- `DataFrame sub(self, scalar other) const`

#### Examples

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"cost", pluss::table::Thread(pluss::make_series({60.0, 80.0, 55.0}))},
});

auto out = left.sub(right);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `sub`

#### Signature

```cpp
DataFrame sub(self, scalar other) const
```

#### Parameters

`other` : `scalar`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### See also

- `DataFrame sub(self, const DataFrame& other) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.sub(10);
```

#### Output

```text
id  sales  city
11  110.0  Accra
12  130.0  Tema
13  100.0  Kumasi
```

### `subtract`

#### Signature

```cpp
DataFrame subtract(self, const DataFrame& other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### See also

- `DataFrame subtract(self, scalar other) const`

#### Examples

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"cost", pluss::table::Thread(pluss::make_series({60.0, 80.0, 55.0}))},
});

auto out = left.subtract(right);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `subtract`

#### Signature

```cpp
DataFrame subtract(self, scalar other) const
```

#### Parameters

`other` : `scalar`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### See also

- `DataFrame subtract(self, const DataFrame& other) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.subtract(10);
```

#### Output

```text
id  sales  city
11  110.0  Accra
12  130.0  Tema
13  100.0  Kumasi
```

### `sum`

#### Signature

```cpp
DataFrame sum(self, bool render = true) const
```

#### Parameters

`render` : `bool`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Computes per-feature sums for the selected numeric features.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.sum(false);
```

#### Output

```text
feature  value
sales    310.0
```

### `tabelize`

#### Signature

```cpp
static DataFrame tabelize(const pluss::kindle::Catalog& catalog)
```

#### Parameters

`catalog` : `const pluss::kindle::Catalog&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### See also

- `static DataFrame tabelize(const pluss::kindle::Catalog& catalog, const pluss::kindle::TabelizePolicy& policy)`

#### Examples

```cpp
using namespace pluss::kindle::literals;
auto catalog = pluss::kindle::Catalog({"sales"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}))});
auto out = pluss::table::DataFrame::tabelize(catalog);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `tabelize`

#### Signature

```cpp
static DataFrame tabelize(const pluss::kindle::Catalog& catalog, const pluss::kindle::TabelizePolicy& policy)
```

#### Parameters

`catalog` : `const pluss::kindle::Catalog&`
    See the signature for the supported argument form.

`policy` : `const pluss::kindle::TabelizePolicy&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### See also

- `static DataFrame tabelize(const pluss::kindle::Catalog& catalog)`

#### Examples

```cpp
using namespace pluss::kindle::literals;
auto catalog = pluss::kindle::Catalog({"sales"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}))});
auto out = pluss::table::DataFrame::tabelize(catalog);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `tail`

#### Signature

```cpp
DataFrame tail(self, std::int64_t n = 5, bool render = true) const
```

#### Parameters

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

`render` : `bool`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Returns the last `n` rows of the frame.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.tail(2, false);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `take`

#### Signature

```cpp
DataFrame take(self, std::span<const std::int64_t> indices) const
```

#### Parameters

`indices` : `std::span<const std::int64_t>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Returns the rows at the requested integer positions.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

std::vector<std::int64_t> picks{0, 2};
auto out = df.take(picks);
```

#### Output

```text
id  sales  city
1   100.0  Accra
3    90.0  Kumasi
```

### `tally`

#### Signature

```cpp
DataFrame tally(self, std::string_view feature, std::int64_t top_n = 0, bool dropna = true, bool render = true) const
```

#### Parameters

`feature` : `std::string_view`
    See the signature for the supported argument form.

`top_n` : `std::int64_t`
    See the signature for the supported argument form.

`dropna` : `bool`
    See the signature for the supported argument form.

`render` : `bool`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Counts how many rows fall into each distinct value of the grouping feature.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema"}))},
    {"product", pluss::table::Thread(pluss::make_note({"A", "B", "A", "B"}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0, 150.0}))},
});

auto out = df.tally("city", 3, true, false);
```

#### Output

```text
city    count
Accra   2
Tema    2
```

### `timeline_features`

#### Signature

```cpp
DataFrame timeline_features(self) const
```

#### Parameters

`None`

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Returns only the timeline-backed features.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.timeline_features();
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `to_string`

#### Signature

```cpp
std::string to_string(self, const display::DisplayOptions& options =
```

#### Parameters

`options` : `const display::DisplayOptions&`
    See the signature for the supported argument form.

#### Returns

`std::string`
    Rendered string result.

#### Notes

Serializes or renders the current object into a text form you can inspect, log, or export.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.to_string();
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `to_utc`

#### Signature

```cpp
DataFrame to_utc(self, std::initializer_list<std::string_view> features) const
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This is part of the time-aware surface and expects timeline or time-index semantics.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"ts", pluss::table::Thread(pluss::make_timeline({
        "2024-01-01T09:10:00Z",
        "2024-01-01T09:40:00Z",
        "2024-01-01T10:05:00Z",
    }))},
    {"load", pluss::table::Thread(pluss::make_series({10.0, 20.0, 15.0}))},
});

auto out = df.to_utc({"ts"});
```

#### Output

```text
ts
2024-01-01T09:00:00Z
2024-01-01T10:00:00Z
```

### `top`

#### Signature

```cpp
DataFrame top(self, std::string_view feature, std::int64_t n) const
```

#### Parameters

`feature` : `std::string_view`
    See the signature for the supported argument form.

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Returns the rows with the largest values in the selected feature.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.top("sales", 2);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `transform`

#### Signature

```cpp
DataFrame transform(self, const std::function<Thread(const Thread&)>& fn) const
```

#### Parameters

`fn` : `const std::function<Thread(const Thread&)>&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Rewrites one feature, several features, or all features by applying a user function to each targeted thread.

#### See also

- `DataFrame transform(self, std::string_view feature, const std::function<Thread(const Thread&)>& fn) const`
- `DataFrame transform(self, std::initializer_list<TransformSpec> specs) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.transform([](const pluss::table::Thread& thread) { return thread.kind() == pluss::table::ThreadKind::Series ? thread.clip(0.0, 110.0) : thread; });
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `transform`

#### Signature

```cpp
DataFrame transform(self, std::initializer_list<TransformSpec> specs) const
```

#### Parameters

`specs` : `std::initializer_list<TransformSpec>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Rewrites one feature, several features, or all features by applying a user function to each targeted thread.

#### See also

- `DataFrame transform(self, const std::function<Thread(const Thread&)>& fn) const`
- `DataFrame transform(self, std::string_view feature, const std::function<Thread(const Thread&)>& fn) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.transform({{.feature = "sales", .expr = [](const pluss::table::Thread& thread) { return thread.clip(0.0, 110.0); }}});
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `transform`

#### Signature

```cpp
DataFrame transform(self, std::string_view feature, const std::function<Thread(const Thread&)>& fn) const
```

#### Parameters

`feature` : `std::string_view`
    See the signature for the supported argument form.

`fn` : `const std::function<Thread(const Thread&)>&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Rewrites one feature, several features, or all features by applying a user function to each targeted thread.

#### See also

- `DataFrame transform(self, const std::function<Thread(const Thread&)>& fn) const`
- `DataFrame transform(self, std::initializer_list<TransformSpec> specs) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.transform("sales", [](const pluss::table::Thread& thread) { return thread.clip(0.0, 110.0); });
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `transpose`

#### Signature

```cpp
DataFrame transpose(self, bool render = false) const
```

#### Parameters

`render` : `bool`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Swaps rows and features so former feature names become row labels.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.transpose(false);
```

#### Output

```text
feature  row0   row1   row2
id       1      2      3
sales    100.0  120.0  90.0
```

### `truediv`

#### Signature

```cpp
DataFrame truediv(self, const DataFrame& other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### See also

- `DataFrame truediv(self, scalar other) const`

#### Examples

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"cost", pluss::table::Thread(pluss::make_series({60.0, 80.0, 55.0}))},
});

auto out = left.truediv(right);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `truediv`

#### Signature

```cpp
DataFrame truediv(self, scalar other) const
```

#### Parameters

`other` : `scalar`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### See also

- `DataFrame truediv(self, const DataFrame& other) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.truediv(10);
```

#### Output

```text
id  sales  city
11  110.0  Accra
12  130.0  Tema
13  100.0  Kumasi
```

### `tz_convert`

#### Signature

```cpp
DataFrame tz_convert(self, std::initializer_list<std::string_view> features, std::string_view tz_name) const
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

`tz_name` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This is part of the time-aware surface and expects timeline or time-index semantics.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"ts", pluss::table::Thread(pluss::make_timeline({
        "2024-01-01T09:10:00Z",
        "2024-01-01T09:40:00Z",
        "2024-01-01T10:05:00Z",
    }))},
    {"load", pluss::table::Thread(pluss::make_series({10.0, 20.0, 15.0}))},
});

auto out = df.tz_convert({"ts"}, "America/Edmonton");
```

#### Output

```text
ts
2024-01-01T09:00:00Z
2024-01-01T10:00:00Z
```

### `tz_localize`

#### Signature

```cpp
DataFrame tz_localize(self, std::initializer_list<std::string_view> features, std::string_view tz_name, int fold = 0, std::string_view nonexistent = "raise") const
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

`tz_name` : `std::string_view`
    See the signature for the supported argument form.

`fold` : `int`
    See the signature for the supported argument form.

`nonexistent` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

This is part of the time-aware surface and expects timeline or time-index semantics.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"ts", pluss::table::Thread(pluss::make_timeline({
        "2024-01-01T09:10:00Z",
        "2024-01-01T09:40:00Z",
        "2024-01-01T10:05:00Z",
    }))},
    {"load", pluss::table::Thread(pluss::make_series({10.0, 20.0, 15.0}))},
});

auto out = df.tz_localize({"ts"}, "America/Edmonton", 0, "shift_forward");
```

#### Output

```text
ts
2024-01-01T09:00:00Z
2024-01-01T10:00:00Z
```

### `unique`

#### Signature

```cpp
bool unique(self) const
```

#### Parameters

`None`

#### Returns

`bool`
    Boolean status or membership result.

#### Notes

Reports whether the object satisfies the uniqueness constraint expected by its model, such as distinct labels or keys.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.unique();
```

#### Output

```text
true
```

### `unique_cases`

#### Signature

```cpp
DataFrame unique_cases(self, std::initializer_list<std::string_view> features =
```

#### Parameters

`features` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Keeps only the first occurrence of each case label and drops later duplicates.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.unique_cases();
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `unsqueeze`

#### Signature

```cpp
Atlas unsqueeze(self) const
```

#### Parameters

`None`

#### Returns

`Atlas`
    Value produced by this call.

#### Notes

Promotes the frame into a depth-1 `Atlas` so it can participate in multi-frame workflows.

#### See also

- `Atlas unsqueeze(self, const Thread& depth_labels) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.unsqueeze();
```

#### Output

```text
frame0:
sales
10.0
20.0

frame1:
sales
30.0
40.0
```

### `unsqueeze`

#### Signature

```cpp
Atlas unsqueeze(self, const Thread& depth_labels) const
```

#### Parameters

`depth_labels` : `const Thread&`
    See the signature for the supported argument form.

#### Returns

`Atlas`
    Value produced by this call.

#### Notes

Promotes the frame into a depth-1 `Atlas` so it can participate in multi-frame workflows.

#### See also

- `Atlas unsqueeze(self) const`

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto labels = pluss::table::Thread(pluss::make_note({"train"}));
auto out = df.unsqueeze(labels);
```

#### Output

```text
frame0:
sales
10.0
20.0

frame1:
sales
30.0
40.0
```

### `update`

#### Signature

```cpp
DataFrame update(self, const DataFrame& other, bool overwrite = true) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

`overwrite` : `bool`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Overlays matching features from another frame onto this frame using aligned cases.

#### Examples

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"cost", pluss::table::Thread(pluss::make_series({60.0, 80.0, 55.0}))},
});

auto out = left.update(right);
```

#### Output

```text
id  sales  cost
1   100.0  60.0
2   120.0  80.0
3    90.0  55.0
```

### `values`

#### Signature

```cpp
std::vector<Thread> values(self) const
```

#### Parameters

`None`

#### Returns

`std::vector<Thread>`
    Materialized collection of thread objects.

#### Notes

Materializes the current members as a collection you can inspect or reuse elsewhere.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.values();
```

#### Output

```text
[Thread(id), Thread(sales), Thread(city)]
```

### `var`

#### Signature

```cpp
DataFrame var(self, bool render = true) const
```

#### Parameters

`render` : `bool`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Computes per-feature variances for the selected numeric features.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.var(false);
```

#### Output

```text
feature  value
sales    310.0
```

### `view`

#### Signature

```cpp
DataFrame view(self, std::int64_t start, std::int64_t stop, bool render = true) const
```

#### Parameters

`start` : `std::int64_t`
    Inclusive starting position of the slice.

`stop` : `std::int64_t`
    Exclusive ending position of the slice.

`render` : `bool`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Returns a lightweight row view of the requested slice.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.view(0, 2, false);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `where`

#### Signature

```cpp
DataFrame where(self, const pluss::Series& bool_mask) const
```

#### Parameters

`bool_mask` : `const pluss::Series&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Keeps only the rows where the supplied boolean mask is true.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = pluss::make_series({true, false, true}, pluss::codem::ScalarType::Bool);
auto out = df.where(mask);
```

#### Output

```text
id  sales  city
1   100.0  Accra
3    90.0  Kumasi
```

### `window`

#### Signature

```cpp
DataFrame window(self, std::initializer_list<WindowSpec> specs, bool keep_input = true) const
```

#### Parameters

`specs` : `std::initializer_list<WindowSpec>`
    See the signature for the supported argument form.

`keep_input` : `bool`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Computes one or more window-based features such as rolling means over selected columns.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.window({{.output = "sales_roll", .feature = "sales", .op = pluss::table::ThreadWindowOp::RollingMean, .window = 2, .min_periods = 1}}, true);
```

#### Output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `with_cases`

#### Signature

```cpp
DataFrame with_cases(self, const Thread& cases) const
```

#### Parameters

`cases` : `const Thread&`
    Case-label thread to attach to the current object.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Replaces the current case axis with the supplied thread. The row values stay in the same order, but later case-aware operations now use the new labels.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto cases = pluss::table::Thread(pluss::make_note({"cs0", "cs1", "cs2"}));
auto out = df.with_cases(cases);
```

#### Output

```text
df.cases():
[0, 1, 2]

out.cases():
["cs0", "cs1", "cs2"]

out.head():
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `with_cases_`

#### Signature

```cpp
DataFrame& with_cases_(self, const Thread& cases)
```

#### Parameters

`cases` : `const Thread&`
    Case-label thread to attach to the current object.

#### Returns

`DataFrame&`
    Value produced by this call.

#### Notes

Replaces the current case axis in place. The row values stay in the same order, but later case-aware operations now use the new labels.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto cases = pluss::table::Thread(pluss::make_note({"cs0", "cs1", "cs2"}));
df.with_cases_(cases);
```

#### Output

```text
df.cases():
["cs0", "cs1", "cs2"]

df.head():
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `with_column`

#### Signature

```cpp
DataFrame with_column(self, std::string name, const Thread& column) const
```

#### Parameters

`name` : `std::string`
    See the signature for the supported argument form.

`column` : `const Thread&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### Notes

Returns a new frame with one added or replaced feature.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.with_column("profit", pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0})));
```

#### Output

```text
id  sales  city    profit
1   100.0  Accra   40.0
2   120.0  Tema    35.0
3    90.0  Kumasi  30.0
```

### `with_column_`

#### Signature

```cpp
DataFrame& with_column_(self, std::string name, const Thread& column)
```

#### Parameters

`name` : `std::string`
    See the signature for the supported argument form.

`column` : `const Thread&`
    See the signature for the supported argument form.

#### Returns

`DataFrame&`
    Value produced by this call.

#### Notes

Adds or replaces one feature in place.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.with_column_("profit", pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0})));
```

#### Output

```text
id  sales  city    profit
1   100.0  Accra   40.0
2   120.0  Tema    35.0
3    90.0  Kumasi  30.0
```
