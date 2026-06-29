# Catalog API Reference

# Practical Catalog API Reference

## API Reference Style

This page documents the full public `Catalog` surface for named stream collections.

### `add_text_features`

#### Signature

```cpp
Catalog add_text_features(self, std::string_view field) const
```

#### Parameters

`field` : `std::string_view`
    Name of the target stream in the catalog.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

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

#### Output

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
Catalog aggregate_text_stats(self, std::string_view field, std::string_view stat_type) const
```

#### Parameters

`field` : `std::string_view`
    Name of the target stream in the catalog.

`stat_type` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

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

#### Output

```text
text_mean_char_len -> Series([20.0])
```

#### Practical Behavior

- `aggregate_text_stats(field, stat_type)` collapses one text field into a one-value summary stream.
- Supported statistics include common length-based summaries such as mean/min/max/count, along with vocabulary size.


### `apply`

#### Signature

```cpp
Catalog apply(self, std::string_view field, const std::function<Stream(const Stream&)>& func) const
```

#### Parameters

`field` : `std::string_view`
    Name of the target stream in the catalog.

`func` : `const std::function<Stream(const Stream&)>&`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Applies a function to one named stream and returns the updated catalog.

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

#### Output

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
Catalog& apply_(self, std::string_view field, const std::function<Stream(const Stream&)>& func)
```

#### Parameters

`field` : `std::string_view`
    Name of the target stream in the catalog.

`func` : `const std::function<Stream(const Stream&)>&`
    See the signature for the supported argument form.

#### Returns

`Catalog&`
    Value produced by this call.

#### Notes

Applies a function to one named stream in place.

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

### `apply_all`

#### Signature

```cpp
Catalog apply_all(self, const std::function<Stream(const Stream&)>& fn) const
```

#### Parameters

`fn` : `const std::function<Stream(const Stream&)>&`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Applies the same function to every stream and returns the updated catalog.

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

#### Output

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
Catalog& apply_all_(self, const std::function<Stream(const Stream&)>& fn)
```

#### Parameters

`fn` : `const std::function<Stream(const Stream&)>&`
    See the signature for the supported argument form.

#### Returns

`Catalog&`
    Value produced by this call.

#### Notes

Applies the same function to every stream in place.

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

### `astype`

#### Signature

```cpp
Catalog astype(self, std::initializer_list<std::string_view> keys, pluss::codem::ScalarType dtype) const
```

#### Parameters

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

`dtype` : `pluss::codem::ScalarType`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This changes the semantic or numeric type so later operations route through the correct implementation path.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.astype({"metric"}, pluss::codem::ScalarType::Float64);
```

#### Output

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
Stream at(self, std::string_view key) const
```

#### Parameters

`key` : `std::string_view`
    Name of the target slot or stream.

#### Returns

`Stream`
    Value produced by this call.

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

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

#### Output

```text
Series([10.0, 20.0, 30.0])
```

#### Practical Behavior

- `at(key)` fetches one named stream from the catalog.
- Use it when you want stream access without routing through table conversion first.


### `begin`

#### Signature

```cpp
const_iterator begin(self) const
```

#### Parameters

`None`

#### Returns

`const_iterator`
    Value produced by this call.

#### Notes

Exposes iterator boundaries so the object can be traversed with a range-based loop or manual iteration.

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

### `bfill`

#### Signature

```cpp
Catalog bfill(self, std::initializer_list<std::string_view> keys = {}) const
```

#### Parameters

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Examples

```cpp
using namespace pluss::kindle::literals;
auto ct = pluss::kindle::Catalog({
    "Final Points Grade"_k = pluss::kindle::Stream(pluss::make_series({71.0, 0.0, 88.0, 0.0})).with_validity_mask(pluss::make_series({true, false, true, false}, pluss::codem::ScalarType::Bool)),
    "Midterm 3"_k = pluss::kindle::Stream(pluss::make_series({60.0, 0.0, 75.0, 91.0})).with_validity_mask(pluss::make_series({true, false, true, true}, pluss::codem::ScalarType::Bool)),
    "Name"_k = pluss::kindle::Stream(pluss::make_note({"A", "B", "C", "D"})),
});

auto out = ct.bfill({"Final Points Grade", "Midterm 3"});
```

#### Output

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
Catalog cast_note(self, std::initializer_list<std::string_view> keys) const
```

#### Parameters

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This changes the semantic or numeric type so later operations route through the correct implementation path.

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

#### Output

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
Catalog cast_series(self, std::initializer_list<std::string_view> keys, std::optional<pluss::codem::ScalarType> dtype = std::nullopt) const
```

#### Parameters

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

`dtype` : `std::optional<pluss::codem::ScalarType>`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This changes the semantic or numeric type so later operations route through the correct implementation path.

#### Examples

```cpp
using namespace pluss::kindle::literals;
auto ct = pluss::kindle::Catalog({"metric"_k = pluss::kindle::Stream(pluss::make_note({"10.0", "20.0", "30.0"}))});

auto out = ct.cast_series({"metric"}, pluss::codem::ScalarType::Float64);
```

#### Output

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
Catalog cast_timeindex(self, std::initializer_list<std::string_view> keys) const
```

#### Parameters

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This changes the semantic or numeric type so later operations route through the correct implementation path.

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

