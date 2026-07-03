# Catalog API Reference

# Practical Catalog API Reference

## API Reference Style

This page documents the full public `Catalog` surface for named stream collections.

### `add_text_features`

#### Signature

```cpp
Catalog::add_text_features(field: std::string_view) -> Catalog
```

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

#### Inputs

`field` : `std::string_view`
    Name of the target stream in the catalog.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "text"_k = pluss::kindle::Stream(pluss::make_note({
        "heavy rain on road",
        "clear weather downtown",
        "storm warning issued",
    })),
});

auto out = ct.add_text_features("text");
```

#### Expected output

```text
ct.head():
text -> Note(["heavy rain on road", "clear weather downtown", "storm warning issued"])

out.head():
text            -> Note(["heavy rain on road", "clear weather downtown", "storm warning issued"])
text_char_len   -> Series([18, 22, 20])
text_word_count -> Series([4, 3, 3])
```

#### Practical Behavior

- `add_text_features(field)` adds `<field>_char_len` and `<field>_word_count` streams.
- The source field is coerced through note/text handling before the text statistics are computed.


### `aggregate_text_stats`

#### Signature

```cpp
Catalog::aggregate_text_stats(field: std::string_view, stat_type: std::string_view) -> Catalog
```

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

#### Inputs

`field` : `std::string_view`
    Name of the target stream in the catalog.

`stat_type` : `std::string_view`
    Statistic selector string understood by this overload.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "text"_k = pluss::kindle::Stream(pluss::make_note({
        "heavy rain on road",
        "clear weather downtown",
        "storm warning issued",
    })),
});

auto out = ct.aggregate_text_stats("text", "mean");
```

#### Expected output

```text
text_mean_char_len -> Series([20.0])
```

#### Practical Behavior

- `aggregate_text_stats(field, stat_type)` collapses one text field into a one-value summary stream.
- Supported statistics include common length-based summaries such as mean/min/max/count, along with vocabulary size.


### `apply`

#### Signature

```cpp
Catalog::apply(field: std::string_view, func: const std::function<Stream(const Stream&)>&) -> Catalog
```

#### Notes

Applies a function to one named stream and returns the updated catalog.

#### Inputs

`field` : `std::string_view`
    Name of the target stream in the catalog.

`func` : `const std::function<Stream(const Stream&)>&`
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.apply("metric", [](const pluss::kindle::Stream& s) { return s.clip(0.0, 25.0); });
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

#### Practical Behavior

- `apply(...)` rewrites one named stream; `apply_all(...)` rewrites every stream.
- The transform must return a defined stream with the same length as the input stream.
- Use the underscore form when you want the mutation in place.


### `apply_`

#### Signature

```cpp
Catalog::apply_(field: std::string_view, func: const std::function<Stream(const Stream&)>&) -> Catalog&
```

#### Notes

Applies a function to one named stream in place.

#### Inputs

`field` : `std::string_view`
    Name of the target stream in the catalog.

`func` : `const std::function<Stream(const Stream&)>&`
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Output

`Catalog&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.apply_("metric", [](const pluss::kindle::Stream& s) { return s.clip(0.0, 25.0); });
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `apply_all`

#### Signature

```cpp
Catalog::apply_all(fn: const std::function<Stream(const Stream&)>&) -> Catalog
```

#### Notes

Applies the same function to every stream and returns the updated catalog.

#### Inputs

`fn` : `const std::function<Stream(const Stream&)>&`
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.apply_all([](const pluss::kindle::Stream& s) { return s.kind() == pluss::kindle::StreamKind::Series ? s.clip(0.0, 25.0) : s; });
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

#### Practical Behavior

- `apply(...)` rewrites one named stream; `apply_all(...)` rewrites every stream.
- The transform must return a defined stream with the same length as the input stream.
- Use the underscore form when you want the mutation in place.


### `apply_all_`

#### Signature

```cpp
Catalog::apply_all_(fn: const std::function<Stream(const Stream&)>&) -> Catalog&
```

#### Notes

Applies the same function to every stream in place.

#### Inputs

`fn` : `const std::function<Stream(const Stream&)>&`
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Output

`Catalog&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.apply_all_([](const pluss::kindle::Stream& s) { return s.kind() == pluss::kindle::StreamKind::Series ? s.clip(0.0, 25.0) : s; });
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `astype`

#### Signature

```cpp
Catalog::astype(keys: std::initializer_list<std::string_view>, dtype: pluss::ScalarType) -> Catalog
```

#### Notes

This changes the semantic or numeric type so later operations route through the correct implementation path.

#### Inputs

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

`dtype` : `pluss::ScalarType`
    Target scalar dtype used by this conversion overload.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.astype({"metric"}, pluss::ScalarType::Float64);
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

#### Conversion Rules

- `astype(keys, dtype)` is the catalog-level numeric-series conversion helper.
- It delegates to stream/thread conversion on the selected keys only.


### `at`

#### Signature

```cpp
Catalog::at(key: std::string_view) -> Stream
```

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### Inputs

`key` : `std::string_view`
    Name of the target slot or stream.

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.at("metric");
```

#### Expected output

```text
Series([10.0, 20.0, 30.0])
```

#### Practical Behavior

- `at(key)` fetches one named stream from the catalog.
- Use it when you want stream access without routing through table conversion first.


### `begin`

#### Signature

```cpp
Catalog::begin() -> const_iterator
```

#### Notes

Exposes iterator boundaries so the object can be traversed with a range-based loop or manual iteration.

#### Inputs

`None`

#### Output

`const_iterator`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

for (const auto& key : ct) {
  (void)key;
}
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `bfill`

#### Signature

```cpp
Catalog::bfill(keys: std::initializer_list<std::string_view> = {}) -> Catalog
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;
auto ct = pluss::kindle::Catalog({
    "Final Points Grade"_k = pluss::kindle::Stream(pluss::make_series({71.0, 0.0, 88.0, 0.0})).with_validity_mask(pluss::make_series({true, false, true, false}, pluss::ScalarType::Bool)),
    "Midterm 3"_k = pluss::kindle::Stream(pluss::make_series({60.0, 0.0, 75.0, 91.0})).with_validity_mask(pluss::make_series({true, false, true, true}, pluss::ScalarType::Bool)),
    "Name"_k = pluss::kindle::Stream(pluss::make_note({"A", "B", "C", "D"})),
});

auto out = ct.bfill({"Final Points Grade", "Midterm 3"});
```

#### Expected output

```text
ct.head():
Final Points Grade -> Series([71.0, null, 88.0, null])
Midterm 3          -> Series([60.0, null, 75.0, 91.0])
Name               -> Note(["A", "B", "C", "D"])

