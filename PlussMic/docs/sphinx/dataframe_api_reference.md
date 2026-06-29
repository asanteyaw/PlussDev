# DataFrame API Reference

# Practical DataFrame API Reference

## API Reference Style

This page documents the full public `DataFrame` surface with usage-first examples generated from the current table API.

## A

### `add`

#### API Signatures

```cpp
DataFrame add(self, const DataFrame& other) const

DataFrame add(self, scalar other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

This applies arithmetic across the compatible numeric payloads in the current object.

#### Examples

Example 1.

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({2, 2, 4}))},
    {"sales", pluss::table::Thread(pluss::make_series({110.0, 110.0, 100.0}))},
});

auto out = left.add(right);
```

```text
id  sales
3   210.0
4   230.0
7   190.0
```

Example 2.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.add(10);
```

```text
id  sales  city
11  110.0  Accra
12  130.0  Tema
13  100.0  Kumasi
```


### `add_prefix`

#### API Signature

```cpp
DataFrame add_prefix(self, std::string_view prefix) const
```

#### Parameters

`prefix` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

#### API Signature

```cpp
DataFrame add_suffix(self, std::string_view suffix) const
```

#### Parameters

`suffix` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

#### API Signature

```cpp
DataFrameView aggregate(self, pluss::msharp::RangeView::Of<SummarySpec> specs, bool render = true) const
```

#### Parameters

`specs` : `pluss::msharp::RangeView::Of<SummarySpec>`
    See the signature for the supported argument form.

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

#### Returns

`DataFrameView`
    Value produced by this call.

#### What It Does

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

```text
metric       value
sales_mean    115.0
```

#### Practical Behavior

- `aggregate(...)` builds a compact summary frame instead of returning grouped rows.
- Each `SummarySpec` chooses an output name, a source feature, and a summary operation.
- Use this when you want one-row summary output such as totals, means, mins, maxes, or counts.


### `align_cases`

#### API Signature

```cpp
DataFrame align_cases(self, const Thread& new_cases) const
```

#### Parameters

`new_cases` : `const Thread&`
    Replacement case labels in the exact order you want the rows to follow.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

#### Practical Behavior

- `align_cases(new_cases)` reorders rows by matching the existing case labels against `new_cases`.
- Every requested case label must already exist or the call throws.
- The output case axis is rewritten to `new_cases`, so both the row order and the labels now follow that target order.


### `all`

#### API Signatures

```cpp
Thread all(self, pluss::msharp::RangeView::Of<std::string_view> features = {}, bool skipna = true) const

Thread all(self, std::int64_t axis, pluss::msharp::RangeView::Of<std::string_view> features = {}, bool skipna = true) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`skipna` : `bool`
    When `true`, missing values are skipped during the reduction. When `false`, missing state can affect the result.

`axis` : `std::int64_t`
    Axis selector. On `DataFrame`, `0` usually means rows/cases and `1` means features/columns.

#### Returns

`Thread`
    Value produced by this call.

#### What It Does

Applies `all` to the current `DataFrame` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"passed_math", pluss::table::Thread(pluss::make_series({true, true, false}, pluss::codem::ScalarType::Bool))},
    {"passed_lab", pluss::table::Thread(pluss::make_series({true, false, false}, pluss::codem::ScalarType::Bool))},
});

auto out = df.all(1, {"passed_math", "passed_lab"}, true);
```

#### Axis Rules

- `axis=0` reduces down the selected boolean features and returns one result per feature.
- `axis=1` reduces across features and returns one result per row.
- These methods expect boolean features; they are not general numeric reducers.
- Nullable boolean input requires `skipna=true`.


### `any`

#### API Signatures

```cpp
Thread any(self, pluss::msharp::RangeView::Of<std::string_view> features = {}, bool skipna = true) const

Thread any(self, std::int64_t axis, pluss::msharp::RangeView::Of<std::string_view> features = {}, bool skipna = true) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`skipna` : `bool`
    When `true`, missing values are skipped during the reduction. When `false`, missing state can affect the result.

`axis` : `std::int64_t`
    Axis selector. On `DataFrame`, `0` usually means rows/cases and `1` means features/columns.

#### Returns

`Thread`
    Value produced by this call.

#### What It Does

Applies `any` to the current `DataFrame` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"passed_math", pluss::table::Thread(pluss::make_series({true, true, false}, pluss::codem::ScalarType::Bool))},
    {"passed_lab", pluss::table::Thread(pluss::make_series({true, false, false}, pluss::codem::ScalarType::Bool))},
});

auto out = df.any(1, {"passed_math", "passed_lab"}, true);
```

#### Axis Rules

- `axis=0` reduces down the selected boolean features and returns one result per feature.
- `axis=1` reduces across features and returns one result per row.
- These methods expect boolean features; they are not general numeric reducers.
- Nullable boolean input requires `skipna=true`.


### `assign`

#### API Signatures

```cpp
DataFrame assign(self, pluss::msharp::RangeView::Of<DeriveSpec> specs) const

DataFrame assign(self, pluss::msharp::RangeView::Of<std::pair<std::string, Thread>> assignments) const
```

#### Parameters

`specs` : `pluss::msharp::RangeView::Of<DeriveSpec>`
    See the signature for the supported argument form.

`assignments` : `pluss::msharp::RangeView::Of<std::pair<std::string, Thread>>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

Adds or replaces derived features computed from the current frame. Use this when the new column depends on one or more existing columns.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.assign({{.feature = "profit", .expr = [](const pluss::table::DataFrame& frame) { return pluss::table::Thread(frame.features("sales").as_series().sub(60)); }}});
```

```text
id  sales  city    profit
1   100.0  Accra   40.0
2   120.0  Tema    35.0
3    90.0  Kumasi  30.0
```

Example 2.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.assign({{"profit", pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0}))}});
```

```text
id  sales  city    profit
1   100.0  Accra   40.0
2   120.0  Tema    35.0
3    90.0  Kumasi  30.0
```

#### Practical Behavior

- `assign(...)` adds new derived features or replaces existing ones without mutating the source frame.
- Each derived thread must match the frame case count.
- Use the pair form when you already have the computed thread, and the spec form when the value should be computed from the current frame inside the call.


### `astype`

#### API Signatures

```cpp
DataFrame astype(self,  pluss::msharp::RangeView::Of<std::pair<std::string_view, pluss::codem::ScalarType>> feature_dtypes) const

DataFrame astype(self, pluss::msharp::RangeView::Of<std::string_view> features, pluss::codem::ScalarType dtype) const
```

#### Parameters

`feature_dtypes` : `pluss::msharp::RangeView::Of<std::pair<std::string_view, pluss::codem::ScalarType>>`
    See the signature for the supported argument form.

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`dtype` : `pluss::codem::ScalarType`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

This changes the semantic or numeric type so later operations route through the correct implementation path.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({{"sales", pluss::table::Thread(pluss::make_note({"100.0", "120.0", "90.0"}))}});

auto out = df.astype({{"sales", pluss::codem::ScalarType::Float64}});
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

Example 2.

```cpp
auto df = pluss::table::DataFrame({{"sales", pluss::table::Thread(pluss::make_note({"100.0", "120.0", "90.0"}))}});

auto out = df.astype({"sales"}, pluss::codem::ScalarType::Float64);
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

#### Conversion Rules

- These methods rewrite only the selected features; unselected features are passed through unchanged.
- Duplicate feature names in the request are ignored after the first occurrence.
- Unknown or empty feature names throw immediately.
- `astype(...)` is the direct numeric-series dtype conversion path, while `cast(...)` is the broader target-selection route.


### `at`

#### API Signature

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

#### What It Does

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

```text
Accra
```

#### Practical Behavior

- `at(row, feature)` returns one scalar value rendered as text.
- The row is positional here; it does not look up by case label.
- Use this for quick single-cell inspection, not for typed column work.


## B

### `bfill`

#### API Signature

```cpp
DataFrame bfill(self) const
```

#### Parameters

`None`

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Examples

```cpp
auto score = pluss::table::Thread(pluss::make_series({71.0, 0.0, 88.0, 0.0}))
                 .with_validity_mask(pluss::make_series({true, false, true, false}, pluss::codem::ScalarType::Bool));
auto df = pluss::table::DataFrame({
    {"score", score},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"}))},
});

auto out = df.bfill();
```

```text
score  city
71.0   Accra
71.0   Tema
88.0   Kumasi
88.0   Tamale
```

#### Practical Behavior

- `bfill()` runs backward fill feature-by-feature across the full frame.
- A null value is replaced by the next valid value below it in the same feature.
- Trailing null runs stay null because there is no later value to pull from.
- If you want to backward-fill only one feature, apply it at the `Thread` level and assign it back with `set(...)` or `set_features(...)`.

#### Common Pattern

```cpp
gradebook.set("Final Points Grade") =
    gradebook.features("Final Points Grade").bfill();
```


### `bottom`

#### API Signature

```cpp
DataFrame bottom(self, std::string_view feature, std::int64_t n) const
```

#### Parameters

`feature` : `std::string_view`
    Name of the source feature used by the operation.

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
id  sales  city
3    90.0  Kumasi
1   100.0  Accra
```

#### Practical Behavior

- `bottom(feature, n)` sorts ascending by `feature`, keeps nulls last, then returns the first `n` rows.
- Because nulls are sorted last, null rows are not chosen unless there are not enough non-null rows to satisfy `n`.
- `n == 0` returns an empty frame with the same schema.
- The output keeps the original row payloads and case labels of the selected rows.

#### Argument Effects

- Small `n` is useful for “lowest few rows” inspection.
- Larger `n` gives you the full ascending slice without writing `sort_values(...).head(...)` yourself.

#### Equivalent Expanded Form

```cpp
auto out = df.sort_values({pluss::table::DataFrame::ArrangeSpec{"sales", true, true}})
             .head(5, false);
```


### `bucketize`

#### API Signatures

```cpp
DataFrame bucketize(self, pluss::msharp::RangeView::Double bins, pluss::msharp::RangeView::Int labels = {}, bool right = true, bool include_lowest = true) const

DataFrame bucketize(self, pluss::msharp::RangeView::Of<std::string_view> feature_names, pluss::msharp::RangeView::Double bins, pluss::msharp::RangeView::Int labels = {}, bool right = true, bool include_lowest = true) const

DataFrame bucketize(self, std::string_view feature_name, pluss::msharp::RangeView::Double bins, pluss::msharp::RangeView::Int labels = {}, bool right = true, bool include_lowest = true) const
```

#### Parameters

`bins` : `pluss::msharp::RangeView::Double`
    Strictly increasing interval edges used to define bucket boundaries.

`labels` : `pluss::msharp::RangeView::Int`
    Optional integer labels assigned to the output buckets. Leave empty to let PlussMic use `0, 1, 2, ...`.

`right` : `bool`
    When `true`, intervals are right-closed. When `false`, they are left-closed and right-open.

`include_lowest` : `bool`
    When `true`, the first bucket also includes its left edge.

`feature_names` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) supplied explicitly for subset, ordering, or axis-replacement operations, depending on the method.

`feature_name` : `std::string_view`
    Name of the target feature to read, rewrite, sort by, or promote to cases.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

Buckets numeric `Series` features into interval labels while preserving the rest of the frame unchanged.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

std::vector<double> bins{0.0, 95.0, 110.0, 130.0};
std::vector<std::int64_t> labels{1, 2, 3};
auto out = df.bucketize("sales", bins, labels, true, true);
```

```text
id  sales  city
1   2      Accra
2   3      Tema
3   1      Kumasi
```

#### Practical Behavior

- `bucketize(...)` delegates to thread-level bucketization only for the selected numeric `Series` features.
- Non-selected features pass through unchanged, so this is safe to use on mixed-type frames.
- The no-feature overload auto-selects every numeric `Series` feature in the frame.
- Bucket labels are returned as Int64-backed `Thread` columns and out-of-range values remain null.


## C

### `cases`

#### API Signatures

```cpp
const Thread& cases(self) const

DataFrame cases(self, std::int64_t row) const

DataFrame cases(self, std::pair<std::int64_t, std::int64_t> row_span) const

std::string cases(self, std::int64_t row, std::size_t feature_idx) const

std::string cases(self, std::int64_t row, std::string_view feature_name) const
```

#### Parameters

`row` : `std::int64_t`
    See the signature for the supported argument form.

`row_span` : `std::pair<std::int64_t, std::int64_t>`
    See the signature for the supported argument form.

`feature_idx` : `std::size_t`
    See the signature for the supported argument form.

`feature_name` : `std::string_view`
    Name of the target feature to read, rewrite, sort by, or promote to cases.

#### Returns

`const Thread&`
    Value produced by this call.

`DataFrame`
    Value produced by this call.

`std::string`
    Rendered string result.

#### What It Does

Fetches one value, member, or slice from the current object without changing the source object.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.cases();
```
Example 2.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.cases(0);
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

Example 3.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.cases({0, 2});
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

Example 4.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.cases(0, 0);
```

```text
1
```

Example 5.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.cases(0, "city");
```

```text
Accra
```

#### Overload Guide

- `cases(row)` and `cases(start, stop)` return row slices.
- `cases(row, feature)` returns one scalar value from a row/feature intersection.
- Use `cases_series()` only when the case axis is actually Series-backed.


### `cases_series`

#### API Signature

```cpp
pluss::Series cases_series(self) const
```

#### Parameters

`None`

#### Returns

`pluss::Series`
    Value produced by this call.

#### What It Does

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

```text
[0, 1, 2]
```


### `cast`

#### API Signatures

```cpp
DataFrame cast(self,  pluss::msharp::RangeView::Of<std::pair<std::string_view, pluss::codem::ScalarType>> feature_dtypes) const

DataFrame cast(self, pluss::msharp::RangeView::Of<CastSpec> specs) const
```

#### Parameters

`feature_dtypes` : `pluss::msharp::RangeView::Of<std::pair<std::string_view, pluss::codem::ScalarType>>`
    See the signature for the supported argument form.

`specs` : `pluss::msharp::RangeView::Of<CastSpec>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

This changes the semantic or numeric type so later operations route through the correct implementation path.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({{"sales", pluss::table::Thread(pluss::make_note({"100.0", "120.0", "90.0"}))}});

auto out = df.astype({{"sales", pluss::codem::ScalarType::Float64}});
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

Example 2.

```cpp
auto df = pluss::table::DataFrame({{"sales", pluss::table::Thread(pluss::make_note({"100.0", "120.0", "90.0"}))}});