#### Output

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
Catalog cast_timeline(self, std::initializer_list<std::string_view> keys) const
```

#### Parameters

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This changes the semantic or numeric type so later operations route through the correct implementation path.

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

#### Output

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
Catalog cloc(self, std::int64_t row) const
```

#### Parameters

`row` : `std::int64_t`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Applies `cloc` to the current `Catalog` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `Catalog cloc(self, std::int64_t start, std::int64_t stop) const`

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

#### Practical Behavior

- `cloc(...)` slices aligned row positions across every stream in the catalog.
- Use it when you want row-wise positional access while keeping stream alignment intact.


### `cloc`

#### Signature

```cpp
Catalog cloc(self, std::int64_t start, std::int64_t stop) const
```

#### Parameters

`start` : `std::int64_t`
    Inclusive starting position of the slice.

`stop` : `std::int64_t`
    Exclusive ending position of the slice.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Applies `cloc` to the current `Catalog` object. Use the example and output together to see which part of the object changes and which structure is preserved.

#### See also

- `Catalog cloc(self, std::int64_t row) const`

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

#### Practical Behavior

- `cloc(...)` slices aligned row positions across every stream in the catalog.
- Use it when you want row-wise positional access while keeping stream alignment intact.


### `compute_similarity`

#### Signature

```cpp
Catalog compute_similarity(self, std::string_view field1, std::string_view field2, std::string_view method = "cosine") const
```

#### Parameters

`field1` : `std::string_view`
    See the signature for the supported argument form.

`field2` : `std::string_view`
    See the signature for the supported argument form.

`method` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

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

#### Output

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
std::int64_t count(self) const
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
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.count();
```

#### Output

```text
3
```

#### Practical Behavior

- `count()` returns the catalog row count after alignment through `tabelize()`.
- This is the quick “how many aligned rows do I have?” answer.


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
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.defined();
```

#### Output

```text
true
```

### `display`

#### Signature

```cpp
void display(self) const
```

#### Parameters

`None`

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

This renders a human-readable view for inspection instead of creating a new analytic object.

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

#### Output

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
Catalog drop(self, const std::vector<std::string>& field_names) const
```

#### Parameters

`field_names` : `const std::vector<std::string>&`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Removes the named streams and keeps the remaining catalog structure intact.

#### See also

- `Catalog drop(self, std::string_view field_name) const`
- `Catalog drop(self, std::initializer_list<std::string_view> keys) const`

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

#### Output

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
Catalog drop(self, std::initializer_list<std::string_view> keys) const
```

#### Parameters

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Removes the named streams and keeps the remaining catalog structure intact.

#### See also

- `Catalog drop(self, std::string_view field_name) const`
- `Catalog drop(self, const std::vector<std::string>& field_names) const`

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

#### Output

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
Catalog drop(self, std::string_view field_name) const
```

#### Parameters

`field_name` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Removes the named streams and keeps the remaining catalog structure intact.

#### See also

- `Catalog drop(self, const std::vector<std::string>& field_names) const`
- `Catalog drop(self, std::initializer_list<std::string_view> keys) const`

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

#### Output

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
Catalog dropna(self, std::initializer_list<std::string_view> keys = {}) const
```

#### Parameters

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### Examples

```cpp
using namespace pluss::kindle::literals;
auto metric = pluss::kindle::Stream(pluss::make_series({10.0, 0.0, 30.0}))
                 .with_validity_mask(pluss::make_series({true, false, true}, pluss::codem::ScalarType::Bool));
auto ct = pluss::kindle::Catalog({
    "metric"_k = metric,
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.dropna({"metric"});
```

#### Output

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
bool empty(self) const
```

#### Parameters

`None`

#### Returns

`bool`
    Boolean status or membership result.

#### Notes

Reports whether the current object has no rows, entries, slots, or depth items.

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

#### Output

```text
false
```

### `end`

#### Signature

```cpp
const_iterator end(self) const
```

#### Parameters

`None`

#### Returns

`const_iterator`
    Value produced by this call.

#### Notes

Exposes iterator boundaries so the object can be traversed with a range-based loop or manual iteration.

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

### `extract_keywords`

#### Signature

```cpp
Catalog extract_keywords(self, std::string_view field, int top_n = 10) const
```

#### Parameters

`field` : `std::string_view`
    Name of the target stream in the catalog.

`top_n` : `int`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

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

#### Output

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
Catalog ffill(self, std::initializer_list<std::string_view> keys = {}) const
```

#### Parameters

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

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

#### Output

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
Catalog fillna(self, const char* value, std::initializer_list<std::string_view> keys = {}) const
```

#### Parameters

`value` : `const char*`
    See the signature for the supported argument form.

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### See also

- `Catalog fillna(self, scalar value, std::initializer_list<std::string_view> keys = {}) const`
- `Catalog fillna(self, std::string_view value, std::initializer_list<std::string_view> keys = {}) const`
- `Catalog fillna(self, const pluss::Timeline& value, std::initializer_list<std::string_view> keys = {}) const`
- `Catalog fillna(self, const pluss::TimeIndex& value, std::initializer_list<std::string_view> keys = {}) const`

#### Examples

```cpp
using namespace pluss::kindle::literals;
auto ct = pluss::kindle::Catalog({"city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"}))});