out.head():
Final Points Grade -> Series([71.0, 88.0, 88.0, null])
Midterm 3          -> Series([60.0, 75.0, 75.0, 91.0])
Name               -> Note(["A", "B", "C", "D"])
```

#### Practical Behavior

- With no keys supplied, `bfill()` is applied to every stream in the catalog.
- If you pass keys, only those streams are backward-filled; all other streams are left unchanged.
- Unknown keys throw immediately instead of silently skipping.

#### Common Pattern

```cpp
auto cleaned = catalog.bfill({"Final Points Grade", "Midterm 3"});
```


### `cast_note`

#### Signature

```cpp
Catalog::cast_note(keys: std::initializer_list<std::string_view>) -> Catalog
```

#### Notes

This changes the semantic or numeric type so later operations route through the correct implementation path.

#### Inputs

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.cast_note({"metric"});
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

#### Conversion Rules

- These helpers rewrite only the selected keys and leave all other streams unchanged.
- Unknown keys throw immediately.


### `cast_series`

#### Signature

```cpp
Catalog::cast_series(keys: std::initializer_list<std::string_view>, dtype: std::optional<pluss::ScalarType> = std::nullopt) -> Catalog
```

#### Notes

This changes the semantic or numeric type so later operations route through the correct implementation path.

#### Inputs

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

`dtype` : `std::optional<pluss::ScalarType>`
    Target scalar dtype used by this conversion overload.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;
auto ct = pluss::kindle::Catalog({"metric"_k = pluss::kindle::Stream(pluss::make_note({"10.0", "20.0", "30.0"}))});

auto out = ct.cast_series({"metric"}, pluss::ScalarType::Float64);
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

#### Conversion Rules

- Catalog cast helpers rewrite only the selected keys.
- Unknown keys throw immediately.


### `cast_timeindex`

#### Signature

```cpp
Catalog::cast_timeindex(keys: std::initializer_list<std::string_view>) -> Catalog
```

#### Notes

This changes the semantic or numeric type so later operations route through the correct implementation path.

#### Inputs

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.cast_timeindex({"metric"});
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

#### Conversion Rules

- These helpers rewrite only the selected keys and leave all other streams unchanged.
- Unknown keys throw immediately.


### `cast_timeline`

#### Signature

```cpp
Catalog::cast_timeline(keys: std::initializer_list<std::string_view>) -> Catalog
```

#### Notes

This changes the semantic or numeric type so later operations route through the correct implementation path.

#### Inputs

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.cast_timeline({"metric"});
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

#### Conversion Rules

- These helpers rewrite only the selected keys and leave all other streams unchanged.
- Unknown keys throw immediately.


### `cloc`

#### Signature

```cpp
Catalog::cloc(row: std::int64_t) -> Catalog
```

#### Notes



#### Inputs

`row` : `std::int64_t`
    Zero-based row position used by this overload.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::cloc(start: std::int64_t, stop: std::int64_t) -> Catalog`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.cloc();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

#### Practical Behavior

- `cloc(...)` slices aligned row positions across every stream in the catalog.
- Use it when you want row-wise positional access while keeping stream alignment intact.


### `cloc`

#### Signature

```cpp
Catalog::cloc(start: std::int64_t, stop: std::int64_t) -> Catalog
```

#### Notes



#### Inputs

`start` : `std::int64_t`
    Inclusive starting position of the slice.

`stop` : `std::int64_t`
    Exclusive ending position of the slice.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::cloc(row: std::int64_t) -> Catalog`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.cloc();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

#### Practical Behavior

- `cloc(...)` slices aligned row positions across every stream in the catalog.
- Use it when you want row-wise positional access while keeping stream alignment intact.


### `compute_similarity`

#### Signature

```cpp
Catalog::compute_similarity(field1: std::string_view, field2: std::string_view, method: std::string_view = "cosine") -> Catalog
```

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

#### Inputs

`field1` : `std::string_view`
    First named field used by this overload.

`field2` : `std::string_view`
    Second named field used by this overload.

`method` : `std::string_view`
    Method selector string understood by this overload.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "text"_k = pluss::kindle::Stream(pluss::make_note({
        "heavy rain on road",
        "clear weather downtown",
        "storm warning issued",
    })),
});

auto out = ct.compute_similarity("text", "text", "cosine");
```

#### Expected output

```text
text__text_similarity -> Series([1.0, 1.0, 1.0])
```

#### Practical Behavior

- `compute_similarity(field1, field2, "cosine")` computes row-wise text similarity and appends the result as a numeric stream.
- At the moment the supported similarity method is cosine.
- Both fields must have the same length.


### `count`

#### Signature

```cpp
Catalog::count() -> std::int64_t
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

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.count();
```

#### Expected output

```text
3
```

#### Practical Behavior

- `count()` returns the catalog row count after alignment through `tabelize()`.
- This is the quick “how many aligned rows do I have?” answer.


### `defined`

#### Signature

```cpp
Catalog::defined() -> bool
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

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.defined();
```

#### Expected output

```text
true
```

### `display`

#### Signature

```cpp
Catalog::display() -> void
```

#### Notes

This renders a human-readable view for inspection instead of creating a new analytic object.

#### Inputs

`None`

#### Output

`void`
    Performs the side effect and returns no value.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.display();
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

#### Practical Behavior

- `display()` renders a human-readable catalog view for inspection.
- Use it for inspection and debugging rather than transformation.


### `drop`

#### Signature

```cpp
Catalog::drop(field_name: std::string_view) -> Catalog
```

#### Notes

Removes the named streams and keeps the remaining catalog structure intact.

#### Inputs

`field_name` : `std::string_view`
    Field name used to select the target text, stream, or grouping column.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::drop(field_names: const std::vector<std::string>&) -> Catalog`
- `Catalog::drop(keys: std::initializer_list<std::string_view>) -> Catalog`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.drop("city");
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
```

#### Practical Behavior

- `drop(...)` removes named streams, not rows.
- The remaining streams keep their alignment.


### `drop`

#### Signature

```cpp
Catalog::drop(field_names: const std::vector<std::string>&) -> Catalog
```

#### Notes

Removes the named streams and keeps the remaining catalog structure intact.

#### Inputs

`field_names` : `const std::vector<std::string>&`
    Field names supplied together for this overload.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::drop(field_name: std::string_view) -> Catalog`
- `Catalog::drop(keys: std::initializer_list<std::string_view>) -> Catalog`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

std::vector<std::string> names{"city"};
auto out = ct.drop(names);
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
```

#### Practical Behavior

- `drop(...)` removes named streams, not rows.
- The remaining streams keep their alignment.


### `drop`

#### Signature

```cpp
Catalog::drop(keys: std::initializer_list<std::string_view>) -> Catalog
```

#### Notes

Removes the named streams and keeps the remaining catalog structure intact.

#### Inputs

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::drop(field_name: std::string_view) -> Catalog`
- `Catalog::drop(field_names: const std::vector<std::string>&) -> Catalog`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.drop({"city"});
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
```

#### Practical Behavior

- `drop(...)` removes named streams, not rows.
- The remaining streams keep their alignment.


### `dropna`

#### Signature

