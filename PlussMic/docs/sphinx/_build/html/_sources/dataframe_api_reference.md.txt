# DataFrame API Reference

# Practical DataFrame API Reference

## API Reference Style

This page documents the full public `DataFrame` surface with usage-first examples generated from the current table API.

## A

### `add`

#### API Signatures

```cpp
DataFrame::add(other: const DataFrame&) -> DataFrame

DataFrame::add(other: double) -> DataFrame
```

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### Inputs

`other` : `const DataFrame&`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Add each compatible numeric cell against another aligned frame.

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

##### Expected output

```text
id  sales
3   210.0
4   230.0
7   190.0
```
Example 2. Add each compatible numeric cell against one broadcast scalar value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.add(10);
```

##### Expected output

```text
id  sales  city
11  110.0  Accra
12  130.0  Tema
13  100.0  Kumasi
```

### `add_prefix`

#### API Signature

```cpp
DataFrame::add_prefix(prefix: std::string_view) -> DataFrame
```

#### Notes

Prepends the supplied text to every feature name. Row values, row order, and case labels stay unchanged.

#### Inputs

`prefix` : `std::string_view`
    Text prefix inserted ahead of the existing label or value.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.add_prefix("feat_");
```

##### Expected output

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
DataFrame::add_suffix(suffix: std::string_view) -> DataFrame
```

#### Notes

Appends the supplied text to every feature name. Row values, row order, and case labels stay unchanged.

#### Inputs

`suffix` : `std::string_view`
    Text suffix appended after the existing label or value.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.add_suffix("_2024");
```

##### Expected output

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
DataFrame::aggregate(specs: pluss::RangeView<SummarySpec>, render: bool = false) -> DataFrameView
```

#### Notes

Builds a compact summary frame from one or more aggregation specs, so you can inspect computed metrics instead of the full row set.

#### Inputs

`specs` : `pluss::RangeView<SummarySpec>`
    Aggregation specs. Each `SummarySpec` has `output`, `feature`, and `op`.

    `op` supports:
    - `Count`: Count observed values in the selected feature.
    - `Nunique`: Count distinct observed values.
    - `Sum`: Compute the numeric sum.
    - `Mean`: Compute the arithmetic mean.
    - `Median`: Compute the median.
    - `Min`: Compute the minimum.
    - `Max`: Compute the maximum.
    - `Std`: Compute the standard deviation.
    - `Var`: Compute the variance.
    - `Prod`: Compute the numeric product.

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

#### Output

`DataFrameView`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Choose whether the returned object should auto-render in display contexts.

```cpp
auto df = pluss::table::DataFrame({
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema"}))},
    {"product", pluss::table::Thread(pluss::make_note({"A", "B", "A", "B"}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0, 150.0}))},
});

auto out = df.aggregate({{"sales_mean", "sales", pluss::table::DataFrame::SummaryOp::Mean}}, false);
```

##### Expected output

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
DataFrame::align_cases(new_cases: const Thread&) -> DataFrame
```

#### Notes

Reorders rows so their case labels match the supplied `new_cases` order. Every requested case must already exist, and the output case axis is rewritten to that order.

#### Inputs

`new_cases` : `const Thread&`
    Replacement case labels in the exact order you want the rows to follow.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

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

##### Expected output

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
DataFrame::all(axis: Axis, features: pluss::RangeView<std::string_view> = {}, skipna: bool = true) -> Thread

DataFrame::all(features: pluss::RangeView<std::string_view> = {}, skipna: bool = true) -> Thread
```

#### Notes

Use this method through the public contract shown above.

#### Inputs

`axis` : `Axis`
    Axis selector. On `DataFrame`, `0` usually means rows/cases and `1` means features/columns.

`features` : `pluss::RangeView<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`skipna` : `bool`
    When `true`, missing values are skipped during the reduction. When `false`, missing state can affect the result.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Evaluate `all` down the selected boolean columns.

```cpp
auto df = pluss::table::DataFrame({
    {"passed_math", pluss::table::Thread(pluss::make_series({true, true, false}, pluss::ScalarType::Bool))},
    {"passed_lab", pluss::table::Thread(pluss::make_series({true, false, false}, pluss::ScalarType::Bool))},
});

auto out = df.all({"passed_math", "passed_lab"}, true);
```

##### Expected output

```text
The call completes and returns the contract described under Output.
```
Example 2. Evaluate `all` down the selected boolean columns.

```cpp
auto df = pluss::table::DataFrame({
    {"passed_math", pluss::table::Thread(pluss::make_series({true, true, false}, pluss::ScalarType::Bool))},
    {"passed_lab", pluss::table::Thread(pluss::make_series({true, false, false}, pluss::ScalarType::Bool))},
});

auto out = df.all({"passed_math", "passed_lab"}, true);
```

##### Expected output

```text
The call completes and returns the contract described under Output.
```

#### Axis Rules

- `axis=0` reduces down the selected boolean features and returns one result per feature.
- `axis=1` reduces across features and returns one result per row.
- These methods expect boolean features; they are not general numeric reducers.
- Nullable boolean input requires `skipna=true`.


### `any`

#### API Signatures

```cpp
DataFrame::any(axis: Axis, features: pluss::RangeView<std::string_view> = {}, skipna: bool = true) -> Thread

DataFrame::any(features: pluss::RangeView<std::string_view> = {}, skipna: bool = true) -> Thread
```

#### Notes

Use this method through the public contract shown above.

#### Inputs

`axis` : `Axis`
    Axis selector. On `DataFrame`, `0` usually means rows/cases and `1` means features/columns.

`features` : `pluss::RangeView<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`skipna` : `bool`
    When `true`, missing values are skipped during the reduction. When `false`, missing state can affect the result.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Evaluate `any` down the selected boolean columns.

```cpp
auto df = pluss::table::DataFrame({
    {"passed_math", pluss::table::Thread(pluss::make_series({true, true, false}, pluss::ScalarType::Bool))},
    {"passed_lab", pluss::table::Thread(pluss::make_series({true, false, false}, pluss::ScalarType::Bool))},
});

auto out = df.any({"passed_math", "passed_lab"}, true);
```

##### Expected output

```text
The call completes and returns the contract described under Output.
```
Example 2. Evaluate `any` down the selected boolean columns.

```cpp
auto df = pluss::table::DataFrame({
    {"passed_math", pluss::table::Thread(pluss::make_series({true, true, false}, pluss::ScalarType::Bool))},
    {"passed_lab", pluss::table::Thread(pluss::make_series({true, false, false}, pluss::ScalarType::Bool))},
});

auto out = df.any({"passed_math", "passed_lab"}, true);
```

##### Expected output

```text
The call completes and returns the contract described under Output.
```

#### Axis Rules

- `axis=0` reduces down the selected boolean features and returns one result per feature.
- `axis=1` reduces across features and returns one result per row.
- These methods expect boolean features; they are not general numeric reducers.
- Nullable boolean input requires `skipna=true`.


### `assign`

#### API Signatures

```cpp
DataFrame::assign(assignments: pluss::RangeView<std::pair<std::string, Thread>>) -> DataFrame

DataFrame::assign(specs: pluss::RangeView<DeriveSpec>) -> DataFrame
```

#### Notes

Adds or replaces derived features computed from the current frame. Use this when the new column depends on one or more existing columns.

#### Inputs

`assignments` : `pluss::RangeView<std::pair<std::string, Thread>>`
    Collection of `(feature_name, thread)` pairs used to add or replace several features in one call.

`specs` : `pluss::RangeView<DeriveSpec>`
    Collection of structured operation specs. Each element describes one requested summary, derive, transform, group, or placement rule depending on the method.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Attach one or more already-computed threads by name.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.assign({{"profit", pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0}))}});
```

##### Expected output

```text
id  sales  city    profit
1   100.0  Accra   40.0
2   120.0  Tema    35.0
3    90.0  Kumasi  30.0
```
Example 2. Derive one or more columns inside the call from the current frame state.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.assign({{.feature = "profit", .expr = [](const pluss::table::DataFrame& frame) { return pluss::table::Thread(frame.features("sales").as_series().sub(60)); }}});
```

##### Expected output

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
DataFrame::astype(feature_dtypes: pluss::RangeView<std::pair<std::string_view, pluss::ScalarType>>) -> DataFrame

DataFrame::astype(features: pluss::RangeView<std::string_view>, dtype: pluss::ScalarType) -> DataFrame
```

#### Notes

This changes the semantic or numeric type so later operations route through the correct implementation path.

#### Inputs

`feature_dtypes` : `pluss::RangeView<std::pair<std::string_view, pluss::ScalarType>>`
    Collection of `(feature_name, dtype)` pairs used to cast several existing features to explicit scalar dtypes in one call.

`features` : `pluss::RangeView<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`dtype` : `pluss::ScalarType`
    Target scalar dtype used by this conversion overload.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Convert several selected features at once by supplying a feature-to-dtype map.

```cpp
auto df = pluss::table::DataFrame({{"sales", pluss::table::Thread(pluss::make_note({"100.0", "120.0", "90.0"}))}});

auto out = df.astype({{"sales", pluss::ScalarType::Float64}});
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 2. Convert one or more selected features to a target numeric Series dtype.

```cpp
auto df = pluss::table::DataFrame({{"sales", pluss::table::Thread(pluss::make_note({"100.0", "120.0", "90.0"}))}});

auto out = df.astype({"sales"}, pluss::ScalarType::Float64);
```

##### Expected output

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


## B

### `bfill`

#### API Signature

```cpp
DataFrame::bfill() -> DataFrame
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`None`

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto score = pluss::table::Thread(pluss::make_series({71.0, 0.0, 88.0, 0.0}))
                 .with_validity_mask(pluss::make_series({true, false, true, false}, pluss::ScalarType::Bool));
auto df = pluss::table::DataFrame({
    {"score", score},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"}))},
});

auto out = df.bfill();
```

##### Expected output

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
DataFrame::bottom(feature: std::string_view, n: std::int64_t) -> DataFrame
```

#### Notes

Returns the rows with the smallest values in the selected feature.

#### Inputs

`feature` : `std::string_view`
    Name of the source feature used by the operation.

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

#### Output

`DataFrame`
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

auto out = df.bottom("sales", 2);
```

##### Expected output

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


### `bounded`

#### API Signature

```cpp
DataFrame::bounded(bounds: const WindowBoundsFn&, statistic: WindowStatistic = WindowStatistic::Mean, options: const WindowOptions& = {}) -> DataFrame
```

#### Notes

Use this method through the public contract shown above.

#### Inputs

`bounds` : `const WindowBoundsFn&`
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

`statistic` : `WindowStatistic`
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

`options` : `const WindowOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Pass an explicit options object to control the operation.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.bounded();
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `bucketize`

#### API Signatures

```cpp
DataFrame::bucketize(bins: pluss::RangeView<double>, labels: pluss::RangeView<std::int64_t> = {}, right: bool = true, include_lowest: bool = true) -> DataFrame

DataFrame::bucketize(feature_name: std::string_view, bins: pluss::RangeView<double>, labels: pluss::RangeView<std::int64_t> = {}, right: bool = true, include_lowest: bool = true) -> DataFrame

DataFrame::bucketize(feature_names: pluss::RangeView<std::string_view>, bins: pluss::RangeView<double>, labels: pluss::RangeView<std::int64_t> = {}, right: bool = true, include_lowest: bool = true) -> DataFrame
```

#### Notes

Buckets numeric `Series` features into interval labels while preserving the rest of the frame unchanged.

#### Inputs

`bins` : `pluss::RangeView<scalar>`
    Strictly increasing interval edges used to define bucket boundaries.

`labels` : `pluss::RangeView<std::int64_t>`
    Optional integer labels assigned to the output buckets. Leave empty to let PlussMic use `0, 1, 2, ...`.

`right` : `bool`
    When `true`, intervals are right-closed. When `false`, they are left-closed and right-open.

`include_lowest` : `bool`
    When `true`, the first bucket also includes its left edge.

`feature_name` : `std::string_view`
    Name of the target feature to read, rewrite, sort by, or promote to cases.

`feature_names` : `pluss::RangeView<std::string_view>`
    Feature name(s) supplied explicitly for subset, ordering, or axis-replacement operations, depending on the method.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Bucketize every compatible numeric column in the frame.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

std::vector<double> bins{0.0, 2.0, 3.0, 4.0};
std::vector<std::int64_t> labels{1, 2, 3};
auto out = df.bucketize(bins, labels, true, true);
```

##### Expected output

```text
id  sales  city
1   2      Accra
2   3      Tema
3   1      Kumasi
```
Example 2. Bucketize one numeric column into interval labels.

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

##### Expected output

```text
id  sales  city
1   2      Accra
2   3      Tema
3   1      Kumasi
```
Example 3. Bucketize several selected numeric columns at once.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

std::vector<double> bins{0.0, 95.0, 110.0, 130.0};
std::vector<std::int64_t> labels{1, 2, 3};
auto out = df.bucketize({"id", "sales"}, bins, labels, true, true);
```

##### Expected output

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

### `case_study`

#### Signatures

```cpp
DataFrame::case_study(pick: const CasePick&) -> DataFrame
DataFrame::case_study(rows: const CaseSelector&) -> DataFrame
DataFrame::case_study(rows: const CaseSelector&, features: const FeatureSelector&) -> DataFrame
```

#### Parameters

`pick` : `const CasePick&`
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

`rows` : `const CaseSelector&`
    Selector describing which rows `case_study(...)` should return.

    Supported values:
    - ``pluss::table::pos(row)``: One row by zero-based position. Negative positions count from the end.
    - ``pluss::table::pos(start, stop)``: A half-open positional row slice `[start, stop)`.
    - ``pluss::table::pos({r1, r2, ...})``: An explicit row-position list in the exact output order you want.
    - ``pluss::table::label(value)``: Every row whose case label matches `value`.
    - ``pluss::table::labels({...})``: Several case labels in requested order. If one label appears multiple times, every matching row is returned.
    - ``pluss::table::label_range(start, stop)``: An inclusive case-label window from the first `start` match through the last `stop` match.
    - ``pluss::table::mask(bool_thread)``: A boolean row mask, equivalent to filtering first and then projecting columns.

`features` : `const FeatureSelector&`
    Selector describing which feature columns `case_study(...)` should keep after resolving the row selection.

    Supported values:
    - ``pluss::table::all_features()``: Keep every feature in the same left-to-right order as the source frame.
    - ``pluss::table::col("name")``: Keep one named feature.
    - ``pluss::table::col(position)``: Keep one feature by zero-based position.
    - ``pluss::table::cols({"a", "b"})``: Keep several named features in the requested order.
    - ``pluss::table::cols({0, 2})``: Keep several feature positions in the requested order.
    - ``pluss::table::cols(start, stop)``: A half-open positional feature slice `[start, stop)`.
    - ``pluss::table::col_range("start", "stop")``: An inclusive feature-name window from `start` through `stop`.

#### Returns

`DataFrame`
    Always returns a `DataFrame`, even when the selection resolves to one row, one column, or one cell.

#### What It Does

`case_study(...)` is the unified DataFrame indexing surface. Use selector helpers to choose rows by position, by case label, or by boolean mask, then optionally project one or more features in the same call.

#### Selector Rules

- Positional row selectors use zero-based indexing. Negative positions count from the end.
- Positional row slices are half-open: `pos(start, stop)` means `[start, stop)`.
- Label selectors match against the current case axis returned by `cases()`.
- `label(value)` returns every matching row, so duplicate case labels are preserved instead of collapsed.
- `label_range(start, stop)` is inclusive on both ends and walks the current case-axis order.
- Feature-position slices are half-open, while `col_range(start, stop)` is inclusive on both ends.
- Because `case_study(...)` always returns a frame, you do not need separate `at(...)` or `iat(...)` methods for single-cell access.

#### Examples

Example 1. Study one row by zero-based position.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"})},
    {"region", pluss::make_note({"south", "south", "middle", "north"})},
    {"units", pluss::make_series({10.0, 7.0, 12.0, 5.0})},
    {"price", pluss::make_series({3.0, 4.0, 2.5, 5.0})},
}).title("sales");

auto labeled = sales.with_cases(
    pluss::make_note({"A100", "B200", "B200", "D400"}));

auto out = sales.case_study(pos(1));
```

```text
city  region  units  price
Tema  south   7.0    4.0
```

Example 2. Study a half-open positional row window.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"})},
    {"region", pluss::make_note({"south", "south", "middle", "north"})},
    {"units", pluss::make_series({10.0, 7.0, 12.0, 5.0})},
    {"price", pluss::make_series({3.0, 4.0, 2.5, 5.0})},
}).title("sales");

auto labeled = sales.with_cases(
    pluss::make_note({"A100", "B200", "B200", "D400"}));

auto out = sales.case_study(pos(1, 3));
```

```text
city    region  units  price
Tema    south   7.0    4.0
Kumasi  middle  12.0   2.5
```

Example 3. Reorder rows by explicit positions and keep only named features.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"})},
    {"region", pluss::make_note({"south", "south", "middle", "north"})},
    {"units", pluss::make_series({10.0, 7.0, 12.0, 5.0})},
    {"price", pluss::make_series({3.0, 4.0, 2.5, 5.0})},
}).title("sales");

auto labeled = sales.with_cases(
    pluss::make_note({"A100", "B200", "B200", "D400"}));

auto out = sales.case_study(pos({3, 0}), cols({"city", "price"}));
```

```text
city    price
Tamale  5.0
Accra   3.0
```

Example 4. Match every row whose case label equals one label.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"})},
    {"region", pluss::make_note({"south", "south", "middle", "north"})},
    {"units", pluss::make_series({10.0, 7.0, 12.0, 5.0})},
    {"price", pluss::make_series({3.0, 4.0, 2.5, 5.0})},
}).title("sales");

auto labeled = sales.with_cases(
    pluss::make_note({"A100", "B200", "B200", "D400"}));

auto out = labeled.case_study(label("B200"));
```

```text
city    region  units  price
Tema    south   7.0    4.0
Kumasi  middle  12.0   2.5
```

Example 5. Request several labels in your own order and project one feature subset.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"})},
    {"region", pluss::make_note({"south", "south", "middle", "north"})},
    {"units", pluss::make_series({10.0, 7.0, 12.0, 5.0})},
    {"price", pluss::make_series({3.0, 4.0, 2.5, 5.0})},
}).title("sales");

auto labeled = sales.with_cases(
    pluss::make_note({"A100", "B200", "B200", "D400"}));

auto out = labeled.case_study(labels({"D400", "A100"}), cols({"city", "units"}));
```

```text
city    units
Tamale  5.0
Accra   10.0
```

Example 6. Slice by case labels inclusively and slice features by name inclusively.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"})},
    {"region", pluss::make_note({"south", "south", "middle", "north"})},
    {"units", pluss::make_series({10.0, 7.0, 12.0, 5.0})},
    {"price", pluss::make_series({3.0, 4.0, 2.5, 5.0})},
}).title("sales");

auto labeled = sales.with_cases(
    pluss::make_note({"A100", "B200", "B200", "D400"}));

auto out = labeled.case_study(label_range("B200", "D400"), col_range("region", "price"));
```

```text
region  units  price
south   7.0    4.0
middle  12.0   2.5
north   5.0    5.0
```

Example 7. Use a boolean mask just like a filter, then keep only the columns you care about.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"})},
    {"region", pluss::make_note({"south", "south", "middle", "north"})},
    {"units", pluss::make_series({10.0, 7.0, 12.0, 5.0})},
    {"price", pluss::make_series({3.0, 4.0, 2.5, 5.0})},
}).title("sales");

auto labeled = sales.with_cases(
    pluss::make_note({"A100", "B200", "B200", "D400"}));

auto out = sales.case_study(mask(sales.features("units") >= 10.0), cols({"city", "units"}));
```

```text
city    units
Accra   10.0
Kumasi  12.0
```

Example 8. Get a one-cell result without a separate scalar-only indexing method.

```cpp
using namespace pluss::table;

auto sales = DataFrame({
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi", "Tamale"})},
    {"region", pluss::make_note({"south", "south", "middle", "north"})},
    {"units", pluss::make_series({10.0, 7.0, 12.0, 5.0})},
    {"price", pluss::make_series({3.0, 4.0, 2.5, 5.0})},
}).title("sales");

auto labeled = sales.with_cases(
    pluss::make_note({"A100", "B200", "B200", "D400"}));

auto out = labeled.case_study(label("A100"), col("price"));
```

```text
price
3.0
```

#### Behavior Notes

- `cases()` remains the accessor for the case axis itself. It no longer doubles as a row-selection API.
- When you only want to preview a row window instead of materializing a selection result for later work, use `observe(...)`.
- If you already know the final feature order and only need column projection, `features({...})` is still the lighter-weight feature-only path.


### `cases`

#### API Signature

```cpp
DataFrame::cases() -> const Thread&
```

#### Notes

Returns the current case axis itself. Use this when you want the row labels as data rather than selecting rows from the frame.

#### Inputs

`None`

#### Output

`const Thread&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Read the current case axis thread.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.cases();
```

##### Expected output

```text
The call completes and returns the contract described under Output.
```

#### Practical Behavior

- `cases()` returns the current case axis exactly as it is stored on the frame.
- The case axis can be numeric, note-based, timeline-based, or time-index based.
- Use `case_study(...)` when you want row selection; use `cases()` when you want the row labels themselves.
- Use `cases_series()` only when the case axis is actually Series-backed.


### `cases_series`

#### API Signature

```cpp
DataFrame::cases_series() -> pluss::Series
```

#### Notes

Returns the current case axis as a `Series`, which is useful when you need the row labels as data.

#### Inputs

`None`

#### Output

`pluss::Series`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.cases_series();
```

##### Expected output

```text
[0, 1, 2]
```

### `cast`

#### API Signatures

```cpp
DataFrame::cast(feature_dtypes: pluss::RangeView<std::pair<std::string_view, pluss::ScalarType>>) -> DataFrame

DataFrame::cast(specs: pluss::RangeView<CastSpec>) -> DataFrame
```

#### Notes

This changes the semantic or numeric type so later operations route through the correct implementation path.

#### Inputs

`feature_dtypes` : `pluss::RangeView<std::pair<std::string_view, pluss::ScalarType>>`
    Collection of `(feature_name, dtype)` pairs used to cast several existing features to explicit scalar dtypes in one call.

`specs` : `pluss::RangeView<CastSpec>`
    Collection of structured operation specs. Each element describes one requested summary, derive, transform, group, or placement rule depending on the method.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Cast selected features into another Pluss payload family.

```cpp
auto df = pluss::table::DataFrame({{"sales", pluss::table::Thread(pluss::make_note({"100.0", "120.0", "90.0"}))}});

auto out = df.astype({{"sales", pluss::ScalarType::Float64}});
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 2. Cast selected features with explicit target-family and dtype control.

```cpp
auto df = pluss::table::DataFrame({{"sales", pluss::table::Thread(pluss::make_note({"100.0", "120.0", "90.0"}))}});

auto out = df.cast({{.feature = "sales", .target = pluss::table::ThreadCastTarget::Series, .series_dtype = pluss::ScalarType::Float64}});
```

##### Expected output

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


### `catalogue`

#### API Signature

```cpp
DataFrame::catalogue(feature: std::string_view, ordered: bool = false, sort_levels: bool = false) -> CategoryEncoding
```

#### Notes

Builds a compact level-and-code encoding for a low-cardinality feature while preserving missingness.

#### Inputs

`feature` : `std::string_view`
    Feature whose distinct observed values should become compact levels.

`ordered` : `bool`
    Records whether level order is semantically meaningful.

`sort_levels` : `bool`
    When `true`, sort levels before assigning integer codes; otherwise preserve first-seen order.

#### Output

`CategoryEncoding`
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

auto out = df.catalogue();
```

##### Expected output

```text
The call completes and returns the contract described under Output.
```

#### Encoding Rules

- `catalogue(feature)` returns explicit `codes`, `levels`, and `ordered` fields instead of hiding codes inside an ordinary numeric feature.
- Missing source values keep an invalid mask and use the conventional payload code `-1`.
- Levels preserve first-seen order by default; `sort_levels=true` assigns codes from sorted levels.
- `encoding.decode()` reconstructs the typed Thread and its validity mask.


### `combine`

#### API Signatures

```cpp
DataFrame::combine(other: const DataFrame&) -> DataFrame

DataFrame::combine(other: const DataFrame&, spec: const CombineSpec&) -> DataFrame
```

#### Notes

Combines this frame with another frame according to the supplied combine policy, such as feature-wise update or join-like alignment.

#### Inputs

`other` : `const DataFrame&`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

`spec` : `const CombineSpec&`
    Combine policy object. Use it when you want explicit control over feature-wise append/update behavior instead of the default combine route.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Combine two aligned frames with the default feature-wise policy.

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

##### Expected output

```text
id  sales  cost
1   100.0  60.0
2   120.0  80.0
3    90.0  55.0
```
Example 2. Combine two aligned frames with an explicit combine-policy object.

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

##### Expected output

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
DataFrame::compare(other: const DataFrame&) -> DataFrame

DataFrame::compare(other: double) -> DataFrame
```