auto out = df.cast({{.feature = "sales", .target = pluss::table::ThreadCastTarget::Series, .series_dtype = pluss::codem::ScalarType::Float64}});
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

#### Conversion Rules

- These methods rewrite only the selected features; unselected features are passed through unchanged.
- Duplicate feature names in the request are ignored after the first occurrence.
- Unknown or empty feature names throw immediately.
- `astype(...)` is the direct numeric-series dtype conversion path, while `cast(...)` is the broader target-selection route.


### `cast_note`

#### API Signature

```cpp
DataFrame cast_note(self, pluss::msharp::RangeView::Of<std::string_view> features) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

#### Conversion Rules

- These methods rewrite only the selected features; unselected features are passed through unchanged.
- Duplicate feature names in the request are ignored after the first occurrence.
- Unknown or empty feature names throw immediately.
- `astype(...)` is the direct numeric-series dtype conversion path, while `cast(...)` is the broader target-selection route.


### `cast_series`

#### API Signature

```cpp
DataFrame cast_series(self, pluss::msharp::RangeView::Of<std::string_view> features, std::optional<pluss::codem::ScalarType> dtype = std::nullopt) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`dtype` : `std::optional<pluss::codem::ScalarType>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

This changes the semantic or numeric type so later operations route through the correct implementation path.

#### Examples

```cpp
auto df = pluss::table::DataFrame({{"sales", pluss::table::Thread(pluss::make_note({"100.0", "120.0", "90.0"}))}});

auto out = df.cast_series({"sales"}, pluss::codem::ScalarType::Float64);
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

#### Conversion Rules

- These methods rewrite only the selected features; unselected features are passed through unchanged.
- Duplicate feature names in the request are ignored after the first occurrence.
- Unknown or empty feature names throw immediately.
- `astype(...)` is the direct numeric-series dtype conversion path, while `cast(...)` is the broader target-selection route.


### `cast_timeindex`

#### API Signature

```cpp
DataFrame cast_timeindex(self, pluss::msharp::RangeView::Of<std::string_view> features) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

#### Conversion Rules

- These methods rewrite only the selected features; unselected features are passed through unchanged.
- Duplicate feature names in the request are ignored after the first occurrence.
- Unknown or empty feature names throw immediately.
- `astype(...)` is the direct numeric-series dtype conversion path, while `cast(...)` is the broader target-selection route.


### `cast_timeline`

#### API Signature

```cpp
DataFrame cast_timeline(self, pluss::msharp::RangeView::Of<std::string_view> features) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

This changes the semantic or numeric type so later operations route through the correct implementation path.

#### Examples

```cpp
auto df = pluss::table::DataFrame({{"ts", pluss::table::Thread(pluss::make_note({"2024-01-01T09:10:00Z", "2024-01-01T09:40:00Z"}))}});

auto out = df.cast_timeline({"ts"});
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

#### Conversion Rules

- These methods rewrite only the selected features; unselected features are passed through unchanged.
- Duplicate feature names in the request are ignored after the first occurrence.
- Unknown or empty feature names throw immediately.
- `astype(...)` is the direct numeric-series dtype conversion path, while `cast(...)` is the broader target-selection route.


### `ceil_time`

#### API Signature

```cpp
DataFrame ceil_time(self, pluss::msharp::RangeView::Of<std::string_view> features, std::string_view unit) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`unit` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
ts
2024-01-01T09:00:00Z
2024-01-01T10:00:00Z
```

#### Practical Behavior

- `ceil_time(...)` rounds selected temporal features upward to the requested unit.
- Only timeline/timeindex features are valid targets.
- Use this when you want time-bucket labels that always round toward the next boundary.


### `cloc`

#### API Signatures

```cpp
DataFrame cloc(self, std::int64_t row) const

DataFrame cloc(self, std::int64_t start, std::int64_t stop) const
```

#### Parameters

`row` : `std::int64_t`
    See the signature for the supported argument form.

`start` : `std::int64_t`
    Inclusive starting position of the slice.

`stop` : `std::int64_t`
    Exclusive ending position of the slice.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

Applies `cloc` to the current `DataFrame` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.cloc();
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

#### Practical Behavior

- `cloc(...)` is the case-axis row slice helper.
- It is equivalent to slicing rows, not selecting by feature names.
- Use it when you want row-position access in a method that reads like case-location.


### `combine`

#### API Signatures

```cpp
DataFrame combine(self, const DataFrame& other) const

DataFrame combine(self, const DataFrame& other, const CombineSpec& spec) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

`spec` : `const CombineSpec&`
    Structured reorder description. It lets you move a subset of features to the front, back, or relative to one anchor feature without listing the entire schema.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

Combines this frame with another frame according to the supplied combine policy, such as feature-wise update or join-like alignment.

#### Examples

Example 1.

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

```text
id  sales  cost
1   100.0  60.0
2   120.0  80.0
3    90.0  55.0
```

Example 2.

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

```text
id  sales  cost
1   100.0  60.0
2   120.0  80.0
3    90.0  55.0
```

#### Practical Behavior

- `combine(...)` is the lower-level composition route behind feature-wise join/update behaviors.
- Collision handling depends on the chosen `CombineSpec` and suffix settings.
- Use the dedicated helpers such as `join(...)`, `update(...)`, or `unify(...)` when they fit; use `combine(...)` when you need explicit combine policy control.


### `compare`

#### API Signatures

```cpp
DataFrame compare(self, const DataFrame& other) const

DataFrame compare(self, scalar other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

This produces a comparison-style result you can inspect directly or reuse as a mask.

#### Examples

Example 1.

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({2, 2, 4}))},
    {"sales", pluss::table::Thread(pluss::make_series({110.0, 110.0, 100.0}))},
});

auto out = left.compare(right);
```

```text
id  sales
1   true
2   true
3   false
```

Example 2.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.compare(10);
```

```text
id  sales
1   true
2   true
3   false
```

#### Practical Behavior

- `compare(...)` is the “not equal” comparison alias on `DataFrame`.
- It returns a boolean-like frame you can inspect directly or reuse as a mask.
- Use `eq/ne/lt/le/gt/ge` when you want the comparison intent spelled out directly.


### `conform_to`

#### API Signature

```cpp
DataFrame conform_to(self, const DataFrame& other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

#### Practical Behavior

- `conform_to(other)` first reorders features to match `other.feature_names()` and then aligns the case axis to `other.cases()`.
- Both frames must have the same feature count and the same feature name set.
- Use this before arithmetic or updates when structure compatibility matters more than preserving the current order.


### `copy`

#### API Signature

```cpp
DataFrame copy(self) const
```

#### Parameters

`None`

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

#### Practical Behavior

- `copy()` gives you a detached frame value for downstream edits.
- Use it when you want to make the independence of the result explicit in user code or examples.


### `corr`

#### API Signatures

```cpp
DataFrameView corr(self, bool render = false) const

DataFrameView corr(self, CorrMethod method, bool render = false) const

DataFrameView corr(self, pluss::msharp::RangeView::Of<std::string_view> features, bool render = false) const

DataFrameView corr(self, pluss::msharp::RangeView::Of<std::string_view> features, CorrMethod method, bool render = false) const
```

#### Parameters

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

`method` : `CorrMethod`
    See the signature for the supported argument form.

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

#### Returns

`DataFrameView`
    Value produced by this call.

#### What It Does

Applies `corr` to the current `DataFrame` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.corr({"id", "sales"}, pluss::table::DataFrame::CorrMethod::Pearson, false);
```

```text
feature  id      sales
id       1.0000  0.3273
sales    0.3273  1.0000
```

#### Practical Behavior

- `corr(...)` builds a correlation matrix across numeric features only.
- If you do not pass a feature list, the method auto-selects numeric features from the frame.
- Supported methods route through thread-level correlation kernels such as Pearson, Spearman, and Kendall Tau.


### `count`

#### API Signatures

```cpp
DataFrameView count(self, bool render = true) const

Thread count(self, int axis) const

Thread count(self, std::int64_t axis) const
```

#### Parameters

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

`axis` : `int`
    Axis selector. On `DataFrame`, `0` usually means rows/cases and `1` means features/columns.

#### Returns

`DataFrameView`
    Value produced by this call.

`Thread`
    Value produced by this call.

#### What It Does

Returns a size/count summary for the current object without changing it.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.count(false);
```

```text
feature  value
id       3
sales    3
city     3
```

#### Practical Behavior

- `count()` returns per-feature non-null counts as a summary frame.
- This is not row count; use `nrows()` when you want total case count.


### `count_by`

#### API Signature

```cpp
DataFrameView count_by(self, std::string_view feature, std::int64_t top_n = 0, bool dropna = true, bool render = true) const
```

#### Parameters

`feature` : `std::string_view`
    Name of the source feature used by the operation.

`top_n` : `std::int64_t`
    See the signature for the supported argument form.

`dropna` : `bool`
    See the signature for the supported argument form.

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

#### Returns

`DataFrameView`
    Value produced by this call.

#### What It Does

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

```text
city    count
Accra   2
Tema    2
```

#### Practical Behavior

- `count_by(feature, ...)` groups by one feature, counts rows per distinct value, sorts by count descending, then trims to `top_n` when requested.
- Ties are broken by key name in ascending order.


### `count_distinct`

#### API Signature

```cpp
DataFrameView count_distinct(self, pluss::msharp::RangeView::Of<std::string_view> features = {}, bool dropna = true, bool render = true) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`dropna` : `bool`
    See the signature for the supported argument form.

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

#### Returns

`DataFrameView`
    Value produced by this call.

#### What It Does

Applies `count_distinct` to the current `DataFrame` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.count_distinct({"id", "sales", "city"}, true, false);
```

```text
case     id  sales  city
summary  3   3      2
```

#### Practical Behavior

- `count_distinct(...)` returns one summary row containing distinct-value counts for the selected features.
- With no feature list, every feature is included.
- `dropna` controls whether null should participate as its own distinct value.


### `count_null_rows`

#### API Signature

```cpp
std::int64_t count_null_rows(self) const
```

#### Parameters

`None`

#### Returns

`std::int64_t`
    Integer count, size, or positional result.

#### What It Does

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

```text
3
```

#### Practical Behavior

- `count_null_rows()` counts how many rows contain at least one null value anywhere in the frame.
- Use this as a quick completeness check before grouping, joining, or export.


## D

### `defined`

#### API Signature

```cpp
bool defined(self) const
```

#### Parameters

`None`

#### Returns

`bool`
    Boolean status or membership result.

#### What It Does

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

```text
true
```


### `describe`

#### API Signature

```cpp
DataFrameView describe(self, bool render = true) const
```

#### Parameters

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

#### Returns

`DataFrameView`
    Value produced by this call.

#### What It Does

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

```text
feature  count  mean   min   max
sales    3      103.3  90.0  120.0
```

#### Practical Behavior

- `describe()` summarizes numeric series features only.
- Non-numeric and non-series features are skipped rather than coerced.
- Null values are removed before the statistics are computed.


### `div`

#### API Signatures

```cpp
DataFrame div(self, const DataFrame& other) const

DataFrame div(self, scalar other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

This applies arithmetic across the compatible numeric payloads in the current object.

#### Examples

Example 1.

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({2, 2, 4}))},
    {"sales", pluss::table::Thread(pluss::make_series({110.0, 110.0, 100.0}))},
});

auto out = left.div(right);
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

Example 2.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.div(10);
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `divide`

#### API Signatures

```cpp
DataFrame divide(self, const DataFrame& other) const

DataFrame divide(self, scalar other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

This applies arithmetic across the compatible numeric payloads in the current object.

#### Examples

Example 1.

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({2, 2, 4}))},
    {"sales", pluss::table::Thread(pluss::make_series({110.0, 110.0, 100.0}))},
});

auto out = left.divide(right);
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

Example 2.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.divide(10);
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `drop`

#### API Signature

```cpp
DataFrame drop(self, pluss::msharp::RangeView::Of<std::string_view> names) const
```

#### Parameters

`names` : `pluss::msharp::RangeView::Of<std::string_view>`
    Collection of feature, slot, or stream names supplied in positional order.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
id  sales
1   100.0
2   120.0
3    90.0
```

#### Practical Behavior

- `drop(...)` removes features, not rows.
- The remaining features keep their existing row alignment and case axis.


### `dropna`

#### API Signature

```cpp
DataFrame dropna(self) const
```

#### Parameters

`None`

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Examples

```cpp
auto sales = pluss::table::Thread(pluss::make_series({100.0, 0.0, 90.0}))
                 .with_validity_mask(pluss::make_series({true, false, true}, pluss::codem::ScalarType::Bool));
auto df = pluss::table::DataFrame({
    {"id", pluss::make_series({1, 2, 3})},
    {"sales", sales},
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi"})},
});

auto out = df.dropna();
```

```text
id  sales  city
1   100.0  Accra
3    90.0  Kumasi
```

#### Practical Behavior

- `dropna()` removes rows that contain at least one null value anywhere in the frame.
- Use `dropna_all(features)` when the rule should apply only to selected features and only when all of them are null.


### `dropna_all`

#### API Signature

```cpp
DataFrame dropna_all(self, pluss::msharp::RangeView::Of<std::string_view> features = {}) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
id  sales
2   null
3   90.0
```

#### Practical Behavior

- `dropna_all(features)` removes rows only when all selected features are null in that row.
- Rows with at least one observed value among the selected features are kept.


## E

### `eq`

#### API Signatures

```cpp
DataFrame eq(self, const DataFrame& other) const

DataFrame eq(self, scalar other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

This produces a comparison-style result you can inspect directly or reuse as a mask.

#### Examples

Example 1.

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({2, 2, 4}))},
    {"sales", pluss::table::Thread(pluss::make_series({110.0, 110.0, 100.0}))},
});

auto out = left.eq(right);
```

```text
id  sales
1   true
2   true
3   false
```

Example 2.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.eq(10);
```

```text
id  sales
1   true
2   true
3   false
```


### `equals`

#### API Signature