```cpp
Catalog::dropna(keys: std::initializer_list<std::string_view> = {}) -> Catalog
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;
auto metric = pluss::kindle::Stream(pluss::make_series({10.0, 0.0, 30.0}))
                 .with_validity_mask(pluss::make_series({true, false, true}, pluss::ScalarType::Bool));
auto ct = pluss::kindle::Catalog({
    "metric"_k = metric,
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.dropna({"metric"});
```

#### Expected output

```text
ct.head():
metric -> Series([10.0, null, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
metric -> Series([10.0, 30.0])
city   -> Note(["Accra", "Kumasi"])
```

#### Practical Behavior

- `dropna(keys)` applies stream-level `dropna()` to the selected keys.
- With no keys supplied, every stream is processed.
- Unknown keys throw immediately.


### `empty`

#### Signature

```cpp
Catalog::empty() -> bool
```

#### Notes

Reports whether the current object has no rows, entries, slots, or depth items.

#### Inputs

`None`

#### Output

`bool`
    Boolean status or membership result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.empty();
```

#### Expected output

```text
false
```

### `end`

#### Signature

```cpp
Catalog::end() -> const_iterator
```

#### Notes

Exposes iterator boundaries so the object can be traversed with a range-based loop or manual iteration.

#### Inputs

`None`

#### Output

`const_iterator`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

for (const auto& key : ct) {
  (void)key;
}
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `extract_keywords`

#### Signature

```cpp
Catalog::extract_keywords(field: std::string_view, top_n: int = 10) -> Catalog
```

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

#### Inputs

`field` : `std::string_view`
    Name of the target stream in the catalog.

`top_n` : `int`
    Optional cap on how many grouped rows to keep after sorting. Use `0` to keep every group.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "text"_k = pluss::kindle::Stream(pluss::make_note({
        "heavy rain on road",
        "clear weather downtown",
        "storm warning issued",
    })),
});

auto out = ct.extract_keywords("text", 5);
```

#### Expected output

```text
keyword -> Note(["clear", "downtown", "heavy", "issued", "on"])
count   -> Series([1, 1, 1, 1, 1])
```

#### Practical Behavior

- `extract_keywords(field, top_n)` tokenizes the text field, counts tokens, sorts by frequency descending then alphabetically, and returns a small keyword/count catalog.
- Use it for quick corpus-level keyword previews rather than row-wise text features.


### `ffill`

#### Signature

```cpp
Catalog::ffill(keys: std::initializer_list<std::string_view> = {}) -> Catalog
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.ffill();
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `fillna`

#### Signature

```cpp
Catalog::fillna(value: const char*, keys: std::initializer_list<std::string_view> = {}) -> Catalog
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`value` : `const char*`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::fillna(value: double, keys: std::initializer_list<std::string_view> = {}) -> Catalog`
- `Catalog::fillna(value: std::string_view, keys: std::initializer_list<std::string_view> = {}) -> Catalog`
- `Catalog::fillna(value: const pluss::Timeline&, keys: std::initializer_list<std::string_view> = {}) -> Catalog`
- `Catalog::fillna(value: const pluss::TimeIndex&, keys: std::initializer_list<std::string_view> = {}) -> Catalog`

#### Examples

```cpp
using namespace pluss::kindle::literals;
auto ct = pluss::kindle::Catalog({"city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"}))});

auto out = ct.fillna("Unknown", {"city"});
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `fillna`

#### Signature

```cpp
Catalog::fillna(value: const pluss::TimeIndex&, keys: std::initializer_list<std::string_view> = {}) -> Catalog
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`value` : `const pluss::TimeIndex&`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::fillna(value: double, keys: std::initializer_list<std::string_view> = {}) -> Catalog`
- `Catalog::fillna(value: std::string_view, keys: std::initializer_list<std::string_view> = {}) -> Catalog`
- `Catalog::fillna(value: const char*, keys: std::initializer_list<std::string_view> = {}) -> Catalog`
- `Catalog::fillna(value: const pluss::Timeline&, keys: std::initializer_list<std::string_view> = {}) -> Catalog`

#### Examples

```cpp
using namespace pluss::kindle::literals;
auto ct = pluss::kindle::Catalog({"city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"}))});

auto out = ct.fillna("Unknown", {"city"});
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `fillna`

#### Signature

```cpp
Catalog::fillna(value: const pluss::Timeline&, keys: std::initializer_list<std::string_view> = {}) -> Catalog
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`value` : `const pluss::Timeline&`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::fillna(value: double, keys: std::initializer_list<std::string_view> = {}) -> Catalog`
- `Catalog::fillna(value: std::string_view, keys: std::initializer_list<std::string_view> = {}) -> Catalog`
- `Catalog::fillna(value: const char*, keys: std::initializer_list<std::string_view> = {}) -> Catalog`
- `Catalog::fillna(value: const pluss::TimeIndex&, keys: std::initializer_list<std::string_view> = {}) -> Catalog`

#### Examples

```cpp
using namespace pluss::kindle::literals;
auto ct = pluss::kindle::Catalog({"city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"}))});

auto out = ct.fillna("Unknown", {"city"});
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `fillna`

#### Signature

```cpp
Catalog::fillna(value: double, keys: std::initializer_list<std::string_view> = {}) -> Catalog
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`value` : `scalar`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::fillna(value: std::string_view, keys: std::initializer_list<std::string_view> = {}) -> Catalog`
- `Catalog::fillna(value: const char*, keys: std::initializer_list<std::string_view> = {}) -> Catalog`
- `Catalog::fillna(value: const pluss::Timeline&, keys: std::initializer_list<std::string_view> = {}) -> Catalog`
- `Catalog::fillna(value: const pluss::TimeIndex&, keys: std::initializer_list<std::string_view> = {}) -> Catalog`

#### Examples

```cpp
using namespace pluss::kindle::literals;
auto ct = pluss::kindle::Catalog({"city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"}))});

auto out = ct.fillna("Unknown", {"city"});
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `fillna`

#### Signature

```cpp
Catalog::fillna(value: std::string_view, keys: std::initializer_list<std::string_view> = {}) -> Catalog
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`value` : `std::string_view`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::fillna(value: double, keys: std::initializer_list<std::string_view> = {}) -> Catalog`
- `Catalog::fillna(value: const char*, keys: std::initializer_list<std::string_view> = {}) -> Catalog`
- `Catalog::fillna(value: const pluss::Timeline&, keys: std::initializer_list<std::string_view> = {}) -> Catalog`
- `Catalog::fillna(value: const pluss::TimeIndex&, keys: std::initializer_list<std::string_view> = {}) -> Catalog`

#### Examples

```cpp
using namespace pluss::kindle::literals;
auto ct = pluss::kindle::Catalog({"city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"}))});

auto out = ct.fillna("Unknown", {"city"});
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `filter`

#### Signature

```cpp
Catalog::filter(condition: const pluss::Series&) -> Catalog
```