#### Notes

This produces a comparison-style result you can inspect directly or reuse as a mask.

#### Inputs

`other` : `const DataFrame&`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Run the not-equal comparison alias against another aligned frame.

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

##### Expected output

```text
id  sales
1   true
2   true
3   false
```
Example 2. Run the not-equal comparison alias against one broadcast scalar value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.compare(10);
```

##### Expected output

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
DataFrame::conform_to(other: const DataFrame&) -> DataFrame
```

#### Notes

Reorders and aligns this frame so it is compatible with another frame for downstream joins or arithmetic.

#### Inputs

`other` : `const DataFrame&`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

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

##### Expected output

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
DataFrame::copy() -> DataFrame
```

#### Notes

Returns a detached copy of the frame so later edits to the result do not affect the source object.

#### Inputs

`None`

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.copy();
```

##### Expected output

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
DataFrame::corr(features: pluss::RangeView<std::string_view>, method: CorrMethod, render: bool = false) -> DataFrameView

DataFrame::corr(features: pluss::RangeView<std::string_view>, render: bool = false) -> DataFrameView

DataFrame::corr(method: CorrMethod, render: bool = false) -> DataFrameView

DataFrame::corr(render: bool = false) -> DataFrameView
```

#### Notes

Use this method through the public contract shown above.

#### Inputs

`features` : `pluss::RangeView<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`method` : `CorrMethod`
    Correlation method.

    Supported values:
    - `Pearson`: Ordinary linear correlation.
    - `Spearman`: Rank correlation.
    - `KendallTau`: Rank-concordance correlation.

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

#### Output

`DataFrameView`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Find the correlation between selected columns using a non-default correlation method.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.corr({"id", "sales"}, pluss::table::DataFrame::CorrMethod::KendallTau, false);
```

##### Expected output

```text
feature  id      sales
id       1.0000  0.3273
sales    0.3273  1.0000
```
Example 2. Find the correlation for a selected subset of numeric columns.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.corr({"id", "sales"}, false);
```

##### Expected output

```text
feature  id      sales
id       1.0000  0.3273
sales    0.3273  1.0000
```
Example 3. Find the correlation across all numeric columns using a non-default correlation method.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.corr(pluss::table::DataFrame::CorrMethod::Spearman, false);
```

##### Expected output

```text
feature  id      sales
id       1.0000  0.3273
sales    0.3273  1.0000
```
Example 4. Find the correlation across all numeric columns using the default correlation method.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.corr(false);
```

##### Expected output

```text
feature  id      sales
id       1.0000  0.3273
sales    0.3273  1.0000
```

#### Practical Behavior

- `corr(...)` builds a correlation matrix across numeric features only.
- If you do not pass a feature list, the method auto-selects numeric features from the frame.
- Supported methods route through thread-level correlation kernels such as Pearson, Spearman, and Kendall Tau.


### `corr_with`

#### API Signatures

```cpp
DataFrame::corr_with(other: const DataFrame&, method: CorrMethod = CorrMethod::Pearson, min_count: std::int64_t = 1) -> Thread

DataFrame::corr_with(others: pluss::RangeView<DataFrame>, method: CorrMethod = CorrMethod::Pearson, min_count: std::int64_t = 1) -> DataFrame
```

#### Notes

Use this method through the public contract shown above.

#### Inputs

`other` : `const DataFrame&`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

`method` : `CorrMethod`
    Supported values:
    - `Pearson`: Ordinary linear correlation.
    - `Spearman`: Rank correlation.
    - `KendallTau`: Rank-concordance correlation.

`min_count` : `std::int64_t`
    Minimum non-null observations required before a grouped or rolling summary is considered valid.

`others` : `pluss::RangeView<DataFrame>`
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

#### Output

`Thread`
    Value produced by this call.

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.corr_with();
```

##### Expected output

```text
The call completes and returns the contract described under Output.
```
Example 2. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.corr_with();
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `count`

#### API Signature

```cpp
DataFrame::count(axis: Axis = Axis::Feature) -> Thread
```

#### Notes

Returns a size/count summary for the current object without changing it.

#### Inputs

`axis` : `Axis`
    Axis selector. On `DataFrame`, `0` usually means rows/cases and `1` means features/columns.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Compute `count` for each numeric column in the frame.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.count(false);
```

##### Expected output

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
DataFrame::count_by(feature: std::string_view, top_n: std::int64_t = 0, dropna: bool = true, render: bool = false) -> DataFrameView
```

#### Notes

Groups by one feature and returns per-group row counts.

#### Inputs

`feature` : `std::string_view`
    Name of the source feature used by the operation.

`top_n` : `std::int64_t`
    Optional cap on how many grouped rows to keep after sorting. Use `0` to keep every group.

`dropna` : `bool`
    When `true`, null grouping keys or null values are excluded where the method supports that policy. When `false`, null participates instead of being removed.

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

#### Output

`DataFrameView`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Choose whether the returned object should auto-render in display contexts.

```cpp
auto df = pluss::table::DataFrame({
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema"}))},
    {"product", pluss::table::Thread(pluss::make_note({"A", "B", "A", "B"}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0, 150.0}))},
});

auto out = df.count_by("city", 3, true, false);
```

##### Expected output

```text
city    count
Accra   2
Tema    2
```

#### Practical Behavior

- `count_by(feature, ...)` groups by one feature, counts rows per distinct value, sorts by count descending, then trims to `top_n` when requested.
- Ties are broken by key name in ascending order.


### `covar`

#### API Signature

```cpp
DataFrame::covar(options: const StatisticOptions& = {}, render: bool = false) -> DataFrameView
```

#### Notes

Use this method through the public contract shown above.

#### Inputs

`options` : `const StatisticOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

#### Output

`DataFrameView`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Choose whether the returned object should auto-render in display contexts.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.covar();
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `csmax`

#### API Signature

```cpp
DataFrame::csmax(axis: Axis = Axis::Feature, skipna: bool = true) -> Thread
```

#### Notes

Use this method through the public contract shown above.

#### Inputs

`axis` : `Axis`
    Axis selector. On `DataFrame`, `0` usually means rows/cases and `1` means features/columns.

`skipna` : `bool`
    When `true`, missing values are skipped during the reduction. When `false`, missing state can affect the result.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.csmax();
```

##### Expected output

```text
The call completes and returns the contract described under Output.
```

### `csmin`

#### API Signature

```cpp
DataFrame::csmin(axis: Axis = Axis::Feature, skipna: bool = true) -> Thread
```

#### Notes

Use this method through the public contract shown above.

#### Inputs

`axis` : `Axis`
    Axis selector. On `DataFrame`, `0` usually means rows/cases and `1` means features/columns.

`skipna` : `bool`
    When `true`, missing values are skipped during the reduction. When `false`, missing state can affect the result.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.csmin();
```

##### Expected output

```text
The call completes and returns the contract described under Output.
```

## D

### `defined`

#### API Signature

```cpp
DataFrame::defined() -> bool
```

#### Notes

Reports whether the current object owns a concrete payload that can be used safely in later operations.

#### Inputs

`None`

#### Output

`bool`
    Boolean status or membership result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.defined();
```

##### Expected output

```text
true
```

### `describe`

#### API Signature

```cpp
DataFrame::describe(render: bool = false) -> DataFrameView
```

#### Notes

Returns descriptive summary statistics for the selected numeric features.

#### Inputs

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

#### Output

`DataFrameView`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Choose whether the returned object should auto-render in display contexts.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.describe(false);
```

##### Expected output

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
DataFrame::div(other: const DataFrame&) -> DataFrame

DataFrame::div(other: double) -> DataFrame
```

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### Inputs

`other` : `const DataFrame&`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Divide each compatible numeric cell against another aligned frame.

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

##### Expected output

```text
id    sales
0.5   0.909091
1.0   1.090909
0.75  0.9
```
Example 2. Divide each compatible numeric cell against one broadcast scalar value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.div(10);
```

##### Expected output

```text
id  sales  city
0.1  10.0  Accra
0.2  12.0  Tema
0.3   9.0  Kumasi
```

### `drop`

#### API Signature

```cpp
DataFrame::drop(names: pluss::RangeView<std::string_view>) -> DataFrame
```

#### Notes

Removes one or more features and returns the remaining frame.

#### Inputs

`names` : `pluss::RangeView<std::string_view>`
    Collection of feature, slot, or stream names supplied in positional order.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.drop({"city"});
```

##### Expected output

```text
id  sales
1   100.0
2   120.0
3    90.0
```

#### Practical Behavior

- `drop(...)` removes features, not rows.
- The remaining features keep their existing row alignment and case axis.


### `drop_duplicates`

#### API Signature

```cpp
DataFrame::drop_duplicates(features: pluss::RangeView<std::string_view> = {}, keep_first: bool = true, keep_last: bool = false) -> DataFrame
```

#### Notes

Use this method through the public contract shown above.

#### Inputs

`features` : `pluss::RangeView<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`keep_first` : `bool`
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

`keep_last` : `bool`
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.drop_duplicates();
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `drop_levels`

#### API Signatures

```cpp
DataFrame::drop_levels(names: pluss::RangeView<std::string_view>) -> DataFrame

DataFrame::drop_levels(names: std::initializer_list<std::string_view>) -> DataFrame
```

#### Notes

Use this method through the public contract shown above.

#### Inputs

`names` : `pluss::RangeView<std::string_view>`
    Collection of feature, slot, or stream names supplied in positional order.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.drop_levels();
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 2. Pass several feature names inline.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.drop_levels();
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `dropna`

#### API Signature

```cpp
DataFrame::dropna(features: pluss::RangeView<std::string_view> = {}, all: bool = false) -> DataFrame
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`features` : `pluss::RangeView<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`all` : `bool`
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Work with a selected feature subset.

```cpp
auto sales = pluss::table::Thread(pluss::make_series({100.0, 0.0, 90.0}))
                 .with_validity_mask(pluss::make_series({true, false, true}, pluss::ScalarType::Bool));
auto df = pluss::table::DataFrame({
    {"id", pluss::make_series({1, 2, 3})},
    {"sales", sales},
    {"city", pluss::make_note({"Accra", "Tema", "Kumasi"})},
});

auto out = df.dropna();
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
3    90.0  Kumasi
```

#### Practical Behavior

- `dropna()` removes rows that contain at least one null value anywhere in the frame.
- Use `dropna_all(features)` when the rule should apply only to selected features and only when all of them are null.


### `duplicated`

#### API Signature

```cpp
DataFrame::duplicated(features: pluss::RangeView<std::string_view> = {}, keep_first: bool = true, keep_last: bool = false) -> Thread
```

#### Notes

Use this method through the public contract shown above.

#### Inputs

`features` : `pluss::RangeView<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`keep_first` : `bool`
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

`keep_last` : `bool`
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.duplicated();
```

##### Expected output

```text
The call completes and returns the contract described under Output.
```

## E

### `eq`

#### API Signatures

```cpp
DataFrame::eq(other: const DataFrame&) -> DataFrame

DataFrame::eq(other: double) -> DataFrame
```

#### Notes

This produces a comparison-style result you can inspect directly or reuse as a mask.

#### Inputs

`other` : `const DataFrame&`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Compare each compatible cell for equality against another aligned frame.

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

##### Expected output

```text
id  sales
1   true
2   true
3   false
```
Example 2. Compare each compatible cell for equality against one broadcast scalar value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.eq(10);
```

##### Expected output

```text
id  sales
1   true
2   true
3   false
```

### `equals`

#### API Signature

```cpp
DataFrame::equals(other: const DataFrame&) -> bool
```

#### Notes

Checks whether two frames have the same structure, labels, and values.

#### Inputs

`other` : `const DataFrame&`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

#### Output

`bool`
    Boolean status or membership result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

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

##### Expected output

```text
true
```

### `extend_cases`

#### API Signature

```cpp
DataFrame::extend_cases(other: const DataFrame&, features: FrameMatchRule = FrameMatchRule::Unite, fresh_cases: bool = false, reject_repeated_cases: bool = false) -> DataFrame
```

#### Notes

Appends another frame beneath this frame after matching the feature schemas and null-filling one-sided features.

#### Inputs

`other` : `const DataFrame&`
    Frame whose cases should be appended beneath this frame.

`features` : `FrameMatchRule`
    Feature matching rule for the non-extension axis. `Unite` keeps the stable union; `Common` keeps shared features.

`fresh_cases` : `bool`
    When `true`, replace the appended case labels with contiguous integer cases starting at zero.

`reject_repeated_cases` : `bool`
    When `true`, fail if preserving the two case axes would produce repeated case labels.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto left = pluss::table::DataFrame(
    {{"x", pluss::make_series({1.0, 2.0})},
     {"y", pluss::make_series({3.0, 4.0})}},
    pluss::table::Thread(pluss::make_note({"A", "C"})));
auto right = pluss::table::DataFrame(
    {{"y", pluss::make_series({5.0, 6.0})},
     {"z", pluss::make_series({7.0, 8.0})}},
    pluss::table::Thread(pluss::make_note({"B", "A"})));
auto out = left.extend_cases(right);
```

##### Expected output

```text
case  x     y     z
A     1.0   3.0   null
C     2.0   4.0   null
B     null  5.0   7.0
A     null  6.0   8.0
```

#### Extension Rules

- `extend_cases(other)` appends cases vertically and matches features by name.
- The default `Unite` feature rule preserves left feature order, appends right-only features, and null-fills one-sided payloads.
- Matching numeric features must have the same dtype and platform; other matching features must have the same Thread kind.
- Use `fresh_cases=true` for contiguous integer cases or `reject_repeated_cases=true` to require a unique preserved case axis.


## F

### `feature_names`

#### API Signature

```cpp
DataFrame::feature_names() -> const std::vector<std::string>&
```

#### Notes

Returns the current labels used to address fields, slots, rows, or depth items.

#### Inputs

`None`

#### Output

`const std::vector<std::string>&`
    Borrowed list of names.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Pass several names together from a standard string container.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.feature_names();
```

##### Expected output

```text
["id", "sales", "city"]
```

### `feature_ref`

#### API Signature

```cpp
DataFrame::feature_ref(name: std::string_view) -> ColumnRef
```

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### Inputs

`name` : `std::string_view`
    Name used to identify the target object, field, feature, or slot for this overload.

#### Output

`ColumnRef`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.feature_ref("sales");
```

##### Expected output

```text
The call completes and returns the contract described under Output.
```

### `features`

#### API Signatures

```cpp
DataFrame::features() -> std::vector<Thread>

DataFrame::features(name: std::string_view) -> Thread

DataFrame::features(name: std::string_view) -> Thread

DataFrame::features(names: const NameRange&) -> DataFrame

DataFrame::features(names: const NameRange&) -> DataFrame

DataFrame::features(names: pluss::RangeView<std::string_view>) -> DataFrame

DataFrame::features(names: pluss::RangeView<std::string_view>) -> DataFrame

DataFrame::features(names: StackArray<std::string_view>) -> DataFrame

DataFrame::features(names: StackArray<std::string_view>) -> DataFrame

DataFrame::features(names: std::initializer_list<const char*>) -> DataFrame

DataFrame::features(names: std::initializer_list<const char*>) -> DataFrame

DataFrame::features(names: std::initializer_list<std::string_view>) -> DataFrame

DataFrame::features(names: std::initializer_list<std::string_view>) -> DataFrame
```

#### Notes

Materializes the current members as a collection you can inspect or reuse elsewhere.

#### Inputs

`name` : `std::string_view`
    Name used to identify the target object, field, feature, or slot for this overload.

`names` : `const NameRange&`
    Collection of feature, slot, or stream names supplied in positional order.

#### Output

`std::vector<Thread>`
    Materialized collection of thread objects.

`Thread`
    Value produced by this call.

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Materialize every feature thread in left-to-right order.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.features();
```

##### Expected output

```text
[Thread(id), Thread(sales), Thread(city)]
```
Example 2. Extract one feature thread by name.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.features("sales");
```

##### Expected output

```text
Thread([100.0, 120.0, 90.0])
```
Example 3. Extract one feature thread by name.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.features("sales");
```

##### Expected output

```text
Thread([100.0, 120.0, 90.0])
```
Example 4. Project a named feature subset from a reusable name-range container.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.features();
```

##### Expected output

```text
[Thread(id), Thread(sales), Thread(city)]
```
Example 5. Project a named feature subset from a reusable name-range container.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.features();
```

##### Expected output

```text
[Thread(id), Thread(sales), Thread(city)]
```
Example 6. Project a named feature subset from a lightweight string view.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.features("sales");
```

##### Expected output

```text
Thread([100.0, 120.0, 90.0])
```
Example 7. Project a named feature subset from a lightweight string view.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.features("sales");
```

##### Expected output

```text
Thread([100.0, 120.0, 90.0])
```
Example 8. Project a named feature subset from a fixed stack-allocated name list.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.features("sales");
```

##### Expected output

```text
Thread([100.0, 120.0, 90.0])
```
Example 9. Project a named feature subset from a fixed stack-allocated name list.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.features("sales");
```

##### Expected output

```text
Thread([100.0, 120.0, 90.0])
```
Example 10. Project a named feature subset from inline C-string feature names.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.features({"id", "sales"});
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 11. Project a named feature subset from inline C-string feature names.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.features({"id", "sales"});
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 12. Project a named feature subset from inline string-view feature names.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.features({"id", "sales"});
```

##### Expected output

```text
Thread([100.0, 120.0, 90.0])
```
Example 13. Project a named feature subset from inline string-view feature names.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.features({"id", "sales"});
```

##### Expected output

```text
Thread([100.0, 120.0, 90.0])
```

### `ffill`

#### API Signature

```cpp
DataFrame::ffill() -> DataFrame
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`None`

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.ffill();
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `fillna`

#### API Signatures

```cpp
DataFrame::fillna(value: const char*) -> DataFrame

DataFrame::fillna(value: const char*, features: pluss::RangeView<std::string_view>) -> DataFrame

DataFrame::fillna(value: const pluss::TimeIndex&) -> DataFrame

DataFrame::fillna(value: const pluss::TimeIndex&, features: pluss::RangeView<std::string_view>) -> DataFrame

DataFrame::fillna(value: const pluss::Timeline&) -> DataFrame

DataFrame::fillna(value: const pluss::Timeline&, features: pluss::RangeView<std::string_view>) -> DataFrame

DataFrame::fillna(value: double) -> DataFrame

DataFrame::fillna(value: double, features: pluss::RangeView<std::string_view>) -> DataFrame

DataFrame::fillna(value: std::string_view) -> DataFrame

DataFrame::fillna(value: std::string_view, features: pluss::RangeView<std::string_view>) -> DataFrame
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`value` : `const char*`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

`features` : `pluss::RangeView<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Fill missing values in a returned copy with a text scalar.

```cpp
auto df = pluss::table::DataFrame({{"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))}});

auto out = df.fillna("Unknown");
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 2. Fill missing values in a returned copy only within a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({{"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))}});

auto out = df.fillna("Unknown");
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 3. Fill missing values in a returned copy with a TimeIndex scalar.

```cpp
auto df = pluss::table::DataFrame({{"idx", pluss::table::Thread(pluss::make_timeindex({"2024-01-01T09:10:00Z", "2024-01-01T09:40:00Z"}))}});