auto out = ct.fillna("Unknown", {"city"});
```

#### Output

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
Catalog fillna(self, const pluss::TimeIndex& value, std::initializer_list<std::string_view> keys = {}) const
```

#### Parameters

`value` : `const pluss::TimeIndex&`
    See the signature for the supported argument form.

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### See also

- `Catalog fillna(self, scalar value, std::initializer_list<std::string_view> keys = {}) const`
- `Catalog fillna(self, std::string_view value, std::initializer_list<std::string_view> keys = {}) const`
- `Catalog fillna(self, const char* value, std::initializer_list<std::string_view> keys = {}) const`
- `Catalog fillna(self, const pluss::Timeline& value, std::initializer_list<std::string_view> keys = {}) const`

#### Examples

```cpp
using namespace pluss::kindle::literals;
auto ct = pluss::kindle::Catalog({"city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"}))});

auto out = ct.fillna("Unknown", {"city"});
```

#### Output

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
Catalog fillna(self, const pluss::Timeline& value, std::initializer_list<std::string_view> keys = {}) const
```

#### Parameters

`value` : `const pluss::Timeline&`
    See the signature for the supported argument form.

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### See also

- `Catalog fillna(self, scalar value, std::initializer_list<std::string_view> keys = {}) const`
- `Catalog fillna(self, std::string_view value, std::initializer_list<std::string_view> keys = {}) const`
- `Catalog fillna(self, const char* value, std::initializer_list<std::string_view> keys = {}) const`
- `Catalog fillna(self, const pluss::TimeIndex& value, std::initializer_list<std::string_view> keys = {}) const`

#### Examples

```cpp
using namespace pluss::kindle::literals;
auto ct = pluss::kindle::Catalog({"city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"}))});

auto out = ct.fillna("Unknown", {"city"});
```

#### Output

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
Catalog fillna(self, scalar value, std::initializer_list<std::string_view> keys = {}) const
```

#### Parameters

`value` : `scalar`
    See the signature for the supported argument form.

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### See also

- `Catalog fillna(self, std::string_view value, std::initializer_list<std::string_view> keys = {}) const`
- `Catalog fillna(self, const char* value, std::initializer_list<std::string_view> keys = {}) const`
- `Catalog fillna(self, const pluss::Timeline& value, std::initializer_list<std::string_view> keys = {}) const`
- `Catalog fillna(self, const pluss::TimeIndex& value, std::initializer_list<std::string_view> keys = {}) const`

#### Examples

```cpp
using namespace pluss::kindle::literals;
auto ct = pluss::kindle::Catalog({"city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"}))});

auto out = ct.fillna("Unknown", {"city"});
```

#### Output

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
Catalog fillna(self, std::string_view value, std::initializer_list<std::string_view> keys = {}) const
```

#### Parameters

`value` : `std::string_view`
    See the signature for the supported argument form.

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

#### See also

- `Catalog fillna(self, scalar value, std::initializer_list<std::string_view> keys = {}) const`
- `Catalog fillna(self, const char* value, std::initializer_list<std::string_view> keys = {}) const`
- `Catalog fillna(self, const pluss::Timeline& value, std::initializer_list<std::string_view> keys = {}) const`
- `Catalog fillna(self, const pluss::TimeIndex& value, std::initializer_list<std::string_view> keys = {}) const`

#### Examples

```cpp
using namespace pluss::kindle::literals;
auto ct = pluss::kindle::Catalog({"city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"}))});

auto out = ct.fillna("Unknown", {"city"});
```

#### Output

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
Catalog filter(self, const pluss::Series& condition) const
```

#### Parameters

`condition` : `const pluss::Series&`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Keeps only the rows where the supplied mask stream is true.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto mask = pluss::make_series({true, false, true}, pluss::codem::ScalarType::Bool);
auto out = ct.filter(mask);
```

#### Output

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
Catalog filter_by_text(self, std::string_view field, std::string_view pattern) const
```

#### Parameters

`field` : `std::string_view`
    Name of the target stream in the catalog.

`pattern` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

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

#### Output

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
static Catalog from_json(std::string_view json_text)
```

#### Parameters

`json_text` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Builds a catalog from JSON input using the PlussMic JSON loader route.

#### Examples

```cpp
auto out = pluss::kindle::Catalog::from_json(R"([{"id":1,"city":"Accra"},{"id":2,"city":"Tema"}])");
```

#### Output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `get`

#### Signature

```cpp
std::optional<Stream> get(self, std::string_view key) const
```

#### Parameters

`key` : `std::string_view`
    Name of the target slot or stream.

#### Returns

`std::optional<Stream>`
    Value produced by this call.

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

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

#### Output

```text
Series([10.0, 20.0, 30.0])
```

### `get_stream`

#### Signature

```cpp
std::optional<Stream> get_stream(self, std::string_view key) const
```

#### Parameters

`key` : `std::string_view`
    Name of the target slot or stream.

#### Returns

`std::optional<Stream>`
    Value produced by this call.

#### Notes

Returns one named stream from the catalog.

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

#### Output

```text
Series([10.0, 20.0, 30.0])
```

### `group_by`

#### Signature

```cpp
std::unordered_map<std::string, Catalog> group_by(self, std::string_view key_field) const
```

#### Parameters

`key_field` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`std::unordered_map<std::string, Catalog>`
    Value produced by this call.

#### Notes

Splits the catalog into sub-catalogs keyed by the distinct values in the grouping stream.

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

#### Output

```text
{"Accra": Catalog(...), "Tema": Catalog(...)}
```

### `has`

#### Signature

```cpp
bool has(self, std::string_view key) const
```

#### Parameters

`key` : `std::string_view`
    Name of the target slot or stream.

#### Returns

`bool`
    Boolean status or membership result.

#### Notes

Checks whether a named stream exists.

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

#### Output

```text
true
```

### `has_stream`

#### Signature

```cpp
bool has_stream(self, std::string_view key) const
```

#### Parameters

`key` : `std::string_view`
    Name of the target slot or stream.

#### Returns

`bool`
    Boolean status or membership result.

#### Notes

Checks whether a named stream exists.

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

#### Output

```text
true
```

### `head`

#### Signature

```cpp
Catalog head(self, std::int64_t n) const
```

#### Parameters

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Returns the first `n` rows across every stream in the catalog.

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

#### Output

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
void info(self) const
```