#### Notes

Keeps only the rows where the supplied mask stream is true.

#### Inputs

`condition` : `const pluss::Series&`
    Boolean mask used by this overload to decide which rows, values, or items participate.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto mask = pluss::make_series({true, false, true}, pluss::ScalarType::Bool);
auto out = ct.filter(mask);
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `filter_by_text`

#### Signature

```cpp
Catalog::filter_by_text(field: std::string_view, pattern: std::string_view) -> Catalog
```

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

#### Inputs

`field` : `std::string_view`
    Name of the target stream in the catalog.

`pattern` : `std::string_view`
    Pattern string used for matching or extraction.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "text"_k = pluss::kindle::Stream(pluss::make_note({
        "heavy rain on road",
        "clear weather downtown",
        "storm warning issued",
    })),
});

auto out = ct.filter_by_text("text", "rain");
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `from_json`

#### Signature

```cpp
Catalog::from_json(json_text: std::string_view) -> Catalog
```

#### Notes

Builds a catalog from JSON input using the PlussMic JSON loader route.

#### Inputs

`json_text` : `std::string_view`
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto out = pluss::kindle::Catalog::from_json(R"([{"id":1,"city":"Accra"},{"id":2,"city":"Tema"}])");
```

#### Expected output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `get`

#### Signature

```cpp
Catalog::get(key: std::string_view) -> std::optional<Stream>
```

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### Inputs

`key` : `std::string_view`
    Name of the target slot or stream.

#### Output

`std::optional<Stream>`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.get("metric");
```

#### Expected output

```text
Series([10.0, 20.0, 30.0])
```

### `get_stream`

#### Signature

```cpp
Catalog::get_stream(key: std::string_view) -> std::optional<Stream>
```

#### Notes

Returns one named stream from the catalog.

#### Inputs

`key` : `std::string_view`
    Name of the target slot or stream.

#### Output

`std::optional<Stream>`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.get_stream("metric");
```

#### Expected output

```text
Series([10.0, 20.0, 30.0])
```

### `group_by`

#### Signature

```cpp
Catalog::group_by(key_field: std::string_view) -> std::unordered_map<std::string, Catalog>
```

#### Notes

Splits the catalog into sub-catalogs keyed by the distinct values in the grouping stream.

#### Inputs

`key_field` : `std::string_view`
    Field name that acts as the key for matching, grouping, or lookup.

#### Output

`std::unordered_map<std::string, Catalog>`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.group_by("city");
```

#### Expected output

```text
{"Accra": Catalog(...), "Tema": Catalog(...)}
```

### `has`

#### Signature

```cpp
Catalog::has(key: std::string_view) -> bool
```

#### Notes

Checks whether a named stream exists.

#### Inputs

`key` : `std::string_view`
    Name of the target slot or stream.

#### Output

`bool`
    Boolean status or membership result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.has("metric");
```

#### Expected output

```text
true
```

### `has_stream`

#### Signature

```cpp
Catalog::has_stream(key: std::string_view) -> bool
```

#### Notes

Checks whether a named stream exists.

#### Inputs

`key` : `std::string_view`
    Name of the target slot or stream.

#### Output

`bool`
    Boolean status or membership result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.has_stream("metric");
```

#### Expected output

```text
true
```

### `head`

#### Signature

```cpp
Catalog::head(n: std::int64_t) -> Catalog
```

#### Notes

Returns the first `n` rows across every stream in the catalog.

#### Inputs

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.head(2);
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `info`

#### Signature

```cpp
Catalog::info() -> void
```

#### Notes

This renders a human-readable view for inspection instead of creating a new analytic object.

#### Inputs

`None`

#### Output

`void`
    Performs the side effect and returns no value.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.info();
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `isnull`

#### Signature

```cpp
Catalog::isnull(keys: std::initializer_list<std::string_view> = {}) -> Catalog
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.isnull();
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `keys`

#### Signature

```cpp
Catalog::keys() -> const std::vector<std::string>&
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

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.keys();
```

#### Expected output

```text
["id", "metric", "city"]
```

### `kindelize`

#### Signature

```cpp
Catalog::kindelize(frame: const pluss::table::DataFrame&, policy: const KindelizePolicy& = {}) -> Catalog
```

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Inputs

`frame` : `const pluss::table::DataFrame&`
    Frame payload supplied to this overload.

`policy` : `const KindelizePolicy&`
    Policy object that changes the structural behavior of this overload.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = pluss::kindle::Catalog::kindelize(df);
```

#### Expected output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `load`

#### Signature

```cpp
Catalog::load(source_type: std::string_view, filepath: std::string_view) -> Catalog
```

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### Inputs

`source_type` : `std::string_view`
    Storage format selector such as `"csv"`, `"xlsx"`, `"json"`, or `"parquet"`.

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::load(source_type: std::string_view, filepath: std::string_view, config: const pluss::loader::Config&) -> Catalog`

#### Examples

```cpp
auto out = pluss::kindle::Catalog::load("csv", "path/to/data.csv");
```

#### Expected output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `load`

#### Signature

```cpp
Catalog::load(source_type: std::string_view, filepath: std::string_view, config: const pluss::loader::Config&) -> Catalog
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

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::load(source_type: std::string_view, filepath: std::string_view) -> Catalog`

#### Examples

```cpp
auto out = pluss::kindle::Catalog::load("csv", "path/to/data.csv");
```

#### Expected output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `load_csv`

#### Signature

```cpp
Catalog::load_csv(filepath: std::string_view) -> Catalog
```

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::load_csv(filepath: std::string_view, options: const pluss::loader::ReadCsvOptions&) -> Catalog`

#### Examples

```cpp
auto out = pluss::kindle::Catalog::load_csv("path/to/data.csv");
```

#### Expected output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `load_csv`

#### Signature

```cpp
Catalog::load_csv(filepath: std::string_view, options: const pluss::loader::ReadCsvOptions&) -> Catalog
```

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`options` : `const pluss::loader::ReadCsvOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::load_csv(filepath: std::string_view) -> Catalog`

#### Examples

```cpp
pluss::loader::ReadCsvOptions options;
auto out = pluss::kindle::Catalog::load_csv("path/to/data.csv", options);
```

#### Expected output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `load_csv_with_key`

#### Signature

```cpp
Catalog::load_csv_with_key(filepath: std::string_view, key_field: std::string_view, options: const pluss::loader::ReadCsvOptions& = {}) -> Catalog
```

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`key_field` : `std::string_view`
    Field name that acts as the key for matching, grouping, or lookup.

`options` : `const pluss::loader::ReadCsvOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
auto out = pluss::kindle::Catalog::load_csv_with_key("path/to/data.csv", "id");
```

#### Expected output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `load_excel`

#### Signature