auto out = df.fillna(pluss::make_timeindex("2024-01-01T00:00:00Z"));
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 4. Fill missing values in a returned copy only within a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({{"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))}});

auto out = df.fillna("Unknown");
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 5. Fill missing values in a returned copy with a Timeline scalar.

```cpp
auto df = pluss::table::DataFrame({{"ts", pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-01-01T09:40:00Z"}))}});

auto out = df.fillna(pluss::make_timeline("2024-01-01T00:00:00Z"));
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 6. Fill missing values in a returned copy only within a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({{"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))}});

auto out = df.fillna("Unknown");
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 7. Fill missing values in a returned copy with a numeric scalar.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.fillna(0.0, {"sales"});
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 8. Fill missing values in a returned copy only within a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({{"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))}});

auto out = df.fillna("Unknown");
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 9. Fill missing values in a returned copy with a text scalar.

```cpp
auto df = pluss::table::DataFrame({{"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))}});

auto out = df.fillna("Unknown");
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 10. Fill missing values in a returned copy only within a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({{"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))}});

auto out = df.fillna("Unknown");
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `fillna_`

#### API Signatures

```cpp
DataFrame::fillna_(value: const char*) -> DataFrame&

DataFrame::fillna_(value: const pluss::TimeIndex&) -> DataFrame&

DataFrame::fillna_(value: const pluss::Timeline&) -> DataFrame&

DataFrame::fillna_(value: double) -> DataFrame&

DataFrame::fillna_(value: std::string_view) -> DataFrame&
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`value` : `const char*`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`DataFrame&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Fill missing values in place with a text scalar.

```cpp
auto df = pluss::table::DataFrame({{"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))}});

df.fillna_("Unknown");
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 2. Fill missing values in place with a TimeIndex scalar.

```cpp
auto df = pluss::table::DataFrame({{"idx", pluss::table::Thread(pluss::make_timeindex({"2024-01-01T09:10:00Z", "2024-01-01T09:40:00Z"}))}});

df.fillna_(pluss::make_timeindex("2024-01-01T00:00:00Z"));
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 3. Fill missing values in place with a Timeline scalar.

```cpp
auto df = pluss::table::DataFrame({{"ts", pluss::table::Thread(pluss::make_timeline({"2024-01-01T09:10:00Z", "2024-01-01T09:40:00Z"}))}});

df.fillna_(pluss::make_timeline("2024-01-01T00:00:00Z"));
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 4. Fill missing values in place with a numeric scalar.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.fillna_(0.0);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 5. Fill missing values in place with a text scalar.

```cpp
auto df = pluss::table::DataFrame({{"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))}});

df.fillna_("Unknown");
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `filter`

#### API Signatures

```cpp
DataFrame::filter(bool_mask: const pluss::Series&) -> DataFrame

DataFrame::filter(bool_mask: const Thread&) -> DataFrame
```

#### Notes

Keeps only the rows where the supplied boolean mask is true.

#### Inputs

`bool_mask` : `const pluss::Series&`
    Boolean row mask. `true` keeps or rewrites the corresponding row, while `false` leaves it out or leaves the original value unchanged depending on the method.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Filter rows using an explicit boolean Series mask.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = pluss::make_series({true, false, true}, pluss::ScalarType::Bool);
auto out = df.filter(mask);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
3    90.0  Kumasi
```
Example 2. Filter rows using a boolean mask built from an existing frame expression.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = df.features("sales") >= 95.0;
auto out = df.filter(mask);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
3    90.0  Kumasi
```

### `floordiv`

#### API Signatures

```cpp
DataFrame::floordiv(other: const DataFrame&) -> DataFrame

DataFrame::floordiv(other: double) -> DataFrame
```

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### Inputs

`other` : `const DataFrame&`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Divide each compatible numeric cell against another aligned frame.

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

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 2. Divide each compatible numeric cell against one broadcast scalar value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.floordiv(10);
```

##### Expected output

```text
id  sales  city
0.0  10.0  Accra
0.0  12.0  Tema
0.0   9.0  Kumasi
```

### `flow`

#### API Signature

```cpp
DataFrame::flow(fn: const std::function<DataFrame(const DataFrame&)>&) -> DataFrame
```

#### Notes

Use this method through the public contract shown above.

#### Inputs

`fn` : `const std::function<DataFrame(const DataFrame&)>&`
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.flow();
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `fold`

#### API Signature

```cpp
DataFrame::fold(fn: const std::function<Thread(const DataFrame&)>&, shape: ResultShape = ResultShape::Reduce) -> Thread
```

#### Notes

Use this method through the public contract shown above.

#### Inputs

`fn` : `const std::function<Thread(const DataFrame&)>&`
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

`shape` : `ResultShape`
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.fold();
```

##### Expected output

```text
The call completes and returns the contract described under Output.
```

### `freq`

#### API Signature

```cpp
DataFrame::freq(features: pluss::RangeView<std::string_view> = {}, top_n: std::int64_t = 0, dropna: bool = true) -> Thread
```

#### Notes

Use this method through the public contract shown above.

#### Inputs

`features` : `pluss::RangeView<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`top_n` : `std::int64_t`
    Optional cap on how many grouped rows to keep after sorting. Use `0` to keep every group.

`dropna` : `bool`
    When `true`, null grouping keys or null values are excluded where the method supports that policy. When `false`, null participates instead of being removed.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.freq();
```

##### Expected output

```text
The call completes and returns the contract described under Output.
```

## G

### `get`

#### API Signature

```cpp
DataFrame::get(feature_name: std::string_view) -> std::optional<Thread>
```

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### Inputs

`feature_name` : `std::string_view`
    Name of the target feature to read, rewrite, sort by, or promote to cases.

#### Output

`std::optional<Thread>`
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

auto out = df.get("sales");
```

##### Expected output

```text
Thread([100.0, 120.0, 90.0])
```

### `group_by`

#### API Signatures

```cpp
DataFrame::group_by(keys: const CaseGroup&, specs: pluss::RangeView<GroupSpec>, dropna_keys: bool = true, render: bool = false) -> DataFrameView

DataFrame::group_by(keys: pluss::RangeView<std::string_view>, specs: pluss::RangeView<GroupSpec>, dropna_keys: bool = true, render: bool = false) -> DataFrameView

DataFrame::group_by(keys: std::initializer_list<std::string_view>, specs: pluss::RangeView<GroupSpec>, dropna_keys: bool = true, render: bool = false) -> DataFrameView
```

#### Notes

Groups rows by one or more key features and computes the requested aggregations for each group.

#### Inputs

`keys` : `const CaseGroup&`
    Feature names whose distinct value combinations define the output groups.

`specs` : `pluss::RangeView<GroupSpec>`
    Grouped aggregation specs. Each `GroupSpec` has `output`, `feature`, and `op`.

    `op` supports:
    - `Count`: Count observed values inside each group.
    - `Nunique`: Count distinct observed values inside each group.
    - `Sum`: Compute the numeric sum inside each group.
    - `Mean`: Compute the arithmetic mean inside each group.
    - `Min`: Compute the minimum inside each group.
    - `Max`: Compute the maximum inside each group.
    - `Std`: Compute the sample standard deviation inside each group.
    - `Var`: Compute the sample variance inside each group.

`dropna_keys` : `bool`
    When `true`, rows with null pivot/group keys are excluded before reshaping.

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

#### Output

`DataFrameView`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Choose whether the returned object should auto-render in display contexts.

```cpp
auto df = pluss::table::DataFrame({
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema"}))},
    {"product", pluss::table::Thread(pluss::make_note({"A", "B", "A", "B"}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0, 150.0}))},
});

auto out = df.group_by({"city"}, {{"sales_sum", "sales", pluss::table::DataFrame::GroupOp::Sum}}, true, false);
```

##### Expected output

```text
city    sales_sum
Accra   220.0
Tema    240.0
```
Example 2. Choose whether the returned object should auto-render in display contexts.

```cpp
auto df = pluss::table::DataFrame({
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema"}))},
    {"product", pluss::table::Thread(pluss::make_note({"A", "B", "A", "B"}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0, 150.0}))},
});

auto out = df.group_by({"city"}, {{"sales_sum", "sales", pluss::table::DataFrame::GroupOp::Sum}}, true, false);
```

##### Expected output

```text
city    sales_sum
Accra   220.0
Tema    240.0
```
Example 3. Choose whether the returned object should auto-render in display contexts.

```cpp
auto df = pluss::table::DataFrame({
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema"}))},
    {"product", pluss::table::Thread(pluss::make_note({"A", "B", "A", "B"}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0, 150.0}))},
});

auto out = df.group_by({"city"}, {{"sales_sum", "sales", pluss::table::DataFrame::GroupOp::Sum}}, true, false);
```

##### Expected output

```text
city    sales_sum
Accra   220.0
Tema    240.0
```

#### Execution Model

- `group_by(...)` factorizes typed key combinations once in stable first-seen order.
- Count, sum, mean, minimum, maximum, sample standard deviation, and sample variance use vectorized Series scatter reductions.
- Numeric payloads and validity masks stay on their source platform; key hashing and arbitrary-value distinct counting require one bulk host materialization.
- Output key features retain their original Thread kind and dtype.


### `gt`

#### API Signatures

```cpp
DataFrame::gt(other: const DataFrame&, strict: bool = true) -> DataFrame

DataFrame::gt(other: double, strict: bool = true) -> DataFrame
```

#### Notes

This produces a comparison-style result you can inspect directly or reuse as a mask.

#### Inputs

`other` : `const DataFrame&`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

`strict` : `bool`
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Compare each compatible cell using the `>` relation against another aligned frame.

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

##### Expected output

```text
id  sales
1   true
2   true
3   false
```
Example 2. Compare each compatible cell using the `>` relation against one broadcast scalar value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.gt(10);
```

##### Expected output

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
DataFrame::has_feature(name: std::string_view) -> bool
```

#### Notes

Checks whether a feature name exists in the frame.

#### Inputs

`name` : `std::string_view`
    Name used to identify the target object, field, feature, or slot for this overload.

#### Output

`bool`
    Boolean status or membership result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.has_feature("sales");
```

##### Expected output

```text
true
```

### `has_levels`

#### API Signature

```cpp
DataFrame::has_levels() -> bool
```

#### Notes

Use this method through the public contract shown above.

#### Inputs

`None`

#### Output

`bool`
    Boolean status or membership result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.has_levels();
```

##### Expected output

```text
The call completes and returns the contract described under Output.
```

### `head`

#### API Signature

```cpp
DataFrame::head(n: std::int64_t = 5, render: bool = false) -> DataFrameView
```

#### Notes

Returns the first `n` rows of the frame.

#### Inputs

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

#### Output

`DataFrameView`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Choose whether the returned object should auto-render in display contexts.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.head(2, false);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `height`

#### API Signature

```cpp
DataFrame::height() -> std::int64_t
```

#### Notes

Use this method through the public contract shown above.

#### Inputs

`None`

#### Output

`std::int64_t`
    Integer count, size, or positional result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.height();
```

##### Expected output

```text
The call completes and returns the contract described under Output.
```

## I

### `info`

#### API Signatures

```cpp
DataFrame::info(options: const display::DisplayOptions& = {}) -> void

DataFrame::info(os: std::ostream&, options: const display::DisplayOptions& = {}) -> void
```

#### Notes

This renders a human-readable view for inspection instead of creating a new analytic object.

#### Inputs

`options` : `const display::DisplayOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

`os` : `std::ostream&`
    Destination output stream used instead of the default display or print target.

#### Output

`void`
    Performs the side effect and returns no value.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Print the frame summary with the default output route.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.info();
```

##### Expected output

```text
rows: 3
features: 3
numeric: id, sales
note: city
```
Example 2. Write the frame summary to a caller-provided output stream.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

std::ostringstream os;
df.info(os);
```

##### Expected output

```text
rows: 3
features: 3
numeric: id, sales
note: city
```

### `info_string`

#### API Signature

```cpp
DataFrame::info_string(options: const display::DisplayOptions& = {}) -> std::string
```

#### Notes

Serializes or renders the current object into a text form you can inspect, log, or export.

#### Inputs

`options` : `const display::DisplayOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`std::string`
    Rendered string result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Pass an explicit options object to control the operation.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.info_string();
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `insert`

#### API Signature

```cpp
DataFrame::insert(feature_index: std::int64_t, name: std::string, feature: const Thread&) -> DataFrame
```

#### Notes

Inserts a new feature at the requested position and shifts the later features to the right.

#### Inputs

`feature_index` : `std::int64_t`
    Zero-based output column position used by structural methods such as `insert(...)`.

`name` : `std::string`
    Name used to identify the target object, field, feature, or slot for this overload.

`feature` : `const Thread&`
    Name of the source feature used by the operation.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.insert(1, "profit", pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0})));
```

##### Expected output

```text
id  sales  city    profit
1   100.0  Accra   40.0
2   120.0  Tema    35.0
3    90.0  Kumasi  30.0
```

### `isnull`

#### API Signature

```cpp
DataFrame::isnull(features: pluss::RangeView<std::string_view> = {}) -> DataFrame
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`features` : `pluss::RangeView<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.isnull({"sales"});
```

##### Expected output

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
DataFrame::join(other: const DataFrame&, collision_suffix: std::string = "_right") -> DataFrame
```

#### Notes

Adds compatible features from another frame, aligning rows by the shared case axis.

#### Inputs

`other` : `const DataFrame&`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

`collision_suffix` : `std::string`
    Suffix added when a second frame contributes a feature name that already exists on the left-hand side.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

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

##### Expected output

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
DataFrame::kindelize() -> pluss::kindle::Catalog

DataFrame::kindelize(policy: const pluss::kindle::KindelizePolicy&) -> pluss::kindle::Catalog
```

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Inputs

`policy` : `const pluss::kindle::KindelizePolicy&`
    Policy object that changes the structural behavior of this overload.

#### Output

`pluss::kindle::Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Convert the frame into a Kindle catalog using the default policy.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.kindelize();
```

##### Expected output

```text
The call completes and returns the contract described under Output.
```
Example 2. Convert the frame into a Kindle catalog using an explicit kindelization policy.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

pluss::kindle::KindelizePolicy policy;
auto out = df.kindelize(policy);
```

##### Expected output

```text
The call completes and returns the contract described under Output.
```

### `kurtosis`

#### API Signature

```cpp
DataFrame::kurtosis(axis: Axis = Axis::Feature, min_count: std::int64_t = 4) -> Thread
```

#### Notes

Use this method through the public contract shown above.

#### Inputs

`axis` : `Axis`
    Axis selector. On `DataFrame`, `0` usually means rows/cases and `1` means features/columns.

`min_count` : `std::int64_t`
    Minimum non-null observations required before a grouped or rolling summary is considered valid.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.kurtosis();
```

##### Expected output

```text
The call completes and returns the contract described under Output.
```

## L

### `lengthen`

#### API Signature

```cpp
DataFrame::lengthen(id_features: pluss::RangeView<std::string_view> = {}, value_features: pluss::RangeView<std::string_view> = {}, feature_name: std::string = "feature", value_name: std::string = "value", fresh_cases: bool = true) -> DataFrame
```

#### Notes

Reshapes a wide frame into long form by tiling identifier features and stacking homogeneous value features.

#### Inputs

`id_features` : `pluss::RangeView<std::string_view>`
    Features repeated unchanged for each selected value feature.

`value_features` : `pluss::RangeView<std::string_view>`
    Homogeneous features stacked into the output value feature. Leave empty to use every non-identifier feature.

`feature_name` : `std::string`
    Output name for the Note-backed feature that identifies each source value feature.

`value_name` : `std::string`
    Output name for the stacked typed values.

`fresh_cases` : `bool`
    When `true`, create contiguous integer cases; when `false`, repeat the original case axis for every value feature.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto wide = pluss::table::DataFrame({
    {"id", pluss::make_note({"r1", "r2"})},
    {"a", pluss::make_series({1.0, 2.0})},
    {"b", pluss::make_series({3.0, 4.0})},
});
auto out = wide.lengthen({"id"}, {"a", "b"});
```

##### Expected output

```text
id  feature  value
r1  a        1.0
r2  a        2.0
r1  b        3.0
r2  b        4.0
```

#### Reshape Rules

- `lengthen(...)` tiles identifier features and stacks each value feature as one contiguous case block.
- Value features must share a Thread kind; numeric value features must also share dtype and platform.
- Payload concatenation and identifier tiling use Series/Thread bulk operations rather than cell-wise conversion.
- Missing-value masks are concatenated with the payload and preserved.


### `level`

#### API Signatures

```cpp
DataFrame::level(name: std::string_view) -> Thread

DataFrame::level(position: std::int64_t) -> Thread
```

#### Notes

Use this method through the public contract shown above.

#### Inputs

`name` : `std::string_view`
    Name used to identify the target object, field, feature, or slot for this overload.

`position` : `std::int64_t`
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.level();
```

##### Expected output

```text
The call completes and returns the contract described under Output.
```
Example 2. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.level();
```

##### Expected output

```text
The call completes and returns the contract described under Output.
```

### `levels`

#### API Signature

```cpp
DataFrame::levels() -> std::vector<std::string>
```

#### Notes

Use this method through the public contract shown above.

#### Inputs

`None`

#### Output

`std::vector<std::string>`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Pass several names together from a standard string container.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.levels();
```

##### Expected output

```text
The call completes and returns the contract described under Output.
```

### `load`

#### API Signatures

```cpp
DataFrame::load(source_type: std::string_view, filepath: std::string_view) -> DataFrame

DataFrame::load(source_type: std::string_view, filepath: std::string_view, config: const pluss::loader::Config&) -> DataFrame

DataFrame::load(source_type: std::string_view, filepath: std::string_view, config_entries: std::initializer_list<pluss::loader::ConfigEntry>) -> DataFrame

DataFrame::load(source_type: std::string_view, filepath: std::string_view, options: const pluss::loader::ReadCsvOptions&) -> DataFrame

DataFrame::load(source_type: std::string_view, filepath: std::string_view, sep: char, ncases: std::optional<std::size_t> = std::nullopt, usefeats: std::vector<std::string> = {}, case_col: std::optional<std::string> = std::nullopt, on_bad_lines: pluss::loader::BadLinePolicy = pluss::loader::BadLinePolicy::Error) -> DataFrame

DataFrame::load(source_type: std::string_view, filepath: std::string_view, sep: char, ncases: std::optional<std::size_t>, usefeats: pluss::RangeView<std::string_view>, case_col: std::optional<std::string> = std::nullopt, on_bad_lines: pluss::loader::BadLinePolicy = pluss::loader::BadLinePolicy::Error) -> DataFrame
```

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### Inputs

`source_type` : `std::string_view`
    Storage format selector such as `"csv"`, `"xlsx"`, `"json"`, or `"parquet"`.

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

`config_entries` : `std::initializer_list<pluss::loader::ConfigEntry>`
    Inline configuration entries used to build a lightweight loader configuration without naming a separate config object.

`options` : `const pluss::loader::ReadCsvOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

`sep` : `char`
    Column separator used for delimited text files such as CSV.

`ncases` : `std::optional<std::size_t>`
    Optional row limit applied while loading. Leave unset to read the whole source.

`usefeats` : `std::vector<std::string>`
    Feature names to load from the source. Leave empty to load every available feature.

`case_col` : `std::optional<std::string>`
    Optional source column that should become the DataFrame case axis while loading.

`on_bad_lines` : `pluss::loader::BadLinePolicy`
    Malformed-line policy for delimited input.

    Supported values:
    - `Error`: Stop immediately when a malformed row is encountered.
    - `Warn`: Continue while reporting the malformed row.
    - `Skip`: Ignore malformed rows silently.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Load a runtime-selected source type with the default reader for that format.

```cpp
auto out = pluss::table::DataFrame::load("csv", "path/to/data.csv");
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 2. Load a runtime-selected source type with a reusable loader configuration object.

```cpp
pluss::loader::Config config{
    {"ncases", 8},
    {"features", {"Agency Name", "State"}},
};
auto out = pluss::table::DataFrame::load("csv", "path/to/data.csv", config);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 3. Load a runtime-selected source type while passing inline loader configuration entries.

```cpp
auto out = pluss::table::DataFrame::load("csv", "path/to/data.csv", {
    {"ncases", 8},
    {"features", {"Agency Name", "State"}},
});
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 4. Load a delimited file through the format-switched entry point while controlling parsing with a typed CSV options object.

```cpp
pluss::loader::ReadCsvOptions options;
options.ncases = 8;
options.case_col = "Agency Name";
auto out = pluss::table::DataFrame::load("csv", "path/to/data.csv", options);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 5. Load a runtime-selected delimited source while overriding the separator, row limit, and projected feature list.

```cpp
auto out = pluss::table::DataFrame::load("csv", "path/to/data.csv", ',', 8, std::vector<std::string>{"Agency Name", "State"}, std::nullopt);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 6. Load a runtime-selected delimited source while projecting only a selected feature subset from a lightweight string view.

```cpp
std::array<std::string_view, 2> usefeats{"Agency Name", "State"};
auto out = pluss::table::DataFrame::load("csv", "path/to/data.csv", ',', 8, usefeats, std::nullopt);
```

##### Expected output

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
DataFrame::load_csv(filepath: std::string_view) -> DataFrame

DataFrame::load_csv(filepath: std::string_view, config: const pluss::loader::Config&) -> DataFrame

DataFrame::load_csv(filepath: std::string_view, config_entries: std::initializer_list<pluss::loader::ConfigEntry>) -> DataFrame

DataFrame::load_csv(filepath: std::string_view, header: std::string_view, case_feature: std::string_view = "none", engine: std::string_view = "auto", ncases: std::optional<std::size_t> = std::nullopt) -> DataFrame

DataFrame::load_csv(filepath: std::string_view, header: std::string_view, ncases: std::size_t) -> DataFrame

DataFrame::load_csv(filepath: std::string_view, options: const pluss::loader::ReadCsvOptions&) -> DataFrame

DataFrame::load_csv(filepath: std::string_view, sep: char, ncases: std::optional<std::size_t> = std::nullopt, usefeats: std::vector<std::string> = {}, case_col: std::optional<std::string> = std::nullopt, on_bad_lines: pluss::loader::BadLinePolicy = pluss::loader::BadLinePolicy::Error) -> DataFrame

DataFrame::load_csv(filepath: std::string_view, sep: char, ncases: std::optional<std::size_t>, usefeats: pluss::RangeView<std::string_view>, case_col: std::optional<std::string> = std::nullopt, on_bad_lines: pluss::loader::BadLinePolicy = pluss::loader::BadLinePolicy::Error) -> DataFrame
```

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

`config_entries` : `std::initializer_list<pluss::loader::ConfigEntry>`
    Inline configuration entries used to build a lightweight loader configuration without naming a separate config object.

`header` : `std::string_view`
    CSV header mode.

    Common values:
    - `infer`: Read the first row as feature names.
    - `none`: Treat the file as headerless and auto-generate feature names such as `feat_0`, `feat_1`, and so on.

`case_feature` : `std::string_view`
    Name to use for the restored case-axis feature when cases are materialized back into ordinary columns.

`engine` : `std::string_view`
    Backend selector. Use `"auto"` to let PlussMic choose, or pass an explicit engine name when you need predictable routing.

`ncases` : `std::optional<std::size_t>`
    Optional row limit applied while loading. Leave unset to read the whole source.

`options` : `const pluss::loader::ReadCsvOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

`sep` : `char`
    Column separator used for delimited text files such as CSV.

`usefeats` : `std::vector<std::string>`
    Feature names to load from the source. Leave empty to load every available feature.

`case_col` : `std::optional<std::string>`
    Optional source column that should become the DataFrame case axis while loading.

`on_bad_lines` : `pluss::loader::BadLinePolicy`
    Malformed-line policy for delimited input.

    Supported values:
    - `Error`: Stop immediately when a malformed row is encountered.
    - `Warn`: Continue while reporting the malformed row.
    - `Skip`: Ignore malformed rows silently.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Load a CSV file with the default parser settings.

```cpp
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv");
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 2. Load a CSV file with a reusable loader configuration object.

```cpp
pluss::loader::Config config{
    {"ncases", 8},
    {"features", {"Agency Name", "State"}},
};
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv", config);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 3. Load a CSV file while supplying inline configuration entries such as row limits or projected features.

```cpp
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv", {
    {"ncases", 8},
    {"features", {"Agency Name", "State"}},
});
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 4. Load a CSV file while explicitly controlling header inference, case-axis promotion, engine selection, and row limit.

```cpp
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv", "infer", "Agency Name", "auto", 8);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 5. Load a headerless CSV file and auto-generate feature names while limiting the number of rows read.

```cpp
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv", "none", static_cast<std::size_t>(8));
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 6. Load a CSV file with a typed options object so parsing rules live in one reusable config.

```cpp
pluss::loader::ReadCsvOptions options;
options.ncases = 8;
options.case_col = "Agency Name";
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv", options);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 7. Load a CSV file with an explicit separator, row limit, and projected feature subset from a standard string container.

```cpp
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv", ',', 8, std::vector<std::string>{"Agency Name", "State"}, std::nullopt);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 8. Load a CSV file with an explicit separator, row limit, and projected feature subset from a lightweight string view.

```cpp
std::array<std::string_view, 2> usefeats{"Agency Name", "State"};
auto out = pluss::table::DataFrame::load_csv("path/to/data.csv", ',', 8, usefeats, std::nullopt);
```

##### Expected output

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
DataFrame::load_excel(filepath: std::string_view) -> DataFrame

DataFrame::load_excel(filepath: std::string_view, arg2: std::nullptr_t, header: bool = true) -> pluss::loader::LoadResult

DataFrame::load_excel(filepath: std::string_view, config: const pluss::loader::Config&) -> DataFrame

DataFrame::load_excel(filepath: std::string_view, config_entries: std::initializer_list<pluss::loader::ConfigEntry>) -> DataFrame

DataFrame::load_excel(filepath: std::string_view, options: const pluss::loader::ReadExcelOptions&) -> DataFrame

DataFrame::load_excel(filepath: std::string_view, sheet_index: std::int64_t, header: bool = true) -> DataFrame

DataFrame::load_excel(filepath: std::string_view, sheet_indices: const std::vector<std::int64_t>&, header: bool = true) -> pluss::loader::LoadResult

DataFrame::load_excel(filepath: std::string_view, sheet_indices: pluss::RangeView<std::int64_t>, header: bool = true) -> pluss::loader::LoadResult

DataFrame::load_excel(filepath: std::string_view, sheet_indices: std::initializer_list<std::int64_t>, header: bool = true) -> pluss::loader::LoadResult

DataFrame::load_excel(filepath: std::string_view, sheet_name: std::string_view, header: bool = true) -> DataFrame

DataFrame::load_excel(filepath: std::string_view, sheet_names: const std::vector<std::string>&, header: bool = true) -> pluss::loader::LoadResult

DataFrame::load_excel(filepath: std::string_view, sheet_names: pluss::RangeView<std::string_view>, header: bool = true) -> pluss::loader::LoadResult

DataFrame::load_excel(filepath: std::string_view, sheet_names: std::initializer_list<std::string_view>, header: bool = true) -> pluss::loader::LoadResult
```

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`arg2` : `std::nullptr_t`
    Pass `nullptr` here to request a multi-sheet Excel load result that includes every sheet.

`header` : `bool`
    Controls whether feature names are written to, or expected from, the file header row.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

`config_entries` : `std::initializer_list<pluss::loader::ConfigEntry>`
    Inline configuration entries used to build a lightweight loader configuration without naming a separate config object.

`options` : `const pluss::loader::ReadExcelOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

`sheet_index` : `std::int64_t`
    Zero-based worksheet position inside an Excel workbook.

`sheet_indices` : `const std::vector<std::int64_t>&`
    Zero-based worksheet positions to load together from one Excel workbook.

`sheet_name` : `std::string_view`
    Worksheet name to read from or write to inside an Excel workbook.

`sheet_names` : `const std::vector<std::string>&`
    Worksheet names to load together from one Excel workbook. This overload returns a multi-sheet load result instead of one frame.

#### Output

`DataFrame`
    Value produced by this call.

`pluss::loader::LoadResult`
    Multi-sheet Excel load bundle containing one frame per requested sheet.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Load the default worksheet from an Excel workbook.

```cpp
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx");
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 2. Load every worksheet from an Excel workbook into one multi-sheet load bundle.

```cpp
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx", nullptr, true);
```

##### Expected output

```text
Sheet1 -> [3 cs x 3 feat]
Sheet2 -> [3 cs x 3 feat]
```
Example 3. Load one Excel sheet with a reusable loader configuration object.

```cpp
pluss::loader::Config config{{"sheet_name", "Sheet 1"}};
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx", config);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 4. Load an Excel workbook while supplying inline configuration entries instead of a named config object.

```cpp
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx", {
    {"sheet_name", "Sheet 1"},
});
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 5. Load one Excel sheet with a typed Excel options object.

```cpp
pluss::loader::ReadExcelOptions options;
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx", options);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 6. Load one worksheet by its zero-based sheet index.

```cpp
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx", 0, true);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 7. Load several worksheets by zero-based sheet index from a standard integer container.

```cpp
std::vector<std::int64_t> sheets{0, 1};
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx", sheets, true);
```

##### Expected output

```text
Sheet1 -> [3 cs x 3 feat]
Sheet2 -> [3 cs x 3 feat]
```
Example 8. Load several worksheets by zero-based sheet index from a lightweight integer view.

```cpp
std::array<std::int64_t, 2> sheets{0, 1};
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx", sheets, true);
```

##### Expected output

```text
Sheet1 -> [3 cs x 3 feat]
Sheet2 -> [3 cs x 3 feat]
```
Example 9. Load several worksheets by zero-based sheet index using an inline integer list.

```cpp
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx", {0, 1}, true);
```

##### Expected output

```text
Sheet1 -> [3 cs x 3 feat]
Sheet2 -> [3 cs x 3 feat]
```
Example 10. Load one worksheet by its sheet name.

```cpp
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx", "Sheet 1", true);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 11. Load several worksheets by name from a standard string container.

```cpp
std::vector<std::string> sheets{"Sheet 1", "Sheet 2"};
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx", sheets, true);
```

##### Expected output

```text
Sheet1 -> [3 cs x 3 feat]
Sheet2 -> [3 cs x 3 feat]
```
Example 12. Load several worksheets by name from a lightweight string view.

```cpp
std::array<std::string_view, 2> sheets{"Sheet 1", "Sheet 2"};
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx", sheets, true);
```

##### Expected output

```text
Sheet1 -> [3 cs x 3 feat]
Sheet2 -> [3 cs x 3 feat]
```
Example 13. Load several worksheets by name using an inline list of sheet labels.

```cpp
auto out = pluss::table::DataFrame::load_excel("path/to/data.xlsx", {"Sheet 1", "Sheet 2"}, true);
```

##### Expected output

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
DataFrame::load_json(filepath: std::string_view) -> DataFrame

DataFrame::load_json(filepath: std::string_view, config: const pluss::loader::Config&) -> DataFrame

DataFrame::load_json(filepath: std::string_view, config_entries: std::initializer_list<pluss::loader::ConfigEntry>) -> DataFrame
```

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

`config_entries` : `std::initializer_list<pluss::loader::ConfigEntry>`
    Inline configuration entries used to build a lightweight loader configuration without naming a separate config object.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Load a JSON source with the default reader for that format.

```cpp
auto out = pluss::table::DataFrame::load_json("path/to/data.json");
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 2. Load a JSON source with a reusable loader configuration object.

```cpp
pluss::loader::Config config{{"ncases", 8}};
auto out = pluss::table::DataFrame::load_json("path/to/data.json", config);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 3. Load a JSON source while supplying inline configuration entries.

```cpp
auto out = pluss::table::DataFrame::load_json("path/to/data.json", {
    {"ncases", 8},
});
```

##### Expected output

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
DataFrame::load_parquet(filepath: std::string_view) -> DataFrame

DataFrame::load_parquet(filepath: std::string_view, config: const pluss::loader::Config&) -> DataFrame

DataFrame::load_parquet(filepath: std::string_view, config_entries: std::initializer_list<pluss::loader::ConfigEntry>) -> DataFrame
```

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

`config_entries` : `std::initializer_list<pluss::loader::ConfigEntry>`
    Inline configuration entries used to build a lightweight loader configuration without naming a separate config object.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Load a PARQUET source with the default reader for that format.

```cpp
auto out = pluss::table::DataFrame::load_parquet("path/to/data.parquet");
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 2. Load a PARQUET source with a reusable loader configuration object.

```cpp
pluss::loader::Config config{{"ncases", 8}};
auto out = pluss::table::DataFrame::load_parquet("path/to/data.parquet", config);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 3. Load a PARQUET source while supplying inline configuration entries.

```cpp
auto out = pluss::table::DataFrame::load_parquet("path/to/data.parquet", {
    {"ncases", 8},
});
```

##### Expected output

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
DataFrame::load_txt(filepath: std::string_view) -> DataFrame

DataFrame::load_txt(filepath: std::string_view, config: const pluss::loader::Config&) -> DataFrame

DataFrame::load_txt(filepath: std::string_view, config_entries: std::initializer_list<pluss::loader::ConfigEntry>) -> DataFrame
```

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

`config_entries` : `std::initializer_list<pluss::loader::ConfigEntry>`
    Inline configuration entries used to build a lightweight loader configuration without naming a separate config object.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Load a TXT source with the default reader for that format.

```cpp
auto out = pluss::table::DataFrame::load_txt("path/to/data.txt");
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 2. Load a TXT source with a reusable loader configuration object.

```cpp
pluss::loader::Config config{{"ncases", 8}};
auto out = pluss::table::DataFrame::load_txt("path/to/data.txt", config);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 3. Load a TXT source while supplying inline configuration entries.

```cpp
auto out = pluss::table::DataFrame::load_txt("path/to/data.txt", {
    {"ncases", 8},
});
```

##### Expected output

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
DataFrame::lt(other: const DataFrame&, strict: bool = true) -> DataFrame

DataFrame::lt(other: double, strict: bool = true) -> DataFrame
```

#### Notes

This produces a comparison-style result you can inspect directly or reuse as a mask.

#### Inputs

`other` : `const DataFrame&`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

`strict` : `bool`
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Compare each compatible cell using the `<` relation against another aligned frame.

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

##### Expected output

```text
id  sales
1   true
2   false
3   true
```
Example 2. Compare each compatible cell using the `<` relation against one broadcast scalar value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.lt(110);
```

##### Expected output

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

### `map`

#### API Signature

```cpp
DataFrame::map(fn: const std::function<ThreadValue(const ThreadValue&)>&, features: pluss::RangeView<std::string_view> = {}) -> DataFrame
```

#### Notes

Adds, replaces, filters, or rewrites part of the current object while preserving the surrounding alignment model. Methods ending with `_` perform the change in place.

#### Inputs

`fn` : `const std::function<ThreadValue(const ThreadValue&)>&`
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

`features` : `pluss::RangeView<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.map();
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `match`

#### API Signature

```cpp
DataFrame::match(other: const DataFrame&, options: const FrameMatchOptions& = {}) -> FrameMatch
```

#### Notes

Returns a pair of frames whose cases and features have been matched according to explicit Pluss-native matching rules.

#### Inputs

`other` : `const DataFrame&`
    Frame whose case labels and feature names should be matched with this frame.

`options` : `const FrameMatchOptions&`
    Matching policy for cases, features, and repeated case labels. Defaults to a stable union on both axes and occurrence-order pairing for repeats.

#### Output

`FrameMatch`
    Pair containing matched `left` and `right` frames with identical case and feature axes.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Pass an explicit options object to control the operation.

```cpp
auto left = pluss::table::DataFrame(
    {{"x", pluss::make_series({1.0, 2.0})},
     {"y", pluss::make_series({10.0, 20.0})}},
    pluss::table::Thread(pluss::make_note({"A", "C"})));
auto right = pluss::table::DataFrame(
    {{"y", pluss::make_series({100.0, 200.0})},
     {"z", pluss::make_series({7.0, 8.0})}},
    pluss::table::Thread(pluss::make_note({"B", "A"})));
auto matched = left.match(right);
```

##### Expected output

```text
matched.left.cases():
["A", "C", "B"]

matched.left.feature_names():
["x", "y", "z"]
```

#### Matching Rules

- `match(other)` matches both case labels and feature names in one call.
- The default `Unite` rule preserves the left order and appends labels found only on the right.
- `Common`, `KeepLeft`, and `KeepRight` select the intersection or one side explicitly.
- Repeated cases pair by occurrence order by default; choose `RepeatedCaseRule::Reject` when repeated labels should be an error.
- Frame-to-frame arithmetic and comparisons use the same default matching behavior automatically.
- Only case-key matching is materialized on the host; feature gathers and arithmetic remain on the source Series platform.


### `matmul`

#### API Signature

```cpp
DataFrame::matmul(other: const DataFrame&) -> DataFrame
```

#### Notes

Use this method through the public contract shown above.

#### Inputs

`other` : `const DataFrame&`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.matmul();
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `matrix`

#### API Signature

```cpp
DataFrame::matrix() -> MatrixValues
```

#### Notes

Packs same-kind features into matrix-shaped pluss blocks so lower-level numeric, text, or temporal kernels can consume them efficiently.

#### Inputs

`None`

#### Output

`MatrixValues`
    Grouped matrix blocks split by payload family (`Series`, `Note`, `Timeline`, and `TimeIndex`).

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Call the simplest overload.

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

##### Expected output

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

#### API Signature

```cpp
DataFrame::max(axis: Axis = Axis::Feature, skipna: bool = true) -> Thread
```

#### Notes

Computes per-feature maxima for the selected numeric features.

#### Inputs

`axis` : `Axis`
    Axis selector. On `DataFrame`, `0` usually means rows/cases and `1` means features/columns.

`skipna` : `bool`
    When `true`, missing values are skipped during the reduction. When `false`, missing state can affect the result.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Compute `max` for each numeric column in the frame.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.max(false);
```

##### Expected output

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

#### API Signature

```cpp
DataFrame::mean(axis: Axis = Axis::Feature, skipna: bool = true) -> Thread
```

#### Notes

Computes per-feature means for the selected numeric features.

#### Inputs

`axis` : `Axis`
    Axis selector. On `DataFrame`, `0` usually means rows/cases and `1` means features/columns.

`skipna` : `bool`
    When `true`, missing values are skipped during the reduction. When `false`, missing state can affect the result.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Compute `mean` for each numeric column in the frame.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.mean(false);
```

##### Expected output

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
DataFrame::mean_by(group_feature: std::string_view, value_feature: std::string_view, top_n: std::int64_t = 0, min_count: std::int64_t = 1, dropna_group: bool = true, render: bool = false) -> DataFrameView
```

#### Notes

Groups by one feature and returns the mean of another feature within each group.

#### Inputs

`group_feature` : `std::string_view`
    Feature whose distinct values define the groups for grouped summaries such as `mean_by(...)`.

`value_feature` : `std::string_view`
    Feature whose values populate pivot cells or grouped summaries.

`top_n` : `std::int64_t`
    Optional cap on how many grouped rows to keep after sorting. Use `0` to keep every group.

`min_count` : `std::int64_t`
    Minimum non-null observations required before a grouped or rolling summary is considered valid.

`dropna_group` : `bool`
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

#### Output

`DataFrameView`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Choose whether the returned object should auto-render in display contexts.

```cpp
auto df = pluss::table::DataFrame({
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema"}))},
    {"product", pluss::table::Thread(pluss::make_note({"A", "B", "A", "B"}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0, 150.0}))},
});

auto out = df.mean_by("city", "sales", 3, 1, true, false);
```

##### Expected output

```text
city    sales_mean
Accra   110.0
Tema    120.0
```

### `median`

#### API Signature

```cpp
DataFrame::median(axis: Axis = Axis::Feature, skipna: bool = true) -> Thread
```

#### Notes

Use this method through the public contract shown above.

#### Inputs

`axis` : `Axis`
    Axis selector. On `DataFrame`, `0` usually means rows/cases and `1` means features/columns.

`skipna` : `bool`
    When `true`, missing values are skipped during the reduction. When `false`, missing state can affect the result.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Compute `median` for each numeric column in the frame.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.median(false);
```

##### Expected output

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

#### API Signature

```cpp
DataFrame::min(axis: Axis = Axis::Feature, skipna: bool = true) -> Thread
```

#### Notes

Computes per-feature minima for the selected numeric features.

#### Inputs

`axis` : `Axis`
    Axis selector. On `DataFrame`, `0` usually means rows/cases and `1` means features/columns.

`skipna` : `bool`
    When `true`, missing values are skipped during the reduction. When `false`, missing state can affect the result.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Compute `min` for each numeric column in the frame.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.min(false);
```

##### Expected output

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
DataFrame::mod(other: const DataFrame&) -> DataFrame

DataFrame::mod(other: double) -> DataFrame
```

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### Inputs

`other` : `const DataFrame&`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Compute the modulus of each compatible numeric cell against another aligned frame.

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

##### Expected output

```text
id  sales
1   100.0
0    10.0
3    90.0
```
Example 2. Compute the modulus of each compatible numeric cell against one broadcast scalar value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.mod(10);
```

##### Expected output

```text
id  sales  city
1   0.0    Accra
2   0.0    Tema
3   0.0    Kumasi
```

### `mode`

#### API Signature

```cpp
DataFrame::mode(drop_nulls: bool = true) -> DataFrame
```

#### Notes

Use this method through the public contract shown above.

#### Inputs

`drop_nulls` : `bool`
    When `true`, missing values do not participate while the most-frequent value is selected.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.mode();
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `mul`

#### API Signatures

```cpp
DataFrame::mul(other: const DataFrame&) -> DataFrame

DataFrame::mul(other: double) -> DataFrame
```

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### Inputs

`other` : `const DataFrame&`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Multiply each compatible numeric cell against another aligned frame.

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

##### Expected output

```text
id  sales
2   11000.0
4   13200.0
12   9000.0
```
Example 2. Multiply each compatible numeric cell against one broadcast scalar value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.mul(10);
```

##### Expected output

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
DataFrame::ncases() -> std::int64_t
```

#### Notes

Use this method through the public contract shown above.

#### Inputs

`None`

#### Output

`std::int64_t`
    Integer count, size, or positional result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.ncases();
```

##### Expected output

```text
3
```

### `ndistinct`

#### API Signature

```cpp
DataFrame::ndistinct(features: pluss::RangeView<std::string_view> = {}, dropna: bool = true) -> Thread
```

#### Notes

Use this method through the public contract shown above.

#### Inputs

`features` : `pluss::RangeView<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`dropna` : `bool`
    When `true`, null grouping keys or null values are excluded where the method supports that policy. When `false`, null participates instead of being removed.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.ndistinct();
```

##### Expected output

```text
The call completes and returns the contract described under Output.
```

### `ne`

#### API Signatures

```cpp
DataFrame::ne(other: const DataFrame&) -> DataFrame

DataFrame::ne(other: double) -> DataFrame
```

#### Notes

This produces a comparison-style result you can inspect directly or reuse as a mask.

#### Inputs

`other` : `const DataFrame&`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Compare each compatible cell for inequality against another aligned frame.

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

##### Expected output

```text
id  sales
1   true
2   true
3   false
```
Example 2. Compare each compatible cell for inequality against one broadcast scalar value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.ne(10);
```

##### Expected output

```text
id  sales
1   true
2   true
3   false
```

### `nfeats`

#### API Signature

```cpp
DataFrame::nfeats() -> std::int64_t
```

#### Notes

Use this method through the public contract shown above.

#### Inputs

`None`

#### Output

`std::int64_t`
    Integer count, size, or positional result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.nfeats();
```

##### Expected output

```text
3
```

### `nfeatures`

#### API Signature

```cpp
DataFrame::nfeatures() -> std::int64_t
```

#### Notes

Returns a size/count summary for the current object without changing it.

#### Inputs

`None`

#### Output

`std::int64_t`
    Integer count, size, or positional result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.nfeatures();
```

##### Expected output

```text
3
```

### `note`

#### API Signatures

```cpp
DataFrame::note(feature: std::string_view) -> DataFrameNoteAccessor

DataFrame::note(features: pluss::RangeView<std::string_view> = {}) -> DataFrameNoteAccessor
```

#### Notes

Use this method through the public contract shown above.

#### Inputs

`feature` : `std::string_view`
    Name of the source feature used by the operation.

`features` : `pluss::RangeView<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

#### Output

`DataFrameNoteAccessor`
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

auto out = df.note();
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 2. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.note();
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `note_features`

#### API Signature

```cpp
DataFrame::note_features() -> DataFrame
```

#### Notes

Returns only the note-backed features.

#### Inputs

`None`

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.note_features();
```

##### Expected output

```text
city
Accra
Tema
Kumasi
```

### `notnull`

#### API Signature

```cpp
DataFrame::notnull(features: pluss::RangeView<std::string_view> = {}) -> DataFrame
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`features` : `pluss::RangeView<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Work with a selected feature subset.

```cpp
auto sales = pluss::table::Thread(pluss::make_series({100.0, 0.0, 90.0}))
                 .with_validity_mask(pluss::make_series({true, false, true}, pluss::ScalarType::Bool));
auto df = pluss::table::DataFrame({
    {"sales", sales},
});

auto out = df.notnull({"sales"});
```

##### Expected output

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


## O

### `observe`

#### API Signatures

```cpp
DataFrame::observe(row: std::int64_t, render: bool = false) -> DataFrameView

DataFrame::observe(start: std::int64_t, stop: std::int64_t, render: bool = false) -> DataFrameView
```

#### Notes

Returns a `DataFrameView` over a positional row window so you can preview rows from the middle of a frame without materializing a new working copy.

#### Inputs

`row` : `std::int64_t`
    Zero-based row position to preview as a one-row `DataFrameView`.

`render` : `bool`
    When `true`, the preview may auto-render in temporary terminal contexts. Pass `false` when you want to call `.show()` yourself.

`start` : `std::int64_t`
    Inclusive zero-based starting row position of the preview window.

`stop` : `std::int64_t`
    Exclusive zero-based ending row position of the preview window.

#### Output

`DataFrameView`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Preview one row without changing the underlying frame.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto preview = df.observe(1, false);
preview.show();
```

##### Expected output

```text
id  sales  city
2   120.0  Tema
```
Example 2. Preview a contiguous row window without changing the underlying frame.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto preview = df.observe(0, 2, false);
preview.show();
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
```

#### Practical Behavior

- `observe(...)` returns a `DataFrameView`, so it is the preview-oriented row window helper alongside `head(...)` and `tail(...)`.
- It uses positional row coordinates, not case-label lookup.
- Use `observe(start, stop).show()` when you want a middle slice without materializing a bespoke helper or mutating the source frame.


## P

### `percentiles`

#### API Signature

```cpp
DataFrame::percentiles(points: pluss::RangeView<double> = {0.25, 0.5, 0.75}, min_count: std::int64_t = 1) -> DataFrame
```

#### Notes

Computes requested numeric distribution points for every numeric feature.

#### Inputs

`points` : `pluss::RangeView<scalar>`
    Requested evaluation-point count for density-style plots. Higher values make smoother curves at higher compute cost.

`min_count` : `std::int64_t`
    Minimum non-null observations required before a grouped or rolling summary is considered valid.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.percentiles();
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `pivot`

#### API Signature

```cpp
DataFrame::pivot(case_feature: std::string_view, feature_feature: std::string_view, value_feature: std::string_view, dropna_keys: bool = true, render: bool = false) -> DataFrameView
```

#### Notes

Reshapes long-form rows into a wider table indexed by one feature and expanded by another.

#### Inputs

`case_feature` : `std::string_view`
    Feature whose distinct values become the output row groups in the pivoted result.

`feature_feature` : `std::string_view`
    Feature whose distinct values become new output columns in pivot-style reshaping.

`value_feature` : `std::string_view`
    Feature whose values populate pivot cells or grouped summaries.

`dropna_keys` : `bool`
    When `true`, rows with null pivot/group keys are excluded before reshaping.

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

#### Output

`DataFrameView`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Choose whether the returned object should auto-render in display contexts.

```cpp
auto df = pluss::table::DataFrame({
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema"}))},
    {"product", pluss::table::Thread(pluss::make_note({"A", "B", "A", "B"}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0, 150.0}))},
});

auto out = df.pivot("city", "product", "sales", true, false);
```

##### Expected output

```text
city   A      B
Accra  100.0  120.0
Tema    90.0  150.0
```

### `pivot_table`

#### API Signature

```cpp
DataFrame::pivot_table(case_feature: std::string_view, feature_feature: std::string_view, value_feature: std::string_view, op: GroupOp = GroupOp::Mean, dropna_keys: bool = true, render: bool = false) -> DataFrameView
```

#### Notes

Builds a pivoted summary table using an aggregation function when multiple rows map to the same cell.

#### Inputs

`case_feature` : `std::string_view`
    Feature whose distinct values become the output row groups in the pivoted summary result.

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

#### Output

`DataFrameView`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Choose whether the returned object should auto-render in display contexts.

```cpp
auto df = pluss::table::DataFrame({
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema"}))},
    {"product", pluss::table::Thread(pluss::make_note({"A", "B", "A", "B"}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0, 150.0}))},
});

auto out = df.pivot_table("city", "product", "sales", pluss::table::DataFrame::GroupOp::Mean, true, false);
```

##### Expected output

```text
city   A      B
Accra  100.0  120.0
Tema    90.0  150.0
```

### `place`

#### API Signatures

```cpp
DataFrame::place(name: std::string, feature: const Thread&, anchor_feature: std::string_view, after: bool = false) -> DataFrame

DataFrame::place(name: std::string_view, anchor_feature: std::string_view, after: bool = false) -> DataFrame
```

#### Notes

Moves one existing feature relative to an anchor feature, or inserts a brand-new feature directly before or after that anchor.

#### Inputs

`name` : `std::string`
    Feature name to insert or move.

`feature` : `const Thread&`
    Full thread payload to insert at the target position. Its length must match the frame case count.

`anchor_feature` : `std::string_view`
    Reference feature used to place a newly inserted or moved feature before or after an existing column.

`after` : `bool`
    When `false`, placement happens before the anchor. When `true`, placement happens after the anchor.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Insert a new feature relative to an anchor feature without rebuilding the whole schema.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.place("profit", pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0})), "sales", true);
```

##### Expected output

```text
id  sales  profit  city
1   100.0  40.0    Accra
2   120.0  35.0    Tema
3    90.0  30.0    Kumasi
```
Example 2. Move one existing feature relative to an anchor feature.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.place("city", "id", false);
```

##### Expected output

```text
city    id  sales
Accra   1   100.0
Tema    2   120.0
Kumasi  3    90.0
```

#### Practical Behavior

- `place(name, feature, anchor, after)` inserts a brand-new feature relative to an existing anchor.
- `place(name, anchor, after)` moves an existing feature without forcing you to restate the full feature order.
- When `after=false`, placement happens before the anchor. When `after=true`, placement happens after the anchor.


### `plot`

#### API Signatures

```cpp
DataFrame::plot() -> DataFramePlotAccessor

DataFrame::plot(options: const DataFramePlotOptions&) -> DataFramePlotAccessor
```

#### Notes

This opens the plotting accessor for the current data object.

#### Inputs

`options` : `const DataFramePlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`DataFramePlotAccessor`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Create the plotting accessor with default plot options.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto fig = df.plot().line("sales");
```

##### Expected output

```text
Figure(line/scatter/bar/hist)
```
Example 2. Create the plotting accessor with an explicit plot-options object.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

pluss::table::DataFramePlotOptions options;
options.default_x_feature = "id";
auto fig = df.plot(options).line("sales");
```

##### Expected output

```text
Figure(line/scatter/bar/hist)
```

### `plot3d`

#### API Signatures

```cpp
DataFrame::plot3d() -> DataFramePlot3DAccessor

DataFrame::plot3d(options: const DataFramePlotOptions&) -> DataFramePlot3DAccessor
```

#### Notes

Use this method through the public contract shown above.

#### Inputs

`options` : `const DataFramePlotOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`DataFramePlot3DAccessor`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Create the 3D plotting accessor with default plot options.

```cpp
auto df = pluss::table::DataFrame({
    {"x", pluss::make_series({1.0, 2.0, 3.0})},
    {"y", pluss::make_series({4.0, 3.0, 2.0})},
    {"z", pluss::make_series({2.0, 5.0, 4.0})},
});

auto fig = df.plot3d().scatter("x", "y", "z");
```

##### Expected output

```text
Figure(scatter3d/line3d)
```
Example 2. Create the 3D plotting accessor with an explicit plot-options object.

```cpp
auto df = pluss::table::DataFrame({
    {"x", pluss::make_series({1.0, 2.0, 3.0})},
    {"y", pluss::make_series({4.0, 3.0, 2.0})},
    {"z", pluss::make_series({2.0, 5.0, 4.0})},
});

pluss::table::DataFramePlotOptions options;
options.width = 900.0;
auto fig = df.plot3d(options).scatter("x", "y", "z");
```

##### Expected output

```text
Figure(scatter3d/line3d)
```

### `pow`

#### API Signatures

```cpp
DataFrame::pow(other: const DataFrame&) -> DataFrame

DataFrame::pow(other: double) -> DataFrame
```

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### Inputs

`other` : `const DataFrame&`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Raise each compatible numeric cell against another aligned frame.

```cpp
auto left = pluss::table::DataFrame({
    {"a", pluss::table::Thread(pluss::make_series({2.0, 3.0, 4.0}))},
    {"b", pluss::table::Thread(pluss::make_series({3.0, 2.0, 5.0}))},
});
auto right = pluss::table::DataFrame({
    {"a", pluss::table::Thread(pluss::make_series({3.0, 2.0, 1.0}))},
    {"b", pluss::table::Thread(pluss::make_series({2.0, 3.0, 2.0}))},
});

auto out = left.pow(right);
```

##### Expected output

```text
a    b
8.0  9.0
9.0  8.0
4.0  25.0
```
Example 2. Raise each compatible numeric cell against one broadcast scalar value.

```cpp
auto df = pluss::table::DataFrame({
    {"a", pluss::table::Thread(pluss::make_series({1.0, 2.0, 3.0}))},
    {"b", pluss::table::Thread(pluss::make_series({2.0, 4.0, 5.0}))},
});

auto out = df.pow(2);
```

##### Expected output

```text
a    b
1.0  4.0
4.0  16.0
9.0  25.0
```

### `prep_sql`

#### API Signatures

```cpp
DataFrame::prep_sql(statement: std::string_view) -> DataFrameSqlPrepared

DataFrame::prep_sql(statement: std::string_view, options: SqlOptions) -> DataFrameSqlPrepared
```

#### Notes

Use this when the transformation is easier to express as SQL than as a chained sequence of table verbs.

#### Inputs

`statement` : `std::string_view`
    SQL statement text to prepare against the current frame.

`options` : `SqlOptions`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`DataFrameSqlPrepared`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Prepare a parameterized SQL statement with default SQL options.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto stmt = df.prep_sql("SELECT id, sales FROM self WHERE sales > {min_sales}");
```

##### Expected output

```text
SQL accessor / prepared statement created
```
Example 2. Prepare a parameterized SQL statement with an explicit SQL options object.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

pluss::table::DataFrame::SqlOptions options;
auto stmt = df.prep_sql("SELECT id, sales FROM self WHERE sales > {min_sales}", options);
```

##### Expected output

```text
SQL accessor / prepared statement created
```

### `prod`

#### API Signature

```cpp
DataFrame::prod(axis: Axis = Axis::Feature, skipna: bool = true) -> Thread
```

#### Notes

Computes per-feature or per-row products across numeric data.

#### Inputs

`axis` : `Axis`
    Reduction axis. `Axis::Feature` computes one product per numeric feature, while `Axis::Case` computes one product per row across numeric features.

`skipna` : `bool`
    When `true`, missing values are skipped during the reduction. When `false`, missing state can affect the result.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Compute products across rows instead of returning one product per numeric feature.

```cpp
auto df = pluss::table::DataFrame({
    {"a", pluss::table::Thread(pluss::make_series({1.0, 2.0, 3.0}))},
    {"b", pluss::table::Thread(pluss::make_series({2.0, 4.0, 5.0}))},
});

auto out = df.prod(1, true);
```

##### Expected output

```text
[2.0, 8.0, 15.0]
```

#### Axis Rules

- `prod(render=false)` returns a compact summary view with one product per numeric feature.
- `prod(axis=0)` returns a numeric thread with one product per numeric feature.
- `prod(axis=1)` returns a numeric thread with one product per row across numeric features.


### `put`

#### API Signatures

```cpp
DataFrame::put(feature: std::string_view, bool_mask: const pluss::Series&, value: const char*) -> DataFrame

DataFrame::put(feature: std::string_view, bool_mask: const pluss::Series&, value: const Thread&) -> DataFrame

DataFrame::put(feature: std::string_view, bool_mask: const pluss::Series&, value: double) -> DataFrame

DataFrame::put(feature: std::string_view, bool_mask: const pluss::Series&, value: pluss::null_t) -> DataFrame

DataFrame::put(feature: std::string_view, bool_mask: const pluss::Series&, value: std::string_view) -> DataFrame

DataFrame::put(feature: std::string_view, bool_mask: const Thread&, value: const char*) -> DataFrame

DataFrame::put(feature: std::string_view, bool_mask: const Thread&, value: const Thread&) -> DataFrame

DataFrame::put(feature: std::string_view, bool_mask: const Thread&, value: double) -> DataFrame

DataFrame::put(feature: std::string_view, bool_mask: const Thread&, value: pluss::null_t) -> DataFrame

DataFrame::put(feature: std::string_view, bool_mask: const Thread&, value: std::string_view) -> DataFrame
```

#### Notes

Writes a replacement value only where a boolean mask is true and returns the updated frame.

#### Inputs

`feature` : `std::string_view`
    Feature name to rewrite.

`bool_mask` : `const pluss::Series&`
    Boolean row mask. `true` keeps or rewrites the corresponding row, while `false` leaves it out or leaves the original value unchanged depending on the method.

`value` : `const char*`
    Replacement scalar, string, null, or thread written only where the mask is `true`.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Rewrite selected rows using a boolean Series mask and a text literal as the replacement value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = pluss::make_series({false, true, false}, pluss::ScalarType::Bool);
auto out = df.put("city", mask, "Tamale");
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   999.0  Tema
3    90.0  Kumasi
```
Example 2. Rewrite selected rows using a boolean Series mask and another full Thread as the replacement values.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = pluss::make_series({false, true, false}, pluss::ScalarType::Bool);
auto repl = pluss::table::Thread(pluss::make_series({500.0, 999.0, 500.0}));
auto out = df.put("sales", mask, repl);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   999.0  Tema
3    90.0  Kumasi
```
Example 3. Rewrite selected rows using a boolean Series mask and a scalar numeric replacement value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = pluss::make_series({false, true, false}, pluss::ScalarType::Bool);
auto out = df.put("sales", mask, 999.0);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   999.0  Tema
3    90.0  Kumasi
```
Example 4. Rewrite selected rows using a boolean Series mask and `pluss::null` to mark the matched cells as missing.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = pluss::make_series({false, true, false}, pluss::ScalarType::Bool);
auto out = df.put("sales", mask, pluss::null);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   999.0  Tema
3    90.0  Kumasi
```
Example 5. Rewrite selected rows using a boolean Series mask and a text literal as the replacement value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = pluss::make_series({false, true, false}, pluss::ScalarType::Bool);
auto out = df.put("city", mask, std::string_view("Tamale"));
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   999.0  Tema
3    90.0  Kumasi
```
Example 6. Rewrite selected rows using a boolean Thread mask and a text literal as the replacement value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = df.features("id") == 2.0;
auto out = df.put("city", mask, "Tamale");
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   999.0  Tema
3    90.0  Kumasi
```
Example 7. Rewrite selected rows using a boolean Thread mask and another full Thread as the replacement values.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = df.features("id") == 2.0;
auto repl = pluss::table::Thread(pluss::make_series({500.0, 999.0, 500.0}));
auto out = df.put("sales", mask, repl);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   999.0  Tema
3    90.0  Kumasi
```
Example 8. Rewrite selected rows using a boolean Thread mask and a scalar numeric replacement value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = df.features("id") == 2.0;
auto out = df.put("sales", mask, 999.0);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   999.0  Tema
3    90.0  Kumasi
```
Example 9. Rewrite selected rows using a boolean Thread mask and `pluss::null` to mark the matched cells as missing.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = df.features("id") == 2.0;
auto out = df.put("sales", mask, pluss::null);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   999.0  Tema
3    90.0  Kumasi
```
Example 10. Rewrite selected rows using a boolean Thread mask and a text literal as the replacement value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = df.features("id") == 2.0;
auto out = df.put("city", mask, std::string_view("Tamale"));
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   999.0  Tema
3    90.0  Kumasi
```

#### Practical Behavior

- `put(feature, mask, value)` rewrites only the rows where the mask evaluates true.
- The mask must match the frame case count.
- Use `put(...)` when you want a new frame result, and `put_(...)` when you want the same rewrite in place.


### `put_`

#### API Signatures

```cpp
DataFrame::put_(feature: std::string_view, bool_mask: const pluss::Series&, value: const char*) -> DataFrame&

DataFrame::put_(feature: std::string_view, bool_mask: const pluss::Series&, value: const Thread&) -> DataFrame&

DataFrame::put_(feature: std::string_view, bool_mask: const pluss::Series&, value: double) -> DataFrame&

DataFrame::put_(feature: std::string_view, bool_mask: const pluss::Series&, value: pluss::null_t) -> DataFrame&

DataFrame::put_(feature: std::string_view, bool_mask: const pluss::Series&, value: std::string_view) -> DataFrame&

DataFrame::put_(feature: std::string_view, bool_mask: const Thread&, value: const char*) -> DataFrame&

DataFrame::put_(feature: std::string_view, bool_mask: const Thread&, value: const Thread&) -> DataFrame&

DataFrame::put_(feature: std::string_view, bool_mask: const Thread&, value: double) -> DataFrame&

DataFrame::put_(feature: std::string_view, bool_mask: const Thread&, value: pluss::null_t) -> DataFrame&

DataFrame::put_(feature: std::string_view, bool_mask: const Thread&, value: std::string_view) -> DataFrame&
```

#### Notes

Writes a replacement value only where a boolean mask is true and mutates the frame in place.

#### Inputs

`feature` : `std::string_view`
    Feature name to rewrite in place.

`bool_mask` : `const pluss::Series&`
    Boolean row mask. `true` keeps or rewrites the corresponding row, while `false` leaves it out or leaves the original value unchanged depending on the method.

`value` : `const char*`
    Replacement scalar, string, null, or thread written in place only where the mask is `true`.

#### Output

`DataFrame&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Rewrite rows in place using a boolean Series mask and a text literal as the replacement value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = pluss::make_series({false, true, false}, pluss::ScalarType::Bool);
df.put_("city", mask, "Tamale");
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tamale
3    90.0  Kumasi
```
Example 2. Rewrite rows in place using a boolean Series mask and another full Thread as the replacement values.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = pluss::make_series({false, true, false}, pluss::ScalarType::Bool);
auto repl = pluss::table::Thread(pluss::make_series({500.0, 999.0, 500.0}));
df.put_("sales", mask, repl);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tamale
3    90.0  Kumasi
```
Example 3. Rewrite rows in place using a boolean Series mask and a scalar numeric replacement value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = pluss::make_series({false, true, false}, pluss::ScalarType::Bool);
df.put_("sales", mask, 999.0);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tamale
3    90.0  Kumasi
```
Example 4. Rewrite rows in place using a boolean Series mask and `pluss::null` to mark the matched cells as missing.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = pluss::make_series({false, true, false}, pluss::ScalarType::Bool);
df.put_("sales", mask, pluss::null);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tamale
3    90.0  Kumasi
```
Example 5. Rewrite rows in place using a boolean Series mask and a text literal as the replacement value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = pluss::make_series({false, true, false}, pluss::ScalarType::Bool);
df.put_("city", mask, std::string_view("Tamale"));
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tamale
3    90.0  Kumasi
```
Example 6. Rewrite rows in place using a boolean Thread mask and a text literal as the replacement value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = df.features("id") == 2.0;
df.put_("city", mask, "Tamale");
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tamale
3    90.0  Kumasi
```
Example 7. Rewrite rows in place using a boolean Thread mask and another full Thread as the replacement values.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = df.features("id") == 2.0;
auto repl = pluss::table::Thread(pluss::make_series({500.0, 999.0, 500.0}));
df.put_("sales", mask, repl);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tamale
3    90.0  Kumasi
```
Example 8. Rewrite rows in place using a boolean Thread mask and a scalar numeric replacement value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = df.features("id") == 2.0;
df.put_("sales", mask, 999.0);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tamale
3    90.0  Kumasi
```
Example 9. Rewrite rows in place using a boolean Thread mask and `pluss::null` to mark the matched cells as missing.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = df.features("id") == 2.0;
df.put_("sales", mask, pluss::null);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tamale
3    90.0  Kumasi
```
Example 10. Rewrite rows in place using a boolean Thread mask and a text literal as the replacement value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = df.features("id") == 2.0;
df.put_("city", mask, std::string_view("Tamale"));
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tamale
3    90.0  Kumasi
```

#### Practical Behavior

- `put_(feature, mask, value)` rewrites only the rows where the mask evaluates true.
- The mask must match the frame case count.
- Use `put(...)` when you want a new frame result, and `put_(...)` when you want the same rewrite in place.


## Q

### `query`

#### API Signature

```cpp
DataFrame::query(expression: std::string_view) -> DataFrame
```

#### Notes

Keeps only the rows that satisfy the supplied filter expression or mask-producing query.

#### Inputs

`expression` : `std::string_view`
    Boolean filter expression written against feature names, such as `"sales > 95"` or `"city == \"Accra\""`. Rows where the expression evaluates true are kept.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.query("sales > 95");
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
```

#### Practical Behavior

- `query(expression)` keeps rows where the expression evaluates true.
- Feature names are referenced directly inside the expression string.
- String comparisons should use quoted values, especially when feature names contain spaces.


## R

### `radd`

#### API Signature

```cpp
DataFrame::radd(other: double) -> DataFrame
```

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### Inputs

`other` : `scalar`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"a", pluss::table::Thread(pluss::make_series({1.0, 2.0, 3.0}))},
    {"b", pluss::table::Thread(pluss::make_series({2.0, 4.0, 5.0}))},
});