#### Parameters

`None`

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

This renders a human-readable view for inspection instead of creating a new analytic object.

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

#### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `isnull`

#### Signature

```cpp
Catalog isnull(self, std::initializer_list<std::string_view> keys = {}) const
```

#### Parameters

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

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

#### Output

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
const std::vector<std::string>& keys(self) const
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
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.keys();
```

#### Output

```text
["id", "metric", "city"]
```

### `kindelize`

#### Signature

```cpp
static Catalog kindelize(const pluss::table::DataFrame& frame, const KindelizePolicy& policy = {})
```

#### Parameters

`frame` : `const pluss::table::DataFrame&`
    See the signature for the supported argument form.

`policy` : `const KindelizePolicy&`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

#### Examples

```cpp
auto df = pluss::table::DataFrame({
    {"id", pluss::table::Thread(pluss::make_series({1, 2, 3}))},
    {"sales", pluss::table::Thread(pluss::make_series({100.0, 120.0, 90.0}))},
    {"city", pluss::table::Thread(pluss::make_note({"Accra", "Tema", "Kumasi"}))},
});

auto out = pluss::kindle::Catalog::kindelize(df);
```

#### Output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `load`

#### Signature

```cpp
static Catalog load(std::string_view source_type, std::string_view filepath)
```

#### Parameters

`source_type` : `std::string_view`
    Storage format selector such as `"csv"`, `"xlsx"`, `"json"`, or `"parquet"`.

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Catalog load(std::string_view source_type, std::string_view filepath, const pluss::loader::Config& config)`

#### Examples

```cpp
auto out = pluss::kindle::Catalog::load("csv", "path/to/data.csv");
```

#### Output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `load`

#### Signature

```cpp
static Catalog load(std::string_view source_type, std::string_view filepath, const pluss::loader::Config& config)
```

#### Parameters

`source_type` : `std::string_view`
    Storage format selector such as `"csv"`, `"xlsx"`, `"json"`, or `"parquet"`.

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Catalog load(std::string_view source_type, std::string_view filepath)`

#### Examples

```cpp
auto out = pluss::kindle::Catalog::load("csv", "path/to/data.csv");
```

#### Output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `load_csv`

#### Signature

```cpp
static Catalog load_csv(std::string_view filepath)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Catalog load_csv(std::string_view filepath, const pluss::loader::ReadCsvOptions& options)`

#### Examples

```cpp
auto out = pluss::kindle::Catalog::load_csv("path/to/data.csv");
```

#### Output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `load_csv`

#### Signature

```cpp
static Catalog load_csv(std::string_view filepath, const pluss::loader::ReadCsvOptions& options)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`options` : `const pluss::loader::ReadCsvOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Catalog load_csv(std::string_view filepath)`

#### Examples

```cpp
pluss::loader::ReadCsvOptions options;
auto out = pluss::kindle::Catalog::load_csv("path/to/data.csv", options);
```

#### Output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `load_csv_with_key`

#### Signature

```cpp
static Catalog load_csv_with_key(std::string_view filepath, std::string_view key_field, const pluss::loader::ReadCsvOptions& options = {})
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`key_field` : `std::string_view`
    See the signature for the supported argument form.

`options` : `const pluss::loader::ReadCsvOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### Examples

```cpp
auto out = pluss::kindle::Catalog::load_csv_with_key("path/to/data.csv", "id");
```

#### Output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `load_excel`

#### Signature

```cpp
static Catalog load_excel(std::string_view filepath)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Catalog load_excel(std::string_view filepath, const pluss::loader::ReadExcelOptions& options)`

#### Examples

```cpp
auto out = pluss::kindle::Catalog::load_excel("path/to/data.xlsx");
```

#### Output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `load_excel`

#### Signature

```cpp
static Catalog load_excel(std::string_view filepath, const pluss::loader::ReadExcelOptions& options)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`options` : `const pluss::loader::ReadExcelOptions&`
    Typed loader, renderer, SQL, plotting, or policy options for this overload.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Catalog load_excel(std::string_view filepath)`

#### Examples

```cpp
pluss::loader::ReadExcelOptions options;
auto out = pluss::kindle::Catalog::load_excel("path/to/data.xlsx", options);
```

#### Output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `load_json`

#### Signature

```cpp
static Catalog load_json(std::string_view filepath)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Catalog load_json(std::string_view filepath, const pluss::loader::Config& config)`