```cpp
Catalog::load_excel(filepath: std::string_view) -> Catalog
```

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::load_excel(filepath: std::string_view, options: const pluss::loader::ReadExcelOptions&) -> Catalog`

#### Examples

```cpp
auto out = pluss::kindle::Catalog::load_excel("path/to/data.xlsx");
```

#### Expected output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `load_excel`

#### Signature

```cpp
Catalog::load_excel(filepath: std::string_view, options: const pluss::loader::ReadExcelOptions&) -> Catalog
```

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`options` : `const pluss::loader::ReadExcelOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::load_excel(filepath: std::string_view) -> Catalog`

#### Examples

```cpp
pluss::loader::ReadExcelOptions options;
auto out = pluss::kindle::Catalog::load_excel("path/to/data.xlsx", options);
```

#### Expected output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `load_json`

#### Signature

```cpp
Catalog::load_json(filepath: std::string_view) -> Catalog
```

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::load_json(filepath: std::string_view, config: const pluss::loader::Config&) -> Catalog`

#### Examples

```cpp
auto out = pluss::kindle::Catalog::load_json("path/to/data.json");
```

#### Expected output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `load_json`

#### Signature

```cpp
Catalog::load_json(filepath: std::string_view, config: const pluss::loader::Config&) -> Catalog
```

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::load_json(filepath: std::string_view) -> Catalog`

#### Examples

```cpp
auto out = pluss::kindle::Catalog::load_json("path/to/data.json", {"nrows", 16});
```

#### Expected output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `load_parquet`

#### Signature

```cpp
Catalog::load_parquet(filepath: std::string_view) -> Catalog
```

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::load_parquet(filepath: std::string_view, config: const pluss::loader::Config&) -> Catalog`

#### Examples

```cpp
auto out = pluss::kindle::Catalog::load_parquet("path/to/data.parquet");
```

#### Expected output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `load_parquet`

#### Signature

```cpp
Catalog::load_parquet(filepath: std::string_view, config: const pluss::loader::Config&) -> Catalog
```

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::load_parquet(filepath: std::string_view) -> Catalog`

#### Examples

```cpp
auto out = pluss::kindle::Catalog::load_parquet("path/to/data.parquet", {"nrows", 16});
```

#### Expected output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `load_txt`

#### Signature

```cpp
Catalog::load_txt(filepath: std::string_view) -> Catalog
```

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::load_txt(filepath: std::string_view, config: const pluss::loader::Config&) -> Catalog`

#### Examples

```cpp
auto out = pluss::kindle::Catalog::load_txt("path/to/data.txt");
```

#### Expected output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `load_txt`

#### Signature

```cpp
Catalog::load_txt(filepath: std::string_view, config: const pluss::loader::Config&) -> Catalog
```

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### Inputs

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::load_txt(filepath: std::string_view) -> Catalog`

#### Examples

```cpp
auto out = pluss::kindle::Catalog::load_txt("path/to/data.txt", {"nrows", 16});
```

#### Expected output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `masked_select`

#### Signature

```cpp
Catalog::masked_select(mask: const pluss::Series&) -> Catalog
```

#### Notes

Keeps only the rows where the supplied mask stream is true.

#### Inputs

`mask` : `const pluss::Series&`
    Boolean mask used by this overload to decide which rows, values, or items participate.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto mask = pluss::make_series({true, false, true}, pluss::ScalarType::Bool);
auto out = ct.masked_select(mask);
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `max`

#### Signature

```cpp
Catalog::max(field: std::string_view) -> Stream
```

#### Notes

Computes the maximum value of one named numeric stream.

#### Inputs

`field` : `std::string_view`
    Name of the target stream in the catalog.

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.max("metric");
```

#### Expected output

```text
Series([20.0])
```

### `mean`

#### Signature

```cpp
Catalog::mean(field: std::string_view) -> Stream
```

#### Notes

Computes the mean value of one named numeric stream.

#### Inputs

`field` : `std::string_view`
    Name of the target stream in the catalog.

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.mean("metric");
```

#### Expected output

```text
Series([20.0])
```

### `merge`

#### Signature

```cpp
Catalog::merge(other: const Catalog&, on: std::string_view, how: std::string_view = "inner") -> Catalog
```

#### Notes

Combines two catalogs by matching rows on a key stream.

#### Inputs

`other` : `const Catalog&`
    Second frame, thread, or scalar operand supplied for binary combination, comparison, alignment, or join-style work.

`on` : `std::string_view`
    Field name used as the match or join key for this overload.

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

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

using namespace pluss::kindle::literals;
auto right = pluss::kindle::Catalog({"id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})), "score"_k = pluss::kindle::Stream(pluss::make_series({0.9, 0.7, 0.8}))});
auto out = ct.merge(right, "id", "inner");
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `min`

#### Signature

```cpp
Catalog::min(field: std::string_view) -> Stream
```

#### Notes

Computes the minimum value of one named numeric stream.

#### Inputs

`field` : `std::string_view`
    Name of the target stream in the catalog.

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.min("metric");
```

#### Expected output

```text
Series([20.0])
```

### `nfields`

#### Signature

```cpp
Catalog::nfields() -> std::int64_t
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

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.nfields();
```

#### Expected output

```text
3
```

### `note`

#### Signature

```cpp
Catalog::note() -> Catalog
```

#### Notes

Returns one named stream interpreted as a note/text stream.

#### Inputs

`None`

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.note();
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `notnull`

#### Signature

```cpp
Catalog::notnull(keys: std::initializer_list<std::string_view> = {}) -> Catalog
```

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Inputs

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.notnull();
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `nstreams`

#### Signature

```cpp
Catalog::nstreams() -> std::int64_t
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

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.nstreams();
```

#### Expected output

```text
3
```

### `query`

#### Signature

```cpp
Catalog::query(fn: const QueryFn&) -> Catalog
```

#### Notes

Keeps only the rows selected by the supplied row-wise query function or expression.

#### Inputs

`fn` : `const QueryFn&`
    Callable used to transform, map, or derive values in this overload.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.query([](const pluss::kindle::Catalog& c) { return c.stream("metric").as_series().gt(15.0); });
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `rename`

#### Signature

```cpp
Catalog::rename(from: std::string_view, to: std::string_view) -> Catalog
```

#### Notes

Renames one or more streams without changing their values.

#### Inputs

`from` : `std::string_view`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

`to` : `std::string_view`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::rename(mapping: std::initializer_list<std::pair<std::string_view, std::string_view>>) -> Catalog`
- `Catalog::rename(mapping: const std::unordered_map<std::string, std::string>&) -> Catalog`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.rename("metric", "distance_km");
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `rename`

#### Signature

```cpp
Catalog::rename(mapping: const std::unordered_map<std::string, std::string>&) -> Catalog
```

#### Notes

Renames one or more streams without changing their values.

#### Inputs

`mapping` : `const std::unordered_map<std::string, std::string>&`
    Name-to-name mapping used by this overload.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::rename(mapping: std::initializer_list<std::pair<std::string_view, std::string_view>>) -> Catalog`