auto out = df.radd(10);
```

##### Expected output

```text
a    b
11.0  12.0
12.0  14.0
13.0  15.0
```

### `rank`

#### API Signature

```cpp
DataFrame::rank(ties: RankTieRule = RankTieRule::Average, nulls: RankNullRule = RankNullRule::Keep, ascending: bool = true, percentage: bool = false) -> DataFrame
```

#### Notes

Use this method through the public contract shown above.

#### Inputs

`ties` : `RankTieRule`
    Tie policy for equal values: `Average`, `Lowest`, `Highest`, `First`, or `Dense`.

`nulls` : `RankNullRule`
    Null ranking policy: `Keep` leaves null ranks missing, `Top` ranks them first, and `Bottom` ranks them last.

`ascending` : `bool`
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

`percentage` : `bool`
    When `true`, divide each rank by the number of ranked observations.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.rank();
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `rdiv`

#### API Signature

```cpp
DataFrame::rdiv(other: double) -> DataFrame
```

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### Inputs

`other` : `scalar`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"a", pluss::table::Thread(pluss::make_series({1.0, 2.0, 3.0}))},
    {"b", pluss::table::Thread(pluss::make_series({2.0, 4.0, 5.0}))},
});

auto out = df.rdiv(10);
```

##### Expected output