```cpp
bool equals(self, const DataFrame& other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`bool`
    Boolean status or membership result.

#### What It Does

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

```text
true
```


## F

### `feature_names`

#### API Signature

```cpp
const std::vector<std::string>& feature_names(self) const
```

#### Parameters

`None`

#### Returns

`const std::vector<std::string>&`
    Borrowed list of names.

#### What It Does

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

```text
["id", "sales", "city"]
```


### `feature_ref`

#### API Signature

```cpp
ColumnRef feature_ref(self, std::string_view name) const
```

#### Parameters

`name` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`ColumnRef`
    Value produced by this call.

#### What It Does

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

#### API Signatures

```cpp
DataFrame features(self, pluss::msharp::RangeView::Of<std::string_view> names)

DataFrame features(self, pluss::msharp::RangeView::Of<std::string_view> names) const

DataFrame features(self, StackArray<std::string_view> names)

DataFrame features(self, StackArray<std::string_view> names) const

DataFrame features(self, std::initializer_list<const char*> names)

DataFrame features(self, std::initializer_list<const char*> names) const

DataFrame features(self, std::initializer_list<std::string_view> names)

DataFrame features(self, std::initializer_list<std::string_view> names) const

std::vector<Thread> features(self) const

Thread features(self, std::string_view name)

Thread features(self, std::string_view name) const
```

#### Parameters

`names` : `pluss::msharp::RangeView::Of<std::string_view>`
    Collection of feature, slot, or stream names supplied in positional order.

`name` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

`std::vector<Thread>`
    Materialized collection of thread objects.

`Thread`
    Value produced by this call.

#### What It Does

Materializes the current members as a collection you can inspect or reuse elsewhere.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.features("sales");
```

```text
Thread([100.0, 120.0, 90.0])
```

Example 2.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.features({"id", "sales"});
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

Example 3.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.features();
```

```text
[Thread(id), Thread(sales), Thread(city)]
```


### `ffill`

#### API Signature

```cpp
DataFrame ffill(self) const
```

#### Parameters

`None`

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `fillna`

#### API Signatures

```cpp
DataFrame fillna(self, const char* value) const

DataFrame fillna(self, const char* value, pluss::msharp::RangeView::Of<std::string_view> features) const

DataFrame fillna(self, const pluss::TimeIndex& value) const

DataFrame fillna(self, const pluss::TimeIndex& value, pluss::msharp::RangeView::Of<std::string_view> features) const

DataFrame fillna(self, const pluss::Timeline& value) const

DataFrame fillna(self, const pluss::Timeline& value, pluss::msharp::RangeView::Of<std::string_view> features) const

DataFrame fillna(self, scalar value) const

DataFrame fillna(self, scalar value, pluss::msharp::RangeView::Of<std::string_view> features) const

DataFrame fillna(self, std::string_view value) const

DataFrame fillna(self, std::string_view value, pluss::msharp::RangeView::Of<std::string_view> features) const
```

#### Parameters

`value` : `const char*`
    See the signature for the supported argument form.

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({{"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))}});

auto out = df.fillna("Unknown");
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

Example 2.

```cpp
auto df = pluss::table::DataFrame({{"idx", pluss::table::Thread(pluss::make_timeindex({"2024-01-01T09:10:00Z", "2024-01-01T09:40:00Z"}))}});

auto out = df.fillna(pluss::make_timeindex("2024-01-01T00:00:00Z"));
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

Example 3.

```cpp
auto df = pluss::table::DataFrame({{"ts", pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-01-01T09:40:00Z"}))}});

auto out = df.fillna(pluss::make_timeline("2024-01-01T00:00:00Z"));
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

Example 4.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.fillna(0.0, {"sales"});
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `fillna_`

#### API Signatures

```cpp
DataFrame& fillna_(self, const char* value)

DataFrame& fillna_(self, const pluss::TimeIndex& value)

DataFrame& fillna_(self, const pluss::Timeline& value)

DataFrame& fillna_(self, scalar value)

DataFrame& fillna_(self, std::string_view value)
```

#### Parameters

`value` : `const char*`
    See the signature for the supported argument form.

#### Returns

`DataFrame&`
    Value produced by this call.

#### What It Does

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({{"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))}});

df.fillna_("Unknown");
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

Example 2.

```cpp
auto df = pluss::table::DataFrame({{"idx", pluss::table::Thread(pluss::make_timeindex({"2024-01-01T09:10:00Z", "2024-01-01T09:40:00Z"}))}});

df.fillna_(pluss::make_timeindex("2024-01-01T00:00:00Z"));
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

Example 3.

```cpp
auto df = pluss::table::DataFrame({{"ts", pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-01-01T09:40:00Z"}))}});

df.fillna_(pluss::make_timeline("2024-01-01T00:00:00Z"));
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

Example 4.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.fillna_(0.0);
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `filter`

#### API Signatures

```cpp
DataFrame filter(self, const pluss::Series& bool_mask) const

DataFrame filter(self, const Thread& bool_mask) const
```

#### Parameters

`bool_mask` : `const pluss::Series&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

Keeps only the rows where the supplied boolean mask is true.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = pluss::make_series({true, false, true}, pluss::codem::ScalarType::Bool);
auto out = df.filter(mask);
```

```text
id  sales  city
1   100.0  Accra
3    90.0  Kumasi
```


### `floor_time`

#### API Signature

```cpp
DataFrame floor_time(self, pluss::msharp::RangeView::Of<std::string_view> features, std::string_view unit) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`unit` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
ts
2024-01-01T09:00:00Z
2024-01-01T10:00:00Z
```


### `floordiv`

#### API Signatures

```cpp
DataFrame floordiv(self, const DataFrame& other) const

DataFrame floordiv(self, scalar other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

This applies arithmetic across the compatible numeric payloads in the current object.

#### Examples

Example 1.

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({2, 2, 4}))},
    {"sales", pluss::table::Thread(pluss::make_series({110.0, 110.0, 100.0}))},
});

auto out = left.floordiv(right);
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

Example 2.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.floordiv(10);
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


## G

### `ge`

#### API Signatures

```cpp
DataFrame ge(self, const DataFrame& other) const

DataFrame ge(self, scalar other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

This produces a comparison-style result you can inspect directly or reuse as a mask.

#### Examples

Example 1.

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({2, 2, 4}))},
    {"sales", pluss::table::Thread(pluss::make_series({110.0, 110.0, 100.0}))},
});

auto out = left.ge(right);
```

```text
id  sales
1   true
2   true
3   false
```

Example 2.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.ge(10);
```

```text
id  sales
1   true
2   true
3   false
```


### `get`

#### API Signature

```cpp
std::optional<Thread> get(self, std::string_view feature_name) const
```

#### Parameters

`feature_name` : `std::string_view`
    Name of the target feature to read, rewrite, sort by, or promote to cases.

#### Returns

`std::optional<Thread>`
    Value produced by this call.

#### What It Does

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

```text
Thread([100.0, 120.0, 90.0])
```


### `group_by`

#### API Signature

```cpp
DataFrameView group_by(self, pluss::msharp::RangeView::Of<std::string_view> keys, pluss::msharp::RangeView::Of<GroupSpec> specs, bool dropna_keys = true, bool render = true) const
```

#### Parameters

`keys` : `pluss::msharp::RangeView::Of<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

`specs` : `pluss::msharp::RangeView::Of<GroupSpec>`
    See the signature for the supported argument form.

`dropna_keys` : `bool`
    When `true`, rows with null pivot/group keys are excluded before reshaping.

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

#### Returns

`DataFrameView`
    Value produced by this call.

#### What It Does

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

```text
city    sales_sum
Accra   220.0
Tema    240.0
```


### `gt`

#### API Signatures

```cpp
DataFrame gt(self, const DataFrame& other) const

DataFrame gt(self, scalar other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

This produces a comparison-style result you can inspect directly or reuse as a mask.

#### Examples

Example 1.

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({2, 2, 4}))},
    {"sales", pluss::table::Thread(pluss::make_series({110.0, 110.0, 100.0}))},
});

auto out = left.gt(right);
```

```text
id  sales
1   true
2   true
3   false
```

Example 2.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.gt(10);
```

```text
id  sales
1   true
2   true
3   false
```


## H

### `has_feature`

#### API Signature

```cpp
bool has_feature(self, std::string_view name) const
```

#### Parameters

`name` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`bool`
    Boolean status or membership result.

#### What It Does

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

```text
true
```


### `head`

#### API Signature

```cpp
DataFrameView head(self, std::int64_t n = 5, bool render = true) const
```

#### Parameters

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

#### Returns

`DataFrameView`
    Value produced by this call.

#### What It Does

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

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `height`

#### API Signature

```cpp
std::int64_t height(self) const
```

#### Parameters

`None`

#### Returns

`std::int64_t`
    Integer count, size, or positional result.

#### What It Does

Applies `height` to the current `DataFrame` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.height();
```

## I

### `iat`

#### API Signature

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

#### What It Does

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

```text
1
```


### `idxmax`

#### API Signature

```cpp
Thread idxmax(self, std::int64_t axis = 0, bool skipna = true) const
```

#### Parameters

`axis` : `std::int64_t`
    Axis selector. On `DataFrame`, `0` usually means rows/cases and `1` means features/columns.

`skipna` : `bool`
    When `true`, missing values are skipped during the reduction. When `false`, missing state can affect the result.

#### Returns

`Thread`
    Value produced by this call.

#### What It Does

Applies `idxmax` to the current `DataFrame` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.idxmax();
```

### `idxmin`

#### API Signature

```cpp
Thread idxmin(self, std::int64_t axis = 0, bool skipna = true) const
```

#### Parameters

`axis` : `std::int64_t`
    Axis selector. On `DataFrame`, `0` usually means rows/cases and `1` means features/columns.

`skipna` : `bool`
    When `true`, missing values are skipped during the reduction. When `false`, missing state can affect the result.

#### Returns

`Thread`
    Value produced by this call.

#### What It Does

Applies `idxmin` to the current `DataFrame` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.idxmin();
```

### `info`

#### API Signatures

```cpp
void info(self, const display::DisplayOptions& options = {}) const

void info(self, std::ostream& os, const display::DisplayOptions& options = {}) const
```

#### Parameters

`options` : `const display::DisplayOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

`os` : `std::ostream&`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### What It Does

This renders a human-readable view for inspection instead of creating a new analytic object.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.info();
```

```text
rows: 3
features: 3
numeric: id, sales
note: city
```


### `info_string`

#### API Signature

```cpp
std::string info_string(self, const display::DisplayOptions& options = {}) const
```

#### Parameters

`options` : `const display::DisplayOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`std::string`
    Rendered string result.

#### What It Does

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

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `insert`

#### API Signature

```cpp
DataFrame insert(self, std::int64_t feature_index, std::string name, const Thread& feature) const
```

#### Parameters

`feature_index` : `std::int64_t`
    Zero-based output column position used by structural methods such as `insert(...)`.

`name` : `std::string`
    See the signature for the supported argument form.

`feature` : `const Thread&`
    Name of the source feature used by the operation.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
id  sales  city    profit
1   100.0  Accra   40.0
2   120.0  Tema    35.0
3    90.0  Kumasi  30.0
```


### `isna`

#### API Signature

```cpp
DataFrame isna(self, pluss::msharp::RangeView::Of<std::string_view> features = {}) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `isnull`

#### API Signature

```cpp
DataFrame isnull(self, pluss::msharp::RangeView::Of<std::string_view> features = {}) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


## J

### `join`

#### API Signature

```cpp
DataFrame join(self, const DataFrame& other, std::string collision_suffix = "_right") const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

`collision_suffix` : `std::string`
    Suffix added when a second frame contributes a feature name that already exists on the left-hand side.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
id  sales  cost
1   100.0  60.0
2   120.0  80.0
3    90.0  55.0
```


## K

### `kindelize`

#### API Signatures

```cpp
pluss::kindle::Catalog kindelize(self) const

pluss::kindle::Catalog kindelize(self, const pluss::kindle::KindelizePolicy& policy) const
```

#### Parameters

`policy` : `const pluss::kindle::KindelizePolicy&`
    See the signature for the supported argument form.

#### Returns

`pluss::kindle::Catalog`
    Value produced by this call.

#### What It Does

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.kindelize();
```

## L

### `le`

#### API Signatures

```cpp
DataFrame le(self, const DataFrame& other) const

DataFrame le(self, scalar other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

This produces a comparison-style result you can inspect directly or reuse as a mask.

#### Examples

Example 1.

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({2, 2, 4}))},
    {"sales", pluss::table::Thread(pluss::make_series({110.0, 110.0, 100.0}))},
});

auto out = left.le(right);
```

```text
id  sales
1   true
2   true
3   false
```

Example 2.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.le(10);
```

```text
id  sales
1   true
2   true
3   false
```


### `load`

#### API Signatures

```cpp
static DataFrame load(std::string_view source_type, std::string_view filepath)

static DataFrame load(std::string_view source_type, std::string_view filepath, char sep, std::optional<std::size_t> ncases = std::nullopt, std::vector<std::string> usefeats = {}, std::optional<std::string> case_col = std::nullopt, pluss::loader::BadLinePolicy on_bad_lines = pluss::loader::BadLinePolicy::Error)

static DataFrame load(std::string_view source_type, std::string_view filepath, char sep, std::optional<std::size_t> ncases, pluss::msharp::RangeView::Of<std::string_view> usefeats, std::optional<std::string> case_col = std::nullopt, pluss::loader::BadLinePolicy on_bad_lines = pluss::loader::BadLinePolicy::Error)

static DataFrame load(std::string_view source_type, std::string_view filepath, const pluss::loader::Config& config)

static DataFrame load(std::string_view source_type, std::string_view filepath, const pluss::loader::ReadCsvOptions& options)

static DataFrame load(std::string_view source_type, std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)
```

#### Parameters

`source_type` : `std::string_view`
    Storage format selector such as `"csv"`, `"xlsx"`, `"json"`, or `"parquet"`.

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`sep` : `char`
    Column separator used for delimited text files such as CSV.

`ncases` : `std::optional<std::size_t>`
    Optional row limit applied while loading. Leave unset to read the whole source.

`usefeats` : `std::vector<std::string>`
    Feature names to load from the source. Leave empty to load every available feature.

`case_col` : `std::optional<std::string>`
    Optional source column that should become the DataFrame case axis while loading.

`on_bad_lines` : `pluss::loader::BadLinePolicy`
    Policy for malformed delimited rows during load. Common choices are to error, warn, or skip.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