#### Examples

```cpp
auto out = pluss::kindle::Catalog::load_json("path/to/data.json");
```

#### Output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `load_json`

#### Signature

```cpp
static Catalog load_json(std::string_view filepath, const pluss::loader::Config& config)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Catalog load_json(std::string_view filepath)`

#### Examples

```cpp
auto out = pluss::kindle::Catalog::load_json("path/to/data.json", {"nrows", 16});
```

#### Output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `load_parquet`

#### Signature

```cpp
static Catalog load_parquet(std::string_view filepath)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Catalog load_parquet(std::string_view filepath, const pluss::loader::Config& config)`

#### Examples

```cpp
auto out = pluss::kindle::Catalog::load_parquet("path/to/data.parquet");
```

#### Output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `load_parquet`

#### Signature

```cpp
static Catalog load_parquet(std::string_view filepath, const pluss::loader::Config& config)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Catalog load_parquet(std::string_view filepath)`

#### Examples

```cpp
auto out = pluss::kindle::Catalog::load_parquet("path/to/data.parquet", {"nrows", 16});
```

#### Output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `load_txt`

#### Signature

```cpp
static Catalog load_txt(std::string_view filepath)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Catalog load_txt(std::string_view filepath, const pluss::loader::Config& config)`

#### Examples

```cpp
auto out = pluss::kindle::Catalog::load_txt("path/to/data.txt");
```

#### Output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `load_txt`

#### Signature

```cpp
static Catalog load_txt(std::string_view filepath, const pluss::loader::Config& config)
```

#### Parameters

`filepath` : `std::string_view`
    File location to read from or write to. Use a neutral project path such as `path/to/data.csv`.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Loads the object from disk using the corresponding PlussMic loader route.

#### See also

- `static Catalog load_txt(std::string_view filepath)`

#### Examples

```cpp
auto out = pluss::kindle::Catalog::load_txt("path/to/data.txt", {"nrows", 16});
```

#### Output

```text
out.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `masked_select`

#### Signature

```cpp
Catalog masked_select(self, const pluss::Series& mask) const
```

#### Parameters

`mask` : `const pluss::Series&`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Keeps only the rows where the supplied mask stream is true.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto mask = pluss::make_series({true, false, true}, pluss::codem::ScalarType::Bool);
auto out = ct.masked_select(mask);
```

#### Output

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
Stream max(self, std::string_view field) const
```

#### Parameters

`field` : `std::string_view`
    Name of the target stream in the catalog.

#### Returns

`Stream`
    Value produced by this call.

#### Notes

Computes the maximum value of one named numeric stream.

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

#### Output

```text
Series([20.0])
```

### `mean`

#### Signature

```cpp
Stream mean(self, std::string_view field) const
```

#### Parameters

`field` : `std::string_view`
    Name of the target stream in the catalog.

#### Returns

`Stream`
    Value produced by this call.

#### Notes

Computes the mean value of one named numeric stream.

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

#### Output

```text
Series([20.0])
```

### `merge`

#### Signature

```cpp
Catalog merge(self, const Catalog& other, std::string_view on, std::string_view how = "inner") const
```

#### Parameters

`other` : `const Catalog&`
    See the signature for the supported argument form.

`on` : `std::string_view`
    See the signature for the supported argument form.

`how` : `std::string_view`
    Join mode. `DataFrame::unify(...)` currently supports `"inner"` and `"left"`.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Combines two catalogs by matching rows on a key stream.

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

#### Output

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
Stream min(self, std::string_view field) const
```

#### Parameters

`field` : `std::string_view`
    Name of the target stream in the catalog.

#### Returns

`Stream`
    Value produced by this call.

#### Notes

Computes the minimum value of one named numeric stream.

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

#### Output

```text
Series([20.0])
```

### `nfields`

#### Signature

```cpp
std::int64_t nfields(self) const
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
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.nfields();
```

#### Output

```text
3
```

### `note`

#### Signature

```cpp
Catalog note(self) const
```

#### Parameters

`None`

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Returns one named stream interpreted as a note/text stream.

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

#### Output

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
Catalog notnull(self, std::initializer_list<std::string_view> keys = {}) const
```

#### Parameters

`keys` : `std::initializer_list<std::string_view>`
    Collection of slot or stream names to keep, drop, or rewrite together.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This is part of the missing-data cleanup path and is usually applied before summaries, grouping, or export.

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

#### Output

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
std::int64_t nstreams(self) const
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
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.nstreams();
```

#### Output

```text
3
```

### `query`

#### Signature

```cpp
Catalog query(self, const QueryFn& fn) const
```

#### Parameters

`fn` : `const QueryFn&`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Keeps only the rows selected by the supplied row-wise query function or expression.

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

#### Output

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
Catalog rename(self, const std::unordered_map<std::string, std::string>& mapping) const
```

#### Parameters

`mapping` : `const std::unordered_map<std::string, std::string>&`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Renames one or more streams without changing their values.

#### See also

- `Catalog rename(self, std::initializer_list<std::pair<std::string_view, std::string_view>> mapping) const`
- `Catalog rename(self, std::string_view from, std::string_view to) const`

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

#### Output

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
Catalog rename(self, std::initializer_list<std::pair<std::string_view, std::string_view>> mapping) const
```

#### Parameters