```text
a         b
10.0      5.0
5.0       2.5
3.333333  2.0
```

### `rename`

#### API Signatures

```cpp
DataFrame::rename(feature_names: pluss::RangeView<std::string_view>) -> DataFrame

DataFrame::rename(feature_names: std::initializer_list<const char*>) -> DataFrame

DataFrame::rename(old_name: std::string_view, new_name: std::string) -> DataFrame

DataFrame::rename(renames: pluss::RangeView<std::pair<std::string_view, std::string_view>>) -> DataFrame

DataFrame::rename(renames: std::initializer_list<std::pair<std::string_view, std::string_view>>) -> DataFrame
```

#### Notes

Renames one feature, several features, or the full feature axis without changing row values.

#### Inputs

`feature_names` : `pluss::RangeView<std::string_view>`
    Full replacement feature-label list in final left-to-right order. The count must match the current number of features.

`old_name` : `std::string_view`
    Existing feature name to rename.

`new_name` : `std::string`
    Replacement feature name.

`renames` : `pluss::RangeView<std::pair<std::string_view, std::string_view>>`
    Collection of `(old_name, new_name)` pairs applied in one rename pass.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.rename("sales", "revenue");
```

##### Expected output

```text
id  revenue  city
1   100.0    Accra
2   120.0    Tema
3    90.0    Kumasi
```
Example 2. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.rename("sales", "revenue");
```

##### Expected output

```text
id  revenue  city
1   100.0    Accra
2   120.0    Tema
3    90.0    Kumasi
```
Example 3. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.rename("sales", "revenue");
```

##### Expected output

```text
id  revenue  city
1   100.0    Accra
2   120.0    Tema
3    90.0    Kumasi
```
Example 4. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.rename("sales", "revenue");
```

##### Expected output

```text
id  revenue  city
1   100.0    Accra
2   120.0    Tema
3    90.0    Kumasi
```
Example 5. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.rename("sales", "revenue");
```

##### Expected output

```text
id  revenue  city
1   100.0    Accra
2   120.0    Tema
3    90.0    Kumasi
```

### `rename_`

#### API Signature

```cpp
DataFrame::rename_(old_name: std::string_view, new_name: std::string) -> DataFrame&
```

#### Notes

Renames one feature in place without changing its values.

#### Inputs

`old_name` : `std::string_view`
    Existing feature name to rename in place.

`new_name` : `std::string`
    Replacement feature name written back in place.

#### Output

`DataFrame&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.rename_("sales", "revenue");
```

##### Expected output

```text
id  revenue  city
1   100.0    Accra
2   120.0    Tema
3    90.0    Kumasi
```

### `rename_levels`

#### API Signatures

```cpp
DataFrame::rename_levels(renames: pluss::RangeView<LevelRename>) -> DataFrame

DataFrame::rename_levels(renames: std::initializer_list<LevelRename>) -> DataFrame
```

#### Notes

Use this method through the public contract shown above.

#### Inputs

`renames` : `pluss::RangeView<LevelRename>`
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.rename_levels();
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 2. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.rename_levels();
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `reorder`

#### API Signatures

```cpp
DataFrame::reorder(names: pluss::RangeView<std::string_view>) -> DataFrame

DataFrame::reorder(spec: const ReorderSpec&) -> DataFrame

DataFrame::reorder(specs: pluss::RangeView<ReorderSpec>) -> DataFrame

DataFrame::reorder(specs: std::initializer_list<ReorderSpec>) -> DataFrame
```

#### Notes

Moves selected features into a new relative position while preserving row data and the untouched feature order.

#### Inputs

`names` : `pluss::RangeView<std::string_view>`
    Column names to move together to the front while preserving the relative order of every column not listed here.

`spec` : `const ReorderSpec&`
    One partial-order move. Use this when one move instruction is enough.

`specs` : `pluss::RangeView<ReorderSpec>`
    Several partial-order moves applied in the exact order you pass them. Each move sees the column order produced by the previous move.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Move the named columns to the front while preserving the relative order of every untouched column.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.reorder({"city"});
```

##### Expected output

```text
city    id  sales
Accra   1   100.0
Tema    2   120.0
Kumasi  3    90.0
```
Example 2. Move selected columns to the front, back, or around an anchor while preserving the rest.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto spec = pluss::table::DataFrame::ReorderSpec::move("sales").after("city");
auto out = df.reorder(spec);
```

##### Expected output

```text
id  city    sales
1   Accra   100.0
2   Tema    120.0
3   Kumasi   90.0
```
Example 3. Apply a reusable multi-step reorder plan stored in a standard container.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

using Move = pluss::table::DataFrame::ReorderSpec;
std::vector<Move> plan{
    Move::move({"city", "sales"}).first(),
    Move::move("id").after("sales"),
};
auto out = df.reorder(plan);
```

##### Expected output

```text
city    sales  id
Accra   100.0  1
Tema    120.0  2
Kumasi  90.0   3
```
Example 4. Apply an inline multi-step reorder plan that moves only the columns you care about.

```cpp
auto df = pluss::table::DataFrame({
    {"col_1", pluss::make_series({1.0, 101.0})},
    {"col_2", pluss::make_series({2.0, 102.0})},
    {"col_3", pluss::make_series({3.0, 103.0})},
    {"col_4", pluss::make_series({4.0, 104.0})},
    {"col_5", pluss::make_series({5.0, 105.0})},
    {"col_6", pluss::make_series({6.0, 106.0})},
    {"col_7", pluss::make_series({7.0, 107.0})},
    {"col_8", pluss::make_series({8.0, 108.0})},
    {"col_9", pluss::make_series({9.0, 109.0})},
    {"col_10", pluss::make_series({10.0, 110.0})},
    {"col_15", pluss::make_series({15.0, 115.0})},
    {"col_16", pluss::make_series({16.0, 116.0})},
    {"col_17", pluss::make_series({17.0, 117.0})},
    {"col_20", pluss::make_series({20.0, 120.0})},
    {"col_21", pluss::make_series({21.0, 121.0})},
    {"col_22", pluss::make_series({22.0, 122.0})},
    {"col_23", pluss::make_series({23.0, 123.0})},
    {"col_24", pluss::make_series({24.0, 124.0})},
    {"col_25", pluss::make_series({25.0, 125.0})},
});

using Move = pluss::table::DataFrame::ReorderSpec;
auto out = df.reorder({
    Move::move({"col_25", "col_10"}).first(),
    Move::move("col_20").before("col_5"),
    Move::move({"col_15", "col_16", "col_17"}).after("col_8"),
    Move::move_regex("^col_2[1-4]$").last(),
});
```

##### Expected output

```text
out.feature_names():
["col_25", "col_10", "col_1", "col_2", "col_3", "col_4", "col_20", "col_5", "col_6", "col_7", "col_8", "col_15", "col_16", "col_17", "col_9", "col_21", "col_22", "col_23", "col_24"]

out.head():
col_25  col_10  col_1  col_2  col_3  col_4  col_20  col_5  col_6  col_7  col_8  col_15  col_16  col_17  col_9  col_21  col_22  col_23  col_24
25.0    10.0    1.0    2.0    3.0    4.0    20.0    5.0    6.0    7.0    8.0    15.0    16.0    17.0    9.0    21.0    22.0    23.0    24.0
125.0   110.0   101.0  102.0  103.0  104.0  120.0   105.0  106.0  107.0  108.0  115.0   116.0   117.0   109.0  121.0   122.0   123.0   124.0
```

#### Practical Behavior

- `reorder({"a", "b"})` now means “move these features together and preserve the relative order of everything else.”
- Use `ReorderSpec` when you need to move a subset to the front, back, before an anchor, or after an anchor.
- Use the multi-spec overload when you want to apply several moves in one call from left to right.
- This avoids listing a 3000-feature schema just to reposition two columns.


### `reorder_levels`

#### API Signatures

```cpp
DataFrame::reorder_levels(names: pluss::RangeView<std::string_view>) -> DataFrame

DataFrame::reorder_levels(names: std::initializer_list<std::string_view>) -> DataFrame
```

#### Notes

Use this method through the public contract shown above.

#### Inputs

`names` : `pluss::RangeView<std::string_view>`
    Collection of feature, slot, or stream names supplied in positional order.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.reorder_levels();
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 2. Pass several feature names inline.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.reorder_levels();
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `repeated_cases`

#### API Signature

```cpp
DataFrame::repeated_cases(features: pluss::RangeView<std::string_view> = {}) -> pluss::Series
```

#### Notes

Returns the rows whose current case labels are duplicates of an earlier row.

#### Inputs

`features` : `pluss::RangeView<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

#### Output

`pluss::Series`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto dup = df.with_cases(pluss::table::Thread(pluss::make_note({"cs0", "cs0", "cs2"})));
auto out = dup.repeated_cases();
```

##### Expected output

```text
[false, true, false]
```

### `replace`

#### API Signatures

```cpp
DataFrame::replace(from: const char*, to: const char*, features: pluss::RangeView<std::string_view> = {}) -> DataFrame

DataFrame::replace(from: const char*, to: pluss::null_t, features: pluss::RangeView<std::string_view> = {}) -> DataFrame

DataFrame::replace(from: const pluss::Note&, to: const pluss::Note&, features: pluss::RangeView<std::string_view> = {}) -> DataFrame

DataFrame::replace(from: const pluss::Note&, to: pluss::null_t, features: pluss::RangeView<std::string_view> = {}) -> DataFrame

DataFrame::replace(from: const pluss::Series&, to: const pluss::Series&, features: pluss::RangeView<std::string_view> = {}) -> DataFrame

DataFrame::replace(from: const pluss::Series&, to: pluss::null_t, features: pluss::RangeView<std::string_view> = {}) -> DataFrame

DataFrame::replace(from: const pluss::TimeIndex&, to: const pluss::TimeIndex&, features: pluss::RangeView<std::string_view> = {}) -> DataFrame

DataFrame::replace(from: const pluss::TimeIndex&, to: pluss::null_t, features: pluss::RangeView<std::string_view> = {}) -> DataFrame

DataFrame::replace(from: const pluss::Timeline&, to: const pluss::Timeline&, features: pluss::RangeView<std::string_view> = {}) -> DataFrame

DataFrame::replace(from: const pluss::Timeline&, to: pluss::null_t, features: pluss::RangeView<std::string_view> = {}) -> DataFrame

DataFrame::replace(from: double, to: double, features: pluss::RangeView<std::string_view> = {}) -> DataFrame

DataFrame::replace(from: double, to: pluss::null_t, features: pluss::RangeView<std::string_view> = {}) -> DataFrame

DataFrame::replace(from: pluss::null_t, to: const char*, features: pluss::RangeView<std::string_view> = {}) -> DataFrame

DataFrame::replace(from: pluss::null_t, to: const pluss::Note&, features: pluss::RangeView<std::string_view> = {}) -> DataFrame

DataFrame::replace(from: pluss::null_t, to: const pluss::Series&, features: pluss::RangeView<std::string_view> = {}) -> DataFrame

DataFrame::replace(from: pluss::null_t, to: const pluss::TimeIndex&, features: pluss::RangeView<std::string_view> = {}) -> DataFrame

DataFrame::replace(from: pluss::null_t, to: const pluss::Timeline&, features: pluss::RangeView<std::string_view> = {}) -> DataFrame

DataFrame::replace(from: pluss::null_t, to: double, features: pluss::RangeView<std::string_view> = {}) -> DataFrame

DataFrame::replace(from: pluss::null_t, to: std::string_view, features: pluss::RangeView<std::string_view> = {}) -> DataFrame

DataFrame::replace(from: std::string_view, to: pluss::null_t, features: pluss::RangeView<std::string_view> = {}) -> DataFrame

DataFrame::replace(from: std::string_view, to: std::string_view, features: pluss::RangeView<std::string_view> = {}) -> DataFrame
```

#### Notes

Replaces matched scalar or pluss-typed values across selected compatible features and returns the updated frame.

#### Inputs

`from` : `const char*`
    Value to search for inside the selected compatible features. Its type decides which feature kinds participate.

`to` : `const char*`
    Replacement value written anywhere `from` matches. Use `pluss::null` when you want matched cells to become missing.

`features` : `pluss::RangeView<std::string_view>`
    Optional feature subset. Leave empty to apply the replacement across every compatible feature of the relevant kind.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace("Tema", "Tamale", {"city"});
```

##### Expected output

```text
midterm_1  midterm_2  city
0.0        8.0        Accra
12.0       0.0        Tamale
0.0        15.0       Tamale
```
Example 2. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace(0.0, 5.0, {"midterm_1", "midterm_2"});
```

##### Expected output

```text
midterm_1  midterm_2  city
5.0        8.0        Accra
12.0       5.0        Tema
5.0        15.0       Tema
```
Example 3. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace(0.0, 5.0, {"midterm_1", "midterm_2"});
```

##### Expected output

```text
midterm_1  midterm_2  city
5.0        8.0        Accra
12.0       5.0        Tema
5.0        15.0       Tema
```
Example 4. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace(0.0, 5.0, {"midterm_1", "midterm_2"});
```

##### Expected output