- `Catalog::rename(from: std::string_view, to: std::string_view) -> Catalog`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

std::unordered_map<std::string, std::string> renames{{"metric", "distance_km"}};
auto out = ct.rename(renames);
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `rename`

#### Signature

```cpp
Catalog::rename(mapping: std::initializer_list<std::pair<std::string_view, std::string_view>>) -> Catalog
```

#### Notes

Renames one or more streams without changing their values.

#### Inputs

`mapping` : `std::initializer_list<std::pair<std::string_view, std::string_view>>`
    Inline collection passed directly at the call site.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::rename(from: std::string_view, to: std::string_view) -> Catalog`
- `Catalog::rename(mapping: const std::unordered_map<std::string, std::string>&) -> Catalog`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.rename({{"metric", "distance_km"}});
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `rename_`

#### Signature

```cpp
Catalog::rename_(from: std::string_view, to: std::string_view) -> Catalog&
```

#### Notes

Renames one or more streams in place without changing their values.

#### Inputs

`from` : `std::string_view`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

`to` : `std::string_view`
    Boundary or replacement value used by this overload. Its type determines which payload family participates.

#### Output

`Catalog&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.rename_("metric", "distance_km");
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `sample`

#### Signature

```cpp
Catalog::sample(n: std::int64_t, random_state: std::optional<std::uint64_t> = std::nullopt) -> Catalog
```

#### Notes

Returns a random sample of rows across all streams.

#### Inputs

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

`random_state` : `std::optional<std::uint64_t>`
    Optional seed used to make sampling or shuffling reproducible.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.sample(2, 7);
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `save_as`

#### Signature

```cpp
Catalog::save_as(target_type: std::string_view, filename: std::string_view, config: const pluss::loader::Config& = {}) -> void
```

#### Notes

Writes the catalog using the explicitly supplied output format selector.

#### Inputs

`target_type` : `std::string_view`
    Format or target selector string used by this overload.

`filename` : `std::string_view`
    Filename or path-like string used by this overload.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Output

`void`
    Performs the side effect and returns no value.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.save_as("json", "path/to/output.json");
```

#### Expected output

```text
saved: out.*
```

### `save_csv`

#### Signature

```cpp
Catalog::save_csv(filename: std::string_view) -> void
```

#### Notes

Writes the catalog to CSV after aligning its streams into a table-like representation.

#### Inputs

`filename` : `std::string_view`
    Filename or path-like string used by this overload.

#### Output

`void`
    Performs the side effect and returns no value.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::save_csv(filename: std::string_view, config: const pluss::loader::Config&) -> void`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.save_csv("path/to/output.csv");
```

#### Expected output

```text
saved: out.*
```

### `save_csv`

#### Signature

```cpp
Catalog::save_csv(filename: std::string_view, config: const pluss::loader::Config&) -> void
```

#### Notes

Writes the catalog to CSV after aligning its streams into a table-like representation.

#### Inputs

`filename` : `std::string_view`
    Filename or path-like string used by this overload.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Output

`void`
    Performs the side effect and returns no value.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::save_csv(filename: std::string_view) -> void`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.save_csv("path/to/output.csv");
```

#### Expected output

```text
saved: out.*
```

### `save_excel`

#### Signature

```cpp
Catalog::save_excel(filename: std::string_view) -> void
```

#### Notes

Writes the catalog to an Excel workbook after aligning its streams into a table-like representation.

#### Inputs

`filename` : `std::string_view`
    Filename or path-like string used by this overload.

#### Output

`void`
    Performs the side effect and returns no value.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::save_excel(filename: std::string_view, config: const pluss::loader::Config&) -> void`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.save_excel("path/to/output.xlsx");
```

#### Expected output

```text
saved: out.*
```

### `save_excel`

#### Signature

```cpp
Catalog::save_excel(filename: std::string_view, config: const pluss::loader::Config&) -> void
```

#### Notes

Writes the catalog to an Excel workbook after aligning its streams into a table-like representation.

#### Inputs

`filename` : `std::string_view`
    Filename or path-like string used by this overload.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Output

`void`
    Performs the side effect and returns no value.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::save_excel(filename: std::string_view) -> void`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.save_excel("path/to/output.xlsx");
```

#### Expected output

```text
saved: out.*
```

### `save_json`

#### Signature

```cpp
Catalog::save_json(filename: std::string_view) -> void
```

#### Notes

Writes the catalog to JSON for interchange or later reload.

#### Inputs

`filename` : `std::string_view`
    Filename or path-like string used by this overload.

#### Output

`void`
    Performs the side effect and returns no value.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::save_json(filename: std::string_view, config: const pluss::loader::Config&) -> void`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.save_json("path/to/output.json");
```

#### Expected output

```text
saved: out.*
```

### `save_json`

#### Signature

```cpp
Catalog::save_json(filename: std::string_view, config: const pluss::loader::Config&) -> void
```

#### Notes

Writes the catalog to JSON for interchange or later reload.

#### Inputs

`filename` : `std::string_view`
    Filename or path-like string used by this overload.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Output

`void`
    Performs the side effect and returns no value.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::save_json(filename: std::string_view) -> void`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.save_json("path/to/output.json");
```

#### Expected output

```text
saved: out.*
```

### `save_parquet`

#### Signature

```cpp
Catalog::save_parquet(filename: std::string_view) -> void
```

#### Notes

Writes the catalog to Parquet for compact typed storage.

#### Inputs

`filename` : `std::string_view`
    Filename or path-like string used by this overload.

#### Output

`void`
    Performs the side effect and returns no value.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::save_parquet(filename: std::string_view, config: const pluss::loader::Config&) -> void`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.save_parquet("path/to/output.parquet");
```

#### Expected output

```text
saved: out.*
```

### `save_parquet`

#### Signature

```cpp
Catalog::save_parquet(filename: std::string_view, config: const pluss::loader::Config&) -> void
```

#### Notes

Writes the catalog to Parquet for compact typed storage.

#### Inputs

`filename` : `std::string_view`
    Filename or path-like string used by this overload.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Output

`void`
    Performs the side effect and returns no value.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::save_parquet(filename: std::string_view) -> void`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.save_parquet("path/to/output.parquet");
```

#### Expected output

```text
saved: out.*
```

### `save_txt`

#### Signature

```cpp
Catalog::save_txt(filename: std::string_view) -> void
```

#### Notes

Writes a text rendering of the catalog for inspection or logging.

#### Inputs

`filename` : `std::string_view`
    Filename or path-like string used by this overload.

#### Output

`void`
    Performs the side effect and returns no value.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::save_txt(filename: std::string_view, config: const pluss::loader::Config&) -> void`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.save_txt("path/to/output.txt");