`mapping` : `std::initializer_list<std::pair<std::string_view, std::string_view>>`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Renames one or more streams without changing their values.

#### See also

- `Catalog rename(self, std::string_view from, std::string_view to) const`
- `Catalog rename(self, const std::unordered_map<std::string, std::string>& mapping) const`

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

#### Output

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
Catalog rename(self, std::string_view from, std::string_view to) const
```

#### Parameters

`from` : `std::string_view`
    See the signature for the supported argument form.

`to` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Renames one or more streams without changing their values.

#### See also

- `Catalog rename(self, std::initializer_list<std::pair<std::string_view, std::string_view>> mapping) const`
- `Catalog rename(self, const std::unordered_map<std::string, std::string>& mapping) const`

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

#### Output

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
Catalog& rename_(self, std::string_view from, std::string_view to)
```

#### Parameters

`from` : `std::string_view`
    See the signature for the supported argument form.

`to` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`Catalog&`
    Value produced by this call.

#### Notes

Renames one or more streams in place without changing their values.

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

### `sample`

#### Signature

```cpp
Catalog sample(self, std::int64_t n, std::optional<std::uint64_t> random_state = std::nullopt) const
```

#### Parameters

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

`random_state` : `std::optional<std::uint64_t>`
    Optional seed used to make sampling or shuffling reproducible.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Returns a random sample of rows across all streams.

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

#### Output

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
void save_as(self, std::string_view target_type, std::string_view filename, const pluss::loader::Config& config = {}) const
```

#### Parameters

`target_type` : `std::string_view`
    See the signature for the supported argument form.

`filename` : `std::string_view`
    See the signature for the supported argument form.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the catalog using the explicitly supplied output format selector.

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

#### Output

```text
saved: out.*
```

### `save_csv`

#### Signature

```cpp
void save_csv(self, std::string_view filename) const
```

#### Parameters

`filename` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the catalog to CSV after aligning its streams into a table-like representation.

#### See also

- `void save_csv(self, std::string_view filename, const pluss::loader::Config& config) const`

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

#### Output

```text
saved: out.*
```

### `save_csv`

#### Signature

```cpp
void save_csv(self, std::string_view filename, const pluss::loader::Config& config) const
```

#### Parameters

`filename` : `std::string_view`
    See the signature for the supported argument form.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the catalog to CSV after aligning its streams into a table-like representation.

#### See also

- `void save_csv(self, std::string_view filename) const`

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

#### Output

```text
saved: out.*
```

### `save_excel`

#### Signature

```cpp
void save_excel(self, std::string_view filename) const
```

#### Parameters

`filename` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the catalog to an Excel workbook after aligning its streams into a table-like representation.

#### See also

- `void save_excel(self, std::string_view filename, const pluss::loader::Config& config) const`

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

#### Output

```text
saved: out.*
```

### `save_excel`

#### Signature

```cpp
void save_excel(self, std::string_view filename, const pluss::loader::Config& config) const
```

#### Parameters

`filename` : `std::string_view`
    See the signature for the supported argument form.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the catalog to an Excel workbook after aligning its streams into a table-like representation.

#### See also

- `void save_excel(self, std::string_view filename) const`

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

#### Output

```text
saved: out.*
```

### `save_json`

#### Signature

```cpp
void save_json(self, std::string_view filename) const
```

#### Parameters

`filename` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the catalog to JSON for interchange or later reload.

#### See also

- `void save_json(self, std::string_view filename, const pluss::loader::Config& config) const`

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

#### Output

```text
saved: out.*
```

### `save_json`

#### Signature

```cpp
void save_json(self, std::string_view filename, const pluss::loader::Config& config) const
```

#### Parameters

`filename` : `std::string_view`
    See the signature for the supported argument form.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the catalog to JSON for interchange or later reload.

#### See also

- `void save_json(self, std::string_view filename) const`

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

#### Output

```text
saved: out.*
```

### `save_parquet`

#### Signature

```cpp
void save_parquet(self, std::string_view filename) const
```

#### Parameters

`filename` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the catalog to Parquet for compact typed storage.

#### See also

- `void save_parquet(self, std::string_view filename, const pluss::loader::Config& config) const`

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

#### Output

```text
saved: out.*
```

### `save_parquet`

#### Signature

```cpp
void save_parquet(self, std::string_view filename, const pluss::loader::Config& config) const
```

#### Parameters

`filename` : `std::string_view`
    See the signature for the supported argument form.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes the catalog to Parquet for compact typed storage.

#### See also

- `void save_parquet(self, std::string_view filename) const`

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

#### Output

```text
saved: out.*
```

### `save_txt`

#### Signature

```cpp
void save_txt(self, std::string_view filename) const
```

#### Parameters

`filename` : `std::string_view`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes a text rendering of the catalog for inspection or logging.

#### See also

- `void save_txt(self, std::string_view filename, const pluss::loader::Config& config) const`

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

#### Output

```text
saved: out.*
```

### `save_txt`

#### Signature

```cpp
void save_txt(self, std::string_view filename, const pluss::loader::Config& config) const
```

#### Parameters

`filename` : `std::string_view`
    See the signature for the supported argument form.

`config` : `const pluss::loader::Config&`
    Loader or saver configuration object. Use this when you want to pass a prebuilt option bundle instead of positional arguments.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

Writes a text rendering of the catalog for inspection or logging.

#### See also

- `void save_txt(self, std::string_view filename) const`

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

#### Output

```text
saved: out.*
```

### `select`

#### Signature

```cpp
Catalog select(self, const std::vector<std::string>& field_names) const
```

#### Parameters

`field_names` : `const std::vector<std::string>&`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Keeps only the named streams and preserves their shared row alignment.

#### See also

- `Catalog select(self, std::initializer_list<std::string_view> field_names) const`

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

#### Output

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
Catalog select(self, std::initializer_list<std::string_view> field_names) const
```