```text
midterm_1  midterm_2  city
5.0        8.0        Accra
12.0       5.0        Tema
5.0        15.0       Tema
```
Example 5. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace(0.0, 5.0, {"midterm_1", "midterm_2"});
```

##### Expected output

```text
midterm_1  midterm_2  city
5.0        8.0        Accra
12.0       5.0        Tema
5.0        15.0       Tema
```
Example 6. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace(0.0, 5.0, {"midterm_1", "midterm_2"});
```

##### Expected output

```text
midterm_1  midterm_2  city
5.0        8.0        Accra
12.0       5.0        Tema
5.0        15.0       Tema
```
Example 7. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace(0.0, 5.0, {"midterm_1", "midterm_2"});
```

##### Expected output

```text
midterm_1  midterm_2  city
5.0        8.0        Accra
12.0       5.0        Tema
5.0        15.0       Tema
```
Example 8. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace(0.0, 5.0, {"midterm_1", "midterm_2"});
```

##### Expected output

```text
midterm_1  midterm_2  city
5.0        8.0        Accra
12.0       5.0        Tema
5.0        15.0       Tema
```
Example 9. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace(0.0, 5.0, {"midterm_1", "midterm_2"});
```

##### Expected output

```text
midterm_1  midterm_2  city
5.0        8.0        Accra
12.0       5.0        Tema
5.0        15.0       Tema
```
Example 10. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace(0.0, 5.0, {"midterm_1", "midterm_2"});
```

##### Expected output

```text
midterm_1  midterm_2  city
5.0        8.0        Accra
12.0       5.0        Tema
5.0        15.0       Tema
```
Example 11. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace(0.0, 5.0, {"midterm_1", "midterm_2"});
```

##### Expected output

```text
midterm_1  midterm_2  city
5.0        8.0        Accra
12.0       5.0        Tema
5.0        15.0       Tema
```
Example 12. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace(0.0, 5.0, {"midterm_1", "midterm_2"});
```

##### Expected output

```text
midterm_1  midterm_2  city
5.0        8.0        Accra
12.0       5.0        Tema
5.0        15.0       Tema
```
Example 13. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace(0.0, 5.0, {"midterm_1", "midterm_2"});
```

##### Expected output

```text
midterm_1  midterm_2  city
5.0        8.0        Accra
12.0       5.0        Tema
5.0        15.0       Tema
```
Example 14. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace(0.0, 5.0, {"midterm_1", "midterm_2"});
```

##### Expected output

```text
midterm_1  midterm_2  city
5.0        8.0        Accra
12.0       5.0        Tema
5.0        15.0       Tema
```
Example 15. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace(0.0, 5.0, {"midterm_1", "midterm_2"});
```

##### Expected output

```text
midterm_1  midterm_2  city
5.0        8.0        Accra
12.0       5.0        Tema
5.0        15.0       Tema
```
Example 16. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace(0.0, 5.0, {"midterm_1", "midterm_2"});
```

##### Expected output

```text
midterm_1  midterm_2  city
5.0        8.0        Accra
12.0       5.0        Tema
5.0        15.0       Tema
```
Example 17. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace(0.0, 5.0, {"midterm_1", "midterm_2"});
```

##### Expected output

```text
midterm_1  midterm_2  city
5.0        8.0        Accra
12.0       5.0        Tema
5.0        15.0       Tema
```
Example 18. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace(0.0, 5.0, {"midterm_1", "midterm_2"});
```

##### Expected output

```text
midterm_1  midterm_2  city
5.0        8.0        Accra
12.0       5.0        Tema
5.0        15.0       Tema
```
Example 19. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace(0.0, 5.0, {"midterm_1", "midterm_2"});
```

##### Expected output

```text
midterm_1  midterm_2  city
5.0        8.0        Accra
12.0       5.0        Tema
5.0        15.0       Tema
```
Example 20. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace(0.0, 5.0, {"midterm_1", "midterm_2"});
```

##### Expected output

```text
midterm_1  midterm_2  city
5.0        8.0        Accra
12.0       5.0        Tema
5.0        15.0       Tema
```
Example 21. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({0.0, 12.0, 0.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({8.0, 0.0, 15.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Tema"}))},
});

auto out = df.replace("Tema", "Tamale", {"city"});
```

##### Expected output

```text
midterm_1  midterm_2  city
0.0        8.0        Accra
12.0       0.0        Tamale
0.0        15.0       Tamale
```

#### Practical Behavior

- `replace(from, to, features)` dispatches by value type, so numeric replacements touch numeric series features while string replacements touch note features.
- If `features` is empty, every compatible feature of the relevant kind participates automatically.
- Use `pluss::null` as `to` when you want matched cells to become missing.


### `resample`

#### API Signature

```cpp
DataFrame::resample(time_feature: std::string_view, value_features: pluss::RangeView<std::string_view>, freq: std::string_view, op: std::string_view = "mean", step: std::int64_t = 1, render: bool = false) -> DataFrameView
```

#### Notes

This is part of the time-aware surface and expects timeline or time-index semantics.

#### Inputs

`time_feature` : `std::string_view`
    Name of the Timeline or TimeIndex feature that defines the resampling, alignment, or calendar axis.

`value_features` : `pluss::RangeView<std::string_view>`
    Feature name(s) to aggregate inside each resampling bin. Leave empty when you want PlussMic to infer compatible value features.

`freq` : `std::string_view`
    Bin or calendar frequency such as `"1h"`, `"D"`, `"W"`, or `"M"`.

`op` : `std::string_view`
    Bin reduction to compute in each time bucket.

    Supported values:
    - `count`: Count observed values within each bucket.
    - `size`: Count rows within each bucket, including missing values.
    - `sum`: Compute the numeric sum within each bucket.
    - `mean`: Compute the arithmetic mean within each bucket.
    - `min`: Compute the minimum within each bucket.
    - `max`: Compute the maximum within each bucket.
    - `first`: Take the first observed value within each bucket.
    - `last`: Take the last observed value within each bucket.

`step` : `std::int64_t`
    Positive frequency multiplier. For example, `6` with hourly frequency means six-hour bins.

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

#### Output

`DataFrameView`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Choose whether the returned object should auto-render in display contexts.

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

##### Expected output

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

#### API Signatures

```cpp
DataFrame::reset_cases(case_feature: std::string = "case") -> DataFrame

DataFrame::reset_cases(options: const ResetCasesOptions&) -> DataFrame
```

#### Notes

Materializes the current case axis as a normal feature, then restores the default positional cases `0..n-1`.

#### Inputs

`case_feature` : `std::string`
    Name to use for the restored case-axis feature when the current cases are materialized back into ordinary columns.

`options` : `const ResetCasesOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto with_cases = df.set_cases("city", true);
auto out = with_cases.reset_cases("case");
```

##### Expected output

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
Example 2. Pass an explicit options object to control the operation.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto with_cases = df.set_cases("city", true);
auto out = with_cases.reset_cases("case");
```

##### Expected output

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
DataFrame::reshuffle(axis: Axis = Axis::Case, random_state: std::optional<std::uint64_t> = std::nullopt) -> DataFrame
```

#### Notes

Use this method through the public contract shown above.

#### Inputs

`axis` : `Axis`
    Shuffle axis. Use `0` to permute rows and `1` to permute feature order.

`random_state` : `std::optional<std::uint64_t>`
    Deterministic seed for reproducible row or feature shuffles.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Shuffle rows or features depending on the selected axis.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto rows = df.reshuffle(0, 7);
auto cols = df.reshuffle(1, 7);
```

##### Expected output

```text
rows.head():
id  sales  city
3    90.0  Kumasi
1   100.0  Accra
2   120.0  Tema

cols.head():
city    id  sales
Accra   1   100.0
Tema    2   120.0
Kumasi  3    90.0
```

#### Practical Behavior

- `reshuffle(axis=0)` permutes rows and keeps the feature order unchanged.
- `reshuffle(axis=1)` permutes feature order and keeps the row order unchanged.
- Pass `random_state` when you need deterministic shuffling for docs, tests, or reproducible data splits.


### `rfloordiv`

#### API Signature

```cpp
DataFrame::rfloordiv(other: double) -> DataFrame
```

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### Inputs

`other` : `scalar`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"a", pluss::table::Thread(pluss::make_series({1.0, 2.0, 3.0}))},
    {"b", pluss::table::Thread(pluss::make_series({2.0, 4.0, 5.0}))},
});

auto out = df.rfloordiv(10);
```

##### Expected output

```text
a   b
10  5
5   2
3   2
```

### `rmod`

#### API Signature

```cpp
DataFrame::rmod(other: double) -> DataFrame
```

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### Inputs

`other` : `scalar`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"a", pluss::table::Thread(pluss::make_series({1.0, 2.0, 3.0}))},
    {"b", pluss::table::Thread(pluss::make_series({2.0, 4.0, 5.0}))},
});

auto out = df.rmod(10);
```

##### Expected output

```text
a    b
0.0  0.0
0.0  0.0
0.0  0.0
```

### `rmul`

#### API Signature

```cpp
DataFrame::rmul(other: double) -> DataFrame
```

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### Inputs

`other` : `scalar`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"a", pluss::table::Thread(pluss::make_series({1.0, 2.0, 3.0}))},
    {"b", pluss::table::Thread(pluss::make_series({2.0, 4.0, 5.0}))},
});

auto out = df.rmul(10);
```

##### Expected output

```text
a    b
3.0  6.0
6.0  12.0
9.0  15.0
```

### `rolling`

#### API Signature

```cpp
DataFrame::rolling(specs: pluss::RangeView<WindowSpec>, keep_input: bool = true) -> DataFrame
```

#### Notes

Computes rolling-window features and returns them as a derived frame.

#### Inputs

`specs` : `pluss::RangeView<WindowSpec>`
    Window specs that define the output name, source feature, rolling operation, window width, and minimum observation policy.

`keep_input` : `bool`
    When `true`, the original features stay in the output and rolling features are appended. When `false`, only the derived rolling outputs are returned.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.rolling({{.output = "sales_roll", .feature = "sales", .op = pluss::table::ThreadWindowOp::RollingMean, .window = 2, .min_periods = 1}}, false);
```

##### Expected output

```text
sales_roll
100.0
110.0
105.0
```

#### Practical Behavior

- `rolling(...)` is the frame-level window-engineering route, so one call can derive several rolling outputs at once.
- `keep_input=true` appends rolling outputs to the original frame, while `keep_input=false` returns only the engineered outputs.
- Each `WindowSpec` controls the output name, source feature, operation, window width, and minimum observation rule.


### `round`

#### API Signature

```cpp
DataFrame::round(features: pluss::RangeView<std::string_view>, unit: std::string_view) -> DataFrame
```

#### Notes

This is part of the time-aware surface and expects timeline or time-index semantics.

#### Inputs

`features` : `pluss::RangeView<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`unit` : `std::string_view`
    Temporal rounding unit such as `"hour"`, `"day"`, or `"month"`.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.round();
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `rpow`

#### API Signature

```cpp
DataFrame::rpow(other: double) -> DataFrame
```

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### Inputs

`other` : `scalar`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"a", pluss::table::Thread(pluss::make_series({1.0, 2.0, 3.0}))},
    {"b", pluss::table::Thread(pluss::make_series({2.0, 4.0, 5.0}))},
});

auto out = df.rpow(10);
```

##### Expected output

```text
a      b
10.0   100.0
100.0  10000.0
1000.0 100000.0
```

### `rsub`

#### API Signature

```cpp
DataFrame::rsub(other: double) -> DataFrame
```

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### Inputs

`other` : `scalar`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"a", pluss::table::Thread(pluss::make_series({1.0, 2.0, 3.0}))},
    {"b", pluss::table::Thread(pluss::make_series({2.0, 4.0, 5.0}))},
});

auto out = df.rsub(10);
```

##### Expected output

```text
a    b
9.0  8.0
8.0  6.0
7.0  5.0
```

### `rtruediv`

#### API Signature

```cpp
DataFrame::rtruediv(other: double) -> DataFrame
```

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### Inputs

`other` : `scalar`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"a", pluss::table::Thread(pluss::make_series({1.0, 2.0, 3.0}))},
    {"b", pluss::table::Thread(pluss::make_series({2.0, 4.0, 5.0}))},
});

auto out = df.rtruediv(10);
```

##### Expected output

```text
a         b
10.0      5.0
5.0       2.5
3.333333  2.0
```

## S

### `sample`

#### API Signatures

```cpp
DataFrame::sample(frac: Float) -> template <class Float> requires std::floating_point<Float> DataFrame

DataFrame::sample(frac: Float, random_state: std::uint64_t, replace: bool = false, axis: Axis = Axis::Case) -> template <class Float> requires std::floating_point<Float> DataFrame

DataFrame::sample(frac: Float, replace: bool, axis: Axis) -> template <class Float> requires std::floating_point<Float> DataFrame

DataFrame::sample(n: std::int64_t) -> DataFrame

DataFrame::sample(n: std::int64_t, random_state: std::uint64_t, replace: bool = false, axis: Axis = Axis::Case) -> DataFrame

DataFrame::sample(n: std::int64_t, replace: bool, axis: Axis) -> DataFrame
```

#### Notes

Returns a random sample of rows.

#### Inputs

`frac` : `Float`
    Fraction of the selected axis to draw. Values above `1.0` require `replace=true`.

`random_state` : `std::uint64_t`
    Deterministic seed for reproducible sampling and shuffling.

`replace` : `bool`
    When `true`, the same row may be drawn more than once. Feature sampling currently requires `replace=false`.

`axis` : `Axis`
    Sampling axis. Use `0` for rows and `1` for features.

`n` : `std::int64_t`
    Number of rows or features to draw from the selected axis.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Sample a fraction of the selected axis without an explicit random seed.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.sample(0.67, false, 0);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
3    90.0  Kumasi
```
Example 2. Sample a fraction of the selected axis with an explicit random seed.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.sample(0.67, 7, false, 0);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
3    90.0  Kumasi
```
Example 3. Sample a fraction of the selected axis without an explicit random seed.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.sample(0.67, false, 0);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
3    90.0  Kumasi
```
Example 4. Sample rows with the default settings.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.sample(2);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
3    90.0  Kumasi
```
Example 5. Sample rows or features with an explicit random seed.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto rows = df.sample(2, 7, false, 0);
auto cols = df.sample(2, 7, false, 1);
```

##### Expected output

```text
rows.head():
id  sales  city
1   100.0  Accra
3    90.0  Kumasi

cols.head():
sales  city
100.0  Accra
120.0  Tema
90.0   Kumasi
```
Example 6. Sample rows with the default settings.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.sample(2);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
3    90.0  Kumasi
```

#### Practical Behavior

- `sample(n, ...)` draws a fixed number of rows by default, or features when `axis=1`.
- `sample(frac, ...)` draws a proportion of the selected axis.
- `replace=true` enables sampling with replacement. For `frac > 1`, replacement is required.


### `save_as`

#### API Signatures

```cpp
DataFrame::save_as(target_type: std::string_view, filepath: std::string_view, config: const pluss::loader::Config& = {}) -> void

DataFrame::save_as(target_type: std::string_view, filepath: std::string_view, config_entries: std::initializer_list<pluss::loader::ConfigEntry>) -> void
```

#### Notes

Writes the frame using the explicitly supplied output format selector rather than a dedicated saver overload.

#### Inputs

`target_type` : `std::string_view`
    Format or target selector string used by this overload.

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

`config_entries` : `std::initializer_list<pluss::loader::ConfigEntry>`
    Inline configuration entries used to build a lightweight loader configuration without naming a separate config object.

#### Output

`void`
    Performs the side effect and returns no value.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Save the frame with an explicit loader-config object.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

pluss::loader::Config config{};
df.save_as("json", "path/to/output.json", config);
```

##### Expected output

```text
saved: out.*
```
Example 2. Save the frame with inline configuration entries.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_as("json", "path/to/output.json", {"indent", 2});
```

##### Expected output

```text
saved: out.*
```

### `save_csv`

#### API Signatures

```cpp
DataFrame::save_csv(filepath: std::string_view) -> void

DataFrame::save_csv(filepath: std::string_view, add_cases: bool, header: bool = true, sep: char = ',', na_rep: std::string = "", case_label: std::optional<std::string> = std::nullopt, float_format: std::optional<std::string> = std::nullopt, quotechar: char = '"', doublequote: bool = true, line_terminator: std::string = "\n", decimal: char = '.', encoding: std::string = "utf-8") -> void

DataFrame::save_csv(filepath: std::string_view, config: const pluss::loader::Config&) -> void

DataFrame::save_csv(filepath: std::string_view, config_entries: std::initializer_list<pluss::loader::ConfigEntry>) -> void

DataFrame::save_csv(filepath: std::string_view, options: const pluss::loader::WriteCsvOptions&) -> void
```

#### Notes

Writes the frame to CSV. Set `add_cases=true` when you want the current case axis exported as a regular column so row labels survive the round trip.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`add_cases` : `bool`
    When `true`, the current case axis is exported as its own column so row labels can be reconstructed when the file is loaded again.

`header` : `bool`
    Controls whether feature names are written to, or expected from, the file header row.

`sep` : `char`
    Column separator used for delimited text files such as CSV.

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

`config_entries` : `std::initializer_list<pluss::loader::ConfigEntry>`
    Inline configuration entries used to build a lightweight loader configuration without naming a separate config object.

`options` : `const pluss::loader::WriteCsvOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`void`
    Performs the side effect and returns no value.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Save the frame using the simplest filepath-only overload.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_csv("path/to/output.csv");
```

##### Expected output

```text
saved: out.*
```
Example 2. Save the frame while controlling case-axis export and file-format details explicitly.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_csv("path/to/output.csv", true, true, ',', "NA", "case_id");
```

##### Expected output

```text
saved: out.*
```
Example 3. Save the frame with an explicit loader-config object.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

pluss::loader::Config config{};
df.save_csv("path/to/output.csv", config);
```

##### Expected output

```text
saved: out.*
```
Example 4. Save the frame with inline configuration entries.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_csv("path/to/output.csv", {"indent", 2});
```

##### Expected output

```text
saved: out.*
```
Example 5. Save the frame with a typed writer-options object.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

pluss::loader::WriteCsvOptions options;
df.save_csv("path/to/output.csv", options);
```

##### Expected output

```text
saved: out.*
```

### `save_excel`

#### API Signatures

```cpp
DataFrame::save_excel(filepath: std::string_view) -> void

DataFrame::save_excel(filepath: std::string_view, config: const pluss::loader::Config&) -> void

DataFrame::save_excel(filepath: std::string_view, config_entries: std::initializer_list<pluss::loader::ConfigEntry>) -> void

DataFrame::save_excel(filepath: std::string_view, options: const pluss::loader::WriteExcelOptions&) -> void

DataFrame::save_excel(filepath: std::string_view, sheet_name: std::string, add_cases: bool = false, header: bool = true, na_rep: std::string = "", case_label: std::optional<std::string> = std::nullopt) -> void
```

#### Notes

Writes the frame to an Excel sheet. Set `add_cases=true` when you want the current case axis exported as a regular column.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

`config_entries` : `std::initializer_list<pluss::loader::ConfigEntry>`
    Inline configuration entries used to build a lightweight loader configuration without naming a separate config object.

`options` : `const pluss::loader::WriteExcelOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

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

#### Output

`void`
    Performs the side effect and returns no value.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Save the frame using the simplest filepath-only overload.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_excel("path/to/output.xlsx");
```

##### Expected output

```text
saved: out.*
```
Example 2. Save the frame with an explicit loader-config object.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

pluss::loader::Config config{};
df.save_excel("path/to/output.xlsx", config);
```

##### Expected output

```text
saved: out.*
```
Example 3. Save the frame with inline configuration entries.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_excel("path/to/output.xlsx", {"indent", 2});
```

##### Expected output

```text
saved: out.*
```
Example 4. Save the frame with a typed writer-options object.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

pluss::loader::WriteExcelOptions options;
df.save_excel("path/to/output.xlsx", options);
```

##### Expected output

```text
saved: out.*
```
Example 5. Save the frame while controlling case-axis export and file-format details explicitly.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_csv("path/to/output.csv", true, true, ',', "NA", "case_id");
```

##### Expected output

```text
saved: out.*
```

### `save_json`

#### API Signatures

```cpp
DataFrame::save_json(filepath: std::string_view) -> void

DataFrame::save_json(filepath: std::string_view, config: const pluss::loader::Config&) -> void

DataFrame::save_json(filepath: std::string_view, config_entries: std::initializer_list<pluss::loader::ConfigEntry>) -> void
```

#### Notes

Writes the frame to JSON using the table export route for later reload or interchange.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

`config_entries` : `std::initializer_list<pluss::loader::ConfigEntry>`
    Inline configuration entries used to build a lightweight loader configuration without naming a separate config object.

#### Output

`void`
    Performs the side effect and returns no value.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Save the frame using the simplest filepath-only overload.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_json("path/to/output.json");
```

##### Expected output

```text
saved: out.*
```
Example 2. Save the frame with an explicit loader-config object.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

pluss::loader::Config config{};
df.save_json("path/to/output.json", config);
```

##### Expected output

```text
saved: out.*
```
Example 3. Save the frame with inline configuration entries.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_json("path/to/output.json", {"indent", 2});
```

##### Expected output

```text
saved: out.*
```

### `save_parquet`

#### API Signatures

```cpp
DataFrame::save_parquet(filepath: std::string_view) -> void

DataFrame::save_parquet(filepath: std::string_view, config: const pluss::loader::Config&) -> void

DataFrame::save_parquet(filepath: std::string_view, config_entries: std::initializer_list<pluss::loader::ConfigEntry>) -> void
```

#### Notes

Writes the frame to Parquet for compact typed storage.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

`config_entries` : `std::initializer_list<pluss::loader::ConfigEntry>`
    Inline configuration entries used to build a lightweight loader configuration without naming a separate config object.

#### Output

`void`
    Performs the side effect and returns no value.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Save the frame using the simplest filepath-only overload.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_parquet("path/to/output.parquet");
```

##### Expected output

```text
saved: out.*
```
Example 2. Save the frame with an explicit loader-config object.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

pluss::loader::Config config{};
df.save_parquet("path/to/output.parquet", config);
```

##### Expected output

```text
saved: out.*
```
Example 3. Save the frame with inline configuration entries.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_parquet("path/to/output.parquet", {"indent", 2});
```

##### Expected output

```text
saved: out.*
```

### `save_txt`

#### API Signatures

```cpp
DataFrame::save_txt(filepath: std::string_view) -> void

DataFrame::save_txt(filepath: std::string_view, config: const pluss::loader::Config&) -> void

DataFrame::save_txt(filepath: std::string_view, config_entries: std::initializer_list<pluss::loader::ConfigEntry>) -> void
```

#### Notes

Writes the frame to a plain-text table preview suitable for inspection or lightweight logging.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

`config_entries` : `std::initializer_list<pluss::loader::ConfigEntry>`
    Inline configuration entries used to build a lightweight loader configuration without naming a separate config object.

#### Output

`void`
    Performs the side effect and returns no value.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Save the frame using the simplest filepath-only overload.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_txt("path/to/output.txt");
```

##### Expected output

```text
saved: out.*
```
Example 2. Save the frame with an explicit loader-config object.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

pluss::loader::Config config{};
df.save_txt("path/to/output.txt", config);
```

##### Expected output

```text
saved: out.*
```
Example 3. Save the frame with inline configuration entries.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.save_txt("path/to/output.txt", {"indent", 2});
```

##### Expected output

```text
saved: out.*
```

### `scan`

#### API Signature

```cpp
DataFrame::scan(statistic: ScanStatistic, skip_nulls: bool = true) -> DataFrame
```

#### Notes

Use this method through the public contract shown above.

#### Inputs

`statistic` : `ScanStatistic`
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

`skip_nulls` : `bool`
    When `true`, a null output remains null only at that case and later cumulative values resume; when `false`, every output after the first null is null.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.scan();
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `se`

#### API Signature

```cpp
DataFrame::se(axis: Axis = Axis::Feature, target: ErrorTarget = ErrorTarget::Mean, options: const StatisticOptions& = {}) -> Thread
```

#### Notes

Use this method through the public contract shown above.

#### Inputs

`axis` : `Axis`
    Axis selector. On `DataFrame`, `0` usually means rows/cases and `1` means features/columns.

`target` : `ErrorTarget`
    Target payload family chosen by this cast overload.

`options` : `const StatisticOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Pass an explicit options object to control the operation.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.se();
```

##### Expected output

```text
The call completes and returns the contract described under Output.
```

### `select`

#### API Signature

```cpp
DataFrame::select(names: pluss::RangeView<std::string_view>) -> DataFrame
```

#### Notes

Use this to reduce the current object to the rows or fields that matter for the next step.

#### Inputs

`names` : `pluss::RangeView<std::string_view>`
    Collection of feature, slot, or stream names supplied in positional order.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.select();
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `select_like`

#### API Signature

```cpp
DataFrame::select_like(needle: std::string_view, case_sensitive: bool = true) -> DataFrame
```

#### Notes

Selects only the features whose names contain the requested substring.

#### Inputs

`needle` : `std::string_view`
    Substring token used to match feature names.

`case_sensitive` : `bool`
    When `true`, substring matching respects letter case. When `false`, feature names are matched case-insensitively.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({70.0, 80.0, 90.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({75.0, 85.0, 95.0}))},
    {"final_points", pluss::table::Thread(pluss::make_series({88.0, 91.0, 94.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.select_like("midterm");
```