```

#### Expected output

```text
saved: out.*
```

### `save_txt`

#### Signature

```cpp
Catalog::save_txt(filename: std::string_view, config: const pluss::loader::Config&) -> void
```

#### Notes

Writes a text rendering of the catalog for inspection or logging.

#### Inputs

`filename` : `std::string_view`
    Filename or path-like string used by this overload.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Output

`void`
    Performs the side effect and returns no value.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::save_txt(filename: std::string_view) -> void`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.save_txt("path/to/output.txt");
```

#### Expected output

```text
saved: out.*
```

### `select`

#### Signature

```cpp
Catalog::select(field_names: const std::vector<std::string>&) -> Catalog
```

#### Notes

Keeps only the named streams and preserves their shared row alignment.

#### Inputs

`field_names` : `const std::vector<std::string>&`
    Field names supplied together for this overload.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::select(field_names: std::initializer_list<std::string_view>) -> Catalog`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

std::vector<std::string> names{"id", "metric"};
auto out = ct.select(names);
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `select`

#### Signature

```cpp
Catalog::select(field_names: std::initializer_list<std::string_view>) -> Catalog
```

#### Notes

Keeps only the named streams and preserves their shared row alignment.

#### Inputs

`field_names` : `std::initializer_list<std::string_view>`
    Inline collection passed directly at the call site.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::select(field_names: const std::vector<std::string>&) -> Catalog`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.select({"id", "metric"});
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `series`

#### Signature

```cpp
Catalog::series() -> Catalog
```

#### Notes

Returns one named stream interpreted as a numeric series stream.

#### Inputs

`None`

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.series();
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `set`

#### Signature

```cpp
Catalog::set(key: std::string_view, value: const Stream&) -> Catalog
```

#### Notes

Adds or replaces one named stream and returns the updated catalog.

#### Inputs

`key` : `std::string_view`
    Name of the target slot or stream.

`value` : `const Stream&`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.set_stream("flag", pluss::kindle::Stream(pluss::make_series({1, 0, 1})));
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `set_`

#### Signature

```cpp
Catalog::set_(key: std::string_view, value: const Stream&) -> Catalog&
```

#### Notes

Adds or replaces one named stream in place.

#### Inputs

`key` : `std::string_view`
    Name of the target slot or stream.

`value` : `const Stream&`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`Catalog&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.set_stream_("flag", pluss::kindle::Stream(pluss::make_series({1, 0, 1})));
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `set_stream`

#### Signature

```cpp
Catalog::set_stream(key: std::string_view, value: const Stream&) -> Catalog
```

#### Notes

Adds or replaces one named stream and returns the updated catalog.

#### Inputs

`key` : `std::string_view`
    Name of the target slot or stream.

`value` : `const Stream&`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.set_stream("flag", pluss::kindle::Stream(pluss::make_series({1, 0, 1})));
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `set_stream_`

#### Signature

```cpp
Catalog::set_stream_(key: std::string_view, value: const Stream&) -> Catalog&
```

#### Notes

Adds or replaces one named stream in place.

#### Inputs

`key` : `std::string_view`
    Name of the target slot or stream.

`value` : `const Stream&`
    Replacement, fill, or write value used by mutation-style methods such as `put(...)`, `fillna(...)`, or `replace(...)`.

#### Output

`Catalog&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.set_stream_("flag", pluss::kindle::Stream(pluss::make_series({1, 0, 1})));
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `set_streams`

#### Signature

```cpp
Catalog::set_streams(items: std::initializer_list<NamedStream>) -> Catalog
```

#### Notes

Adds or replaces several named streams and returns the updated catalog.

#### Inputs

`items` : `std::initializer_list<NamedStream>`
    Inline collection passed directly at the call site.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::set_streams(items: std::initializer_list<std::pair<std::string_view, Stream>>) -> Catalog`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

using namespace pluss::kindle::literals;
auto out = ct.set_streams({
    "flag"_k = pluss::kindle::Stream(pluss::make_series({1, 0, 1})),
});
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `set_streams`

#### Signature

```cpp
Catalog::set_streams(items: std::initializer_list<std::pair<std::string_view, Stream>>) -> Catalog
```

#### Notes

Adds or replaces several named streams and returns the updated catalog.

#### Inputs

`items` : `std::initializer_list<std::pair<std::string_view, Stream>>`
    Inline collection passed directly at the call site.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::set_streams(items: std::initializer_list<NamedStream>) -> Catalog`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.set_streams({
    {"flag", pluss::kindle::Stream(pluss::make_series({1, 0, 1}))},
});
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `set_streams_`

#### Signature

```cpp
Catalog::set_streams_(items: std::initializer_list<NamedStream>) -> Catalog&
```

#### Notes

Adds or replaces several named streams in place.

#### Inputs

`items` : `std::initializer_list<NamedStream>`
    Inline collection passed directly at the call site.

#### Output

`Catalog&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::set_streams_(items: std::initializer_list<std::pair<std::string_view, Stream>>) -> Catalog&`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

using namespace pluss::kindle::literals;
ct.set_streams_({
    "flag"_k = pluss::kindle::Stream(pluss::make_series({1, 0, 1})),
});
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `set_streams_`

#### Signature

```cpp
Catalog::set_streams_(items: std::initializer_list<std::pair<std::string_view, Stream>>) -> Catalog&
```

#### Notes

Adds or replaces several named streams in place.

#### Inputs

`items` : `std::initializer_list<std::pair<std::string_view, Stream>>`
    Inline collection passed directly at the call site.

#### Output

`Catalog&`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::set_streams_(items: std::initializer_list<NamedStream>) -> Catalog&`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

ct.set_streams_({
    {"flag", pluss::kindle::Stream(pluss::make_series({1, 0, 1}))},
});
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `show`

#### Signature

```cpp
Catalog::show(os: std::ostream&) -> void
```

#### Notes

This renders a human-readable view for inspection instead of creating a new analytic object.

#### Inputs

`os` : `std::ostream&`
    Destination output stream used instead of the default display or print target.

#### Output

`void`
    Performs the side effect and returns no value.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.show();
```

#### Expected output

```text
The call completes and returns the contract described under Output.
```

### `slice`

#### Signature

```cpp
Catalog::slice(start: std::int64_t, stop: std::int64_t) -> Catalog
```

#### Notes

Returns a contiguous row slice `[start, stop)` across every stream.

#### Inputs

`start` : `std::int64_t`
    Inclusive starting position of the slice.

`stop` : `std::int64_t`
    Exclusive ending position of the slice.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.slice(0, 2);
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `sort`

#### Signature

```cpp
Catalog::sort(by_field: std::string_view, ascending: bool = true) -> Catalog
```

#### Notes

Reorders rows using one stream as the sort key, while keeping the other streams aligned to the same row order.

#### Inputs

`by_field` : `std::string_view`
    Field name that controls the grouping or sorting basis of the operation.

`ascending` : `bool`
    Boolean switch that changes how this overload behaves. Read the method notes and examples to see the effect of `true` versus `false`.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.sort("metric", false);
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `sort_by`

#### Signature

```cpp
Catalog::sort_by(fields: const std::vector<std::string>&, ascending: const std::vector<bool>& = {}) -> Catalog
```

#### Notes

Reorders rows using multiple sort keys, while keeping all streams aligned.

#### Inputs

`fields` : `const std::vector<std::string>&`
    Parameter used by this overload. The examples below show the supported call shapes and how the argument changes behavior.

`ascending` : `const std::vector<bool>&`
    Sort-order switch. `true` means ascending order and `false` means descending order.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.sort_by({"city", "id"}, {true, false});
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `stream`

#### Signature

```cpp
Catalog::stream(key: std::string_view) -> Stream
```

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

#### Inputs

`key` : `std::string_view`
    Name of the target slot or stream.

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.stream("metric");
```