`options` : `const pluss::loader::ReadCsvOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

`config_entries` : `std::initializer_list<pluss::loader::ConfigEntry>`
    Inline configuration entries used to build a lightweight loader configuration without naming a separate config object.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

Loads the object from disk using the corresponding PlussMic loader route.

#### Examples

Example 1.

```cpp
auto out = pluss::table::DataFrame::load("csv", "path/to/data.csv");
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

Example 2.

```cpp
auto out = pluss::table::DataFrame::load("path/to/data.csv", {"nrows", 16});
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

Example 3.

```cpp
pluss::loader::ReadCsvOptions options;
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv", options);
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

#### Practical Behavior

- `load(...)` is the format-switched entry point behind the explicit file loaders.
- Use it when the source format is chosen at runtime; otherwise, the format-specific loader is clearer.
- CSV-style overloads let you control separator, row limits, feature projection, case-axis promotion, and bad-line policy.


### `load_csv`

#### API Signatures

```cpp
static DataFrame load_csv(std::string_view filepath)

static DataFrame load_csv(std::string_view filepath, char sep, std::optional<std::size_t> ncases = std::nullopt, std::vector<std::string> usefeats = {}, std::optional<std::string> case_col = std::nullopt, pluss::loader::BadLinePolicy on_bad_lines = pluss::loader::BadLinePolicy::Error)

static DataFrame load_csv(std::string_view filepath, char sep, std::optional<std::size_t> ncases, pluss::msharp::RangeView::Of<std::string_view> usefeats, std::optional<std::string> case_col = std::nullopt, pluss::loader::BadLinePolicy on_bad_lines = pluss::loader::BadLinePolicy::Error)

static DataFrame load_csv(std::string_view filepath, const pluss::loader::Config& config)

static DataFrame load_csv(std::string_view filepath, const pluss::loader::ReadCsvOptions& options)

static DataFrame load_csv(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)

static DataFrame load_csv(std::string_view filepath, std::string_view header, std::size_t ncases)

static DataFrame load_csv(std::string_view filepath, std::string_view header, std::string_view case_feature = "none", std::string_view engine = "auto", std::optional<std::size_t> ncases = std::nullopt)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`sep` : `char`
    Column separator used for delimited text files such as CSV.

`ncases` : `std::optional<std::size_t>`
    Optional row limit applied while loading. Leave unset to read the whole source.

`usefeats` : `std::vector<std::string>`
    Feature names to load from the source. Leave empty to load every available feature.

`case_col` : `std::optional<std::string>`
    Optional source column that should become the DataFrame case axis while loading.

`on_bad_lines` : `pluss::loader::BadLinePolicy`
    Policy for malformed delimited rows during load. Common choices are to error, warn, or skip.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

`options` : `const pluss::loader::ReadCsvOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

`config_entries` : `std::initializer_list<pluss::loader::ConfigEntry>`
    Inline configuration entries used to build a lightweight loader configuration without naming a separate config object.

`header` : `std::string_view`
    Controls whether feature names are written to, or expected from, the file header row.

`case_feature` : `std::string_view`
    Name to use for the restored case-axis feature when cases are materialized back into ordinary columns.

`engine` : `std::string_view`
    Backend selector. Use `"auto"` to let PlussMic choose, or pass an explicit engine name when you need predictable routing.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

Loads the object from disk using the corresponding PlussMic loader route.

#### Examples

Example 1.

```cpp
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv");
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

Example 2.

```cpp
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv", {"nrows", 16});
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

Example 3.

```cpp
pluss::loader::ReadCsvOptions options;
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv", options);
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

#### Practical Behavior

- `load_csv(...)` loads one external source into typed PlussMic columns.
- Positional overloads are quick for ad-hoc work, while option/config overloads are better when you want repeatable loader settings.
- Excel multi-sheet overloads return `pluss::loader::LoadResult` because several frames may be produced at once.


### `load_excel`

#### API Signatures

```cpp
static DataFrame load_excel(std::string_view filepath)

static DataFrame load_excel(std::string_view filepath, const pluss::loader::Config& config)

static DataFrame load_excel(std::string_view filepath, const pluss::loader::ReadExcelOptions& options)

static DataFrame load_excel(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)

static DataFrame load_excel(std::string_view filepath, std::int64_t sheet_index, bool header = true)

static DataFrame load_excel(std::string_view filepath, std::string_view sheet_name, bool header = true)

static pluss::loader::LoadResult load_excel(std::string_view filepath, const std::vector<std::int64_t>& sheet_indices, bool header = true)

static pluss::loader::LoadResult load_excel(std::string_view filepath, const std::vector<std::string>& sheet_names, bool header = true)

static pluss::loader::LoadResult load_excel(std::string_view filepath, pluss::msharp::RangeView::Int sheet_indices, bool header = true)

static pluss::loader::LoadResult load_excel(std::string_view filepath, pluss::msharp::RangeView::Of<std::string_view> sheet_names, bool header = true)

static pluss::loader::LoadResult load_excel(std::string_view filepath, std::initializer_list<std::int64_t> sheet_indices, bool header = true)

static pluss::loader::LoadResult load_excel(std::string_view filepath, std::initializer_list<std::string_view> sheet_names, bool header = true)

static pluss::loader::LoadResult load_excel(std::string_view filepath, std::nullptr_t, bool header = true)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

`options` : `const pluss::loader::ReadExcelOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

`config_entries` : `std::initializer_list<pluss::loader::ConfigEntry>`
    Inline configuration entries used to build a lightweight loader configuration without naming a separate config object.

`sheet_index` : `std::int64_t`
    Zero-based worksheet position inside an Excel workbook.

`header` : `bool`
    Controls whether feature names are written to, or expected from, the file header row.

`sheet_name` : `std::string_view`
    Worksheet name to read from or write to inside an Excel workbook.

`sheet_indices` : `const std::vector<std::int64_t>&`
    Zero-based worksheet positions to load together from one Excel workbook.

`sheet_names` : `const std::vector<std::string>&`
    Worksheet names to load together from one Excel workbook. This overload returns a multi-sheet load result instead of one frame.

`arg2` : `std::nullptr_t`
    Pass `nullptr` here to request a multi-sheet Excel load result that includes every sheet.

#### Returns

`DataFrame`
    Value produced by this call.

`pluss::loader::LoadResult`
    Multi-sheet Excel load bundle containing one frame per requested sheet.

#### What It Does

Loads the object from disk using the corresponding PlussMic loader route.

#### Examples

Example 1.

```cpp
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx");
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

Example 2.

```cpp
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx", {"nrows", 16});
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

Example 3.

```cpp
pluss::loader::ReadExcelOptions options;
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx", options);
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

Example 4.

```cpp
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx", 0, true);
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

Example 5.

```cpp
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx", "Sheet1", true);
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

Example 6.

```cpp
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx", nullptr, true);
```

```text
Sheet1 -> [3 cs x 3 feat]
Sheet2 -> [3 cs x 3 feat]
```

#### Practical Behavior

- `load_excel(...)` loads one external source into typed PlussMic columns.
- Positional overloads are quick for ad-hoc work, while option/config overloads are better when you want repeatable loader settings.
- Excel multi-sheet overloads return `pluss::loader::LoadResult` because several frames may be produced at once.


### `load_json`

#### API Signatures

```cpp
static DataFrame load_json(std::string_view filepath)

static DataFrame load_json(std::string_view filepath, const pluss::loader::Config& config)

static DataFrame load_json(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

`config_entries` : `std::initializer_list<pluss::loader::ConfigEntry>`
    Inline configuration entries used to build a lightweight loader configuration without naming a separate config object.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

Loads the object from disk using the corresponding PlussMic loader route.

#### Examples

Example 1.

```cpp
auto out = pluss::table::DataFrame::load_json("path/to/data.json");
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

Example 2.

```cpp
auto out = pluss::table::DataFrame::load_json("path/to/data.json", {"nrows", 16});
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

#### Practical Behavior

- `load_json(...)` loads one external source into typed PlussMic columns.
- Positional overloads are quick for ad-hoc work, while option/config overloads are better when you want repeatable loader settings.
- Excel multi-sheet overloads return `pluss::loader::LoadResult` because several frames may be produced at once.


### `load_parquet`

#### API Signatures

```cpp
static DataFrame load_parquet(std::string_view filepath)

static DataFrame load_parquet(std::string_view filepath, const pluss::loader::Config& config)

static DataFrame load_parquet(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

`config_entries` : `std::initializer_list<pluss::loader::ConfigEntry>`
    Inline configuration entries used to build a lightweight loader configuration without naming a separate config object.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

Loads the object from disk using the corresponding PlussMic loader route.

#### Examples

Example 1.

```cpp
auto out = pluss::table::DataFrame::load_parquet("path/to/data.parquet");
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

Example 2.

```cpp
auto out = pluss::table::DataFrame::load_parquet("path/to/data.parquet", {"nrows", 16});
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

#### Practical Behavior

- `load_parquet(...)` loads one external source into typed PlussMic columns.
- Positional overloads are quick for ad-hoc work, while option/config overloads are better when you want repeatable loader settings.
- Excel multi-sheet overloads return `pluss::loader::LoadResult` because several frames may be produced at once.


### `load_txt`

#### API Signatures

```cpp
static DataFrame load_txt(std::string_view filepath)

static DataFrame load_txt(std::string_view filepath, const pluss::loader::Config& config)

static DataFrame load_txt(std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

`config_entries` : `std::initializer_list<pluss::loader::ConfigEntry>`
    Inline configuration entries used to build a lightweight loader configuration without naming a separate config object.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

Loads the object from disk using the corresponding PlussMic loader route.

#### Examples

Example 1.

```cpp
auto out = pluss::table::DataFrame::load_txt("path/to/data.txt");
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

Example 2.

```cpp
auto out = pluss::table::DataFrame::load_txt("path/to/data.txt", {"nrows", 16});
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

#### Practical Behavior

- `load_txt(...)` loads one external source into typed PlussMic columns.
- Positional overloads are quick for ad-hoc work, while option/config overloads are better when you want repeatable loader settings.
- Excel multi-sheet overloads return `pluss::loader::LoadResult` because several frames may be produced at once.


### `lt`

#### API Signatures

```cpp
DataFrame lt(self, const DataFrame& other) const

DataFrame lt(self, scalar other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

This produces a comparison-style result you can inspect directly or reuse as a mask.

#### Examples

Example 1.

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({2, 2, 4}))},
    {"sales", pluss::table::Thread(pluss::make_series({110.0, 110.0, 100.0}))},
});

auto out = left.lt(right);
```

```text
id  sales
1   true
2   false
3   true
```

Example 2.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.lt(110);
```

```text
id  sales
1   true
2   false
3   true
```

#### Comparison Rules

- `lt(...)` returns a boolean-style frame with the same case axis and selected feature layout as the input.
- Scalar/native comparisons broadcast the right-hand value across every compatible numeric cell.
- Frame-vs-frame comparisons are row-wise and require compatible shape and feature alignment.


## M

### `matrix`

#### API Signature

```cpp
MatrixValues matrix(self) const
```

#### Parameters

`None`

#### Returns

`MatrixValues`
    Grouped matrix blocks split by payload family (`Series`, `Note`, `Timeline`, and `TimeIndex`).

#### What It Does

Packs same-kind features into matrix-shaped pluss blocks so lower-level numeric, text, or temporal kernels can consume them efficiently.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto blocks = df.matrix();
auto out = fmt::format(
    "series blocks: {}\nnote blocks: {}\ntimeline blocks: {}\ntimeindex blocks: {}",
    blocks.series_span().size(),
    blocks.note_span().size(),
    blocks.timeline_span().size(),
    blocks.timeindex_span().size());
```

```text
series blocks: 1
note blocks: 1
timeline blocks: 0
timeindex blocks: 0
```

#### Practical Behavior

- `matrix()` does not coerce every feature into one mixed block.
- Instead, it packs features by payload family so numeric, note, timeline, and timeindex data stay type-stable.
- Use `as_series()`, `as_note()`, `as_timeline()`, or `as_timeindex()` only when the frame contributes exactly one block of that family.


### `max`

#### API Signatures

```cpp
DataFrameView max(self, bool render = true) const

Thread max(self, int axis, bool skipna = true) const

Thread max(self, std::int64_t axis, bool skipna = true) const
```

#### Parameters

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

`axis` : `int`
    Axis selector. On `DataFrame`, `0` usually means rows/cases and `1` means features/columns.

`skipna` : `bool`
    When `true`, missing values are skipped during the reduction. When `false`, missing state can affect the result.

#### Returns

`DataFrameView`
    Value produced by this call.

`Thread`
    Value produced by this call.

#### What It Does

Computes per-feature maxima for the selected numeric features.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.max(false);
```

```text
feature  value
id       3
sales    120.0
```

#### Practical Behavior

- `max(...)` reduces the selected numeric features to one summary row.
- Non-numeric features are skipped rather than coerced.
- The output stays in DataFrame form, which makes these reducers easy to reuse in downstream frame pipelines.


### `mean`

#### API Signatures

```cpp
DataFrameView mean(self, bool render = true) const

Thread mean(self, int axis, bool skipna = true) const

Thread mean(self, std::int64_t axis, bool skipna = true) const
```

#### Parameters

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

`axis` : `int`
    Axis selector. On `DataFrame`, `0` usually means rows/cases and `1` means features/columns.

`skipna` : `bool`
    When `true`, missing values are skipped during the reduction. When `false`, missing state can affect the result.

#### Returns

`DataFrameView`
    Value produced by this call.

`Thread`
    Value produced by this call.

#### What It Does

Computes per-feature means for the selected numeric features.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.mean(false);
```

```text
feature  value
id       2.0
sales    103.333333333
```

#### Practical Behavior

- `mean(...)` reduces the selected numeric features to one summary row.
- Non-numeric features are skipped rather than coerced.
- The output stays in DataFrame form, which makes these reducers easy to reuse in downstream frame pipelines.


### `mean_by`

#### API Signature

```cpp
DataFrameView mean_by(self, std::string_view group_feature, std::string_view value_feature, std::int64_t top_n = 0, std::int64_t min_count = 1, bool dropna_group = true, bool render = true) const
```

#### Parameters

`group_feature` : `std::string_view`
    See the signature for the supported argument form.

`value_feature` : `std::string_view`
    Feature whose values populate pivot cells or grouped summaries.

`top_n` : `std::int64_t`
    See the signature for the supported argument form.

`min_count` : `std::int64_t`
    See the signature for the supported argument form.

`dropna_group` : `bool`
    See the signature for the supported argument form.

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

#### Returns

`DataFrameView`
    Value produced by this call.

#### What It Does

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

```text
city    sales_mean
Accra   110.0
Tema    120.0
```


### `median`

#### API Signatures

```cpp
DataFrameView median(self, bool render = true) const