##### Expected output

```text
midterm_1  midterm_2
70.0       75.0
80.0       85.0
90.0       95.0
```

#### Practical Behavior

- `select_like(...)` filters feature names, not row values.
- The output keeps the original row order and case axis unchanged.
- Use this when you want schema-driven feature selection without listing every matching feature manually.


### `select_regex`

#### API Signature

```cpp
DataFrame::select_regex(pattern: std::string_view) -> DataFrame
```

#### Notes

Selects only the features whose names match the requested regular expression.

#### Inputs

`pattern` : `std::string_view`
    ECMAScript regular expression used to match feature names.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"midterm_1", pluss::table::Thread(pluss::make_series({70.0, 80.0, 90.0}))},
    {"midterm_2", pluss::table::Thread(pluss::make_series({75.0, 85.0, 95.0}))},
    {"final_points", pluss::table::Thread(pluss::make_series({88.0, 91.0, 94.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.select_regex("^midterm_[12]$");
```

##### Expected output

```text
midterm_1  midterm_2
70.0       75.0
80.0       85.0
90.0       95.0
```

#### Practical Behavior

- `select_regex(...)` filters feature names, not row values.
- The output keeps the original row order and case axis unchanged.
- Use this when you want schema-driven feature selection without listing every matching feature manually.


### `series_features`

#### API Signature

```cpp
DataFrame::series_features() -> DataFrame
```

#### Notes

Returns only the numeric series-backed features.

#### Inputs

`None`

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.series_features();
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `set`

#### API Signatures

```cpp
DataFrame::set(name: std::string_view) -> LetSlot

DataFrame::set(names: pluss::RangeView<std::string_view>) -> LetSlots

DataFrame::set(names: std::initializer_list<const char*>) -> LetSlots

DataFrame::set(names: std::initializer_list<std::string_view>) -> LetSlots
```

#### Notes

Opens an assignment helper so you can write `df.set(...) = ...` and replace or create features declaratively.

#### Inputs

`name` : `std::string_view`
    Name used to identify the target object, field, feature, or slot for this overload.

`names` : `pluss::RangeView<std::string_view>`
    Collection of feature, slot, or stream names supplied in positional order.

#### Output

`LetSlot`
    Value produced by this call.

`LetSlots`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Open the assignment helper for one feature name.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.set("profit") = pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0}));
```

##### Expected output

```text
The call completes and returns the contract described under Output.
```
Example 2. Open the multi-feature assignment helper from a reusable feature-name range.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.set("profit") = pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0}));
```

##### Expected output

```text
The call completes and returns the contract described under Output.
```
Example 3. Open the multi-feature assignment helper from inline C-string names.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.set({"profit", "cost"}) = {pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0})), pluss::table::Thread(pluss::make_series({60.0, 85.0, 60.0}))};
```

##### Expected output

```text
The call completes and returns the contract described under Output.
```
Example 4. Open the multi-feature assignment helper from inline string-view names.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.set({"profit", "cost"}) = {pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0})), pluss::table::Thread(pluss::make_series({60.0, 85.0, 60.0}))};
```

##### Expected output

```text
The call completes and returns the contract described under Output.
```

### `set_axis`

#### API Signature

```cpp
DataFrame::set_axis(feature_names: pluss::RangeView<std::string_view>) -> DataFrame
```

#### Notes

Replaces the full feature label set with the supplied names in positional order.

#### Inputs

`feature_names` : `pluss::RangeView<std::string_view>`
    Full replacement feature-label list in final left-to-right order. The count must match the current number of features.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.set_axis({"revenue", "location", "id_new"});
```

##### Expected output

```text
revenue  location  id_new
100.0    Accra     1
120.0    Tema      2
90.0     Kumasi    3
```

### `set_cases`

#### API Signatures

```cpp
DataFrame::set_cases(feature_name: std::string_view, drop_feature: bool = false) -> DataFrame

DataFrame::set_cases(feature_names: pluss::RangeView<std::string_view>, drop_features: bool = false, separator: std::string_view = "\x1F") -> DataFrame

DataFrame::set_cases(feature_names: std::initializer_list<std::string_view>, drop_features: bool = false, separator: std::string_view = "\x1F") -> DataFrame

DataFrame::set_cases(options: const SetCasesOptions&) -> DataFrame
```

#### Notes

Promotes one feature to become the case axis. With the default `drop_feature=true`, that feature stops being a normal column and becomes the row labels instead.

#### Inputs

`feature_name` : `std::string_view`
    Name of the target feature to read, rewrite, sort by, or promote to cases.

`drop_feature` : `bool`
    When `true`, the feature promoted to cases is removed from the ordinary feature columns.

`feature_names` : `pluss::RangeView<std::string_view>`
    Feature name(s) supplied explicitly for subset, ordering, or axis-replacement operations, depending on the method.

`drop_features` : `bool`
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

`separator` : `std::string_view`
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

`options` : `const SetCasesOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`DataFrame`
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

auto out = df.set_cases("city");
```

##### Expected output

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
Example 2. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.set_cases("city");
```

##### Expected output

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
Example 3. Pass several feature names inline.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.set_cases("city");
```

##### Expected output

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
Example 4. Pass an explicit options object to control the operation.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.set_cases("city");
```

##### Expected output

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

### `set_features`

#### API Signatures

```cpp
DataFrame::set_features(name: std::string_view, value: const Thread&) -> DataFrame

DataFrame::set_features(names: pluss::RangeView<std::string_view>, values: pluss::RangeView<Thread>) -> DataFrame
```

#### Notes

Returns a new frame with one added or replaced feature.

#### Inputs

`name` : `std::string_view`
    Name used to identify the target object, field, feature, or slot for this overload.

`value` : `const Thread&`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

`names` : `pluss::RangeView<std::string_view>`
    Collection of feature, slot, or stream names supplied in positional order.

`values` : `pluss::RangeView<Thread>`
    Value collection supplied to this overload. The ordering must match the corresponding names or positions expected by the method.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Add or replace one column by name.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.set_features("profit", pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0})));
```

##### Expected output

```text
id  sales  city    profit
1   100.0  Accra   40.0
2   120.0  Tema    35.0
3    90.0  Kumasi  30.0
```
Example 2. Add or replace several columns in one call.

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

##### Expected output

```text
id  sales  city    profit
1   100.0  Accra   40.0
2   120.0  Tema    35.0
3    90.0  Kumasi  30.0
```

### `set_features_`

#### API Signatures

```cpp
DataFrame::set_features_(name: std::string_view, value: const Thread&) -> DataFrame&

DataFrame::set_features_(names: pluss::RangeView<std::string_view>, values: pluss::RangeView<Thread>) -> DataFrame&
```

#### Notes

Adds or replaces one feature in place.

#### Inputs

`name` : `std::string_view`
    Name used to identify the target object, field, feature, or slot for this overload.

`value` : `const Thread&`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

`names` : `pluss::RangeView<std::string_view>`
    Collection of feature, slot, or stream names supplied in positional order.

`values` : `pluss::RangeView<Thread>`
    Value collection supplied to this overload. The ordering must match the corresponding names or positions expected by the method.

#### Output

`DataFrame&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Add or replace one column by name.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.set_features_("profit", pluss::table::Thread(pluss::make_series({40.0, 35.0, 30.0})));
```

##### Expected output

```text
id  sales  city    profit
1   100.0  Accra   40.0
2   120.0  Tema    35.0
3    90.0  Kumasi  30.0
```
Example 2. Add or replace several columns in one call.

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

##### Expected output

```text
id  sales  city    profit
1   100.0  Accra   40.0
2   120.0  Tema    35.0
3    90.0  Kumasi  30.0
```

### `shift`

#### API Signatures

```cpp
DataFrame::shift(features: pluss::RangeView<std::string_view>, periods: int, freq: std::string_view) -> DataFrame

DataFrame::shift(features: pluss::RangeView<std::string_view>, periods: std::int64_t = 1) -> DataFrame

DataFrame::shift(periods: std::int64_t = 1) -> DataFrame
```

#### Notes

This is part of the time-aware surface and expects timeline or time-index semantics.

#### Inputs

`features` : `pluss::RangeView<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`periods` : `int`
    Number of rows to shift selected feature values. Positive values move values downward and introduce missing entries at the top; negative values move upward.

`freq` : `std::string_view`
    Bin or calendar frequency such as `"1h"`, `"D"`, `"W"`, or `"M"`.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Shift only selected columns by a fixed number of rows.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.shift({"sales"}, 2);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 2. Shift only selected columns by a fixed number of rows.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.shift({"sales"}, 2);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 3. Shift every compatible column by a fixed number of rows.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.shift(2);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `show`

#### API Signatures

```cpp
DataFrame::show(options: const display::DisplayOptions& = {}) -> void

DataFrame::show(os: std::ostream&, options: const display::DisplayOptions& = {}) -> void
```

#### Notes

This renders a human-readable view for inspection instead of creating a new analytic object.

#### Inputs

`options` : `const display::DisplayOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

`os` : `std::ostream&`
    Destination output stream used instead of the default display or print target.

#### Output

`void`
    Performs the side effect and returns no value.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Render the frame using the default display route.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.show();
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 2. Render the frame to a caller-provided output stream.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

std::ostringstream os;
df.show(os);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `shuffle`

#### API Signature

```cpp
DataFrame::shuffle(random_state: std::optional<std::uint64_t> = std::nullopt) -> DataFrame
```

#### Notes

Use this method through the public contract shown above.

#### Inputs

`random_state` : `std::optional<std::uint64_t>`
    Deterministic seed for reproducible row shuffles.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Shuffle row order while keeping the feature order unchanged.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.shuffle(7);
```

##### Expected output

```text
id  sales  city
3    90.0  Kumasi
1   100.0  Accra
2   120.0  Tema
```

#### Practical Behavior

- `shuffle(random_state)` is the row-order helper for the common case where you want to permute rows and leave the feature order alone.
- It is equivalent to `reshuffle(axis=0, ...)`, but reads more directly in train/test split and stochastic preview workflows.


### `skew`

#### API Signature

```cpp
DataFrame::skew(axis: Axis = Axis::Feature, min_count: std::int64_t = 3) -> Thread
```

#### Notes

Use this method through the public contract shown above.

#### Inputs

`axis` : `Axis`
    Axis selector. On `DataFrame`, `0` usually means rows/cases and `1` means features/columns.

`min_count` : `std::int64_t`
    Minimum non-null observations required before a grouped or rolling summary is considered valid.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.skew();
```

##### Expected output

```text
The call completes and returns the contract described under Output.
```

### `slice`

#### API Signature

```cpp
DataFrame::slice(start: std::int64_t, stop: std::int64_t) -> DataFrame
```

#### Notes

Returns a contiguous row slice `[start, stop)` from the frame.

#### Inputs

`start` : `std::int64_t`
    Inclusive starting position of the slice.

`stop` : `std::int64_t`
    Exclusive ending position of the slice.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.slice(0, 2);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `sort_cases`

#### API Signatures

```cpp
DataFrame::sort_cases(ascending: bool = true, nulls_last: bool = true) -> DataFrame

DataFrame::sort_cases(options: const SortCasesOptions&) -> DataFrame
```

#### Notes

Sorts rows by the current case axis rather than by a data feature.

#### Inputs

`ascending` : `bool`
    When `true`, case labels are sorted ascending. When `false`, they are sorted descending.

`nulls_last` : `bool`
    When `true`, null case labels are placed after observed labels. When `false`, they are placed first.

`options` : `const SortCasesOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto with_cases = df.with_cases(pluss::table::Thread(pluss::make_note({"cs2", "cs0", "cs1"})));
auto out = with_cases.sort_cases(true, true);
```

##### Expected output

```text
with_cases.cases():
["cs2", "cs0", "cs1"]

out.cases():
["cs0", "cs1", "cs2"]

out.head():
id  sales  city
2   120.0  Tema
3    90.0  Kumasi
1   100.0  Accra
```
Example 2. Pass an explicit options object to control the operation.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto with_cases = df.with_cases(pluss::table::Thread(pluss::make_note({"cs2", "cs0", "cs1"})));
auto out = with_cases.sort_cases(true, true);
```

##### Expected output

```text
with_cases.cases():
["cs2", "cs0", "cs1"]

out.cases():
["cs0", "cs1", "cs2"]

out.head():
id  sales  city
2   120.0  Tema
3    90.0  Kumasi
1   100.0  Accra
```

### `sort_values`

#### API Signatures

```cpp
DataFrame::sort_values(features: pluss::RangeView<std::string_view>, ascending: bool = true, nulls_last: bool = true) -> DataFrame

DataFrame::sort_values(specs: pluss::RangeView<ArrangeSpec>) -> DataFrame
```

#### Notes

Reorders rows using one or more feature values as sort keys.

#### Inputs

`features` : `pluss::RangeView<std::string_view>`
    Feature names used as sort keys when you want the same ascending/null policy for every key.

`ascending` : `bool`
    When you use the simple feature-list overload, this one switch applies to every sort key. `true` sorts ascending and `false` sorts descending.

`nulls_last` : `bool`
    When `true`, null values are placed after observed values for every sort key in the simple overload. When `false`, nulls are placed first.

`specs` : `pluss::RangeView<ArrangeSpec>`
    Collection of structured operation specs. Each element describes one requested summary, derive, transform, group, or placement rule depending on the method.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Sort rows using one shared ascending and null-order policy for every selected sort key.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.sort_values({"sales"}, false, true);
```

##### Expected output

```text
id  sales  city
2   120.0  Tema
1   100.0  Accra
3    90.0  Kumasi
```
Example 2. Sort rows using explicit per-column sort specifications.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.sort_values({{.feature = "sales", .ascending = false}});
```

##### Expected output

```text
id  sales  city
2   120.0  Tema
1   100.0  Accra
3    90.0  Kumasi
```

### `sql`

#### API Signatures

```cpp
DataFrame::sql() -> DataFrameSqlAccessor

DataFrame::sql(options: SqlOptions) -> DataFrameSqlAccessor
```

#### Notes

Use this when the transformation is easier to express as SQL than as a chained sequence of table verbs.

#### Inputs

`options` : `SqlOptions`
    SQL execution options such as engine or binding policy for the returned SQL accessor.

#### Output

`DataFrameSqlAccessor`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Create the SQL accessor with default options and run a query immediately.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.sql().execute("SELECT id, sales FROM self WHERE sales > 95");
```

##### Expected output

```text
id  sales
1   100.0
2   120.0
```
Example 2. Create the SQL accessor with an explicit SQL options object before running a query.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

pluss::table::DataFrame::SqlOptions options;
auto sql = df.sql(options);
auto out = sql.execute("SELECT id, sales FROM self WHERE sales > 95");
```

##### Expected output

```text
id  sales
1   100.0
2   120.0
```

#### Practical Behavior

- `sql()` returns the SQL accessor; it does not execute a statement by itself.
- Call `.execute(...)` on the accessor when you want a result frame immediately.
- Use `prepare_sql(...)` when the same query will be executed repeatedly with changing bindings.


### `stack`

#### API Signature

```cpp
DataFrame::stack(feature_name: std::string = "feature", value_name: std::string = "value") -> DataFrame
```

#### Notes

Use this method through the public contract shown above.

#### Inputs

`feature_name` : `std::string`
    Name of the target feature to read, rewrite, sort by, or promote to cases.

`value_name` : `std::string`
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.stack();
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `std`

#### API Signatures

```cpp
DataFrame::std(axis: Axis = Axis::Feature, skipna: bool = true) -> Thread

DataFrame::std(axis: Axis, options: const StatisticOptions&) -> Thread
```

#### Notes

Computes per-feature standard deviations for the selected numeric features.

#### Inputs

`axis` : `Axis`
    Axis selector. On `DataFrame`, `0` usually means rows/cases and `1` means features/columns.

`skipna` : `bool`
    When `true`, missing values are skipped during the reduction. When `false`, missing state can affect the result.

`options` : `const StatisticOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Compute `std` for each numeric column in the frame.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.std(false);
```

##### Expected output

```text
feature  value
id       1.0
sales    15.275252317
```
Example 2. Compute `std` for each numeric column in the frame.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.std(false);
```

##### Expected output

```text
feature  value
id       1.0
sales    15.275252317
```

#### Practical Behavior

- `std(...)` reduces the selected numeric features to one summary row.
- Non-numeric features are skipped rather than coerced.
- The output stays in DataFrame form, which makes these reducers easy to reuse in downstream frame pipelines.


### `sub`

#### API Signatures

```cpp
DataFrame::sub(other: const DataFrame&) -> DataFrame

DataFrame::sub(other: double) -> DataFrame
```

#### Notes

This applies arithmetic across the compatible numeric payloads in the current object.

#### Inputs

`other` : `const DataFrame&`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Subtract each compatible numeric cell against another aligned frame.

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

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 2. Subtract each compatible numeric cell against one broadcast scalar value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.sub(10);
```

##### Expected output

```text
id  sales  city
-9  90.0   Accra
-8  110.0  Tema
-7  80.0   Kumasi
```

### `sum`

#### API Signature

```cpp
DataFrame::sum(axis: Axis = Axis::Feature, skipna: bool = true) -> Thread
```

#### Notes

Computes per-feature sums for the selected numeric features.

#### Inputs

`axis` : `Axis`
    Axis selector. On `DataFrame`, `0` usually means rows/cases and `1` means features/columns.

`skipna` : `bool`
    When `true`, missing values are skipped during the reduction. When `false`, missing state can affect the result.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Compute `sum` for each numeric column in the frame.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.sum(false);
```

##### Expected output

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
DataFrame::tabelize(catalog: const pluss::kindle::Catalog&) -> DataFrame

DataFrame::tabelize(catalog: const pluss::kindle::Catalog&, policy: const pluss::kindle::TabelizePolicy&) -> DataFrame
```

#### Notes

Converts a Kindle `Catalog` into a tabular `DataFrame`, preserving stream alignment as rows.

#### Inputs

`catalog` : `const pluss::kindle::Catalog&`
    Catalog payload supplied to this overload.

`policy` : `const pluss::kindle::TabelizePolicy&`
    Policy object that changes the structural behavior of this overload.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Convert a Kindle catalog into a frame with the default tabelization policy.

```cpp
using namespace pluss::kindle::literals;
auto catalog = pluss::kindle::Catalog({"sales"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}))});
auto out = pluss::table::DataFrame::tabelize(catalog);
```

##### Expected output

```text
sales
10.0
20.0
30.0
```
Example 2. Convert a Kindle catalog into a frame with an explicit tabelization policy.

```cpp
using namespace pluss::kindle::literals;
auto catalog = pluss::kindle::Catalog({"sales"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0}))});
pluss::kindle::TabelizePolicy policy;
auto out = pluss::table::DataFrame::tabelize(catalog, policy);
```

##### Expected output

```text
sales
10.0
20.0
30.0
```

#### Practical Behavior

- `tabelize(catalog)` is the explicit bridge from Kindle stream collections into the DataFrame model.
- Aligned streams become DataFrame features, and stream position becomes row position.
- Use the policy overload when you want explicit tabelization rules instead of the defaults.


### `tail`

#### API Signature

```cpp
DataFrame::tail(n: std::int64_t = 5, render: bool = false) -> DataFrameView
```

#### Notes

Returns the last `n` rows of the frame.

#### Inputs

`n` : `std::int64_t`
    Number of trailing rows to keep from the bottom of the frame.

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

#### Output

`DataFrameView`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Choose whether the returned object should auto-render in display contexts.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.tail(2, false);
```

##### Expected output

```text
id  sales  city
2   120.0  Tema
3    90.0  Kumasi
```

#### Practical Behavior

- `tail(n)` preserves the original feature order and returns the last `n` rows.
- If `n` is larger than the frame height, the whole frame is returned.
- The returned object is a `DataFrameView`, so it behaves like a lightweight preview slice rather than a deep copy.


### `take`

#### API Signature

```cpp
DataFrame::take(indices: pluss::RangeView<std::int64_t>) -> DataFrame
```

#### Notes

Returns the rows at the requested integer positions.

#### Inputs

`indices` : `pluss::RangeView<std::int64_t>`
    Integer positions used to select, reorder, or address entries.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Pass several integer positions together.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

std::vector<std::int64_t> picks{0, 2};
auto out = df.take(picks);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
3    90.0  Kumasi
```

### `tally`

#### API Signatures

```cpp
DataFrame::tally(feature: std::string_view, top_n: std::int64_t = 0, dropna: bool = true, render: bool = false) -> DataFrameView

DataFrame::tally(features: pluss::RangeView<std::string_view>, top_n: std::int64_t = 0, dropna: bool = true, render: bool = false) -> DataFrameView

DataFrame::tally(top_n: std::int64_t = 0, dropna: bool = true, render: bool = false) -> DataFrameView
```

#### Notes

Counts how many rows fall into each distinct value of the grouping feature.

#### Inputs

`feature` : `std::string_view`
    Name of the source feature used by the operation.

`top_n` : `std::int64_t`
    Optional cap on how many grouped rows to keep after sorting. Use `0` to keep every group.

`dropna` : `bool`
    When `true`, null grouping keys or null values are excluded where the method supports that policy. When `false`, null participates instead of being removed.

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

`features` : `pluss::RangeView<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

#### Output

`DataFrameView`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Count the distinct values in one named feature.

```cpp
auto df = pluss::table::DataFrame({
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema"}))},
    {"product", pluss::table::Thread(pluss::make_note({"A", "B", "A", "B"}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0, 150.0}))},
});

auto out = df.tally("city", 3, true, false);
```

##### Expected output

```text
city    count
Accra   2
Tema    2
```
Example 2. Count distinct combinations across several features.

```cpp
auto df = pluss::table::DataFrame({
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema"}))},
    {"product", pluss::table::Thread(pluss::make_note({"A", "B", "A", "B"}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0, 150.0}))},
});

auto out = df.tally({"city", "product"}, 5, true, false);
```

##### Expected output

```text
city    count
Accra   2
Tema    2
```
Example 3. Count distinct row groups using the default tally target.

```cpp
auto df = pluss::table::DataFrame({
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Accra", "Tema", "Tema"}))},
    {"product", pluss::table::Thread(pluss::make_note({"A", "B", "A", "B"}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0, 150.0}))},
});

auto out = df.tally(3, true, false);
```

##### Expected output

```text
city    count
Accra   2
Tema    2
```

### `timeline_features`

#### API Signature

```cpp
DataFrame::timeline_features() -> DataFrame
```

#### Notes

Returns only the timeline-backed features.

#### Inputs