#### Expected output

```text
Series([10.0, 20.0, 30.0])
```

### `streams`

#### Signature

```cpp
Catalog::streams() -> std::vector<Stream>
```

#### Notes

Materializes the current members as a collection you can inspect or reuse elsewhere.

#### Inputs

`None`

#### Output

`std::vector<Stream>`
    Materialized collection of stream objects.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.streams();
```

#### Expected output

```text
[Stream(id), Stream(metric), Stream(city)]
```

### `sum`

#### Signature

```cpp
Catalog::sum(field: std::string_view) -> Stream
```

#### Notes

Computes the sum of one named numeric stream.

#### Inputs

`field` : `std::string_view`
    Name of the target stream in the catalog.

#### Output

`Stream`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.sum("metric");
```

#### Expected output

```text
Series([20.0])
```

### `tabelize`

#### Signature

```cpp
Catalog::tabelize(policy: const TabelizePolicy& = {}) -> pluss::table::DataFrame
```

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Inputs

`policy` : `const TabelizePolicy&`
    Policy object that changes the structural behavior of this overload.

#### Output

`pluss::table::DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

pluss::kindle::TabelizePolicy policy;
policy.align = pluss::kindle::AlignPolicy::Inner;
auto out = ct.tabelize(policy);
```

#### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `tail`

#### Signature

```cpp
Catalog::tail(n: std::int64_t) -> Catalog
```

#### Notes

Returns the last `n` rows across every stream in the catalog.

#### Inputs

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.tail(2);
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `take`

#### Signature

```cpp
Catalog::take(indices: std::span<const std::int64_t>) -> Catalog
```

#### Notes

Returns the rows at the requested integer positions across every stream.

#### Inputs

`indices` : `std::span<const std::int64_t>`
    Integer positions used to select, reorder, or address entries.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

std::vector<std::int64_t> picks{0, 2};
auto out = ct.take(picks);
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `text_preprocess`

#### Signature

```cpp
Catalog::text_preprocess(field: std::string_view) -> Catalog
```

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

#### Inputs

`field` : `std::string_view`
    Name of the target stream in the catalog.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "text"_k = pluss::kindle::Stream(pluss::make_note({
        "heavy rain on road",
        "clear weather downtown",
        "storm warning issued",
    })),
});

auto out = ct.text_preprocess("text");
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `tfidf`

#### Signature

```cpp
Catalog::tfidf(field: std::string_view) -> Catalog
```

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

#### Inputs

`field` : `std::string_view`
    Name of the target stream in the catalog.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "text"_k = pluss::kindle::Stream(pluss::make_note({
        "heavy rain on road",
        "clear weather downtown",
        "storm warning issued",
    })),
});

auto out = ct.tfidf("text");
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `timeindex`

#### Signature

```cpp
Catalog::timeindex() -> Catalog
```

#### Notes

Returns one named stream interpreted as a time-index stream.

#### Inputs

`None`

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.timeindex();
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `timeline`

#### Signature

```cpp
Catalog::timeline() -> Catalog
```

#### Notes

Returns one named stream interpreted as a timeline stream.

#### Inputs

`None`

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.timeline();
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `to_json`

#### Signature

```cpp
Catalog::to_json() -> std::string
```

#### Notes

Serializes or renders the current object into a text form you can inspect, log, or export.

#### Inputs

`None`

#### Output

`std::string`
    Rendered string result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.to_json();
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `to_string`

#### Signature

```cpp
Catalog::to_string() -> std::string
```

#### Notes

Serializes or renders the current object into a text form you can inspect, log, or export.

#### Inputs

`None`

#### Output

`std::string`
    Rendered string result.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.to_string();
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `to_table`

#### Signature

```cpp
Catalog::to_table(policy: const TabelizePolicy& = {}, render: bool = true) -> pluss::table::DataFrame
```

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Inputs

`policy` : `const TabelizePolicy&`
    Policy object that changes the structural behavior of this overload.

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

#### Output

`pluss::table::DataFrame`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

pluss::kindle::TabelizePolicy policy;
policy.align = pluss::kindle::AlignPolicy::Inner;
auto out = ct.to_table(policy, true);
```

#### Expected output

```text
id  sales  city
1   100.0  Accra
2   120.0  Tema
3    90.0  Kumasi
```

### `transform`

#### Signature

```cpp
Catalog::transform(field: std::string_view, fn: const std::function<Stream(const Stream&)>&) -> Catalog
```

#### Notes

Rewrites one stream, several streams, or all streams by applying a user function.

#### Inputs

`field` : `std::string_view`
    Name of the target stream in the catalog.

`fn` : `const std::function<Stream(const Stream&)>&`
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::transform(fn: const std::function<Stream(const Stream&)>&) -> Catalog`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.transform("metric", [](const pluss::kindle::Stream& s) { return s.clip(0.0, 25.0); });
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `transform`

#### Signature

```cpp
Catalog::transform(fn: const std::function<Stream(const Stream&)>&) -> Catalog
```

#### Notes

Rewrites one stream, several streams, or all streams by applying a user function.

#### Inputs

`fn` : `const std::function<Stream(const Stream&)>&`
    Callback function used by this overload. The callable signature shown in the type tells you what input is provided and what you must return.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### See also

- `Catalog::transform(field: std::string_view, fn: const std::function<Stream(const Stream&)>&) -> Catalog`

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.transform([](const pluss::kindle::Stream& s) { return s.kind() == pluss::kindle::StreamKind::Series ? s.clip(0.0, 25.0) : s; });
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `unique`

#### Signature

```cpp
Catalog::unique() -> bool
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

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.unique();
```

#### Expected output

```text
true
```

### `where`

#### Signature

```cpp
Catalog::where(condition: const pluss::Series&) -> Catalog
```

#### Notes

Keeps only the rows where the supplied mask stream is true.

#### Inputs

`condition` : `const pluss::Series&`
    Boolean mask used by this overload to decide which rows, values, or items participate.

#### Output

`Catalog`
    Value produced by this call.

#### Options

Option and enum values, when present, are listed with their effects under **Inputs**.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto mask = pluss::make_series({true, false, true}, pluss::ScalarType::Bool);
auto out = ct.where(mask);
```

#### Expected output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])

out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```