Thread median(self, int axis, bool skipna = true) const

Thread median(self, std::int64_t axis, bool skipna = true) const
```

#### Parameters

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

`axis` : `int`
    Axis selector. On `DataFrame`, `0` usually means rows/cases and `1` means features/columns.

`skipna` : `bool`
    When `true`, missing values are skipped during the reduction. When `false`, missing state can affect the result.

#### Returns

`DataFrameView`
    Value produced by this call.

`Thread`
    Value produced by this call.

#### What It Does

Applies `median` to the current `DataFrame` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.median(false);
```

```text
feature  value
id       2.0
sales    100.0
```

#### Practical Behavior

- `median(...)` reduces the selected numeric features to one summary row.
- Non-numeric features are skipped rather than coerced.
- The output stays in DataFrame form, which makes these reducers easy to reuse in downstream frame pipelines.


### `min`

#### API Signatures

```cpp
DataFrameView min(self, bool render = true) const

Thread min(self, int axis, bool skipna = true) const

Thread min(self, std::int64_t axis, bool skipna = true) const
```

#### Parameters

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

`axis` : `int`
    Axis selector. On `DataFrame`, `0` usually means rows/cases and `1` means features/columns.

`skipna` : `bool`
    When `true`, missing values are skipped during the reduction. When `false`, missing state can affect the result.

#### Returns

`DataFrameView`
    Value produced by this call.

`Thread`
    Value produced by this call.

#### What It Does

Computes per-feature minima for the selected numeric features.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.min(false);
```

```text
feature  value
id       1
sales    90.0
```

#### Practical Behavior

- `min(...)` reduces the selected numeric features to one summary row.
- Non-numeric features are skipped rather than coerced.
- The output stays in DataFrame form, which makes these reducers easy to reuse in downstream frame pipelines.


### `mod`

#### API Signatures

```cpp
DataFrame mod(self, const DataFrame& other) const

DataFrame mod(self, scalar other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

This applies arithmetic across the compatible numeric payloads in the current object.

#### Examples

Example 1.

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({2, 2, 4}))},
    {"sales", pluss::table::Thread(pluss::make_series({110.0, 110.0, 100.0}))},
});

auto out = left.mod(right);
```

```text
id  sales
1   100.0
0    10.0
3    90.0
```

Example 2.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.mod(10);
```

```text
id  sales  city
1   0.0    Accra
2   0.0    Tema
3   0.0    Kumasi
```


### `mul`

#### API Signatures

```cpp
DataFrame mul(self, const DataFrame& other) const

DataFrame mul(self, scalar other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

This applies arithmetic across the compatible numeric payloads in the current object.

#### Examples

Example 1.

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({2, 2, 4}))},
    {"sales", pluss::table::Thread(pluss::make_series({110.0, 110.0, 100.0}))},
});

auto out = left.mul(right);
```

```text
id  sales
2   11000.0
4   13200.0
12   9000.0
```

Example 2.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.mul(10);
```

```text
id  sales  city
10  1000.0  Accra
20  1200.0  Tema
30   900.0  Kumasi
```


### `multiply`

#### API Signatures

```cpp
DataFrame multiply(self, const DataFrame& other) const

DataFrame multiply(self, scalar other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

This applies arithmetic across the compatible numeric payloads in the current object.

#### Examples

Example 1.

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({2, 2, 4}))},
    {"sales", pluss::table::Thread(pluss::make_series({110.0, 110.0, 100.0}))},
});

auto out = left.multiply(right);
```

```text
id  sales
2   11000.0
4   13200.0
12   9000.0
```

Example 2.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.multiply(10);
```

```text
id  sales  city
10  1000.0  Accra
20  1200.0  Tema
30   900.0  Kumasi
```


## N

### `ncases`

#### API Signature

```cpp
std::int64_t ncases(self) const
```

#### Parameters

`None`

#### Returns

`std::int64_t`
    Integer count, size, or positional result.

#### What It Does

Applies `ncases` to the current `DataFrame` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.ncases();
```

```text
3
```


### `ne`

#### API Signatures

```cpp
DataFrame ne(self, const DataFrame& other) const

DataFrame ne(self, scalar other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

This produces a comparison-style result you can inspect directly or reuse as a mask.

#### Examples

Example 1.

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({2, 2, 4}))},
    {"sales", pluss::table::Thread(pluss::make_series({110.0, 110.0, 100.0}))},
});

auto out = left.ne(right);
```

```text
id  sales
1   true
2   true
3   false
```

Example 2.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.ne(10);
```

```text
id  sales
1   true
2   true
3   false
```


### `nfeats`

#### API Signature

```cpp
std::int64_t nfeats(self) const
```

#### Parameters

`None`

#### Returns

`std::int64_t`
    Integer count, size, or positional result.

#### What It Does

Applies `nfeats` to the current `DataFrame` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.nfeats();
```

```text
3
```


### `nfeatures`

#### API Signature

```cpp
std::int64_t nfeatures(self) const
```

#### Parameters

`None`

#### Returns

`std::int64_t`
    Integer count, size, or positional result.

#### What It Does

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

```text
3
```


### `note_features`

#### API Signature

```cpp
DataFrame note_features(self) const
```

#### Parameters

`None`

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
city
Accra
Tema
Kumasi
```


### `notna`

#### API Signature

```cpp
DataFrame notna(self, pluss::msharp::RangeView::Of<std::string_view> features = {}) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Examples

```cpp
auto sales = pluss::table::Thread(pluss::make_series({100.0, 0.0, 90.0}))
                 .with_validity_mask(pluss::make_series({true, false, true}, pluss::codem::ScalarType::Bool));
auto df = pluss::table::DataFrame({
    {"sales", sales},
});

auto out = df.notna({"sales"});
```

```text
sales
true
false
true
```

#### Practical Behavior

- `notna(...)` returns a boolean-style frame marking which cells are observed rather than missing.
- Use it to build masks before `filter(...)`, replacement policies, or diagnostic summaries.
- `notna(...)` and `notnull(...)` are equivalent.


### `notnull`

#### API Signature

```cpp
DataFrame notnull(self, pluss::msharp::RangeView::Of<std::string_view> features = {}) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Examples

```cpp
auto sales = pluss::table::Thread(pluss::make_series({100.0, 0.0, 90.0}))
                 .with_validity_mask(pluss::make_series({true, false, true}, pluss::codem::ScalarType::Bool));
auto df = pluss::table::DataFrame({
    {"sales", sales},
});

auto out = df.notnull({"sales"});
```

```text
sales
true
false
true
```

#### Practical Behavior

- `notnull(...)` returns a boolean-style frame marking which cells are observed rather than missing.
- Use it to build masks before `filter(...)`, replacement policies, or diagnostic summaries.
- `notna(...)` and `notnull(...)` are equivalent.


## P

### `pivot`

#### API Signature

```cpp
DataFrameView pivot(self, std::string_view case_feature, std::string_view feature_feature, std::string_view value_feature, bool dropna_keys = true, bool render = true) const
```

#### Parameters

`case_feature` : `std::string_view`
    Name to use for the restored case-axis feature when cases are materialized back into ordinary columns.

`feature_feature` : `std::string_view`
    Feature whose distinct values become new output columns in pivot-style reshaping.

`value_feature` : `std::string_view`
    Feature whose values populate pivot cells or grouped summaries.

`dropna_keys` : `bool`
    When `true`, rows with null pivot/group keys are excluded before reshaping.

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

#### Returns

`DataFrameView`
    Value produced by this call.

#### What It Does

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

```text
city   A      B
Accra  100.0  120.0
Tema    90.0  150.0
```


### `pivot_table`

#### API Signature

```cpp
DataFrameView pivot_table(self, std::string_view case_feature, std::string_view feature_feature, std::string_view value_feature, GroupOp op = GroupOp::Mean, bool dropna_keys = true, bool render = true) const
```

#### Parameters

`case_feature` : `std::string_view`
    Name to use for the restored case-axis feature when cases are materialized back into ordinary columns.

`feature_feature` : `std::string_view`
    Feature whose distinct values become new output columns in pivot-style reshaping.

`value_feature` : `std::string_view`
    Feature whose values populate pivot cells or grouped summaries.

`op` : `GroupOp`
    Operation key or enum that chooses the aggregation, comparison, or fill behavior for the call.

`dropna_keys` : `bool`
    When `true`, rows with null pivot/group keys are excluded before reshaping.

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

#### Returns

`DataFrameView`
    Value produced by this call.

#### What It Does

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

```text
city   A      B
Accra  100.0  120.0
Tema    90.0  150.0
```


### `place`

#### API Signatures

```cpp
DataFrame place(self, std::string name, const Thread& feature, std::string_view anchor_feature, bool after = false) const

DataFrame place(self, std::string_view name, std::string_view anchor_feature, bool after = false) const
```

#### Parameters

`name` : `std::string`
    See the signature for the supported argument form.

`feature` : `const Thread&`
    Name of the source feature used by the operation.

`anchor_feature` : `std::string_view`
    Reference feature used to place a newly inserted or moved feature before or after an existing column.

`after` : `bool`
    When `false`, placement happens before the anchor. When `true`, placement happens after the anchor.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

Applies `place` to the current `DataFrame` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.place();
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `plot`

#### API Signatures

```cpp
DataFramePlotAccessor plot(self) const

DataFramePlotAccessor plot(self, const DataFramePlotOptions& options) const
```

#### Parameters

`options` : `const DataFramePlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`DataFramePlotAccessor`
    Value produced by this call.

#### What It Does

This opens the plotting accessor for the current data object.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto fig = df.plot().line("sales");
```

```text
Figure(line/scatter/bar/hist)
```


### `plot3d`

#### API Signatures

```cpp
DataFramePlot3DAccessor plot3d(self) const

DataFramePlot3DAccessor plot3d(self, const DataFramePlotOptions& options) const
```

#### Parameters

`options` : `const DataFramePlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`DataFramePlot3DAccessor`
    Value produced by this call.

#### What It Does

Applies `plot3d` to the current `DataFrame` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.plot3d();
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `pow`

#### API Signatures

```cpp
DataFrame pow(self, const DataFrame& other) const

DataFrame pow(self, scalar other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

This applies arithmetic across the compatible numeric payloads in the current object.

#### Examples

Example 1.

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({2, 2, 4}))},
    {"sales", pluss::table::Thread(pluss::make_series({110.0, 110.0, 100.0}))},
});

auto out = left.pow(right);
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

Example 2.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.pow(10);
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `prepare_sql`

#### API Signatures

```cpp
DataFrameSqlPrepared prepare_sql(self, std::string_view statement) const

DataFrameSqlPrepared prepare_sql(self, std::string_view statement, SqlOptions options) const
```

#### Parameters

`statement` : `std::string_view`
    SQL statement text to prepare against the current frame.

`options` : `SqlOptions`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`DataFrameSqlPrepared`
    Value produced by this call.

#### What It Does

Use this when the transformation is easier to express as SQL than as a chained sequence of table verbs.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto stmt = df.prepare_sql("SELECT id, sales FROM self WHERE sales > {min_sales}");
```

```text
SQL accessor / prepared statement created
```


### `prod`

#### API Signatures

```cpp
DataFrameView prod(self, bool render = true) const

Thread prod(self, int axis, bool skipna = true) const

Thread prod(self, std::int64_t axis, bool skipna = true) const
```

#### Parameters

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

`axis` : `int`
    Axis selector. On `DataFrame`, `0` usually means rows/cases and `1` means features/columns.

`skipna` : `bool`
    When `true`, missing values are skipped during the reduction. When `false`, missing state can affect the result.

#### Returns

`DataFrameView`
    Value produced by this call.

`Thread`
    Value produced by this call.

#### What It Does

Applies `prod` to the current `DataFrame` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.prod();
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `put`

#### API Signatures

```cpp
DataFrame put(self, std::string_view feature, const pluss::Series& bool_mask, const char* value) const

DataFrame put(self, std::string_view feature, const pluss::Series& bool_mask, const Thread& value) const

DataFrame put(self, std::string_view feature, const pluss::Series& bool_mask, pluss::null_t value) const

DataFrame put(self, std::string_view feature, const pluss::Series& bool_mask, scalar value) const

DataFrame put(self, std::string_view feature, const pluss::Series& bool_mask, std::string_view value) const

DataFrame put(self, std::string_view feature, const Thread& bool_mask, const char* value) const

DataFrame put(self, std::string_view feature, const Thread& bool_mask, const Thread& value) const

DataFrame put(self, std::string_view feature, const Thread& bool_mask, pluss::null_t value) const

DataFrame put(self, std::string_view feature, const Thread& bool_mask, scalar value) const

DataFrame put(self, std::string_view feature, const Thread& bool_mask, std::string_view value) const
```

#### Parameters

`feature` : `std::string_view`
    Name of the source feature used by the operation.

`bool_mask` : `const pluss::Series&`
    See the signature for the supported argument form.

`value` : `const char*`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

Applies `put` to the current `DataFrame` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.put();
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `put_`

#### API Signatures

```cpp
DataFrame& put_(self, std::string_view feature, const pluss::Series& bool_mask, const char* value)

DataFrame& put_(self, std::string_view feature, const pluss::Series& bool_mask, const Thread& value)

DataFrame& put_(self, std::string_view feature, const pluss::Series& bool_mask, pluss::null_t value)

DataFrame& put_(self, std::string_view feature, const pluss::Series& bool_mask, scalar value)

DataFrame& put_(self, std::string_view feature, const pluss::Series& bool_mask, std::string_view value)

DataFrame& put_(self, std::string_view feature, const Thread& bool_mask, const char* value)

DataFrame& put_(self, std::string_view feature, const Thread& bool_mask, const Thread& value)

DataFrame& put_(self, std::string_view feature, const Thread& bool_mask, pluss::null_t value)

DataFrame& put_(self, std::string_view feature, const Thread& bool_mask, scalar value)