`None`

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto df = pluss::table::DataFrame({
    {"start", pluss::table::Thread(pluss::make_timeline({
        "2020-01-01T00:00:00Z",
        "2020-01-02T08:00:00Z",
    }))},
    {"end", pluss::table::Thread(pluss::make_timeindex({
        "2020-01-01T01:30:00Z",
        "2020-01-02T09:00:00Z",
    }))},
    {"load", pluss::table::Thread(pluss::make_series({10.0, 20.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema"}))},
});

auto out = df.timeline_features();
```

##### Expected output

```text
start                 end
2020-01-01T00:00:00Z  2020-01-01T01:30:00Z
2020-01-02T08:00:00Z  2020-01-02T09:00:00Z
```

### `title`

#### API Signatures

```cpp
DataFrame::title() -> std::string

DataFrame::title(title: std::string) -> DataFrame
```

#### Notes

Gets the current frame title or returns a titled copy, depending on the overload.

#### Inputs

`title` : `std::string`
    Human-readable title stamp to attach to the returned frame.

#### Output

`std::string`
    Rendered string result.

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Return a titled copy of the current frame.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto titled = df.title("sales_report");
auto out = titled.title();
```

##### Expected output

```text
sales_report
```
Example 2. Return a titled copy of the current frame.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.title("sales_report");
```

##### Expected output

```text
out.title():
sales_report

out.head():
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

#### Practical Behavior

- `title()` with no argument reads the current frame title.
- `title("...")` returns a titled copy and leaves the source frame unchanged.
- `title_("...")` mutates the current frame title in place.


### `title_`

#### API Signature

```cpp
DataFrame::title_(title: std::string) -> DataFrame&
```

#### Notes

Overwrites the current frame title in place.

#### Inputs

`title` : `std::string`
    Human-readable title stamp written back onto the current frame.

#### Output

`DataFrame&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

df.title_("sales_report");
```

##### Expected output

```text
df.title():
sales_report

df.head():
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

#### Practical Behavior

- `title()` with no argument reads the current frame title.
- `title("...")` returns a titled copy and leaves the source frame unchanged.
- `title_("...")` mutates the current frame title in place.


### `tl`

#### API Signatures

```cpp
DataFrame::tl(feature: std::string_view) -> DataFrameTimelineAccessor

DataFrame::tl(features: const std::vector<std::string>&) -> DataFrameTimelineAccessor

DataFrame::tl(features: pluss::RangeView<std::string_view> = {}) -> DataFrameTimelineAccessor

DataFrame::tl(features: std::initializer_list<std::string_view>) -> DataFrameTimelineAccessor
```

#### Notes

This is part of the time-aware surface and expects timeline or time-index semantics.

#### Inputs

`feature` : `std::string_view`
    Name of the source feature used by the operation.

`features` : `const std::vector<std::string>&`
    Feature name(s) that select the target DataFrame columns for the operation.

#### Output

`DataFrameTimelineAccessor`
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

auto out = df.tl();
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 2. Pass several names together from a standard string container.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.tl();
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 3. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.tl();
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 4. Pass several feature names inline.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.tl();
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `to_string`

#### API Signature

```cpp
DataFrame::to_string(options: const display::DisplayOptions& = {}) -> std::string
```

#### Notes

Serializes or renders the current object into a text form you can inspect, log, or export.

#### Inputs

`options` : `const display::DisplayOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`std::string`
    Rendered string result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Pass an explicit options object to control the operation.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.to_string();
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `to_utc`

#### API Signature

```cpp
DataFrame::to_utc(features: pluss::RangeView<std::string_view>) -> DataFrame
```

#### Notes

Converts selected temporal features into UTC timezone representation.

#### Inputs

`features` : `pluss::RangeView<std::string_view>`
    Temporal feature names to convert into UTC timezone representation.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"ts", pluss::table::Thread(pluss::make_timeline({
        "2024-01-01 09:10:00",
        "2024-01-01 09:40:00",
        "2024-01-01 10:05:00",
    }))},
    {"load", pluss::table::Thread(pluss::make_series({10.0, 20.0, 15.0}))},
});

auto localized = df.tz_localize({"ts"}, "UTC");
auto out = localized.to_utc({"ts"});
```

##### Expected output

```text
ts
2024-01-01T09:10:00Z
2024-01-01T09:40:00Z
2024-01-01T10:05:00Z
```

### `top`

#### API Signature

```cpp
DataFrame::top(feature: std::string_view, n: std::int64_t) -> DataFrame
```

#### Notes

Returns the rows with the largest values in the selected feature.

#### Inputs

`feature` : `std::string_view`
    Name of the source feature used by the operation.

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

#### Output

`DataFrame`
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

auto out = df.top("sales", 2);
```

##### Expected output

```text
id  sales  city
2   120.0  Tema
1   100.0  Accra
```

#### Practical Behavior

- `top(feature, n)` sorts descending by the selected feature, keeps nulls last, then returns the first `n` rows.
- The output rows stay aligned across every feature; this is not a single-column extraction.
- Use `bottom(...)` when you want the smallest rows instead of the largest ones.


### `transform`

#### API Signatures

```cpp
DataFrame::transform(feature: std::string_view, fn: const std::function<Thread(const DataFrame&)>&) -> DataFrame

DataFrame::transform(feature: std::string_view, fn: const std::function<Thread(const Thread&)>&) -> DataFrame

DataFrame::transform(fn: const std::function<DataFrame(const DataFrame&)>&, axis: Axis, shape: ResultShape = ResultShape::Infer) -> DataFrame

DataFrame::transform(fn: const std::function<Thread(const Thread&)>&) -> DataFrame

DataFrame::transform(fn: const std::function<Thread(const Thread&)>&, axis: Axis) -> DataFrame

DataFrame::transform(fn: const std::function<ThreadValue(const DataFrame&)>&, axis: Axis) -> Thread

DataFrame::transform(specs: pluss::RangeView<TransformSpec>) -> DataFrame
```

#### Notes

Rewrites one feature, several features, or all features by applying a user function to each targeted thread.

#### Inputs

`feature` : `std::string_view`
    Target feature name to rewrite, or output feature name when you use the frame-aware derive overload.

`fn` : `const std::function<Thread(const DataFrame&)>&`
    Thread-to-thread or frame-to-thread transform function, depending on the overload. The returned thread must match the frame case count.

`axis` : `Axis`
    Axis selector. On `DataFrame`, `0` usually means rows/cases and `1` means features/columns.

`shape` : `ResultShape`
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

`specs` : `pluss::RangeView<TransformSpec>`
    Collection of structured operation specs. Each element describes one requested summary, derive, transform, group, or placement rule depending on the method.

#### Output

`DataFrame`
    Value produced by this call.

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Derive or replace one feature by computing it from the full frame.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.transform("sales_plus_10", [](const pluss::table::DataFrame& frame) { return pluss::table::Thread(frame.features("sales").as_series().add(10.0)); });
```

##### Expected output

```text
id  sales  city    sales_plus_10
1   100.0  Accra   110.0
2   120.0  Tema    130.0
3    90.0  Kumasi  100.0
```
Example 2. Rewrite one named feature with a thread-level transform.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.transform("sales", [](const pluss::table::Thread& thread) { return thread.clip(0.0, 110.0); });
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   110.0  Tema
3    90.0  Kumasi
```
Example 3. Rewrite one named feature with a thread-level transform.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.transform([](const pluss::table::Thread& thread) { return thread.kind() == pluss::table::ThreadKind::Series ? thread.clip(0.0, 110.0) : thread; });
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   110.0  Tema
3    90.0  Kumasi
```
Example 4. Rewrite every feature with the same thread-level transform.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.transform([](const pluss::table::Thread& thread) { return thread.kind() == pluss::table::ThreadKind::Series ? thread.clip(0.0, 110.0) : thread; });
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   110.0  Tema
3    90.0  Kumasi
```
Example 5. Rewrite every feature with the same thread-level transform.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.transform([](const pluss::table::Thread& thread) { return thread.kind() == pluss::table::ThreadKind::Series ? thread.clip(0.0, 110.0) : thread; });
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   110.0  Tema
3    90.0  Kumasi
```
Example 6. Rewrite one named feature with a thread-level transform.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.transform([](const pluss::table::Thread& thread) { return thread.kind() == pluss::table::ThreadKind::Series ? thread.clip(0.0, 110.0) : thread; });
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   110.0  Tema
3    90.0  Kumasi
```
Example 7. Rewrite one or more selected features with structured transform specs.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.transform({{.feature = "sales", .expr = [](const pluss::table::Thread& thread) { return thread.clip(0.0, 110.0); }}});
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   110.0  Tema
3    90.0  Kumasi
```

#### Practical Behavior

- `transform(fn)` rewrites every feature with the same thread-level function.
- `transform(feature, thread_fn)` rewrites one existing feature in place within the returned frame.
- `transform(feature, frame_fn)` is the derive-style overload: it can create a new feature from the whole frame, as long as the returned thread matches the frame case count.


### `transpose`

#### API Signature

```cpp
DataFrame::transpose(render: bool = false) -> DataFrameView
```

#### Notes

Swaps rows and features so former feature names become row labels.

#### Inputs

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

#### Output

`DataFrameView`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Choose whether the returned object should auto-render in display contexts.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.transpose(false);
```

##### Expected output

```text
feature  row0   row1   row2
id       1      2      3
sales    100.0  120.0  90.0
```

### `truediv`

#### API Signatures

```cpp
DataFrame::truediv(other: const DataFrame&) -> DataFrame

DataFrame::truediv(other: double) -> DataFrame
```

#### Notes

Applies elementwise true division to compatible numeric features.

#### Inputs

`other` : `const DataFrame&`
    Scalar divisor or right-hand frame used for elementwise true division.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Divide each compatible numeric cell against another aligned frame.

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

##### Expected output

```text
id    sales
0.5   0.909091
1.0   1.090909
0.75  0.9
```
Example 2. Divide each compatible numeric cell against one broadcast scalar value.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.truediv(10);
```

##### Expected output

```text
id  sales  city
0.1  10.0  Accra
0.2  12.0  Tema
0.3   9.0  Kumasi
```

#### Practical Behavior

- Scalar `truediv(...)` divides every compatible numeric cell by the supplied scalar.
- Frame-vs-frame `truediv(...)` divides aligned numeric features row by row.
- Non-numeric features pass through unchanged in the scalar form and are excluded from numeric combination work.


### `ts`

#### API Signatures

```cpp
DataFrame::ts(feature: std::string_view) -> DataFrameTsAccessor

DataFrame::ts(features: pluss::RangeView<std::string_view> = {}) -> DataFrameTsAccessor

DataFrame::ts(features: std::initializer_list<std::string_view>) -> DataFrameTsAccessor
```

#### Notes

Returns the time-series accessor bound to one or more selected features.

#### Inputs

`feature` : `std::string_view`
    Feature name to bind into the returned time-series accessor.

`features` : `pluss::RangeView<std::string_view>`
    Feature names to bind into the returned time-series accessor.

#### Output

`DataFrameTsAccessor`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Bind the time-series accessor to one feature.

```cpp
auto df = pluss::table::DataFrame({
    {"ts", pluss::table::Thread(pluss::make_timeline({
        "2024-01-01T09:10:00Z",
        "2024-01-01T09:40:00Z",
        "2024-01-01T10:05:00Z",
    }))},
    {"load", pluss::table::Thread(pluss::make_series({10.0, 20.0, 15.0}))},
});

auto out = df.features({"load"}).ts();
```

##### Expected output

```text
DataFrameTsAccessor(features=["load"])
```
Example 2. Bind the time-series accessor to a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"ts", pluss::table::Thread(pluss::make_timeline({
        "2024-01-01T09:10:00Z",
        "2024-01-01T09:40:00Z",
        "2024-01-01T10:05:00Z",
    }))},
    {"load", pluss::table::Thread(pluss::make_series({10.0, 20.0, 15.0}))},
});

auto out = df.features({"load"}).ts();
```

##### Expected output

```text
DataFrameTsAccessor(features=["load"])
```
Example 3. Bind the time-series accessor by passing several feature names inline.

```cpp
auto df = pluss::table::DataFrame({
    {"ts", pluss::table::Thread(pluss::make_timeline({
        "2024-01-01T09:10:00Z",
        "2024-01-01T09:40:00Z",
        "2024-01-01T10:05:00Z",
    }))},
    {"load", pluss::table::Thread(pluss::make_series({10.0, 20.0, 15.0}))},
});

auto out = df.features({"load"}).ts();
```

##### Expected output

```text
DataFrameTsAccessor(features=["load"])
```

#### Practical Behavior

- `ts(...)` does not materialize a new DataFrame by itself; it returns the time-series accessor bound to the selected features.
- Use the accessor when you want downstream resampling, lag blocks, rolling blocks, or calendar features on those bound features.


### `tz_convert`

#### API Signature

```cpp
DataFrame::tz_convert(features: pluss::RangeView<std::string_view>, tz_name: std::string_view) -> DataFrame
```

#### Notes

This is part of the time-aware surface and expects timeline or time-index semantics.

#### Inputs

`features` : `pluss::RangeView<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`tz_name` : `std::string_view`
    Destination timezone name such as `"UTC"` or `"America/Edmonton"`.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"ts", pluss::table::Thread(pluss::make_timeline({
        "2024-01-01 09:10:00",
        "2024-01-01 09:40:00",
        "2024-01-01 10:05:00",
    }))},
    {"load", pluss::table::Thread(pluss::make_series({10.0, 20.0, 15.0}))},
});

auto localized = df.tz_localize({"ts"}, "UTC");
auto out = localized.tz_convert({"ts"}, "UTC");
```

##### Expected output

```text
ts
2024-01-01T09:10:00Z
2024-01-01T09:40:00Z
2024-01-01T10:05:00Z
```

### `tz_localize`

#### API Signature

```cpp
DataFrame::tz_localize(features: pluss::RangeView<std::string_view>, tz_name: std::string_view, fold: int = 0, nonexistent: std::string_view = "raise") -> DataFrame
```

#### Notes

This is part of the time-aware surface and expects timeline or time-index semantics.

#### Inputs

`features` : `pluss::RangeView<std::string_view>`
    Feature name(s) that select the target DataFrame columns for the operation.

`tz_name` : `std::string_view`
    Timezone name to attach to naive temporal values.

`fold` : `int`
    Fold flag used when localizing ambiguous wall-clock times around a DST fall-back transition.

`nonexistent` : `std::string_view`
    Policy for nonexistent local times, such as `"raise"` or `"shift_forward"`.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"ts", pluss::table::Thread(pluss::make_timeline({
        "2024-01-01 09:10:00",
        "2024-01-01 09:40:00",
        "2024-01-01 10:05:00",
    }))},
    {"load", pluss::table::Thread(pluss::make_series({10.0, 20.0, 15.0}))},
});

auto out = df.tz_localize({"ts"}, "UTC", 0, "shift_forward");
```

##### Expected output

```text
ts
2024-01-01T09:10:00Z
2024-01-01T09:40:00Z
2024-01-01T10:05:00Z
```

## U

### `unify`

#### API Signatures

```cpp
DataFrame::unify(left_on: pluss::RangeView<std::string_view>, right: const DataFrame&, right_on: pluss::RangeView<std::string_view> = {}, how: std::string_view = "common", right_suffix: std::string = "_right", validate: std::string_view = "many_to_many") -> DataFrame

DataFrame::unify(left_on: std::initializer_list<std::string_view>, right: const DataFrame&, right_on: std::initializer_list<std::string_view> = {}, how: std::string_view = "common", right_suffix: std::string = "_right", validate: std::string_view = "many_to_many") -> DataFrame

DataFrame::unify(left_on: std::string_view, right: const DataFrame&, right_on: std::string_view = "", how: std::string_view = "common", right_suffix: std::string = "_right", validate: std::string_view = "many_to_many") -> DataFrame
```

#### Notes

Combines two frames by key values. Use `right_on=""` when both frames use the same key name, Pluss-native modes such as `"common"` or `"unite"` to choose retained keys, and `validate` for duplicate-key checks.

#### Inputs

`left_on` : `pluss::RangeView<std::string_view>`
    Key feature name(s) on the left frame used to match rows during `unify(...)`.

`right` : `const DataFrame&`
    Right-hand frame whose rows are matched against the left frame by the requested key features.

`right_on` : `pluss::RangeView<std::string_view>`
    Key feature name(s) on the right frame used to match rows during `unify(...)`. Leave empty to reuse the same names as `left_on`.

`how` : `std::string_view`
    Join mode for `DataFrame::unify(...)`.

    Supported values:
    - `common`: Keep only rows whose key values match on both sides.
    - `keep_left`: Preserve left keys and attach right-hand matches where they exist.
    - `keep_right`: Preserve right keys and attach left-hand matches where they exist.
    - `unite`: Keep the stable union of keys from both frames.
    - `cross`: Create the cartesian product; key feature arguments must be empty.
    - `left_only`: Keep left rows whose keys do not occur on the right.
    - `right_only`: Keep right rows whose keys do not occur on the left.

`right_suffix` : `std::string`
    Suffix added to right-hand feature names only when a non-key right feature would otherwise collide with an existing left feature name.

`validate` : `std::string_view`
    Optional key-cardinality check for `DataFrame::unify(...)`.

    Supported values:
    - `many_to_many`: Allow duplicate key values on both sides. This is the default.
    - `one_to_one`: Require each key to appear at most once on both sides.
    - `one_to_many`: Require unique keys on the left and allow duplicates on the right.
    - `many_to_one`: Allow duplicates on the left and require unique keys on the right.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Join two frames on several key features supplied through a reusable string view.

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

##### Expected output

```text
OrgDefinedId  midterm  Final Points Grade
10            71       null
20            82       88
30            93       99
```
Example 2. Join two frames on several key features listed inline.

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

##### Expected output

```text
course   OrgDefinedId  midterm  course_code  Username  Final Points Grade
STAT200  20            82       STAT200      20        88
STAT300  30            93       STAT300      30        99
```
Example 3. Join two frames on one key feature name.

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

##### Expected output

```text
OrgDefinedId  midterm  Final Points Grade
10            71       null
20            82       88
30            93       99
```

#### Join Semantics

- `unify(...)` matches rows by key values, not by current row order.
- `common`, `keep_left`, `keep_right`, and `unite` retain the corresponding key set while preserving stable input order.
- `left_only` and `right_only` select unmatched keys; `cross` creates the cartesian case product without key features.
- Repeated keys expand to every matching pair, and `validate` can enforce one-to-one or one-sided uniqueness.
- When right-hand non-key names collide with left-hand names, `right_suffix` is applied only to the right-hand collisions.


### `unique`

#### API Signature

```cpp
DataFrame::unique() -> bool
```

#### Notes

Reports whether the object satisfies the uniqueness constraint expected by its model, such as distinct labels or keys.

#### Inputs

`None`

#### Output

`bool`
    Boolean status or membership result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Call the simplest overload.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.unique();
```

##### Expected output

```text
true
```

### `unique_cases`

#### API Signature

```cpp
DataFrame::unique_cases(features: pluss::RangeView<std::string_view> = {}) -> DataFrame
```

#### Notes

Keeps only the first occurrence of each case label and drops later duplicates.

#### Inputs

`features` : `pluss::RangeView<std::string_view>`
    Optional feature subset used to decide which rows count as duplicates. Leave empty to compare the full row payload.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Work with a selected feature subset.

```cpp
auto df = pluss::table::DataFrame({
    {"campus", pluss::table::Thread(pluss::make_note({"North", "North", "West"}))},
    {"student", pluss::table::Thread(pluss::make_note({"Ada", "Kojo", "Esi"}))},
    {"score", pluss::table::Thread(pluss::make_series({88.0, 91.0, 84.0}))},
});

auto out = df.unique_cases({"campus"});
```

##### Expected output

```text
campus  student  score
North   Ada      88.0
West    Esi      84.0
```

#### Practical Behavior

- `unique_cases(features)` keeps only the first row for each distinct key combination defined by `features`.
- With no feature list, the full row payload is used as the duplicate key.
- This is the row-filtering companion to `repeated_cases(...)`.


### `unsqueeze`

#### API Signatures

```cpp
DataFrame::unsqueeze() -> Atlas

DataFrame::unsqueeze(depth_labels: const Thread&) -> Atlas
```

#### Notes

Promotes the frame into a depth-1 `Atlas` so it can participate in multi-frame workflows.

#### Inputs

`depth_labels` : `const Thread&`
    Single depth label to attach to the one-frame atlas produced by `unsqueeze(...)`.

#### Output

`Atlas`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Lift one frame into a one-layer atlas.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.unsqueeze();
```

##### Expected output

```text
frame0:
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 2. Lift one frame into a one-layer atlas.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto labels = pluss::table::Thread(pluss::make_note({"train"}));
auto out = df.unsqueeze(labels);
```

##### Expected output

```text
depth labels:
["train"]

frame0:
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

#### Practical Behavior

- `unsqueeze()` wraps one frame into a depth-1 `Atlas`.
- The `depth_labels` overload lets you attach exactly one depth label to that one-frame atlas.
- This is useful when downstream APIs expect atlas depth even though you currently have only one frame.


### `unstack`

#### API Signatures

```cpp
DataFrame::unstack(level: std::string_view) -> DataFrame

DataFrame::unstack(options: const UnstackOptions&) -> DataFrame
```

#### Notes

Use this method through the public contract shown above.

#### Inputs

`level` : `std::string_view`
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

`options` : `const UnstackOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.unstack();
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
Example 2. Pass an explicit options object to control the operation.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.unstack();
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `update`

#### API Signature

```cpp
DataFrame::update(other: const DataFrame&, overwrite: bool = true) -> DataFrame
```

#### Notes

Overlays matching features from another frame onto this frame using aligned cases.

#### Inputs

`other` : `const DataFrame&`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

`overwrite` : `bool`
    When `true`, matching values from the right-hand side overwrite existing left-hand values during update-style operations.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

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

##### Expected output

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
DataFrame::var(axis: Axis = Axis::Feature, skipna: bool = true) -> Thread

DataFrame::var(axis: Axis, options: const StatisticOptions&) -> Thread
```

#### Notes

Computes per-feature variances for the selected numeric features.

#### Inputs

`axis` : `Axis`
    Axis selector. On `DataFrame`, `0` usually means rows/cases and `1` means features/columns.

`skipna` : `bool`
    When `true`, missing values are skipped during the reduction. When `false`, missing state can affect the result.

`options` : `const StatisticOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`Thread`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Compute `var` for each numeric column in the frame.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.var(false);
```

##### Expected output

```text
feature  value
id       1.0
sales    233.333333333
```
Example 2. Compute `var` for each numeric column in the frame.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.var(false);
```

##### Expected output

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
DataFrame::view(start: std::int64_t, stop: std::int64_t, render: bool = false) -> DataFrameView
```

#### Notes

Returns a lightweight row view of the requested slice.

#### Inputs

`start` : `std::int64_t`
    Slice start position. Negative values count backward from the end of the frame.

`stop` : `std::int64_t`
    Slice stop position. Negative values count backward from the end, and `-1` includes the last row.

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

#### Output

`DataFrameView`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Choose whether the returned object should auto-render in display contexts.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.view(0, 2, false);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
```

#### Slice Semantics

- `view(start, stop)` behaves like a positional row slice `[start, stop)`.
- Negative `start` or `stop` values count backward from the end of the frame.
- Unlike `slice(...)`, `view(...)` returns a `DataFrameView` wrapper intended for lightweight inspection.


## W

### `weigh`

#### API Signature

```cpp
DataFrame::weigh(weights: pluss::RangeView<double>, statistic: WindowStatistic = WindowStatistic::Mean, min_count: std::int64_t = 1) -> DataFrame
```

#### Notes

Use this method through the public contract shown above.

#### Inputs

`weights` : `pluss::RangeView<scalar>`
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

`statistic` : `WindowStatistic`
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

`min_count` : `std::int64_t`
    Minimum non-null observations required before a grouped or rolling summary is considered valid.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.weigh();
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `where`

#### API Signatures

```cpp
DataFrame::where(bool_mask: const pluss::Series&) -> DataFrame

DataFrame::where(bool_mask: const Thread&) -> DataFrame
```

#### Notes

Keeps only the rows where the supplied boolean mask is true.

#### Inputs

`bool_mask` : `const pluss::Series&`
    Boolean row mask. Rows where the mask is true are kept; rows where it is false are dropped.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Filter rows using an explicit boolean Series mask.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = pluss::make_series({true, false, true}, pluss::ScalarType::Bool);
auto out = df.where(mask);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
3    90.0  Kumasi
```
Example 2. Filter rows using a boolean mask built from an existing frame expression.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto mask = df.features("sales") >= 95.0;
auto out = df.where(mask);
```

##### Expected output

```text
id  sales  city
1   100.0  Accra
3    90.0  Kumasi
```

#### Practical Behavior

- `where(mask)` keeps rows where the boolean mask is true and drops the rest.
- The mask length must match the frame case count exactly.
- Use `put(...)` when you want masked replacement inside one feature instead of masked row filtering.


### `window`

#### API Signature

```cpp
DataFrame::window(specs: pluss::RangeView<WindowSpec>, keep_input: bool = true) -> DataFrame
```

#### Notes

Computes one or more window-based features such as rolling means over selected columns.

#### Inputs

`specs` : `pluss::RangeView<WindowSpec>`
    Collection of structured operation specs. Each element describes one requested summary, derive, transform, group, or placement rule depending on the method.

`keep_input` : `bool`
    When `true`, the original input features are preserved and engineered window outputs are appended. When `false`, only the engineered outputs are returned.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = df.window({{.output = "sales_roll", .feature = "sales", .op = pluss::table::ThreadWindowOp::RollingMean, .window = 2, .min_periods = 1}}, false);
```

##### Expected output

```text
sales_roll
100.0
110.0
105.0
```

#### Practical Behavior

- `window(...)` engineers one or more derived features from rolling/window operations described by `WindowSpec`.
- `keep_input=false` returns only the engineered outputs, which is useful for model-matrix style pipelines.
- `rolling(...)` is the alias that delegates to the same frame-level implementation.


### `with_cases`

#### API Signature

```cpp
DataFrame::with_cases(cases: const Thread&) -> DataFrame
```

#### Notes

Replaces the current case axis with the supplied thread. The row values stay in the same order, but later case-aware operations now use the new labels.

#### Inputs

`cases` : `const Thread&`
    Case-label thread to attach to the current object.

#### Output

`DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto cases = pluss::table::Thread(pluss::make_note({"cs0", "cs1", "cs2"}));
auto out = df.with_cases(cases);
```

##### Expected output

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
DataFrame::with_cases_(cases: const Thread&) -> DataFrame&
```

#### Notes

Replaces the current case axis in place. The row values stay in the same order, but later case-aware operations now use the new labels.

#### Inputs

`cases` : `const Thread&`
    Case-label thread to attach to the current object.

#### Output

`DataFrame&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

Example 1. Use this overload when its parameter combination matches the result shape or control level you need.

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto cases = pluss::table::Thread(pluss::make_note({"cs0", "cs1", "cs2"}));
df.with_cases_(cases);
```

##### Expected output

```text
df.cases():
["cs0", "cs1", "cs2"]

df.head():
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```