#### Parameters

`field_names` : `std::initializer_list<std::string_view>`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Keeps only the named streams and preserves their shared row alignment.

#### See also

- `Catalog select(self, const std::vector<std::string>& field_names) const`

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

#### Output

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
Catalog series(self) const
```

#### Parameters

`None`

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Returns one named stream interpreted as a numeric series stream.

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

#### Output

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
Catalog set(self, std::string_view key, const Stream& value) const
```

#### Parameters

`key` : `std::string_view`
    Name of the target slot or stream.

`value` : `const Stream&`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Adds or replaces one named stream and returns the updated catalog.

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

#### Output

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
Catalog& set_(self, std::string_view key, const Stream& value)
```

#### Parameters

`key` : `std::string_view`
    Name of the target slot or stream.

`value` : `const Stream&`
    See the signature for the supported argument form.

#### Returns

`Catalog&`
    Value produced by this call.

#### Notes

Adds or replaces one named stream in place.

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

### `set_stream`

#### Signature

```cpp
Catalog set_stream(self, std::string_view key, const Stream& value) const
```

#### Parameters

`key` : `std::string_view`
    Name of the target slot or stream.

`value` : `const Stream&`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Adds or replaces one named stream and returns the updated catalog.

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

#### Output

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
Catalog& set_stream_(self, std::string_view key, const Stream& value)
```

#### Parameters

`key` : `std::string_view`
    Name of the target slot or stream.

`value` : `const Stream&`
    See the signature for the supported argument form.

#### Returns

`Catalog&`
    Value produced by this call.

#### Notes

Adds or replaces one named stream in place.

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

### `set_streams`

#### Signature

```cpp
Catalog set_streams(self, std::initializer_list<NamedStream> items) const
```

#### Parameters

`items` : `std::initializer_list<NamedStream>`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Adds or replaces several named streams and returns the updated catalog.

#### See also

- `Catalog set_streams(self, std::initializer_list<std::pair<std::string_view, Stream>> items) const`

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

#### Output

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
Catalog set_streams(self, std::initializer_list<std::pair<std::string_view, Stream>> items) const
```

#### Parameters

`items` : `std::initializer_list<std::pair<std::string_view, Stream>>`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Adds or replaces several named streams and returns the updated catalog.

#### See also

- `Catalog set_streams(self, std::initializer_list<NamedStream> items) const`

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

#### Output

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
Catalog& set_streams_(self, std::initializer_list<NamedStream> items)
```

#### Parameters

`items` : `std::initializer_list<NamedStream>`
    See the signature for the supported argument form.

#### Returns

`Catalog&`
    Value produced by this call.

#### Notes

Adds or replaces several named streams in place.

#### See also

- `Catalog& set_streams_(self, std::initializer_list<std::pair<std::string_view, Stream>> items)`

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

### `set_streams_`

#### Signature

```cpp
Catalog& set_streams_(self, std::initializer_list<std::pair<std::string_view, Stream>> items)
```

#### Parameters

`items` : `std::initializer_list<std::pair<std::string_view, Stream>>`
    See the signature for the supported argument form.

#### Returns

`Catalog&`
    Value produced by this call.

#### Notes

Adds or replaces several named streams in place.

#### See also

- `Catalog& set_streams_(self, std::initializer_list<NamedStream> items)`

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

### `show`

#### Signature

```cpp
void show(self, std::ostream& os) const
```

#### Parameters

`os` : `std::ostream&`
    See the signature for the supported argument form.

#### Returns

`void`
    Performs the side effect and returns no value.

#### Notes

This renders a human-readable view for inspection instead of creating a new analytic object.

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

### `slice`

#### Signature

```cpp
Catalog slice(self, std::int64_t start, std::int64_t stop) const
```

#### Parameters

`start` : `std::int64_t`
    Inclusive starting position of the slice.

`stop` : `std::int64_t`
    Exclusive ending position of the slice.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Returns a contiguous row slice `[start, stop)` across every stream.

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

#### Output

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
Catalog sort(self, std::string_view by_field, bool ascending = true) const
```

#### Parameters

`by_field` : `std::string_view`
    See the signature for the supported argument form.

`ascending` : `bool`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Reorders rows using one stream as the sort key, while keeping the other streams aligned to the same row order.

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

#### Output

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
Catalog sort_by(self, const std::vector<std::string>& fields, const std::vector<bool>& ascending = {}) const
```

#### Parameters

`fields` : `const std::vector<std::string>&`
    See the signature for the supported argument form.

`ascending` : `const std::vector<bool>&`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Reorders rows using multiple sort keys, while keeping all streams aligned.

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