DataFrame& put_(self, std::string_view feature, const Thread& bool_mask, std::string_view value)
```

#### Parameters

`feature` : `std::string_view`
    Name of the source feature used by the operation.

`bool_mask` : `const pluss::Series&`
    See the signature for the supported argument form.

`value` : `const char*`
    See the signature for the supported argument form.

#### Returns

`DataFrame&`
    Value produced by this call.

#### What It Does

Applies `put_` to the current `DataFrame` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.put_();
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


## Q

### `query`

#### API Signature

```cpp
DataFrame query(self, std::string_view expression) const
```

#### Parameters

`expression` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
id  sales  city
1   100.0  Accra
3    90.0  Kumasi
```


## R

### `radd`

#### API Signature

```cpp
DataFrame radd(self, scalar other) const
```

#### Parameters

`other` : `scalar`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
id  sales  city
11  110.0  Accra
12  130.0  Tema
13  100.0  Kumasi
```


### `rdiv`

#### API Signature

```cpp
DataFrame rdiv(self, scalar other) const
```

#### Parameters

`other` : `scalar`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `reindex_cases`

#### API Signature

```cpp
DataFrame reindex_cases(self, const Thread& new_cases) const
```

#### Parameters

`new_cases` : `const Thread&`
    Replacement case labels in the exact order you want the rows to follow.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `reindex_like`

#### API Signature

```cpp
DataFrame reindex_like(self, const DataFrame& other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `relabel_features`

#### API Signature

```cpp
DataFrame relabel_features(self, pluss::msharp::RangeView::Of<std::string_view> feature_names) const
```

#### Parameters

`feature_names` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) supplied explicitly for subset, ordering, or axis-replacement operations, depending on the method.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
revenue  location  id_new
100.0    Accra     1
120.0    Tema      2
90.0     Kumasi    3
```


### `rename`

#### API Signature

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

#### What It Does

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

```text
id  revenue  city
1   100.0    Accra
2   120.0    Tema
3    90.0    Kumasi
```


### `rename_`

#### API Signature

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

#### What It Does

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

```text
id  revenue  city
1   100.0    Accra
2   120.0    Tema
3    90.0    Kumasi
```


### `rename_features`

#### API Signature

```cpp
DataFrame rename_features(self,  pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>> renames) const
```

#### Parameters

`renames` : `pluss::msharp::RangeView::Of<std::pair<std::string_view, std::string_view>>`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
id  revenue  location
1   100.0    Accra
2   120.0    Tema
3    90.0    Kumasi
```


### `reorder`

#### API Signatures

```cpp
DataFrame reorder(self, const ReorderSpec& spec) const

DataFrame reorder(self, pluss::msharp::RangeView::Of<std::string_view> names) const
```

#### Parameters

`spec` : `const ReorderSpec&`
    Structured reorder description. It lets you move a subset of features to the front, back, or relative to one anchor feature without listing the entire schema.

`names` : `pluss::msharp::RangeView::Of<std::string_view>`
    Collection of feature, slot, or stream names supplied in positional order.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

Moves selected features into a new relative position while preserving row data and the untouched feature order.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

pluss::table::DataFrame::ReorderSpec spec;
spec.features = {"city"};
spec.placement = pluss::table::DataFrame::ReorderPlacement::Front;
auto out = df.reorder(spec);
```

```text
city    id  sales
Accra   1   100.0
Tema    2   120.0
Kumasi  3    90.0
```

Example 2.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.reorder({"sales", "id", "city"});
```

```text
sales  id  city
100.0  1  Accra
120.0  2  Tema
90.0   3  Kumasi
```

#### Practical Behavior

- `reorder({"a", "b"})` now means “move these features together and preserve the relative order of everything else.”
- Use `ReorderSpec` when you need to move a subset to the front, back, before an anchor, or after an anchor.
- This avoids listing a 3000-feature schema just to reposition two columns.


### `repeated_cases`

#### API Signature

```cpp
pluss::Series repeated_cases(self, pluss::msharp::RangeView::Of<std::string_view> features = {}) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

#### Returns

`pluss::Series`
    Value produced by this call.

#### What It Does

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

### `replace`

#### API Signatures

```cpp
DataFrame replace(self, const char* from, const char* to, pluss::msharp::RangeView::Of<std::string_view> features = {}) const

DataFrame replace(self, const char* from, pluss::null_t to, pluss::msharp::RangeView::Of<std::string_view> features = {}) const

DataFrame replace(self, const pluss::Note& from, const pluss::Note& to, pluss::msharp::RangeView::Of<std::string_view> features = {}) const

DataFrame replace(self, const pluss::Note& from, pluss::null_t to, pluss::msharp::RangeView::Of<std::string_view> features = {}) const

DataFrame replace(self, const pluss::Series& from, const pluss::Series& to, pluss::msharp::RangeView::Of<std::string_view> features = {}) const

DataFrame replace(self, const pluss::Series& from, pluss::null_t to, pluss::msharp::RangeView::Of<std::string_view> features = {}) const

DataFrame replace(self, const pluss::TimeIndex& from, const pluss::TimeIndex& to, pluss::msharp::RangeView::Of<std::string_view> features = {}) const

DataFrame replace(self, const pluss::TimeIndex& from, pluss::null_t to, pluss::msharp::RangeView::Of<std::string_view> features = {}) const

DataFrame replace(self, const pluss::Timeline& from, const pluss::Timeline& to, pluss::msharp::RangeView::Of<std::string_view> features = {}) const

DataFrame replace(self, const pluss::Timeline& from, pluss::null_t to, pluss::msharp::RangeView::Of<std::string_view> features = {}) const

DataFrame replace(self, pluss::null_t from, const char* to, pluss::msharp::RangeView::Of<std::string_view> features = {}) const

DataFrame replace(self, pluss::null_t from, const pluss::Note& to, pluss::msharp::RangeView::Of<std::string_view> features = {}) const

DataFrame replace(self, pluss::null_t from, const pluss::Series& to, pluss::msharp::RangeView::Of<std::string_view> features = {}) const

DataFrame replace(self, pluss::null_t from, const pluss::TimeIndex& to, pluss::msharp::RangeView::Of<std::string_view> features = {}) const

DataFrame replace(self, pluss::null_t from, const pluss::Timeline& to, pluss::msharp::RangeView::Of<std::string_view> features = {}) const

DataFrame replace(self, pluss::null_t from, scalar to, pluss::msharp::RangeView::Of<std::string_view> features = {}) const

DataFrame replace(self, pluss::null_t from, std::string_view to, pluss::msharp::RangeView::Of<std::string_view> features = {}) const

DataFrame replace(self, scalar from, pluss::null_t to, pluss::msharp::RangeView::Of<std::string_view> features = {}) const

DataFrame replace(self, scalar from, scalar to, pluss::msharp::RangeView::Of<std::string_view> features = {}) const

DataFrame replace(self, std::string_view from, pluss::null_t to, pluss::msharp::RangeView::Of<std::string_view> features = {}) const

DataFrame replace(self, std::string_view from, std::string_view to, pluss::msharp::RangeView::Of<std::string_view> features = {}) const
```

#### Parameters

`from` : `const char*`
    See the signature for the supported argument form.

`to` : `const char*`
    See the signature for the supported argument form.

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

This is part of the text-processing surface for note-backed or text-oriented data.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.replace();
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `requires`

#### API Signatures

```cpp
template <class NameRange> requires(self, std::ranges::input_range<NameRange> && !std::is_convertible_v<NameRange, std::string_view> && !detail::IsInitializerListV<NameRange> && !std::same_as<std::remove_cvref_t<NameRange>, StackArray<std::string_view>> && std::convertible_to<std::remove_cvref_t<std::ranges::range_reference_t<NameRange>>, std::string_view>) DataFrame features(const NameRange& names)

template <class NameRange> requires(self, std::ranges::input_range<NameRange> && !std::is_convertible_v<NameRange, std::string_view> && !detail::IsInitializerListV<NameRange> && !std::same_as<std::remove_cvref_t<NameRange>, StackArray<std::string_view>> && std::convertible_to<std::remove_cvref_t<std::ranges::range_reference_t<NameRange>>, std::string_view>) DataFrame features(const NameRange& names) const
```

#### Parameters

`names` : `std::ranges::input_range<NameRange> && !std::is_convertible_v<NameRange, std::string_view> && !detail::IsInitializerListV<NameRange> && !std::same_as<std::remove_cvref_t<NameRange>, StackArray<std::string_view>> && std::convertible_to<std::remove_cvref_t<std::ranges::range_reference_t<NameRange>>, std::string_view>) DataFrame features(const NameRange&`
    Collection of feature, slot, or stream names supplied in positional order.

#### Returns

`template <class NameRange>`
    Value produced by this call.

#### What It Does

Applies `requires` to the current `DataFrame` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.requires();
```

### `resample`

#### API Signature

```cpp
DataFrameView resample(self, std::string_view time_feature, pluss::msharp::RangeView::Of<std::string_view> value_features, std::string_view freq, std::string_view op = "mean", std::int64_t step = 1, bool render = true) const
```

#### Parameters

`time_feature` : `std::string_view`
    Name of the Timeline or TimeIndex feature that defines the resampling, alignment, or calendar axis.

`value_features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) to aggregate inside each resampling bin. Leave empty when you want PlussMic to infer compatible value features.

`freq` : `std::string_view`
    Bin or calendar frequency such as `"1h"`, `"D"`, `"W"`, or `"M"`.

`op` : `std::string_view`
    Operation key or enum that chooses the aggregation, comparison, or fill behavior for the call.

`step` : `std::int64_t`
    Positive frequency multiplier. For example, `6` with hourly frequency means six-hour bins.

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

#### Returns

`DataFrameView`
    Value produced by this call.

#### What It Does

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

```text
ts                   load
2024-01-01T09:00:00Z  15.0
2024-01-01T10:00:00Z  15.0
```

#### Practical Behavior

- `resample(...)` groups rows into temporal bins and aggregates one value per bin and selected feature.
- `count` ignores missing values, while `size` counts rows whether the value is missing or not.
- For fixed frequencies, bins are anchored from the start of the local day rather than from the first observation timestamp.
- Numeric reductions such as `mean`, `sum`, `min`, `max`, `first`, and `last` require numeric Series features.


### `reset_cases`

#### API Signature

```cpp
DataFrame reset_cases(self, std::string case_feature = "case") const
```

#### Parameters

`case_feature` : `std::string`
    Name to use for the restored case-axis feature when cases are materialized back into ordinary columns.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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


### `reshuffle`

#### API Signature

```cpp
DataFrame reshuffle(self, int axis = 0, std::optional<std::uint64_t> random_state = std::nullopt) const
```

#### Parameters

`axis` : `int`
    Axis selector. On `DataFrame`, `0` usually means rows/cases and `1` means features/columns.

`random_state` : `std::optional<std::uint64_t>`
    Optional seed used to make sampling or shuffling reproducible.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

Applies `reshuffle` to the current `DataFrame` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.reshuffle();
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

#### Practical Behavior

- `reshuffle(axis=0)` permutes rows and keeps the feature order unchanged.
- `reshuffle(axis=1)` permutes feature order and keeps the row order unchanged.
- Pass `random_state` when you need deterministic shuffling for docs, tests, or reproducible data splits.


### `rfloordiv`

#### API Signature

```cpp
DataFrame rfloordiv(self, scalar other) const
```

#### Parameters

`other` : `scalar`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `rmod`

#### API Signature

```cpp
DataFrame rmod(self, scalar other) const
```

#### Parameters

`other` : `scalar`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
id  sales  city
1   0.0    Accra
2   0.0    Tema
3   0.0    Kumasi
```


### `rmul`

#### API Signature

```cpp
DataFrame rmul(self, scalar other) const
```

#### Parameters

`other` : `scalar`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
id  sales  city
10  1000.0  Accra
20  1200.0  Tema
30   900.0  Kumasi
```


### `rolling`

#### API Signature

```cpp
DataFrame rolling(self, pluss::msharp::RangeView::Of<WindowSpec> specs, bool keep_input = true) const
```

#### Parameters

`specs` : `pluss::msharp::RangeView::Of<WindowSpec>`
    See the signature for the supported argument form.

`keep_input` : `bool`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `round_time`

#### API Signature

```cpp
DataFrame round_time(self, pluss::msharp::RangeView::Of<std::string_view> features, std::string_view unit) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`unit` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
ts
2024-01-01T09:00:00Z
2024-01-01T10:00:00Z
```


### `rpow`

#### API Signature

```cpp
DataFrame rpow(self, scalar other) const
```

#### Parameters

`other` : `scalar`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `rsub`

#### API Signature

```cpp
DataFrame rsub(self, scalar other) const
```

#### Parameters

`other` : `scalar`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `rtruediv`

#### API Signature

```cpp
DataFrame rtruediv(self, scalar other) const
```

#### Parameters

`other` : `scalar`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


## S

### `sample`

#### API Signature

```cpp
DataFrame sample(self, std::int64_t n, std::optional<std::uint64_t> random_state = std::nullopt) const
```

#### Parameters

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

`random_state` : `std::optional<std::uint64_t>`
    Optional seed used to make sampling or shuffling reproducible.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `save_as`

#### API Signatures

```cpp
void save_as(self, std::string_view target_type, std::string_view filepath, const pluss::loader::Config& config = {}) const

void save_as(self, std::string_view target_type, std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) const
```

#### Parameters

`target_type` : `std::string_view`
    See the signature for the supported argument form.

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

`config_entries` : `std::initializer_list<pluss::loader::ConfigEntry>`
    Inline configuration entries used to build a lightweight loader configuration without naming a separate config object.

#### Returns

`void`
    Performs the side effect and returns no value.

#### What It Does

Writes the frame using the explicitly supplied output format selector rather than a dedicated saver overload.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_as("json", "path/to/output.json");
```

```text
saved: out.*
```


### `save_csv`

#### API Signatures

```cpp
void save_csv(self, std::string_view filepath) const

void save_csv(self, std::string_view filepath, bool add_cases, bool header = true, char sep = ',', std::string na_rep = "", std::optional<std::string> case_label = std::nullopt, std::optional<std::string> float_format = std::nullopt, char quotechar = '"', bool scalarquote = true, std::string line_terminator = "\n", char decimal = '.', std::string encoding = "utf-8") const

void save_csv(self, std::string_view filepath, const pluss::loader::Config& config) const

void save_csv(self, std::string_view filepath, const pluss::loader::WriteCsvOptions& options) const

void save_csv(self, std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) const
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

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

`options` : `const pluss::loader::WriteCsvOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

`config_entries` : `std::initializer_list<pluss::loader::ConfigEntry>`
    Inline configuration entries used to build a lightweight loader configuration without naming a separate config object.

#### Returns

`void`
    Performs the side effect and returns no value.

#### What It Does

Writes the frame to CSV. Set `add_cases=true` when you want the current case axis exported as a regular column so row labels survive the round trip.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_csv("path/to/output.csv");
```

```text
saved: out.*
```


### `save_excel`

#### API Signatures

```cpp
void save_excel(self, std::string_view filepath) const

void save_excel(self, std::string_view filepath, const pluss::loader::Config& config) const

void save_excel(self, std::string_view filepath, const pluss::loader::WriteExcelOptions& options) const

void save_excel(self, std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) const

void save_excel(self, std::string_view filepath, std::string sheet_name, bool add_cases = false, bool header = true, std::string na_rep = "", std::optional<std::string> case_label = std::nullopt) const
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

`options` : `const pluss::loader::WriteExcelOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

`config_entries` : `std::initializer_list<pluss::loader::ConfigEntry>`
    Inline configuration entries used to build a lightweight loader configuration without naming a separate config object.

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

#### What It Does

Writes the frame to an Excel sheet. Set `add_cases=true` when you want the current case axis exported as a regular column.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_excel("path/to/output.xlsx");
```

```text
saved: out.*
```


### `save_json`

#### API Signatures

```cpp
void save_json(self, std::string_view filepath) const

void save_json(self, std::string_view filepath, const pluss::loader::Config& config) const

void save_json(self, std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) const
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

`config_entries` : `std::initializer_list<pluss::loader::ConfigEntry>`
    Inline configuration entries used to build a lightweight loader configuration without naming a separate config object.

#### Returns

`void`
    Performs the side effect and returns no value.

#### What It Does

Writes the frame to JSON using the table export route for later reload or interchange.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_json("path/to/output.json");
```

```text
saved: out.*
```


### `save_parquet`

#### API Signatures

```cpp
void save_parquet(self, std::string_view filepath) const

void save_parquet(self, std::string_view filepath, const pluss::loader::Config& config) const

void save_parquet(self, std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) const
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

`config_entries` : `std::initializer_list<pluss::loader::ConfigEntry>`
    Inline configuration entries used to build a lightweight loader configuration without naming a separate config object.

#### Returns

`void`
    Performs the side effect and returns no value.

#### What It Does

Writes the frame to Parquet for compact typed storage.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_parquet("path/to/output.parquet");
```

```text
saved: out.*
```


### `save_txt`

#### API Signatures

```cpp
void save_txt(self, std::string_view filepath) const

void save_txt(self, std::string_view filepath, const pluss::loader::Config& config) const

void save_txt(self, std::string_view filepath, std::initializer_list<pluss::loader::ConfigEntry> config_entries) const
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

`config_entries` : `std::initializer_list<pluss::loader::ConfigEntry>`
    Inline configuration entries used to build a lightweight loader configuration without naming a separate config object.

#### Returns

`void`
    Performs the side effect and returns no value.

#### What It Does

Writes the frame to a plain-text table preview suitable for inspection or lightweight logging.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_txt("path/to/output.txt");
```

```text
saved: out.*
```


### `select`

#### API Signature

```cpp
DataFrame select(self, pluss::msharp::RangeView::Of<std::string_view> names) const
```

#### Parameters

`names` : `pluss::msharp::RangeView::Of<std::string_view>`
    Collection of feature, slot, or stream names supplied in positional order.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

Use this to reduce the current object to the rows or fields that matter for the next step.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.select();
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `select_like`

#### API Signature

```cpp
DataFrame select_like(self, std::string_view needle, bool case_sensitive = true) const
```

#### Parameters

`needle` : `std::string_view`
    See the signature for the supported argument form.

`case_sensitive` : `bool`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

Applies `select_like` to the current `DataFrame` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.select_like();
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `select_regex`

#### API Signature

```cpp
DataFrame select_regex(self, std::string_view pattern) const
```

#### Parameters

`pattern` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

Applies `select_regex` to the current `DataFrame` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.select_regex();
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `series_features`

#### API Signature

```cpp
DataFrame series_features(self) const
```

#### Parameters

`None`

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `set`

#### API Signatures

```cpp
LetSlot set(self, std::string_view name)

LetSlots set(self, pluss::msharp::RangeView::Of<std::string_view> names)

LetSlots set(self, std::initializer_list<const char*> names)

LetSlots set(self, std::initializer_list<std::string_view> names)
```

#### Parameters

`name` : `std::string_view`
    See the signature for the supported argument form.

`names` : `pluss::msharp::RangeView::Of<std::string_view>`
    Collection of feature, slot, or stream names supplied in positional order.

#### Returns

`LetSlot`
    Value produced by this call.

`LetSlots`
    Value produced by this call.

#### What It Does

Opens an assignment helper so you can write `df.set(...) = ...` and replace or create features declaratively.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.set("profit") = pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0}));
```
Example 2.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.set({"profit", "cost"}) = {pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0})), pluss::table::Thread(pluss::make_series({60.0, 85.0, 60.0}))};
```

### `set_axis`

#### API Signature

```cpp
DataFrame set_axis(self, pluss::msharp::RangeView::Of<std::string_view> feature_names) const
```

#### Parameters

`feature_names` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) supplied explicitly for subset, ordering, or axis-replacement operations, depending on the method.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
revenue  location  id_new
100.0    Accra     1
120.0    Tema      2
90.0     Kumasi    3
```


### `set_cases`

#### API Signature

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

#### What It Does

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

#### API Signature

```cpp
DataFrame set_feature_axis(self, pluss::msharp::RangeView::Of<std::string_view> feature_names) const
```

#### Parameters

`feature_names` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) supplied explicitly for subset, ordering, or axis-replacement operations, depending on the method.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
revenue  location  id_new
100.0    Accra     1
120.0    Tema      2
90.0     Kumasi    3
```


### `set_features`

#### API Signatures

```cpp
DataFrame set_features(self, pluss::msharp::RangeView::Of<std::string_view> names, pluss::msharp::RangeView::Of<Thread> values) const

DataFrame set_features(self, std::string_view name, const Thread& value) const
```

#### Parameters

`names` : `pluss::msharp::RangeView::Of<std::string_view>`
    Collection of feature, slot, or stream names supplied in positional order.

`values` : `pluss::msharp::RangeView::Of<Thread>`
    See the signature for the supported argument form.

`name` : `std::string_view`
    See the signature for the supported argument form.

`value` : `const Thread&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

Returns a new frame with one added or replaced feature.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.set_features("profit", pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0})));
```

```text
id  sales  city    profit
1   100.0  Accra   40.0
2   120.0  Tema    35.0
3    90.0  Kumasi  30.0
```


### `set_features_`

#### API Signatures

```cpp
DataFrame& set_features_(self, pluss::msharp::RangeView::Of<std::string_view> names, pluss::msharp::RangeView::Of<Thread> values)

DataFrame& set_features_(self, std::string_view name, const Thread& value)
```

#### Parameters

`names` : `pluss::msharp::RangeView::Of<std::string_view>`
    Collection of feature, slot, or stream names supplied in positional order.

`values` : `pluss::msharp::RangeView::Of<Thread>`
    See the signature for the supported argument form.

`name` : `std::string_view`
    See the signature for the supported argument form.

`value` : `const Thread&`
    See the signature for the supported argument form.

#### Returns

`DataFrame&`
    Value produced by this call.

#### What It Does

Adds or replaces one feature in place.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.set_features_("profit", pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0})));
```

```text
id  sales  city    profit
1   100.0  Accra   40.0
2   120.0  Tema    35.0
3    90.0  Kumasi  30.0
```


### `shift`

#### API Signatures

```cpp
DataFrame shift(self, pluss::msharp::RangeView::Of<std::string_view> features, std::int64_t periods = 1) const

DataFrame shift(self, std::int64_t periods = 1) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`periods` : `std::int64_t`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

Applies `shift` to the current `DataFrame` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.shift();
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `shift_time`

#### API Signature

```cpp
DataFrame shift_time(self, pluss::msharp::RangeView::Of<std::string_view> features, int periods, std::string_view freq) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`periods` : `int`
    See the signature for the supported argument form.

`freq` : `std::string_view`
    Bin or calendar frequency such as `"1h"`, `"D"`, `"W"`, or `"M"`.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
ts
2024-01-01T09:00:00Z
2024-01-01T10:00:00Z
```


### `show`

#### API Signatures

```cpp
void show(self, const display::DisplayOptions& options = {}) const

void show(self, std::ostream& os, const display::DisplayOptions& options = {}) const
```

#### Parameters

`options` : `const display::DisplayOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

`os` : `std::ostream&`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### What It Does

This renders a human-readable view for inspection instead of creating a new analytic object.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.show();
```

### `slice`

#### API Signature

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

#### What It Does

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

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `sort_cases`

#### API Signature

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

#### What It Does

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

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `sort_values`

#### API Signatures

```cpp
DataFrame sort_values(self, pluss::msharp::RangeView::Of<ArrangeSpec> specs) const

DataFrame sort_values(self, pluss::msharp::RangeView::Of<std::string_view> features, bool ascending = true, bool nulls_last = true) const
```

#### Parameters

`specs` : `pluss::msharp::RangeView::Of<ArrangeSpec>`
    See the signature for the supported argument form.

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`ascending` : `bool`
    See the signature for the supported argument form.

`nulls_last` : `bool`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

Reorders rows using one or more feature values as sort keys.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.sort_values({{.feature = "sales", .ascending = false}});
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

Example 2.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.sort_values({"sales"}, false, true);
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `sql`

#### API Signatures

```cpp
DataFrameSqlAccessor sql(self) const

DataFrameSqlAccessor sql(self, SqlOptions options) const
```

#### Parameters

`options` : `SqlOptions`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`DataFrameSqlAccessor`
    Value produced by this call.

#### What It Does

Use this when the transformation is easier to express as SQL than as a chained sequence of table verbs.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.sql().execute("SELECT id, sales FROM self WHERE sales > 95");
```

```text
SQL accessor / prepared statement created
```


### `std`

#### API Signatures

```cpp
DataFrameView std(self, bool render = true) const

Thread std(self, int axis, bool skipna = true) const

Thread std(self, std::int64_t axis, bool skipna = true) const
```

#### Parameters

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

`axis` : `int`
    Axis selector. On `DataFrame`, `0` usually means rows/cases and `1` means features/columns.

`skipna` : `bool`
    When `true`, missing values are skipped during the reduction. When `false`, missing state can affect the result.

#### Returns

`DataFrameView`
    Value produced by this call.

`Thread`
    Value produced by this call.

#### What It Does

Computes per-feature standard deviations for the selected numeric features.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.std(false);
```

```text
feature  value
id       1.0
sales    15.275252317
```

#### Practical Behavior

- `std(...)` reduces the selected numeric features to one summary row.
- Non-numeric features are skipped rather than coerced.
- The output stays in DataFrame form, which makes these reducers easy to reuse in downstream frame pipelines.


### `stitch_on`

#### API Signatures

```cpp
DataFrame stitch_on(self, pluss::msharp::RangeView::Of<std::string_view> left_features, const DataFrame& right, pluss::msharp::RangeView::Of<std::string_view> right_features = {}, std::string right_suffix = "_right") const

DataFrame stitch_on(self, std::initializer_list<std::string_view> left_features, const DataFrame& right, std::initializer_list<std::string_view> right_features = {}, std::string right_suffix = "_right") const

DataFrame stitch_on(self, std::string_view left_feature, const DataFrame& right, std::string_view right_feature = "", std::string right_suffix = "_right") const
```

#### Parameters

`left_features` : `pluss::msharp::RangeView::Of<std::string_view>`
    See the signature for the supported argument form.

`right` : `const DataFrame&`
    When `true`, intervals are right-closed. When `false`, they are left-closed and right-open.

`right_features` : `pluss::msharp::RangeView::Of<std::string_view>`
    See the signature for the supported argument form.

`right_suffix` : `std::string`
    Suffix added to right-hand feature names only when a non-key right feature would otherwise collide with an existing left feature name.

`left_feature` : `std::string_view`
    See the signature for the supported argument form.

`right_feature` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

Matches rows from two frames using key features and appends the matched features from the right-hand frame.

#### Examples

Example 1.

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

```text
id  sales  cost
1   100.0  60.0
2   120.0  80.0
3    90.0  55.0
```

Example 2.

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

```text
id  sales  cost
1   100.0  60.0
2   120.0  80.0
3    90.0  55.0
```


### `sub`

#### API Signatures

```cpp
DataFrame sub(self, const DataFrame& other) const

DataFrame sub(self, scalar other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

This applies arithmetic across the compatible numeric payloads in the current object.

#### Examples

Example 1.

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({2, 2, 4}))},
    {"sales", pluss::table::Thread(pluss::make_series({110.0, 110.0, 100.0}))},
});

auto out = left.sub(right);
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

Example 2.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.sub(10);
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `subtract`

#### API Signatures

```cpp
DataFrame subtract(self, const DataFrame& other) const

DataFrame subtract(self, scalar other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

This applies arithmetic across the compatible numeric payloads in the current object.

#### Examples

Example 1.

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({2, 2, 4}))},
    {"sales", pluss::table::Thread(pluss::make_series({110.0, 110.0, 100.0}))},
});

auto out = left.subtract(right);
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

Example 2.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.subtract(10);
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `sum`

#### API Signatures

```cpp
DataFrameView sum(self, bool render = true) const

Thread sum(self, int axis, bool skipna = true) const

Thread sum(self, std::int64_t axis, bool skipna = true) const
```

#### Parameters

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

`axis` : `int`
    Axis selector. On `DataFrame`, `0` usually means rows/cases and `1` means features/columns.

`skipna` : `bool`
    When `true`, missing values are skipped during the reduction. When `false`, missing state can affect the result.

#### Returns

`DataFrameView`
    Value produced by this call.

`Thread`
    Value produced by this call.

#### What It Does