#### Output

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
Stream stream(self, std::string_view key) const
```

#### Parameters

`key` : `std::string_view`
    Name of the target slot or stream.

#### Returns

`Stream`
    Value produced by this call.

#### Notes

Fetches one value, member, or slice from the current object without changing the source object.

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

#### Output

```text
Series([10.0, 20.0, 30.0])
```

### `streams`

#### Signature

```cpp
std::vector<Stream> streams(self) const
```

#### Parameters

`None`

#### Returns

`std::vector<Stream>`
    Materialized collection of stream objects.

#### Notes

Materializes the current members as a collection you can inspect or reuse elsewhere.

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

#### Output

```text
[Stream(id), Stream(metric), Stream(city)]
```

### `sum`

#### Signature

```cpp
Stream sum(self, std::string_view field) const
```

#### Parameters

`field` : `std::string_view`
    Name of the target stream in the catalog.

#### Returns

`Stream`
    Value produced by this call.

#### Notes

Computes the sum of one named numeric stream.

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

#### Output

```text
Series([20.0])
```

### `tabelize`

#### Signature

```cpp
pluss::table::DataFrame tabelize(self, const TabelizePolicy& policy = {}) const
```

#### Parameters

`policy` : `const TabelizePolicy&`
    See the signature for the supported argument form.

#### Returns

`pluss::table::DataFrame`
    Value produced by this call.

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

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
Catalog tail(self, std::int64_t n) const
```

#### Parameters

`n` : `std::int64_t`
    Number of rows, entries, or depth items to keep in the preview.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Returns the last `n` rows across every stream in the catalog.

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

#### Output

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
Catalog take(self, std::span<const std::int64_t> indices) const
```

#### Parameters

`indices` : `std::span<const std::int64_t>`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Returns the rows at the requested integer positions across every stream.

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

#### Output

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
Catalog text_preprocess(self, std::string_view field) const
```

#### Parameters

`field` : `std::string_view`
    Name of the target stream in the catalog.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

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

#### Output

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
Catalog tfidf(self, std::string_view field) const
```

#### Parameters

`field` : `std::string_view`
    Name of the target stream in the catalog.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

This is part of the text-processing surface for note-backed or text-oriented data.

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

#### Output

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
Catalog timeindex(self) const
```

#### Parameters

`None`

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Returns one named stream interpreted as a time-index stream.

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

#### Output

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
Catalog timeline(self) const
```

#### Parameters

`None`

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Returns one named stream interpreted as a timeline stream.

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

#### Output

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
std::string to_json(self) const
```

#### Parameters

`None`

#### Returns

`std::string`
    Rendered string result.

#### Notes

Serializes or renders the current object into a text form you can inspect, log, or export.

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

#### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `to_string`

#### Signature

```cpp
std::string to_string(self) const
```

#### Parameters

`None`

#### Returns

`std::string`
    Rendered string result.

#### Notes

Serializes or renders the current object into a text form you can inspect, log, or export.

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

#### Output

```text
ct.head():
id     -> Series([1, 2, 3])
metric -> Series([10.0, 20.0, 30.0])
city   -> Note(["Accra", "Tema", "Kumasi"])
```

### `to_table`

#### Signature

```cpp
pluss::table::DataFrame to_table(self, const TabelizePolicy& policy = {}, bool render = true) const
```

#### Parameters

`policy` : `const TabelizePolicy&`
    See the signature for the supported argument form.

`render` : `bool`
    When `true`, the returned view can auto-render in temporary terminal contexts. Pass `false` when you want explicit output control.

#### Returns

`pluss::table::DataFrame`
    Value produced by this call.

#### Notes

This is an explicit bridge between PlussMic models so you can switch to the representation that fits the next task.

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
Catalog transform(self, const std::function<Stream(const Stream&)>& fn) const
```

#### Parameters

`fn` : `const std::function<Stream(const Stream&)>&`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Rewrites one stream, several streams, or all streams by applying a user function.

#### See also

- `Catalog transform(self, std::string_view field, const std::function<Stream(const Stream&)>& fn) const`

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

#### Output

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
Catalog transform(self, std::string_view field, const std::function<Stream(const Stream&)>& fn) const
```

#### Parameters

`field` : `std::string_view`
    Name of the target stream in the catalog.

`fn` : `const std::function<Stream(const Stream&)>&`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Rewrites one stream, several streams, or all streams by applying a user function.

#### See also

- `Catalog transform(self, const std::function<Stream(const Stream&)>& fn) const`

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

#### Output

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
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto out = ct.unique();
```

#### Output

```text
true
```

### `where`

#### Signature

```cpp
Catalog where(self, const pluss::Series& condition) const
```

#### Parameters

`condition` : `const pluss::Series&`
    See the signature for the supported argument form.

#### Returns

`Catalog`
    Value produced by this call.

#### Notes

Keeps only the rows where the supplied mask stream is true.

#### Examples

```cpp
using namespace pluss::kindle::literals;

auto ct = pluss::kindle::Catalog({
    "id"_k = pluss::kindle::Stream(pluss::make_series({1, 2, 3})),
    "metric"_k = pluss::kindle::Stream(pluss::make_series({10.0, 20.0, 30.0})),
    "city"_k = pluss::kindle::Stream(pluss::make_note({"Accra", "Tema", "Kumasi"})),
});

auto mask = pluss::make_series({true, false, true}, pluss::codem::ScalarType::Bool);
auto out = ct.where(mask);
```

#### Output

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