Computes per-feature sums for the selected numeric features.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.sum(false);
```

```text
feature  value
id       6
sales    310.0
```

#### Practical Behavior

- `sum(...)` reduces the selected numeric features to one summary row.
- Non-numeric features are skipped rather than coerced.
- The output stays in DataFrame form, which makes these reducers easy to reuse in downstream frame pipelines.


## T

### `tabelize`

#### API Signatures

```cpp
static DataFrame tabelize(const pluss::kindle::Catalog& catalog)

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

#### What It Does

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Examples

Example 1.

```cpp
using namespace pluss::kindle::literals;
auto catalog = pluss::kindle::Catalog({"sales"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}))});
auto out = pluss::table::DataFrame::tabelize(catalog);
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `tail`

#### API Signature

```cpp
DataFrameView tail(self, std::int64_t n = 5, bool render = true) const
```

#### Parameters

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

#### Returns

`DataFrameView`
    Value produced by this call.

#### What It Does

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

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `take`

#### API Signature

```cpp
DataFrame take(self, pluss::msharp::RangeView::Int indices) const
```

#### Parameters

`indices` : `pluss::msharp::RangeView::Int`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
id  sales  city
1   100.0  Accra
3    90.0  Kumasi
```


### `tally`

#### API Signatures

```cpp
DataFrameView tally(self, pluss::msharp::RangeView::Of<std::string_view> features, std::int64_t top_n = 0, bool dropna = true, bool render = true) const

DataFrameView tally(self, std::int64_t top_n = 0, bool dropna = true, bool render = true) const

DataFrameView tally(self, std::string_view feature, std::int64_t top_n = 0, bool dropna = true, bool render = true) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`top_n` : `std::int64_t`
    See the signature for the supported argument form.

`dropna` : `bool`
    See the signature for the supported argument form.

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

`feature` : `std::string_view`
    Name of the source feature used by the operation.

#### Returns

`DataFrameView`
    Value produced by this call.

#### What It Does

Counts how many rows fall into each distinct value of the grouping feature.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema"}))},
    {"product", pluss::table::Thread(pluss::make_note({"A", "B", "A", "B"}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0, 150.0}))},
});

auto out = df.tally("city", 3, true, false);
```

```text
city    count
Accra   2
Tema    2
```


### `timeline_features`

#### API Signature

```cpp
DataFrame timeline_features(self) const
```

#### Parameters

`None`

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `title`

#### API Signatures

```cpp
DataFrame title(self, std::string title) const

std::string title(self) const
```

#### Parameters

`title` : `std::string`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

`std::string`
    Rendered string result.

#### What It Does

Applies `title` to the current `DataFrame` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.title();
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `title_`

#### API Signature

```cpp
DataFrame& title_(self, std::string title)
```

#### Parameters

`title` : `std::string`
    See the signature for the supported argument form.

#### Returns

`DataFrame&`
    Value produced by this call.

#### What It Does

Applies `title_` to the current `DataFrame` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.title_();
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `to_string`

#### API Signature

```cpp
std::string to_string(self, const display::DisplayOptions& options = {}) const
```

#### Parameters

`options` : `const display::DisplayOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`std::string`
    Rendered string result.

#### What It Does

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

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `to_utc`

#### API Signature

```cpp
DataFrame to_utc(self, pluss::msharp::RangeView::Of<std::string_view> features) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
ts
2024-01-01T09:00:00Z
2024-01-01T10:00:00Z
```


### `top`

#### API Signature

```cpp
DataFrame top(self, std::string_view feature, std::int64_t n) const
```

#### Parameters

`feature` : `std::string_view`
    Name of the source feature used by the operation.

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `transform`

#### API Signatures

```cpp
DataFrame transform(self, const std::function<Thread(const Thread&)>& fn) const

DataFrame transform(self, pluss::msharp::RangeView::Of<TransformSpec> specs) const

DataFrame transform(self, std::string_view feature, const std::function<Thread(const DataFrame&)>& fn) const

DataFrame transform(self, std::string_view feature, const std::function<Thread(const Thread&)>& fn) const
```

#### Parameters

`fn` : `const std::function<Thread(const Thread&)>&`
    See the signature for the supported argument form.

`specs` : `pluss::msharp::RangeView::Of<TransformSpec>`
    See the signature for the supported argument form.

`feature` : `std::string_view`
    Name of the source feature used by the operation.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

Rewrites one feature, several features, or all features by applying a user function to each targeted thread.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.transform([](const pluss::table::Thread& thread) { return thread.kind() == pluss::table::ThreadKind::Series ? thread.clip(0.0, 110.0) : thread; });
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

Example 2.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.transform({{.feature = "sales", .expr = [](const pluss::table::Thread& thread) { return thread.clip(0.0, 110.0); }}});
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

Example 3.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.transform("sales", [](const pluss::table::Thread& thread) { return thread.clip(0.0, 110.0); });
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `transpose`

#### API Signature

```cpp
DataFrameView transpose(self, bool render = false) const
```

#### Parameters

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

#### Returns

`DataFrameView`
    Value produced by this call.

#### What It Does

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

```text
feature  row0   row1   row2
id       1      2      3
sales    100.0  120.0  90.0
```


### `truediv`

#### API Signatures

```cpp
DataFrame truediv(self, const DataFrame& other) const

DataFrame truediv(self, scalar other) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

This applies arithmetic across the compatible numeric payloads in the current object.

#### Examples

Example 1.

```cpp
auto left = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
});
auto right = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({2, 2, 4}))},
    {"sales", pluss::table::Thread(pluss::make_series({110.0, 110.0, 100.0}))},
});

auto out = left.truediv(right);
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

Example 2.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.truediv(10);
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `ts`

#### API Signatures

```cpp
DataFrameTsAccessor ts(self, pluss::msharp::RangeView::Of<std::string_view> features) const

DataFrameTsAccessor ts(self, std::initializer_list<std::string_view> features) const

DataFrameTsAccessor ts(self, std::string_view feature) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`feature` : `std::string_view`
    Name of the source feature used by the operation.

#### Returns

`DataFrameTsAccessor`
    Value produced by this call.

#### What It Does

Applies `ts` to the current `DataFrame` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.ts();
```

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `tz_convert`

#### API Signature

```cpp
DataFrame tz_convert(self, pluss::msharp::RangeView::Of<std::string_view> features, std::string_view tz_name) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`tz_name` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
ts
2024-01-01T09:00:00Z
2024-01-01T10:00:00Z
```


### `tz_localize`

#### API Signature

```cpp
DataFrame tz_localize(self, pluss::msharp::RangeView::Of<std::string_view> features, std::string_view tz_name, int fold = 0, std::string_view nonexistent = "raise") const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`tz_name` : `std::string_view`
    See the signature for the supported argument form.

`fold` : `int`
    See the signature for the supported argument form.

`nonexistent` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
ts
2024-01-01T09:00:00Z
2024-01-01T10:00:00Z
```


## U

### `unify`

#### API Signatures

```cpp
DataFrame unify(self, pluss::msharp::RangeView::Of<std::string_view> left_on, const DataFrame& right, pluss::msharp::RangeView::Of<std::string_view> right_on = {}, std::string_view how = "inner", std::string right_suffix = "_right", std::string_view validate = "many_to_many") const

DataFrame unify(self, std::initializer_list<std::string_view> left_on, const DataFrame& right, std::initializer_list<std::string_view> right_on = {}, std::string_view how = "inner", std::string right_suffix = "_right", std::string_view validate = "many_to_many") const

DataFrame unify(self, std::string_view left_on, const DataFrame& right, std::string_view right_on = "", std::string_view how = "inner", std::string right_suffix = "_right", std::string_view validate = "many_to_many") const
```

#### Parameters

`left_on` : `pluss::msharp::RangeView::Of<std::string_view>`
    Key feature name(s) on the left frame used to match rows during `unify(...)`.

`right` : `const DataFrame&`
    When `true`, intervals are right-closed. When `false`, they are left-closed and right-open.

`right_on` : `pluss::msharp::RangeView::Of<std::string_view>`
    Key feature name(s) on the right frame used to match rows during `unify(...)`. Leave empty to reuse the same names as `left_on`.

`how` : `std::string_view`
    Join mode. `DataFrame::unify(...)` currently supports `"inner"` and `"left"`.

`right_suffix` : `std::string`
    Suffix added to right-hand feature names only when a non-key right feature would otherwise collide with an existing left feature name.

`validate` : `std::string_view`
    Optional key-cardinality check for `unify(...)`: `"many_to_many"` (default), `"one_to_one"`, `"one_to_many"`, or `"many_to_one"`.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

Joins two frames by key values. Use `right_on=""` when both frames use the same key name, `how="left"` to preserve left rows, and `validate` when you want duplicate-key checks such as `"many_to_one"`.

#### Examples

Example 1.

```cpp
auto left = pluss::table::DataFrame({
    {"OrgDefinedId", pluss::make_note({"10", "20", "30"})},
    {"midterm", pluss::make_series({71, 82, 93})},
});
auto right = pluss::table::DataFrame({
    {"OrgDefinedId", pluss::make_note({"20", "30"})},
    {"Final Points Grade", pluss::make_series({88, 99})},
});

auto out = left.unify(
    "OrgDefinedId",
    right,
    "",
    "left",
    "_right",
    "many_to_one"
);
```

```text
OrgDefinedId  midterm  Final Points Grade
10            71       null
20            82       88
30            93       99
```

Example 2.

```cpp
auto left = pluss::table::DataFrame({
    {"course", pluss::make_note({"STAT200", "STAT200", "STAT300"})},
    {"OrgDefinedId", pluss::make_note({"10", "20", "30"})},
    {"midterm", pluss::make_series({71, 82, 93})},
});
auto right = pluss::table::DataFrame({
    {"course_code", pluss::make_note({"STAT200", "STAT300"})},
    {"Username", pluss::make_note({"20", "30"})},
    {"Final Points Grade", pluss::make_series({88, 99})},
});

auto out = left.unify(
    {"course", "OrgDefinedId"},
    right,
    {"course_code", "Username"},
    "inner",
    "_right",
    "one_to_one"
);
```

```text
OrgDefinedId  midterm  Final Points Grade
10            71       null
20            82       88
30            93       99
```


### `unique`

#### API Signature

```cpp
bool unique(self) const
```

#### Parameters

`None`

#### Returns

`bool`
    Boolean status or membership result.

#### What It Does

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

```text
true
```


### `unique_cases`

#### API Signature

```cpp
DataFrame unique_cases(self, pluss::msharp::RangeView::Of<std::string_view> features = {}) const
```

#### Parameters

`features` : `pluss::msharp::RangeView::Of<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `unsqueeze`

#### API Signatures

```cpp
Atlas unsqueeze(self) const

Atlas unsqueeze(self, const Thread& depth_labels) const
```

#### Parameters

`depth_labels` : `const Thread&`
    See the signature for the supported argument form.

#### Returns

`Atlas`
    Value produced by this call.

#### What It Does

Promotes the frame into a depth-1 `Atlas` so it can participate in multi-frame workflows.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.unsqueeze();
```

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

Example 2.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto labels = pluss::table::Thread(pluss::make_note({"train"}));
auto out = df.unsqueeze(labels);
```

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

#### API Signature

```cpp
DataFrame update(self, const DataFrame& other, bool overwrite = true) const
```

#### Parameters

`other` : `const DataFrame&`
    See the signature for the supported argument form.

`overwrite` : `bool`
    When `true`, matching values from the right-hand side overwrite existing left-hand values during update-style operations.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
id  sales  cost
1   100.0  60.0
2   120.0  80.0
3    90.0  55.0
```


## V

### `var`

#### API Signatures

```cpp
DataFrameView var(self, bool render = true) const

Thread var(self, int axis, bool skipna = true) const

Thread var(self, std::int64_t axis, bool skipna = true) const
```

#### Parameters

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

`axis` : `int`
    Axis selector. On `DataFrame`, `0` usually means rows/cases and `1` means features/columns.

`skipna` : `bool`
    When `true`, missing values are skipped during the reduction. When `false`, missing state can affect the result.

#### Returns

`DataFrameView`
    Value produced by this call.

`Thread`
    Value produced by this call.

#### What It Does

Computes per-feature variances for the selected numeric features.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.var(false);
```

```text
feature  value
id       1.0
sales    233.333333333
```

#### Practical Behavior

- `var(...)` reduces the selected numeric features to one summary row.
- Non-numeric features are skipped rather than coerced.
- The output stays in DataFrame form, which makes these reducers easy to reuse in downstream frame pipelines.


### `view`

#### API Signature

```cpp
DataFrameView view(self, std::int64_t start, std::int64_t stop, bool render = true) const
```

#### Parameters

`start` : `std::int64_t`
    Inclusive starting position of the slice.

`stop` : `std::int64_t`
    Exclusive ending position of the slice.

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

#### Returns

`DataFrameView`
    Value produced by this call.

#### What It Does

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

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


## W

### `where`

#### API Signatures

```cpp
DataFrame where(self, const pluss::Series& bool_mask) const

DataFrame where(self, const Thread& bool_mask) const
```

#### Parameters

`bool_mask` : `const pluss::Series&`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

Keeps only the rows where the supplied boolean mask is true.

#### Examples

Example 1.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = pluss::make_series({true, false, true}, pluss::codem::ScalarType::Bool);
auto out = df.where(mask);
```

```text
id  sales  city
1   100.0  Accra
3    90.0  Kumasi
```


### `window`

#### API Signature

```cpp
DataFrame window(self, pluss::msharp::RangeView::Of<WindowSpec> specs, bool keep_input = true) const
```

#### Parameters

`specs` : `pluss::msharp::RangeView::Of<WindowSpec>`
    See the signature for the supported argument form.

`keep_input` : `bool`
    See the signature for the supported argument form.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```


### `with_cases`

#### API Signature

```cpp
DataFrame with_cases(self, const Thread& cases) const
```

#### Parameters

`cases` : `const Thread&`
    Case-label thread to attach to the current object.

#### Returns

`DataFrame`
    Value produced by this call.

#### What It Does

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

#### API Signature

```cpp
DataFrame& with_cases_(self, const Thread& cases)
```

#### Parameters

`cases` : `const Thread&`
    Case-label thread to attach to the current object.

#### Returns

`DataFrame&`
    Value produced by this call.

#### What It Does

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

```text
df.cases():
["cs0", "cs1", "cs2"]

df.head():
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

